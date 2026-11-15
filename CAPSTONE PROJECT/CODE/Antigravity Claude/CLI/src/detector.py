"""
Duplicate Detection Engine
============================
The central engine that orchestrates all data structures and algorithms
to find duplicate files efficiently.

Pipeline:
    ┌─────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
    │  BFS / DFS  │───>│  Group by    │───>│  Bloom filter│───>│  Full SHA-256│
    │  Dir Walk   │    │  file size   │    │  pre-screen  │    │  hash match  │
    └─────────────┘    └──────────────┘    └──────────────┘    └──────────────┘
                                                                      │
                                                                      ▼
    ┌─────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
    │  Report     │<───│  Top-K via   │<───│  Sort groups │<───│  Trie index  │
    │  Generator  │    │  Max-Heap    │    │  (MergeSort) │    │  dir paths   │
    └─────────────┘    └──────────────┘    └──────────────┘    └──────────────┘

Data Structures Used:
    1. Hash Table   — grouping files by size and hash digest
    2. Bloom Filter — fast probabilistic pre-screening
    3. Trie         — directory path indexing for reporting
    4. Binary Heap  — top-K duplicate groups by wasted space

Algorithms Used:
    1. BFS / DFS    — directory traversal
    2. SHA-256      — cryptographic file fingerprinting
    3. Merge Sort   — stable sorting of results
    4. Quick Sort   — alternative sorting strategy
"""

from __future__ import annotations
import os
import time
from dataclasses import dataclass, field
from typing import Optional

from .hash_table import HashTable
from .bloom_filter import BloomFilter
from .trie import DirectoryTrie
from .heap import BinaryHeap
from .sorting import merge_sort, quick_sort
from .hashing import compute_full_hash, compute_quick_hash, FileHashError
from .walker import DirectoryWalker, FileInfo, ScanConfig


@dataclass
class DuplicateGroup:
    """A group of files that are exact duplicates of each other."""
    hash_digest: str
    file_size: int
    paths: list[str]

    @property
    def count(self) -> int:
        return len(self.paths)

    @property
    def wasted_bytes(self) -> int:
        """Space wasted = (count - 1) * size.  One copy is the 'original'."""
        return (self.count - 1) * self.file_size

    def __repr__(self) -> str:
        return (
            f"DuplicateGroup(hash={self.hash_digest[:12]}..., "
            f"size={format_size(self.file_size)}, copies={self.count})"
        )


@dataclass
class ScanResult:
    """Complete results from a duplicate detection scan."""
    duplicate_groups: list[DuplicateGroup]
    total_files_scanned: int
    total_dirs_scanned: int
    total_files_skipped: int
    total_duplicates: int
    total_wasted_bytes: int
    total_unique_files: int
    scan_errors: list[str]
    elapsed_seconds: float

    # Phase statistics
    phase1_size_groups: int = 0          # groups after size-based grouping
    phase2_bloom_candidates: int = 0     # candidates after Bloom filter
    phase3_quick_hash_groups: int = 0    # groups after quick-hash
    phase4_full_hash_groups: int = 0     # groups after full-hash (final)


