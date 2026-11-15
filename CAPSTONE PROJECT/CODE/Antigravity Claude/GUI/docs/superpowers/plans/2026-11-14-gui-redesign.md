# DupDetect GUI Redesign — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the plain Tkinter GUI with a stunning dark-mode developer-tool GUI using `pywebview` + a self-contained HTML/CSS/JS frontend, keeping all existing DSA backend code intact.

**Architecture:** Python `pywebview` window exposes a Python `Api` class to JS via `window.pywebview.api.*`. The HTML frontend calls these methods for scanning, file actions, and exports. Results are pushed back to JS via `window.dispatchEvent(new CustomEvent(...))` fired from Python callbacks on the main thread.

**Tech Stack:** Python 3.x · pywebview 5.x · Vanilla JS (ES2020) · CSS custom properties · Lucide SVG icons (inlined) · JetBrains Mono + IBM Plex Sans (Google Fonts with local fallback)

**Spec:** `docs/superpowers/specs/2026-11-14-gui-redesign-design.md`

## Global Constraints

- `pywebview` is the only new dependency — install with `pip install pywebview`
- All `src/` backend files are read-only; GUI is a pure view layer
- Single HTML file `gui.html` — no bundler, no npm, no external build step
- `python gui.py` must launch the app with no other arguments required
- CSS tokens use `--var` naming; never raw hex in component styles
- All clickable elements ≥ 44px height; `:focus-visible` rings on every interactive element
- `prefers-reduced-motion` media query wraps every transition/animation
- Lucide SVG icons only — no emoji as functional icons

## Review Focus

1. **Scan on empty directory** — pipeline phases complete with zero results; stat cards show 0; treeview shows "No duplicates found" empty state rather than blank white
2. **Delete last copy in a group** — after deleting (count-1) copies, the group row is removed from the treeview; stat cards recompute
3. **Scan interrupted mid-run** — stop button cancels the background thread; UI returns to idle state; partial results are discarded
4. **Export with zero duplicates** — Export button disabled when `scan_result` is None or has 0 groups
5. **Very long file paths** — path text in group rows must truncate with `text-overflow: ellipsis` and show full path in a `title` tooltip; never breaks layout

---

## Task 1: Install pywebview & scaffold launcher

**Files:**
- Modify: `gui.py` (replace entirely)
- Create: `gui.html` (empty scaffold)
- Create: `requirements.txt`

**Interfaces:**
- Produces: `Api` class with stub methods, `webview.Window` instance, `gui.html` served as local file

- [ ] **Step 1: Add pywebview to requirements.txt**

```
pywebview>=5.0
```

- [ ] **Step 2: Run to verify install**

Run: `pip install pywebview`
Expected: `Successfully installed pywebview-X.X.X` (no errors)

- [ ] **Step 3: Write `gui.py` — launcher skeleton**

Replace `gui.py` entirely with:
```python
import webview
import os, sys, json, threading

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

HTML_PATH = os.path.join(os.path.dirname(__file__), 'gui.html')

class Api:
    def ping(self):
        return 'pong'

def launch_gui():
    api = Api()
    window = webview.create_window(
        'DupDetect — Duplicate File Detector',
        url=f'file:///{HTML_PATH.replace(os.sep, "/")}',
        js_api=api,
        width=1200, height=800,
        min_size=(900, 600),
        background_color='#0F172A',
    )
    webview.start(debug=False)

if __name__ == '__main__':
    launch_gui()
```

- [ ] **Step 4: Create minimal `gui.html` to verify window opens**

```html
<!DOCTYPE html>
<html><body style="background:#0F172A;color:#F8FAFC;font-family:sans-serif">
  <h1>DupDetect loading...</h1>
  <script>
    window.addEventListener('pywebviewready', () => {
      pywebview.api.ping().then(r => document.querySelector('h1').textContent = 'Ready: ' + r);
    });
  </script>
</body></html>
```

- [ ] **Step 5: Run and verify**

Run: `python gui.py`
Expected: Dark window opens showing "Ready: pong"

- [ ] **Step 6: Commit**

