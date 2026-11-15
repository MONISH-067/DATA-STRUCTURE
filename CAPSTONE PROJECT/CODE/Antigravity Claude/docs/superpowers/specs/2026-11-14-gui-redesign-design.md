# DupDetect GUI — Design Specification

**Date:** 2026-11-14  
**Mode (impeccable):** Operate — user completes a task (scanning, reviewing, deleting duplicates)  
**Surface:** Desktop GUI application (Python / HTML+CSS rendered via webview OR Tkinter canvas)

---

## Intent & Why

The existing `gui.py` is functional but visually generic — plain Tkinter grey, unstyled treeview, no hierarchy, no brand identity. The user wants the **most attractive and perfect UI** for this DSA capstone project.

The new GUI must:
- Look like a **professional developer tool** (the bar is VS Code's File Explorer panel + Linear's issue list)
- Demonstrate that the DSA backend (Bloom Filter, Hash Table, Heap, Trie) is at work — make the pipeline *visible*
- Let students/professors immediately understand the data flow, results, and wasted space
- Be **complete enough to submit** as a capstone project without embarrassment

---

## Goals / Non-Goals / Anti-Goals

| | Item |
|--|------|
| ✅ Goal | Visually stunning dark-mode desktop GUI |
| ✅ Goal | Real-time scan pipeline visualisation (phases animate as they complete) |
| ✅ Goal | Expandable duplicate group tree with full path display |
| ✅ Goal | KPI dashboard cards (Scanned, Duplicates, Groups, Space Wasted) |
| ✅ Goal | In-app file actions (Open, Reveal in Explorer, Delete with undo toast) |
| ✅ Goal | Export report (JSON / CSV / TXT) via native save dialog |
| ✅ Goal | WCAG AA accessible (4.5:1 contrast, keyboard nav, focus rings) |
| ✅ Goal | Non-blocking background threading (UI never freezes) |
| ❌ Non-goal | Web deployment / browser-based version |
| ❌ Non-goal | Cloud storage integration |
| ❌ Anti-goal | Generic Bootstrap / Material template look |
| ❌ Anti-goal | Emoji used as functional icons |
| ❌ Anti-goal | Light mode default |

---

## Constraints

- **Stack:** Python 3.x, standard library only for the GUI layer. Options are Tkinter (ttk + canvas) or a lightweight HTML webview (`tkinterweb` / `pywebview`).
- **No new heavy dependencies** (avoid PyQt5, wxPython, Electron — installer complexity kills the capstone grade).
- **Must run** with `python gui.py` from the project root.
- **Reuses all existing backend** (`src/detector.py`, `src/walker.py`, etc.) — GUI is a pure view layer on top.
- Background thread already exists in `gui.py` — keep it, improve it.

---

## Approaches Considered

### Approach A — Enhanced Tkinter (ttk + Canvas painting)
Style Tkinter's ttk widgets with a custom dark theme using `ttk.Style`, paint custom elements (pipeline bar, stat cards) on a `tk.Canvas`. Use `tkinter.font` for JetBrains Mono / IBM Plex Sans fallback.

**Trade-off:** Tkinter is severely limited for custom visuals. The pipeline progress bar, glassmorphic cards, and stagger animations are very hard without painting every pixel manually. Result will look dated.

### Approach B — Embedded HTML/CSS/JS inside Python via `pywebview` ⭐ CHOSEN
Render a **fully custom HTML+CSS GUI** inside a native OS window using `pywebview`. The frontend is beautiful HTML with CSS custom properties, the backend is Python. Communication happens via `window.pywebview.api.*` calls (the Python `api` object is exposed to JS).

**Why this wins:**
- Zero visual constraints — full CSS, `backdrop-filter`, CSS grid, smooth transitions, Web Fonts via local fallback
- Design system from the UI/UX search can be applied 1:1 (JetBrains Mono, dark OLED palette, glassmorphism cards)
- The DSA pipeline phases can be animated with CSS transitions as Python fires JS events
- File is still one `gui.py` + one `gui.html` — still runs with `python gui.py`

**Install:** `pip install pywebview` — one dependency, cross-platform (Windows/Mac/Linux).

### Approach C — Tkinter + ttkthemes
Use `ttkthemes` with the `arc` or `equilux` dark theme for a quick visual upgrade.

**Trade-off:** Still Tkinter constraints, still no glassmorphism/custom pipeline visualizer. Better than A, far worse than B.

---

## Design System (from UI/UX search)

### Palette — Dark OLED + Code Green
```css
--bg:          #0F172A;   /* deep navy black */
--surface:     #1B2336;   /* card surface */
--surface-2:   #242F45;   /* hover / elevated */
--border:      rgba(255,255,255,0.08);
--text:        #F8FAFC;   /* primary text */
--text-muted:  #94A3B8;   /* secondary labels */
--accent:      #22C55E;   /* scan green / CTA */
--accent-glow: rgba(34,197,94,0.15);
--warn:        #F59E0B;   /* caution / wasted space */
--danger:      #EF4444;   /* delete / error */
--phase-1:     #3B82F6;   /* size-group blue */
--phase-2:     #8B5CF6;   /* bloom filter purple */
--phase-3:     #F59E0B;   /* quick-hash amber */
--phase-4:     #22C55E;   /* full-hash green */
```

### Typography — Developer Mono
- **Headings / Labels / Hash Digests:** JetBrains Mono (bundled fallback: Consolas)
- **Body / Descriptions / Paths:** IBM Plex Sans (bundled fallback: Segoe UI)
- Base size: 13px (dense dashboard), line-height: 1.5
- Metric values: JetBrains Mono 24px bold

