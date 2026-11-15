# 🔍 Duplicate File Detection System — DSA Capstone Project

> [!IMPORTANT]
> Project location: [`C:\Users\Monish\.gemini\antigravity\scratch\duplicate-file-detector`](file:///C:/Users/Monish/.gemini/antigravity/scratch/duplicate-file-detector)
> **51/51 tests passing** · Fully functional CLI · Text/JSON/CSV reports

---

## Project Overview

A **multi-phase duplicate file detection system** that scans a directory tree and identifies exact duplicate files using a pipeline of classic data structures and algorithms. Each phase progressively eliminates non-duplicate candidates to minimise expensive disk I/O.

---

## Data Structures & Algorithms Inventory

### Data Structures (5)

| # | Data Structure | File | Purpose | Key Operations |
|---|---------------|------|---------|----------------|
| 1 | **Hash Table** (separate chaining, DJB2 hash) | [`hash_table.py`](file:///C:/Users/Monish/.gemini/antigravity/scratch/duplicate-file-detector/src/hash_table.py) | Group files by size & hash digest | `put` O(1), `get` O(1), dynamic resize |
| 2 | **Bloom Filter** (bit array + double hashing) | [`bloom_filter.py`](file:///C:/Users/Monish/.gemini/antigravity/scratch/duplicate-file-detector/src/bloom_filter.py) | Probabilistic pre-screening — skip guaranteed-unique files | `add` O(k), `might_contain` O(k), zero false negatives |
| 3 | **Trie** (prefix tree) | [`trie.py`](file:///C:/Users/Monish/.gemini/antigravity/scratch/duplicate-file-detector/src/trie.py) | Index directory paths for prefix search & autocomplete | `insert` O(d), `search_prefix` O(d+k), `longest_common_prefix` |
| 4 | **Binary Heap** (min/max, array-based) | [`heap.py`](file:///C:/Users/Monish/.gemini/antigravity/scratch/duplicate-file-detector/src/heap.py) | Extract Top-K duplicate groups by wasted space | `push` O(log n), `pop` O(log n), Floyd's `heapify` O(n) |
| 5 | **Queue** (collections.deque) | [`walker.py`](file:///C:/Users/Monish/.gemini/antigravity/scratch/duplicate-file-detector/src/walker.py) | FIFO frontier for BFS directory traversal | `append` O(1), `popleft` O(1) |

### Algorithms (7)

| # | Algorithm | File | Purpose | Complexity |
|---|-----------|------|---------|------------|
| 1 | **DFS** (recursive) | [`walker.py`](file:///C:/Users/Monish/.gemini/antigravity/scratch/duplicate-file-detector/src/walker.py#L89-L121) | Depth-first directory traversal | O(V + E) |
| 2 | **BFS** (iterative queue) | [`walker.py`](file:///C:/Users/Monish/.gemini/antigravity/scratch/duplicate-file-detector/src/walker.py#L54-L87) | Breadth-first directory traversal | O(V + E) |
| 3 | **SHA-256 / MD5** (cryptographic hashing) | [`hashing.py`](file:///C:/Users/Monish/.gemini/antigravity/scratch/duplicate-file-detector/src/hashing.py) | File fingerprinting (full + quick 4KB) | O(file_size) |
| 4 | **Merge Sort** (stable, divide & conquer) | [`sorting.py`](file:///C:/Users/Monish/.gemini/antigravity/scratch/duplicate-file-detector/src/sorting.py#L27-L66) | Sort duplicate groups by wasted space | O(n log n) guaranteed |
| 5 | **Quick Sort** (median-of-three pivot) | [`sorting.py`](file:///C:/Users/Monish/.gemini/antigravity/scratch/duplicate-file-detector/src/sorting.py#L72-L146) | Alternative sort with insertion-sort cutoff | O(n log n) average |
| 6 | **DJB2 Hash Function** | [`hash_table.py`](file:///C:/Users/Monish/.gemini/antigravity/scratch/duplicate-file-detector/src/hash_table.py#L47-L58) | Non-cryptographic hash for hash table buckets | O(key_length) |
| 7 | **Double Hashing** (Bloom filter) | [`bloom_filter.py`](file:///C:/Users/Monish/.gemini/antigravity/scratch/duplicate-file-detector/src/bloom_filter.py#L72-L90) | Generate k bit positions from one MD5 digest | O(k) |

---

## Detection Pipeline

```mermaid
flowchart LR
    A["Phase 1\nBFS / DFS\nDir Walk"] --> B["Phase 2\nGroup by\nFile Size\n(Hash Table)"]
    B --> C["Phase 3\nBloom Filter\nPre-screen"]
    C --> D["Phase 4\nQuick Hash\n(first 4 KB)"]
    D --> E["Phase 5\nFull SHA-256\nHash Match"]
    E --> F["Phase 6\nTrie Index +\nHeap Top-K +\nMerge Sort"]
    F --> G["Report\nText / JSON\n/ CSV"]
```

Each phase **eliminates non-duplicate candidates early**, so expensive full-file reads only happen for true candidates:

| Phase | What it does | Eliminates |
|-------|-------------|------------|
| 1 — Dir Walk | BFS/DFS scan with size/extension filters | Excluded files |
| 2 — Size Group | Hash table groups files by size | Files with unique sizes (~80-90% eliminated) |
| 3 — Bloom Filter | Probabilistic "seen before?" check | Likely-unique files (fast, zero false negatives) |
| 4 — Quick Hash | Hash first 4 KB only | Files differing in header/prefix |
| 5 — Full Hash | Full SHA-256 of entire file | Confirms true duplicates |
| 6 — Report | Trie path index, Max-Heap top-K, Merge Sort | Final ranked output |

---

## Project File Structure

```
duplicate-file-detector/
├── main.py                        # CLI entry point with argparse
├── README.md                      # Project documentation
├── src/
│   ├── __init__.py
│   ├── hash_table.py              # Custom Hash Table (separate chaining)
│   ├── bloom_filter.py            # Bloom Filter (bit array)
│   ├── trie.py                    # Directory Trie (prefix tree)
│   ├── heap.py                    # Binary Heap (priority queue)
│   ├── sorting.py                 # Merge Sort & Quick Sort
│   ├── hashing.py                 # SHA-256 / MD5 file fingerprinting
│   ├── walker.py                  # BFS & DFS directory traversal
│   ├── detector.py                # Core detection engine (orchestrator)
│   └── report.py                  # Text / JSON / CSV report generator
└── tests/
    ├── test_all.py                # 51 unit + integration tests
    ├── generate_sample_data.py    # Random test data generator
    └── sample_files/              # Generated test data
```

---

## Complexity Analysis

| Operation | Time Complexity | Space Complexity | Notes |
|-----------|:-:|:-:|-------|
| Directory walk (BFS/DFS) | O(V + E) | O(V) | V = directories, E = files |
| Size grouping | O(n) amortised | O(n) | Hash table with dynamic resize |
| Bloom filter screen | O(n · k) | O(m) bits | k ≈ 7 hash funcs, m auto-sized |
| Quick hash (4 KB) | O(n · 4096) | O(1) per file | Constant-size read |
| Full hash (SHA-256) | O(d · F̄) | O(1) per file | d = candidates, F̄ = avg file size |
| Merge sort groups | O(g log g) | O(g) | g = number of duplicate groups |
| Top-K via max-heap | O(g + k log g) | O(g) | Heapify + k extractions |
| **End-to-end** | **O(n + d · F̄)** | **O(n)** | Dominated by full-hash I/O |

> [!TIP]
> The multi-phase pipeline ensures `d ≪ n` in practice, so the expensive full-hash phase runs on only a small fraction of files.

---

## Test Results

```
============================= 51 passed in 0.42s ==============================

Test Suites:
  TestHashTable .............. 10 tests  ✓
  TestBloomFilter ............ 5 tests   ✓
  TestBinaryHeap ............. 7 tests   ✓
  TestDirectoryTrie .......... 4 tests   ✓
  TestSorting ................ 11 tests  ✓
  TestFileHashing ............ 4 tests   ✓
  TestDirectoryWalker ........ 5 tests   ✓
  TestDuplicateDetector ...... 5 tests   ✓  (integration)
```

---

## Sample Output (Demo Run)

```
🔍 Scanning: tests/sample_files/generated
   Traversal: DFS | Hash: SHA256

══════════════════════════════════════════════════════════════════════
  DUPLICATE FILE DETECTION REPORT
══════════════════════════════════════════════════════════════════════
  Files scanned       : 63
  Directories scanned : 10
  Unique files        : 26
  Duplicate files     : 37
  Duplicate groups    : 18
  Wasted disk space   : 592.3 KB
  Scan time           : 0.02 seconds

── PIPELINE STATISTICS ───────────────────────────────────
  Phase 1 — Size groups (≥2 files) : 8
  Phase 2 — Bloom filter candidates : 62
  Phase 3 — Quick-hash groups       : 18
  Phase 4 — Full-hash groups (dupes): 18

── BLOOM FILTER DIAGNOSTICS ──────────────────────────────
  Bit array size   : 479,253 bits (59,906 bytes)
  Hash functions   : 7
  Fill ratio       : 0.09%
  Est. FP rate     : 0.0000%

── TOP SPACE-WASTERS (via Max-Heap) ──────────────────────
  #1: 192.0 KB wasted (4 copies of 64.0 KB file)
  #2: 96.0 KB wasted (4 copies of 32.0 KB file)
  #3: 96.0 KB wasted (4 copies of 32.0 KB file)

⚠️  Found 37 duplicate files in 18 groups
   Potential space savings: 592.3 KB
```

---

## How to Run

```bash
# Basic scan
python main.py C:\Users\Me\Documents

# BFS traversal, top 5 results
python main.py ./photos --traversal bfs --top 5

# Export as JSON
python main.py ./data --export report.json --format json

# Filter by size and show Bloom stats
python main.py ./files --min-size 1024 --show-bloom-stats

# Run all tests
python -m pytest tests/test_all.py -v
```