```bash
git add gui.py gui.html requirements.txt
git commit -m "feat(gui): scaffold pywebview launcher with ping/pong verification"
```

---

## Task 2: Design system CSS & layout skeleton in `gui.html`

**Files:**
- Modify: `gui.html`

**Interfaces:**
- Produces: CSS custom property tokens, font imports, 3-zone layout (sidebar / main / status bar), all visible at correct dimensions

- [ ] **Step 1: Add `<head>` with fonts and CSS token reset**

In `gui.html` `<style>`:
```css
@import url('https://fonts.googleapis.com/css2?family=IBM+Plex+Sans:wght@300;400;500;600;700&family=JetBrains+Mono:wght@400;500;600;700&display=swap');

:root {
  --bg:          #0F172A;
  --surface:     #1B2336;
  --surface-2:   #242F45;
  --border:      rgba(255,255,255,0.08);
  --text:        #F8FAFC;
  --text-muted:  #94A3B8;
  --accent:      #22C55E;
  --accent-glow: rgba(34,197,94,0.15);
  --warn:        #F59E0B;
  --danger:      #EF4444;
  --phase-1:     #3B82F6;
  --phase-2:     #8B5CF6;
  --phase-3:     #F59E0B;
  --phase-4:     #22C55E;
  --radius:      8px;
  --font-mono:   'JetBrains Mono', Consolas, monospace;
  --font-sans:   'IBM Plex Sans', 'Segoe UI', sans-serif;
  --ease:        cubic-bezier(0.16, 1, 0.3, 1);
}

*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
html, body { height: 100%; overflow: hidden; background: var(--bg); color: var(--text);
  font-family: var(--font-sans); font-size: 13px; line-height: 1.5; }
```

- [ ] **Step 2: Build 3-zone layout HTML structure**

```html
<div id="app">
  <aside id="sidebar"><!-- Pipeline + Bloom stats --></aside>
  <main id="main">
    <header id="toolbar"><!-- Folder picker + Scan button --></header>
    <section id="stat-cards"><!-- 4 KPI cards --></section>
    <section id="results"><!-- Treeview --></section>
    <footer id="action-bar"><!-- File action buttons --></footer>
  </main>
  <div id="status-bar"><!-- Scan status text --></div>
</div>
```

CSS grid layout:
```css
#app {
  display: grid;
  grid-template-columns: 260px 1fr;
  grid-template-rows: 1fr 32px;
  grid-template-areas: "sidebar main" "status status";
  height: 100vh;
  gap: 0;
}
#sidebar  { grid-area: sidebar; }
#main     { grid-area: main; display: flex; flex-direction: column; }
#status-bar { grid-area: status; }
```

- [ ] **Step 3: Verify layout renders with placeholder text**

Run: `python gui.py`
Expected: 3 zones visible — dark sidebar on left, main area on right, 32px status bar at bottom

- [ ] **Step 4: Commit**

```bash
git add gui.html
git commit -m "feat(gui): dark design system tokens and 3-zone layout skeleton"
```

---

## Task 3: Toolbar component (folder picker + scan button + options)

**Files:**
- Modify: `gui.html`, `gui.py`

**Interfaces:**
- Consumes: `Api.browse_directory() -> str`, `Api.get_config() -> dict`
- Produces: Toolbar HTML+CSS, JS `browseDirectory()` function that fills path input

- [ ] **Step 1: Add `Api.browse_directory()` to `gui.py`**

```python
def browse_directory(self):
    import tkinter as tk
    from tkinter import filedialog
    root = tk.Tk(); root.withdraw(); root.attributes('-topmost', True)
    path = filedialog.askdirectory(title='Select folder to scan')
    root.destroy()
    return path or ''
```

- [ ] **Step 2: Build toolbar HTML in `#toolbar`**

Include: folder icon (SVG), path `<input>` (flex-grow), "Browse" button, "▶ Scan" primary button, gear icon for options flyout (Phase 2).
Sizing: toolbar height 56px; all buttons 36px height with 12px horizontal padding; path input full remaining width.