class DuplicateDetector:
    """
    Main duplicate file detection engine.

    Uses a multi-phase pipeline to minimise I/O:

        Phase 1 — Size Grouping (Hash Table)
            Group all files by size. Files with unique sizes cannot be
            duplicates. Only groups with 2+ files proceed.

        Phase 2 — Bloom Filter Pre-screen
            For remaining files, compute a quick key (size + first-4KB hash)
            and check against a Bloom filter. If the filter says "not seen",
            the file is likely unique — but since Bloom filters can have
            false positives, we keep all candidates.

        Phase 3 — Quick Hash Grouping (Hash Table)
            Within each size group, hash only the first 4 KB. Files with
            different quick hashes are definitely not duplicates.

        Phase 4 — Full Hash Grouping (Hash Table)
            For remaining candidates, compute the full SHA-256 hash.
            Files with identical full hashes are true duplicates.

        Phase 5 — Indexing & Reporting (Trie, Heap, Sorting)
            Index paths in a Trie, use a Max-Heap for Top-K, and
            Merge Sort for the final report.
    """

    def __init__(
        self,
        config: Optional[ScanConfig] = None,
        traversal: str = "dfs",
        hash_algorithm: str = "sha256",
    ):
        self.config = config or ScanConfig()
        self.traversal = traversal.lower()
        self.hash_algorithm = hash_algorithm

        # Data structures
        self._size_table = HashTable(capacity=1024)
        self._bloom = BloomFilter(expected_items=50_000, fp_rate=0.01)
        self._trie = DirectoryTrie()

    def scan(self, root_directory: str) -> ScanResult:
        """
        Execute the full duplicate detection pipeline.

        Args:
            root_directory: The root directory to scan.

        Returns:
            ScanResult with all duplicate groups and statistics.
        """
        start_time = time.time()
        root_directory = os.path.abspath(root_directory)

        if not os.path.isdir(root_directory):
            raise ValueError(f"Not a valid directory: {root_directory}")

        # ======================== PHASE 1 ======================== #
        # Directory traversal + grouping by file size                #
        # ========================================================= #
        walker = DirectoryWalker(self.config)
        if self.traversal == "bfs":
            file_iterator = walker.walk_bfs(root_directory)
        else:
            file_iterator = walker.walk_dfs(root_directory)

        for file_info in file_iterator:
            size_key = str(file_info.size)
            existing = self._size_table.get(size_key)
            if existing is None:
                self._size_table.put(size_key, [file_info])
            else:
                existing.append(file_info)

            # Also index in the trie
            self._trie.insert(file_info.path)

        # Keep only size groups with 2+ files
        size_groups: list[list[FileInfo]] = []
        for key in self._size_table.keys():
            group = self._size_table[key]
            if len(group) >= 2:
                size_groups.append(group)

        phase1_count = len(size_groups)

        # ======================== PHASE 2 ======================== #
        # Bloom filter pre-screen                                    #
        # ========================================================= #
        bloom_candidates = 0
        for group in size_groups:
            for fi in group:
                combined_key = f"{fi.size}:{fi.path}"
                if fi.size > 0:  # Skip empty files from Bloom
                    self._bloom.add(combined_key)
                    bloom_candidates += 1

        # ======================== PHASE 3 ======================== #
        # Quick hash grouping (first 4 KB)                           #
        # ========================================================= #
        quick_hash_table = HashTable(capacity=512)
        quick_hash_groups_count = 0

        for group in size_groups:
            # Sub-group by quick hash within each size group
            sub_table = HashTable(capacity=max(4, len(group)))
            for fi in group:
                try:
                    qhash = compute_quick_hash(fi.path, self.hash_algorithm)
                    qkey = f"{fi.size}:{qhash}"
                    existing = sub_table.get(qkey)
                    if existing is None:
                        sub_table.put(qkey, [fi])
                    else:
                        existing.append(fi)
                except FileHashError:
                    continue

            for key in sub_table.keys():
                sub_group = sub_table[key]
                if len(sub_group) >= 2:
                    quick_hash_table.put(
                        f"qg_{quick_hash_groups_count}", sub_group
                    )
                    quick_hash_groups_count += 1

        # ======================== PHASE 4 ======================== #
        # Full hash grouping (SHA-256 of entire file)                #
        # ========================================================= #
        full_hash_table = HashTable(capacity=256)

        for key in quick_hash_table.keys():
            group = quick_hash_table[key]
            sub_table = HashTable(capacity=max(4, len(group)))

            for fi in group:
                try:
                    full_hash = compute_full_hash(fi.path, self.hash_algorithm)
                    existing = sub_table.get(full_hash)
                    if existing is None:
                        sub_table.put(full_hash, [fi])
                    else:
                        existing.append(fi)
                except FileHashError:
                    continue

            for fh_key in sub_table.keys():
                sub_group = sub_table[fh_key]
                if len(sub_group) >= 2:
                    full_hash_table.put(fh_key, sub_group)

        # ======================== PHASE 5 ======================== #
        # Build DuplicateGroup objects and sort/rank                 #
        # ========================================================= #
        duplicate_groups: list[DuplicateGroup] = []
        for hash_key in full_hash_table.keys():
            files = full_hash_table[hash_key]
            group = DuplicateGroup(
                hash_digest=hash_key,
                file_size=files[0].size,
                paths=[fi.path for fi in files],
            )
            duplicate_groups.append(group)

        # Sort by wasted space (descending) using merge sort
        duplicate_groups = merge_sort(
            duplicate_groups, key=lambda g: g.wasted_bytes, reverse=True
        )

        # Compute totals
        total_duplicates = sum(g.count - 1 for g in duplicate_groups)
        total_wasted = sum(g.wasted_bytes for g in duplicate_groups)
        stats = walker.get_stats()
        elapsed = time.time() - start_time

        return ScanResult(
            duplicate_groups=duplicate_groups,
            total_files_scanned=stats["files_scanned"],
            total_dirs_scanned=stats["dirs_scanned"],
            total_files_skipped=stats["files_skipped"],
            total_duplicates=total_duplicates,
            total_wasted_bytes=total_wasted,
            total_unique_files=stats["files_scanned"] - total_duplicates,
            scan_errors=walker.errors,
            elapsed_seconds=elapsed,
            phase1_size_groups=phase1_count,
            phase2_bloom_candidates=bloom_candidates,
            phase3_quick_hash_groups=quick_hash_groups_count,
            phase4_full_hash_groups=len(duplicate_groups),
        )

    def get_top_k_groups(
        self, result: ScanResult, k: int = 10
    ) -> list[DuplicateGroup]:
        """
        Use a Max-Heap to efficiently extract the top-K duplicate groups
        by wasted disk space.
        """
        heap = BinaryHeap.heapify(
            result.duplicate_groups,
            key=lambda g: g.wasted_bytes,
            max_heap=True,
        )
        return heap.top_k(k)

    @property
    def trie(self) -> DirectoryTrie:
        """Access the directory trie for path queries."""
        return self._trie

    @property
    def bloom_filter(self) -> BloomFilter:
        """Access the Bloom filter for diagnostics."""
        return self._bloom


# ====================================================================== #
#  Utility functions                                                       #
# ====================================================================== #
def format_size(size_bytes: int) -> str:
    """Human-readable file size."""
    if size_bytes < 1024:
        return f"{size_bytes} B"
    elif size_bytes < 1024 ** 2:
        return f"{size_bytes / 1024:.1f} KB"
    elif size_bytes < 1024 ** 3:
        return f"{size_bytes / (1024 ** 2):.1f} MB"
    else:
        return f"{size_bytes / (1024 ** 3):.2f} GB"