### Motion — Standard (6/10)
- Transitions: `cubic-bezier(0.16, 1, 0.3, 1)` at 150–250ms
- Stat card count-up: `CountUp.js`-style JS animation on scan complete
- Pipeline phase bars: width transitions with staggered `transitionDelay`
- Duplicate group rows: stagger-in with `opacity 0→1 + translateY(8px→0)` at 60ms intervals
- Button active: `transform: scale(0.97)` at 100ms
- `prefers-reduced-motion` bypass: skip all transitions, render final state

### Layout — 3-Zone Sidebar + Main
```
┌─────────────────────────────────────────────────────────────────────┐
│  Title bar: "DupDetect"  ·  DSA Capstone  ·  [min][max][close]     │
├──────────────┬──────────────────────────────────────────────────────┤
│              │  ┌──────────────────────────────────────────────┐   │
│  LEFT PANEL  │  │  HEADER TOOLBAR                              │   │
│  ─────────── │  │  [📂 Folder path input]  [▶ Scan] [⚙ Opts] │   │
│  Pipeline    │  └──────────────────────────────────────────────┘   │
│  Visualizer  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────────────┐      │
│  (4 phases)  │  │Files │ │Dupes │ │Groups│ │Space Wasted  │      │
│              │  │      │ │      │ │      │ │              │      │
│  ─────────── │  └──────┘ └──────┘ └──────┘ └──────────────┘      │
│  Bloom Stats │  ┌──────────────────────────────────────────────┐   │
│  (bit fill)  │  │  RESULTS TREEVIEW                            │   │
│              │  │  Group #1 ▼  3 copies · 64KB · 128KB wasted │   │
│  ─────────── │  │    ├─ [original]  path/to/file.png          │   │
│  Quick Tips  │  │    ├─ [copy #2]   path/to/copy.png          │   │
│              │  │    └─ [copy #3]   another/path.png          │   │
│              │  │  Group #2 ▶  ...                            │   │
│              │  └──────────────────────────────────────────────┘   │
│              │  ┌──────────────────────────────────────────────┐   │
│              │  │  ACTION BAR: [Open][Explorer][Delete][Export]│   │
├──────────────┴──────────────────────────────────────────────────────┤
│  STATUS BAR: "Scanned 62 files in 0.02s · 21 groups · 509 KB"     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Component Specifications

### 1. Pipeline Visualiser (Left Panel)
Four numbered phase cards, each showing:
- Phase number + name (e.g. "① Size Grouping")
- DSA label (e.g. "Hash Table")
- Status: `idle | running | done`
- Animated fill bar (width transitions from 0→100% when phase completes)
- Count badge (e.g. "8 groups")

Colour-coded: phase 1=blue, 2=purple, 3=amber, 4=green.

### 2. KPI Stat Cards (4 × horizontal)
- Glassmorphic surface (`background: rgba(255,255,255,0.03); backdrop-filter: blur(12px)`)
- JetBrains Mono metric value animates count-up from 0 on scan complete
- IBM Plex Sans label beneath
- Accent border-left (2px) colour-coded: neutral / neutral / neutral / `--warn` for wasted space

### 3. Results Treeview
Custom HTML `<ul>` tree (not a native `<select>` or Tkinter Treeview):
- Group rows: clickable header showing hash snippet, copy count, file size, wasted space badge
- Expand/collapse `▶/▼` chevron with CSS rotate transition
- File rows: file type icon (SVG), name in bold, full path in `--text-muted`
- Selected row: `--surface-2` background + `--accent` left border
- Hover: 150ms background transition
- Multi-select via checkboxes (phase 2 feature; single select for MVP)

### 4. Action Bar
Sticky bottom bar with four icon-labelled buttons:
- 📄 **Open File** — `os.startfile(path)` via pywebview API
- 📁 **Reveal** — `explorer /select, path`
- 🗑 **Delete** — confirmation slide-down panel (not modal), then animated row removal + undo toast (5s)
- 💾 **Export** — native file dialog, three format tabs (TXT / JSON / CSV)

No emoji as icons — use Lucide SVG icons (self-contained, bundled inline).

### 5. Bloom Filter Mini-Stats (Left Panel)
Compact visual showing:
- Bit array fill ratio as a horizontal segmented bar
- Hash function count `k=7`
- Estimated FP rate
- Items inserted counter

### 6. Toast Notifications
Slide-up from bottom-right, 3s auto-dismiss, manual ✕:
- Success: green left border
- Error: red left border
- Undo: amber + "Undo Delete" button (5s window)

---

## Accessibility (WCAG AA — Mandatory)

| Requirement | Implementation |
|-------------|----------------|
| Contrast 4.5:1 | All text on `--bg` and `--surface` verified |
| Focus rings | `:focus-visible` with `--accent` 2px offset ring |
| Keyboard nav | Tab order: toolbar → stat cards → treeview → action bar |
| Screen reader | `aria-label` on all icon buttons, `role="tree"` on treeview, `aria-busy` during scan |
| Reduced motion | `@media (prefers-reduced-motion: reduce)` removes all transitions |
| Min touch target | All buttons ≥ 44×44px equivalent |

---

## File Structure (after implementation)

```
duplicate-file-detector/
├── gui.py                    # pywebview launcher + Python API class
├── gui.html                  # single-file HTML/CSS/JS GUI
└── src/                      # unchanged backend
```

The HTML is a **single self-contained file** (no webpack, no bundler):
- Google Fonts loaded from `<link>` with local fallback stack
- Lucide icons inlined as SVG `<symbol>` sprite
- All CSS in `<style>` block with CSS custom properties
- All JS in `<script>` block (vanilla JS, no framework dependency)