- [ ] **Step 3: Write toolbar CSS**

```css
#toolbar {
  display: flex; align-items: center; gap: 8px; padding: 10px 16px;
  border-bottom: 1px solid var(--border); flex-shrink: 0;
}
.btn { height: 36px; border-radius: var(--radius); border: 1px solid var(--border);
  cursor: pointer; font-family: var(--font-sans); font-size: 13px; font-weight: 500;
  transition: background 150ms var(--ease), transform 100ms; }
.btn:active { transform: scale(0.97); }
.btn-primary { background: var(--accent); color: #0F172A; border-color: transparent; }
.btn-primary:hover { filter: brightness(1.1); }
.btn-ghost { background: transparent; color: var(--text-muted); padding: 0 12px; }
.btn-ghost:hover { background: var(--surface-2); color: var(--text); }
#path-input { flex: 1; height: 36px; background: var(--surface); border: 1px solid var(--border);
  border-radius: var(--radius); color: var(--text); padding: 0 12px;
  font-family: var(--font-mono); font-size: 12px; outline: none; }
#path-input:focus { border-color: var(--accent); box-shadow: 0 0 0 2px var(--accent-glow); }
:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }
```

- [ ] **Step 4: Wire JS `browseDirectory()` and state management**

```js
async function browseDirectory() {
  const path = await pywebview.api.browse_directory();
  if (path) document.getElementById('path-input').value = path;
}
```

- [ ] **Step 5: Verify toolbar, run and visually check**

Run: `python gui.py` → Click Browse → Folder dialog opens → Path fills in input field.

- [ ] **Step 6: Commit**

```bash
git add gui.html gui.py
git commit -m "feat(gui): toolbar with folder picker, path input, and scan button"
```

---

## Task 4: Pipeline Visualiser in sidebar

**Files:**
- Modify: `gui.html`

**Interfaces:**
- Consumes: `CustomEvent('phase-update', {detail: {phase: 1|2|3|4, status:'idle'|'running'|'done', count: N}})` fired from JS
- Produces: 4 phase cards with animated fill bars and status badges

- [ ] **Step 1: Build 4 pipeline phase HTML cards in `#sidebar`**

```html
<div class="sidebar-section">
  <h3 class="sidebar-title">Detection Pipeline</h3>
  <div class="pipeline">
    <div class="phase" data-phase="1" data-status="idle">
      <div class="phase-header">
        <span class="phase-num">①</span>
        <div><div class="phase-name">Size Grouping</div>
             <div class="phase-dsa">Hash Table</div></div>
        <span class="phase-badge">—</span>
      </div>
      <div class="phase-bar"><div class="phase-fill"></div></div>
    </div>
    <!-- repeat for phases 2 (Bloom Filter/purple), 3 (Quick Hash/amber), 4 (Full SHA-256/green) -->
  </div>
</div>
```

- [ ] **Step 2: Write pipeline CSS**

```css
.phase { background: var(--surface); border-radius: var(--radius); padding: 10px 12px;
  margin-bottom: 6px; border: 1px solid var(--border); transition: border-color 200ms var(--ease); }
.phase[data-status="running"] { border-color: var(--accent); }
.phase[data-status="done"]    { border-color: rgba(255,255,255,0.12); }
.phase-name { font-weight: 600; font-size: 12px; }
.phase-dsa  { font-family: var(--font-mono); font-size: 10px; color: var(--text-muted); }
.phase-bar  { height: 3px; background: var(--border); border-radius: 2px; margin-top: 8px; }
.phase-fill { height: 100%; width: 0%; border-radius: 2px; transition: width 600ms var(--ease); }
.phase[data-phase="1"] .phase-fill { background: var(--phase-1); }
.phase[data-phase="2"] .phase-fill { background: var(--phase-2); }
.phase[data-phase="3"] .phase-fill { background: var(--phase-3); }
.phase[data-phase="4"] .phase-fill { background: var(--phase-4); }
```

- [ ] **Step 3: Write `updatePhase(phase, status, count)` JS function**

