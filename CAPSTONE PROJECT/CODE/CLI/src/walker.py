"""
Directory Walker — BFS & DFS File System Traversal
====================================================
Algorithms: Breadth-First Search (BFS), Depth-First Search (DFS)

Walks a directory tree to discover all files, applying optional filters
for minimum size, file extensions, and exclusion patterns.

BFS (using a queue) is used for level-order traversal — useful when we
want to process shallower directories first.

DFS (using recursion / a stack) mirrors the natural file-system hierarchy
and is the default traversal strategy.

Time Complexity:  O(V + E) where V = directories, E = files
Space Complexity: O(V) for the queue/stack
"""

from __future__ import annotations
import os
from collections import deque
from dataclasses import dataclass, field
from typing import Generator, Optional, Set
import fnmatch


@dataclass
class FileInfo:
    """Metadata for a discovered file."""
    path: str
    size: int
    name: str
    extension: str

    @classmethod
    def from_path(cls, path: str) -> "FileInfo":
        stat = os.stat(path)
        name = os.path.basename(path)
        _, ext = os.path.splitext(name)
        return cls(path=os.path.abspath(path), size=stat.st_size, name=name, extension=ext.lower())


@dataclass
class ScanConfig:
    """Configuration for directory scanning."""
    min_size: int = 0                          # skip files smaller than this (bytes)
    max_size: Optional[int] = None             # skip files larger than this
    include_extensions: Optional[Set[str]] = None  # e.g., {".jpg", ".png"}
    exclude_extensions: Optional[Set[str]] = None  # e.g., {".tmp", ".log"}
    exclude_patterns: list[str] = field(default_factory=lambda: [
        ".git", "__pycache__", "node_modules", ".DS_Store", "Thumbs.db"
    ])
    follow_symlinks: bool = False


class DirectoryWalker:
    """
    Walks a directory tree using BFS or DFS to discover files.

    Demonstrates:
        - BFS using collections.deque (queue)
        - DFS using recursion (call stack)
        - Generator pattern for memory-efficient iteration
    """

    def __init__(self, config: Optional[ScanConfig] = None):
        self.config = config or ScanConfig()
        self.files_scanned = 0
        self.dirs_scanned = 0
        self.files_skipped = 0
        self.errors: list[str] = []

    def walk_bfs(self, root: str) -> Generator[FileInfo, None, None]:
        """
        Breadth-First Search traversal of the directory tree.

        Uses a FIFO queue (deque). Processes all files at depth d
        before any files at depth d+1.

        Algorithm:
            1. Enqueue the root directory.
            2. While queue is not empty:
                a. Dequeue a directory.
                b. For each entry:
                   - If directory: enqueue it.
                   - If file: yield it (if it passes filters).
        """
        root = os.path.abspath(root)
        if not os.path.isdir(root):
            raise ValueError(f"Not a directory: {root}")

        queue: deque[str] = deque([root])

        while queue:
            current_dir = queue.popleft()
            self.dirs_scanned += 1

            try:
                entries = os.scandir(current_dir)
            except PermissionError as e:
                self.errors.append(str(e))
                continue

            for entry in entries:
                try:
                    if self._should_exclude(entry.name):
                        continue

                    if entry.is_dir(follow_symlinks=self.config.follow_symlinks):
                        queue.append(entry.path)
                    elif entry.is_file(follow_symlinks=self.config.follow_symlinks):
                        file_info = FileInfo.from_path(entry.path)
                        if self._passes_filters(file_info):
                            self.files_scanned += 1
                            yield file_info
                        else:
                            self.files_skipped += 1
                except (PermissionError, OSError) as e:
                    self.errors.append(str(e))

    def walk_dfs(self, root: str) -> Generator[FileInfo, None, None]:
        """
        Depth-First Search traversal of the directory tree.

        Uses recursion (implicit call stack). Processes the deepest
        files first before backtracking.

        Algorithm:
            1. List entries in the current directory.
            2. For each entry:
               - If file: yield it.
               - If directory: recurse into it.
        """
        root = os.path.abspath(root)
        if not os.path.isdir(root):
            raise ValueError(f"Not a directory: {root}")

        yield from self._dfs_recursive(root)

    def _dfs_recursive(self, directory: str) -> Generator[FileInfo, None, None]:
        """Recursive DFS helper."""
        self.dirs_scanned += 1
        try:
            entries = sorted(os.scandir(directory), key=lambda e: e.name)
        except PermissionError as e:
            self.errors.append(str(e))
            return

        for entry in entries:
            try:
                if self._should_exclude(entry.name):
                    continue

                if entry.is_file(follow_symlinks=self.config.follow_symlinks):
                    file_info = FileInfo.from_path(entry.path)
                    if self._passes_filters(file_info):
                        self.files_scanned += 1
                        yield file_info
                    else:
                        self.files_skipped += 1
                elif entry.is_dir(follow_symlinks=self.config.follow_symlinks):
                    yield from self._dfs_recursive(entry.path)
            except (PermissionError, OSError) as e:
                self.errors.append(str(e))

    # ------------------------------------------------------------------ #
    #  Filter helpers                                                      #
    # ------------------------------------------------------------------ #
    def _should_exclude(self, name: str) -> bool:
        """Check if a file/directory name matches exclusion patterns."""
        for pattern in self.config.exclude_patterns:
            if fnmatch.fnmatch(name, pattern):
                return True
        return False

    def _passes_filters(self, fi: FileInfo) -> bool:
        """Check if a file passes all configured filters."""
        if fi.size < self.config.min_size:
            return False
        if self.config.max_size is not None and fi.size > self.config.max_size:
            return False
        if self.config.include_extensions and fi.extension not in self.config.include_extensions:
            return False
        if self.config.exclude_extensions and fi.extension in self.config.exclude_extensions:
            return False
        return True

    def get_stats(self) -> dict:
        """Return scanning statistics."""
        return {
            "files_scanned": self.files_scanned,
            "dirs_scanned": self.dirs_scanned,
            "files_skipped": self.files_skipped,
            "errors": len(self.errors),
        }
