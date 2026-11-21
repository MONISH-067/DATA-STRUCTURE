# Datastructure Duplicate file Detection system (DDD) — DSA Capstone Project

This project implements a high-performance multi-phase duplicate file detection and safe cleanup platform using custom Data Structures and Algorithms:
- **Data Structures**: Separate Chaining Hash Table, Bloom Filter, Directory Trie, Binary Max-Heap
- **Algorithms**: BFS / DFS Directory Traversal, Two-Phase Hashing (4KB Quick Hash + SHA-256), Merge Sort, Quick Sort
- **Safety Engine**: Checkbox multi-file staging, 1-click batch cleanup with danger confirmation alertdialog, immutable original (`paths[0]`) protection
- **User Interface**: High-fidelity Neumorphic Dark/Light theme with chromatic 3D shadows and WCAG 2.2 AA accessibility

---

## Directory Organization

```
CODE/
├── GUI/            # Desktop GUI application (pywebview + Neumorphic HTML5/CSS3/JS UI)
├── CLI/            # High-performance CLI engine (pure Python standard library)
├── DOCUMENTATION/  # Presentations (.pptx), capstone reports, design specs & brand assets
├── gui.py          # Root convenience forwarder for GUI
├── main.py         # Root convenience forwarder for CLI
├── run_gui.bat     # 1-click launcher for GUI
├── run_cli.bat     # 1-click launcher for CLI
└── README.md       # Project documentation
```

---

## 1. GUI Application (`GUI/`)

Interactive desktop interface featuring:
- Monochrome gradient dark/light theme with high-fidelity Neumorphic toggle switch
- Real-time pipeline visualizer and telemetry
- Interactive expandable duplicate group treeview
- Checkbox multi-file staging and 1-click "Delete All Duplicates" with safety modal
- Report exporter (JSON, CSV, TXT)

```powershell
# 1-click launch from root:
run_gui.bat

# Or from GUI directory:
cd GUI
python -m pip install -r requirements.txt
python gui.py

# Run GUI unit tests (51/51 passing)
python -m pytest tests/ -q
```

---

## 2. CLI Application (`CLI/`)

Zero external dependencies (pure Python standard library).

```powershell
# 1-click launch from root:
run_cli.bat

# Or from CLI directory:
cd CLI

# Scan a folder
python main.py "C:\Path\To\Scan"

# Scan with options (BFS traversal, top 10 space-wasters, export JSON)
python main.py "C:\Path\To\Scan" --traversal bfs --top 10 --export report.json --format json

# Run CLI unit tests (51/51 passing)
python -m pytest tests/ -q
```

---

## 3. Documentation & Presentations (`DOCUMENTATION/`)

All capstone documentation and presentation materials are organized here:
- `Duplicate_File_Detection_System.pptx`: 16-slide presentation matching institutional template
- `Datastructure_Duplicate_File_Detection_System.pptx`: 20-slide comprehensive updated presentation
- `capstone-report.md`: Technical capstone engineering report
- `brand/`: Brand guidelines, Concept B "Layered Cascade" logo SVG suite, presentation board, and 53 exports
- `specs/` & `plans/`: Architectural redesign specifications and implementation plans