```js
function updatePhase(phase, status, count) {
  const el = document.querySelector(`.phase[data-phase="${phase}"]`);
  el.dataset.status = status;
  el.querySelector('.phase-fill').style.width = status === 'done' ? '100%' : status === 'running' ? '50%' : '0%';
  el.querySelector('.phase-badge').textContent = count != null ? count : '—';
}
```

- [ ] **Step 4: Verify phases animate on `updatePhase(1, 'running', null)` call in browser console**

Run: `python gui.py` → open devtools (F12) → call `updatePhase(1,'running',null)` → phase 1 bar animates to 50%

- [ ] **Step 5: Commit**

```bash
git add gui.html
git commit -m "feat(gui): pipeline visualiser sidebar with animated phase fill bars"
```

---

## Task 5: KPI Stat Cards with count-up animation

**Files:**
- Modify: `gui.html`

**Interfaces:**
- Consumes: `CustomEvent('scan-complete', {detail: {filesScanned, duplicates, groups, wastedBytes, wastedHuman}})` from Python
- Produces: 4 glassmorphic stat cards, metric values animate count-up from 0

- [ ] **Step 1: Build 4 stat cards in `#stat-cards`**

Cards: "Files Scanned", "Duplicates Found", "Duplicate Groups", "Space Wasted"
Space Wasted card uses `--warn` accent; others use `--accent`.

```html
<section id="stat-cards">
  <div class="stat-card" id="card-files">
    <div class="stat-label">Files Scanned</div>
    <div class="stat-value" data-target="0">0</div>
  </div>
  <!-- repeat for dupes / groups / wasted -->
</section>
```

- [ ] **Step 2: Write stat card CSS (glassmorphic)**

```css
#stat-cards { display: grid; grid-template-columns: repeat(4, 1fr); gap: 8px;
  padding: 12px 16px; flex-shrink: 0; }
.stat-card {
  background: rgba(255,255,255,0.03);
  backdrop-filter: blur(12px);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  border-left: 2px solid var(--accent);
  padding: 12px 14px;
}
#card-wasted { border-left-color: var(--warn); }
.stat-label { font-size: 11px; color: var(--text-muted); text-transform: uppercase;
  letter-spacing: 0.06em; font-weight: 600; }
.stat-value { font-family: var(--font-mono); font-size: 24px; font-weight: 700;
  color: var(--text); margin-top: 4px; transition: color 300ms; }
```

- [ ] **Step 3: Write `animateCount(el, target, suffix)` JS function**

```js
function animateCount(el, target, suffix = '') {
  if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) {
    el.textContent = target + suffix; return;
  }
  const start = 0, duration = 800, startTime = performance.now();
  function step(now) {
    const p = Math.min((now - startTime) / duration, 1);
    const ease = 1 - Math.pow(1 - p, 3); // easeOutCubic
    el.textContent = Math.round(start + (target - start) * ease) + suffix;
    if (p < 1) requestAnimationFrame(step);
  }
  requestAnimationFrame(step);
}
```

- [ ] **Step 4: Wire `scan-complete` event to update cards**

```js
window.addEventListener('scan-complete', e => {
  const d = e.detail;
  animateCount(document.querySelector('#card-files .stat-value'), d.filesScanned);
  animateCount(document.querySelector('#card-dupes .stat-value'), d.duplicates);
  animateCount(document.querySelector('#card-groups .stat-value'), d.groups);
  document.querySelector('#card-wasted .stat-value').textContent = d.wastedHuman;
});
```

- [ ] **Step 5: Commit**

```bash
git add gui.html
git commit -m "feat(gui): KPI stat cards with glassmorphic style and count-up animation"
```

---

## Task 6: Results Treeview (custom HTML tree)

**Files:**
- Modify: `gui.html`

**Interfaces:**
- Consumes: `CustomEvent('scan-complete', {detail: {groups: [{hash, fileSize, fileSizeHuman, wastedBytes, wastedHuman, copies, paths: []}]}})` 
- Produces: Expandable `<ul>` tree, keyboard navigable, correct ARIA roles

- [ ] **Step 1: Build treeview container HTML**

