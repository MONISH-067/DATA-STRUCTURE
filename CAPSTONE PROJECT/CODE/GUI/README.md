# Duplicate File Detection System

A **Data Structures and Algorithms Capstone Project** that efficiently detects duplicate files in a directory tree using a multi-phase pipeline of classic DSA techniques.

## Data Structures & Algorithms Used

| Component | Data Structure / Algorithm | Purpose |
|-----------|--------------------------|---------|
| `hash_table.py` | **Hash Table** (separate chaining, DJB2) | Group files by size and hash digest |
| `bloom_filter.py` | **Bloom Filter** (bit array + double hashing) | Probabilistic pre-screening to skip unique files early |
| `trie.py` | **Trie** (prefix tree) | Index directory paths for prefix queries and autocomplete |
| `heap.py` | **Binary Heap** (min/max, array-based) | Top-K duplicate groups by wasted space |
| `sorting.py` | **Merge Sort** (stable) | Sort results by wasted space / file count |
| `sorting.py` | **Quick Sort** (median-of-three) | Alternative sorting with small-array insertion-sort optimisation |
| `walker.py` | **BFS** (queue) / **DFS** (recursion) | Directory tree traversal |
| `hashing.py` | **SHA-256** / MD5 | Cryptographic file fingerprinting |

## Detection Pipeline

```
   Phase 1              Phase 2             Phase 3             Phase 4
┌───────────┐      ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
│ BFS / DFS │─────>│  Group by    │───>│ Bloom Filter │───>│  Quick Hash  │
│ Dir Walk  │      │  File Size   │    │ Pre-screen   │    │ (first 4 KB) │
└───────────┘      └──────────────┘    └──────────────┘    └──────────────┘
                                                                   │
                         Phase 5                                   ▼
                    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
                    │  Report      │<───│ Top-K Heap + │<───│  Full SHA256 │
                    │  Generator   │    │  Merge Sort  │    │  Hash Match  │
                    └──────────────┘    └──────────────┘    └──────────────┘
```

**Why multi-phase?** Each phase eliminates non-duplicate candidates early, drastically reducing expensive I/O operations (full file reads).

## Project Structure

```
duplicate-file-detector/
├── main.py                      # CLI entry point
├── README.md
├── src/
│   ├── __init__.py
│   ├── hash_table.py            # Custom hash table (separate chaining)
│   ├── bloom_filter.py          # Bloom filter (probabilistic set)
│   ├── trie.py                  # Directory path trie
│   ├── heap.py                  # Binary heap (priority queue)
│   ├── sorting.py               # Merge sort & quick sort
│   ├── hashing.py               # SHA-256 / MD5 file hashing
│   ├── walker.py                # BFS & DFS directory traversal
│   ├── detector.py              # Core detection engine (orchestrator)
│   └── report.py                # Text / JSON / CSV report generation
└── tests/
    ├── test_all.py              # Unit + integration tests
    └── generate_sample_data.py  # Test data generator
```

## Usage

### Graphical User Interface (GUI)
You can launch the desktop GUI in either of two ways:
```bash
python gui.py
# or
python main.py --gui
# or simply run without arguments
python main.py
```

### Basic CLI scan
```bash
python main.py C:\Users\Me\Documents
```

### With options
```bash
python main.py ./photos --traversal bfs --top 10 --min-size 1024
python main.py ./data --export report.json --format json
python main.py ./files --algorithm md5 --show-bloom-stats
```

### Command-line arguments

| Argument | Default | Description |
|----------|---------|-------------|
| `directory` | (required) | Root directory to scan |
| `--traversal` | `dfs` | Traversal strategy: `dfs` or `bfs` |
| `--min-size` | `1` | Minimum file size (bytes) |
| `--max-size` | ∞ | Maximum file size (bytes) |
| `--include-ext` | all | Only scan these extensions |
| `--exclude-ext` | none | Skip these extensions |
| `--top` | `20` | Show top K groups |
| `--export` | none | Export report to file |
| `--format` | `text` | Export format: `text`, `json`, `csv` |
| `--algorithm` | `sha256` | Hash algorithm: `sha256`, `md5`, `sha1` |
| `--show-bloom-stats` | off | Show Bloom filter diagnostics |

### Run tests
```bash
python -m pytest tests/test_all.py -v
# or
python tests/test_all.py
```

### Generate sample data
```bash
python tests/generate_sample_data.py
```

## Complexity Analysis

| Operation | Time | Space | Notes |
|-----------|------|-------|-------|
| Directory walk | O(V + E) | O(V) | V = dirs, E = files |
| Size grouping | O(n) | O(n) | Hash table insert/lookup |
| Bloom pre-screen | O(n·k) | O(m) bits | k = hash funcs, m = bit array |
| Quick hash | O(n·4KB) | O(n) | Reads only first 4 KB |
| Full hash | O(d·F) | O(1) per file | d = candidates, F = avg size |
| Merge sort | O(g log g) | O(g) | g = duplicate groups |
| Top-K heap | O(g + k log g) | O(g) | Build heap + extract K |
| **Overall** | **O(n + d·F)** | **O(n)** | Dominated by hashing phase |

## Licence

Academic capstone project — free to use for educational purposes.
