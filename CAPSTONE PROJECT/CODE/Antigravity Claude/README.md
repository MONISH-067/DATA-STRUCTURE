# Duplicate File Detection System — DSA Capstone Project

This project implements a high-performance multi-phase duplicate file detection pipeline using custom Data Structures and Algorithms:
- **Data Structures**: Hash Table, Bloom Filter, Directory Trie, Binary Max-Heap
- **Algorithms**: BFS / DFS Directory Traversal, SHA-256 / MD5 Fingerprinting, Merge Sort, Quick Sort

---

## Directory Organization

```
Antigravity Claude/
├── CLI/       # Pure Python CLI version (zero third-party dependencies)
├── GUI/       # Modern Desktop GUI version (pywebview + HTML/CSS/JS frontend)
└── docs/      # Capstone report, system specifications, and design plans
```

---

## 1. CLI Version (`CLI/`)

Zero external dependencies (uses Python standard library).

```powershell
cd CLI

# Scan a folder
python main.py "C:\Path\To\Scan"

# Scan with options (BFS traversal, top 10 space-wasters, export JSON)
python main.py "C:\Path\To\Scan" --traversal bfs --top 10 --export report.json --format json

# Run unit tests
python -m pytest tests/ -q
```

---

## 2. GUI Version (`GUI/`)

Interactive desktop interface featuring:
- Monochrome gradient dark/light theme with neumorphic toggle switch
- Real-time 4-phase pipeline visualizer
- Interactive expandable duplicate group treeview
- Quick actions: Open file, Reveal in Explorer, Safe delete with confirmation
- Report exporter (JSON, CSV, TXT)

```powershell
cd GUI

# Install GUI dependency (once)
python -m pip install -r requirements.txt

# Launch GUI
python gui.py
# or
python main.py

# Run unit tests
python -m pytest tests/ -q
```