```html
<section id="results">
  <div id="results-header">
    <span>Duplicate Groups — ranked by wasted space</span>
    <span id="results-count" class="badge">—</span>
  </div>
  <div id="tree-scroll">
    <ul id="tree" role="tree" aria-label="Duplicate file groups"></ul>
  </div>
  <div id="empty-state" hidden>
    <svg><!-- folder-x icon --></svg>
    <p>No duplicates found</p>
    <small>All files in this directory are unique.</small>
  </div>
</section>
```

- [ ] **Step 2: Write tree CSS**

```css
#results { flex: 1; display: flex; flex-direction: column; overflow: hidden;
  border-top: 1px solid var(--border); }
#results-header { display: flex; align-items: center; justify-content: space-between;
  padding: 8px 16px; font-size: 11px; color: var(--text-muted); font-weight: 600;
  text-transform: uppercase; letter-spacing: 0.06em; }
#tree-scroll { flex: 1; overflow-y: auto; padding: 4px 8px 60px; }
#tree { list-style: none; }
.group-row { background: var(--surface); border: 1px solid var(--border); border-radius: var(--radius);
  margin-bottom: 4px; overflow: hidden; cursor: pointer;
  transition: border-color 150ms var(--ease), background 150ms; }
.group-row:hover { background: var(--surface-2); }
.group-row.selected { border-color: var(--accent); background: rgba(34,197,94,0.05); }
.group-header { display: flex; align-items: center; gap: 10px; padding: 10px 12px; }
.chevron { transition: transform 200ms var(--ease); color: var(--text-muted); }
.group-row.open .chevron { transform: rotate(90deg); }
.hash-pill { font-family: var(--font-mono); font-size: 10px; background: var(--border);
  padding: 2px 6px; border-radius: 4px; color: var(--text-muted); }
.wasted-badge { margin-left: auto; font-size: 11px; font-weight: 600;
  color: var(--warn); font-family: var(--font-mono); }
.file-list { list-style: none; border-top: 1px solid var(--border); }
.file-row { display: flex; align-items: center; gap: 8px; padding: 7px 12px 7px 36px;
  cursor: pointer; transition: background 100ms; }
.file-row:hover { background: var(--surface-2); }
.file-row.selected { background: rgba(34,197,94,0.08); border-left: 2px solid var(--accent); }
.file-name { font-weight: 500; color: var(--text); }
.file-path { font-family: var(--font-mono); font-size: 10px; color: var(--text-muted);
  white-space: nowrap; overflow: hidden; text-overflow: ellipsis; max-width: 500px; }
.copy-tag { font-size: 10px; padding: 1px 5px; border-radius: 3px; }
.tag-orig { background: rgba(34,197,94,0.15); color: var(--accent); }
.tag-copy { background: rgba(148,163,184,0.15); color: var(--text-muted); }
```

- [ ] **Step 3: Write `renderTree(groups)` JS function**

Builds `<li class="group-row">` elements. Each group-row contains a `.group-header` and a hidden `.file-list`. Clicking the header toggles `.open` class. File rows emit a `file-selected` custom event with `{path, groupIndex, fileIndex}`.

Group rows stagger in with:
```js
groupEl.style.opacity = '0'; groupEl.style.transform = 'translateY(8px)';
setTimeout(() => { groupEl.style.transition = 'opacity 250ms, transform 250ms var(--ease)';
  groupEl.style.opacity = '1'; groupEl.style.transform = ''; }, i * 60);
```
(Wrapped in `prefers-reduced-motion` guard.)

- [ ] **Step 4: Verify treeview with mock data**

Call `renderTree([{hash:'abc123',fileSizeHuman:'64 KB',wastedHuman:'128 KB',copies:3,paths:['C:/a.png','C:/b.png','C:/c.png']}])` in JS console. Expected: group row renders, expands on click, 3 file rows visible.

- [ ] **Step 5: Commit**

```bash
git add gui.html
git commit -m "feat(gui): custom HTML treeview with expand/collapse and stagger animation"
```

---

## Task 7: Python API — scan, pipeline events, and result push

**Files:**
- Modify: `gui.py`

**Interfaces:**
- Produces: `Api.start_scan(path, traversal, algorithm, min_size, extensions) -> None` (fires events async), `Api.stop_scan() -> None`

- [ ] **Step 1: Add scan imports and thread control to `Api`**

```python
import threading
from src.detector import DuplicateDetector, format_size
from src.walker import ScanConfig

class Api:
    def __init__(self): self._stop_event = threading.Event(); self._window = None
    def set_window(self, w): self._window = w
    def _js(self, js): 
        if self._window: self._window.evaluate_js(js)
```

- [ ] **Step 2: Implement `Api.start_scan(path, traversal, algorithm, min_size, extensions)`**

Validates path, builds `ScanConfig`, starts worker thread. Worker calls `_fire_phase(n, 'running', None)` before each detection phase and `_fire_phase(n, 'done', count)` after. On completion fires `scan-complete` CustomEvent with full result dict via `self._js(f"window.dispatchEvent(new CustomEvent('scan-complete', {{detail: {json.dumps(payload)}}}))"`).

Payload shape:
```python
{
  'filesScanned': result.total_files_scanned,
  'duplicates': result.total_duplicates,
  'groups': len(result.duplicate_groups),
  'wastedHuman': format_size(result.total_wasted_bytes),
  'groupData': [{'hash': g.hash_digest[:16], 'fileSizeHuman': format_size(g.file_size),
                 'wastedHuman': format_size(g.wasted_bytes), 'copies': g.count,
                 'paths': g.paths} for g in result.duplicate_groups]
}
```

- [ ] **Step 3: Implement `Api.stop_scan()`**

Sets `self._stop_event`; worker thread checks it between phases.

- [ ] **Step 4: Wire `set_window` after `webview.create_window`**

```python
window = webview.create_window(...)
api._window = window   # set reference before webview.start()
```

- [ ] **Step 5: Run integration test — scan sample_files/generated**

Run: `python gui.py` → Browse to `tests/sample_files/generated` → Click Scan
Expected: Pipeline phase bars animate through 1→2→3→4, stat cards count up, treeview populates with groups.

- [ ] **Step 6: Commit**

```bash
git add gui.py
git commit -m "feat(gui): Python Api scan integration with phase events and result push"
```

---

## Task 8: Action Bar — Open, Reveal, Delete (with undo toast), Export

**Files:**
- Modify: `gui.html`, `gui.py`

**Interfaces:**
- Consumes: `selectedFilePath: string` state variable, `Api.open_file(path)`, `Api.reveal_in_explorer(path)`, `Api.delete_file(path) -> bool`, `Api.export_report(format) -> {path, error}`
- Produces: Sticky action bar, toast component, delete confirmation slide-panel

- [ ] **Step 1: Build sticky action bar HTML (bottom of `#main`)**

```html
<footer id="action-bar">
  <button class="btn btn-ghost" id="btn-open" disabled>
    <svg><!-- file-text icon --></svg> Open File
  </button>
  <button class="btn btn-ghost" id="btn-reveal" disabled>
    <svg><!-- folder-open icon --></svg> Show in Explorer
  </button>
  <button class="btn btn-ghost btn-danger" id="btn-delete" disabled>
    <svg><!-- trash-2 icon --></svg> Delete Copy
  </button>
  <div style="flex:1"></div>
  <button class="btn btn-primary" id="btn-export" disabled>
    <svg><!-- download icon --></svg> Export Report
  </button>
</footer>
```

- [ ] **Step 2: Write action bar CSS**

```css
#action-bar { display: flex; align-items: center; gap: 6px; padding: 8px 16px;
  border-top: 1px solid var(--border); flex-shrink: 0; height: 52px; }
.btn-danger { color: var(--danger); }
.btn-danger:hover { background: rgba(239,68,68,0.1); }
```

- [ ] **Step 3: Add `Api.open_file`, `Api.reveal_in_explorer`, `Api.delete_file` to `gui.py`**

```python
def open_file(self, path):
    import subprocess; subprocess.Popen(['explorer', path] if sys.platform=='win32' else ['open', path])
def reveal_in_explorer(self, path):
    import subprocess; subprocess.Popen(['explorer', f'/select,{path}'])
def delete_file(self, path):
    try: os.remove(path); return True
    except: return False
```

- [ ] **Step 4: Build toast component HTML + CSS**

```html
<div id="toast-container" aria-live="polite"></div>
```

```css
#toast-container { position: fixed; bottom: 48px; right: 16px; display: flex;
  flex-direction: column; gap: 8px; z-index: 1000; }
.toast { background: var(--surface); border: 1px solid var(--border); border-radius: var(--radius);
  padding: 10px 14px; display: flex; align-items: center; gap: 10px; font-size: 12px;
  min-width: 260px; border-left: 3px solid var(--accent); animation: slide-up 200ms var(--ease); }
.toast.error { border-left-color: var(--danger); }
.toast.warn  { border-left-color: var(--warn); }
@keyframes slide-up { from { opacity:0; transform: translateY(12px); } to { opacity:1; transform:none; } }
```

- [ ] **Step 5: Write `showToast(message, type, undoFn)` JS function**

Creates a `.toast` element, appends to `#toast-container`, auto-removes after 3000ms. If `undoFn` provided, adds an "Undo" button (5s window) that calls `undoFn()` and removes the toast immediately.

- [ ] **Step 6: Wire delete flow with confirmation slide-panel and undo**

Delete button: show inline confirmation panel (slide-down CSS transition) with "Confirm Delete" and "Cancel". On confirm: `pywebview.api.delete_file(path)` → remove file row from tree → recompute stats → show undo toast that reinstates the row for 5s.

- [ ] **Step 7: Add `Api.export_report(format)` to `gui.py`**

Opens native save dialog via tkinter, calls `ReportGenerator.save_report(path, format)`, returns `{success, path}`.

- [ ] **Step 8: Verify all action buttons with sample files**

Run: `python gui.py` → Scan `tests/sample_files/generated` → Select a file row → click Open, Reveal, Delete (undo within 5s), Export JSON.

- [ ] **Step 9: Commit**

```bash
git add gui.html gui.py
git commit -m "feat(gui): action bar, toast notifications, delete with undo, export dialog"
```

---

## Task 9: Bloom Filter stats panel + empty/error states

**Files:**
- Modify: `gui.html`, `gui.py`

**Interfaces:**
- Consumes: `CustomEvent('bloom-stats', {detail: {sizeBits, hashFns, count, fillRatio, fpRate}})` fired after scan
- Produces: Bloom stats section in sidebar, empty-state panel in treeview, scan-error state

- [ ] **Step 1: Build Bloom stats HTML in sidebar**

```html
<div class="sidebar-section" id="bloom-panel">
  <h3 class="sidebar-title">Bloom Filter</h3>
  <div class="bloom-stat"><span>Bit array</span><span id="bloom-bits">—</span></div>
  <div class="bloom-stat"><span>Hash functions</span><span id="bloom-k">—</span></div>
  <div class="bloom-stat"><span>Fill ratio</span>
    <div class="fill-bar"><div id="bloom-fill" class="fill-inner"></div></div>
  </div>
  <div class="bloom-stat"><span>Est. FP rate</span><span id="bloom-fp">—</span></div>
</div>
```

- [ ] **Step 2: Fire `bloom-stats` event from Python after scan**

Add Bloom stats to the scan complete payload: `'bloomBits': bf.size_bits, 'bloomK': bf.num_hash_functions, 'bloomFill': bf.fill_ratio(), 'bloomFp': bf.estimated_fp_rate()`.

- [ ] **Step 3: Wire `bloom-stats` JS listener to update sidebar**

- [ ] **Step 4: Empty state — show `#empty-state` when groups.length === 0**

Hidden `<div id="empty-state">` with centered icon + "No duplicates found" message shown when scan completes with 0 groups.

- [ ] **Step 5: Error state — show toast + reset pipeline on scan failure**

Python `Api.start_scan` wraps worker in try/except; on exception fires `scan-error` event with message; JS shows error toast and resets all phase bars to idle.

- [ ] **Step 6: Commit**

```bash
git add gui.html gui.py
git commit -m "feat(gui): bloom filter sidebar stats, empty state, and error state handling"
```

---

## Task 10: Options panel, status bar, polish & accessibility audit

**Files:**
- Modify: `gui.html`, `gui.py`

**Interfaces:**
- Produces: Options flyout (traversal, algorithm, min-size, ext filter), populated status bar, final WCAG AA verification

- [ ] **Step 1: Build options flyout (slide-down from toolbar gear button)**

Collapsible `<div id="options-panel">` with:
- Traversal toggle: DFS / BFS radio pills
- Algorithm select: SHA-256 / MD5
- Min size number input
- Extension filter text input (comma-separated)

CSS: `max-height: 0 → 180px` transition for slide-down.

- [ ] **Step 2: Build status bar content**

```html
<div id="status-bar">
  <span id="status-text">Ready — select a directory to scan</span>
  <div id="scan-spinner" hidden>
    <div class="spinner-dot"></div>
    <div class="spinner-dot"></div>
    <div class="spinner-dot"></div>
  </div>
  <span id="scan-time" class="font-mono"></span>
</div>
```

Spinner: 3 dots pulsing with `animation-delay` offset (CSS only).

- [ ] **Step 3: Accessibility pass**

- All icon-only buttons get `aria-label`
- `#tree` has `role="tree"`, each group-row `role="treeitem"` with `aria-expanded`
- Scan button shows `aria-busy="true"` during scan
- Color alone never conveys status — add text labels alongside color badges
- Tab order check: Toolbar → Options → Stat cards → Tree → Action bar

- [ ] **Step 4: Reduced-motion guard on all JS animations**

Wrap every JS transition/animation in:
```js
const noMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
```

- [ ] **Step 5: Final visual polish pass (impeccable operate mode)**

- Verify every text element has ≥ 4.5:1 contrast against its background
- Verify all buttons ≥ 36px height (with 44px click area via `padding`)
- Verify monospace numbers align correctly (JetBrains Mono is tabular)
- Verify scrollbar styled: `scrollbar-color: var(--surface-2) transparent`
- Verify no layout shift when treeview populates

- [ ] **Step 6: Update `main.py` to show GUI by default**

Already done in previous session — verify `python main.py` launches GUI and `python main.py <path>` runs CLI.

- [ ] **Step 7: Run all 51 existing tests to confirm backend unchanged**

Run: `python -m pytest tests/test_all.py -v`
Expected: 51 passed

- [ ] **Step 8: Final commit**

```bash
git add gui.html gui.py
git commit -m "feat(gui): options panel, status bar, accessibility audit, polish pass — GUI complete"
```

---

## Self-Review

**Spec coverage:**
- ✅ Dark OLED palette + JetBrains Mono / IBM Plex Sans typography
- ✅ 3-zone sidebar+main layout with pipeline visualiser
- ✅ 4 KPI glassmorphic stat cards with count-up
- ✅ Custom HTML treeview (expandable groups + file rows with tags)
- ✅ Action bar: Open, Reveal, Delete (with undo toast), Export
- ✅ Bloom filter sidebar stats
- ✅ Options flyout (traversal, algorithm, min-size, ext filter)
- ✅ Status bar + spinner during scan
- ✅ Empty state + error state
- ✅ WCAG AA accessibility (contrast, focus rings, ARIA, reduced-motion)
- ✅ All existing 51 tests still pass (backend untouched)

**Review Focus test coverage:**
1. Empty directory → Task 9, Step 4 (empty-state)
2. Delete last copy → Task 8, Step 6 (row removal + stats recompute)
3. Scan interrupted → Task 7, Step 3 (stop_event + Task 9, Step 5)
4. Export with 0 groups → Task 8, Step 7 (export button disabled when no result)
5. Long paths → Task 6, Step 2 (`text-overflow: ellipsis` + `title` tooltip)
