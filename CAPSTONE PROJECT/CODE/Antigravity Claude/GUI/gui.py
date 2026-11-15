"""
Duplicate File Detection System — pywebview GUI Launcher
=========================================================
Launches a native OS window (Chromium webview) rendering gui.html.
Python exposes an `Api` class to JavaScript via window.pywebview.api.
Events are pushed from Python to JS via window.evaluate_js().

Run with:  python gui.py
"""

from __future__ import annotations

import json
import math
import os
import subprocess
import sys
import threading
import time
import tkinter as tk
from tkinter import filedialog
from typing import Optional

# Ensure project root is on path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    import webview  # pywebview ≥ 5
except ImportError:
    print("[ERROR] pywebview is not installed. Run: python -m pip install pywebview")
    sys.exit(1)

from src.detector import DuplicateDetector, ScanResult, DuplicateGroup, format_size
from src.walker import ScanConfig
from src.report import ReportGenerator


# ─────────────────────────────────────────────────────────────────────────────
# API CLASS  — All public methods callable from JS as window.pywebview.api.*
# ─────────────────────────────────────────────────────────────────────────────

class Api:
    """Bridge between JavaScript (gui.html) and Python backend."""

    def __init__(self):
        self._window: Optional[webview.Window] = None
        self._scan_thread: Optional[threading.Thread] = None
        self._stop_event = threading.Event()
        self._scan_result: Optional[ScanResult] = None
        self._pending_delete: Optional[str] = None   # path staged for undo

    # ── window reference (set after create_window) ──────────────────────────
    def set_window(self, window: webview.Window) -> None:
        self._window = window

    # ── helper: push JS event to the webview ────────────────────────────────
    def _js(self, event_name: str, detail: dict) -> None:
        payload = json.dumps(detail, ensure_ascii=False).replace("\\", "\\\\").replace("`", "\\`")
        self._window.evaluate_js(
            f"window.dispatchEvent(new CustomEvent({json.dumps(event_name)}, {{detail: JSON.parse(`{payload}`)}}));"
        )

    # ── ping (startup verification) ─────────────────────────────────────────
    def ping(self) -> str:
        return "pong"

    # ── directory picker (native dialog via tkinter) ─────────────────────────
    def browse_directory(self) -> str:
        """Open a native folder-picker dialog and return chosen path."""
        root = tk.Tk()
        root.withdraw()
        root.attributes("-topmost", True)
        chosen = filedialog.askdirectory(title="Select Folder to Scan")
        root.destroy()
        return chosen or ""

    # ── scan ─────────────────────────────────────────────────────────────────
    def start_scan(
        self,
        path: str,
        traversal: str = "dfs",
        algorithm: str = "sha256",
        min_size: int = 1,
        extensions: str = "",
    ) -> None:
        """Start the detection pipeline in a background thread."""
        if self._scan_thread and self._scan_thread.is_alive():
            return

        self._stop_event.clear()
        self._scan_result = None

        include_ext = None
        if extensions.strip():
            include_ext = {
                e.strip().lower() if e.strip().startswith(".") else f".{e.strip().lower()}"
                for e in extensions.split(",")
                if e.strip()
            }

        config = ScanConfig(min_size=int(min_size), include_extensions=include_ext)

        self._scan_thread = threading.Thread(
            target=self._run_scan,
            args=(path, traversal, algorithm, config),
            daemon=True,
        )
        self._scan_thread.start()

    def stop_scan(self) -> None:
        """Signal the scan thread to abort (best-effort)."""
        self._stop_event.set()

    def _run_scan(self, path: str, traversal: str, algorithm: str, config: ScanConfig) -> None:
        """Worker — runs in background thread, fires JS events."""
        try:
            # ── Phase 1 event ────────────────────────────────────────────
            self._js("phase-update", {"phase": 1, "status": "running", "count": 0})

            detector = DuplicateDetector(
                config=config, traversal=traversal, hash_algorithm=algorithm
            )

            # Monkey-patch progress: wrap scan() with phase callbacks
            # We use time.sleep(0) calls to yield; real phases fire via result fields
            result: ScanResult = detector.scan(path)

            if self._stop_event.is_set():
                self._js("scan-stopped", {})
                return

            # ── Synthetic phase updates from result stats ─────────────────
            self._js("phase-update", {"phase": 1, "status": "done",    "count": result.phase1_size_groups})
            self._js("phase-update", {"phase": 2, "status": "done",    "count": result.phase2_bloom_candidates})
            self._js("phase-update", {"phase": 3, "status": "done",    "count": result.phase3_quick_hash_groups})
            self._js("phase-update", {"phase": 4, "status": "done",    "count": result.phase4_full_hash_groups})

            # ── Bloom stats ───────────────────────────────────────────────
            bloom = detector.bloom_filter
            bloom_detail = {
                "bits":    bloom.size_bits,
                "k":       bloom.num_hash_functions,
                "fill":    round(bloom.fill_ratio() * 100, 2),
                "fp":      round(bloom.estimated_fp_rate() * 100, 4),
                "items":   bloom.count,
            }

            # ── Serialise groups for JS ───────────────────────────────────
            groups = []
            for g in result.duplicate_groups:
                groups.append({
                    "hash":    g.hash_digest,
                    "size":    g.file_size,
                    "sizeStr": format_size(g.file_size),
                    "wasted":  g.wasted_bytes,
                    "wastedStr": format_size(g.wasted_bytes),
                    "count":   g.count,
                    "paths":   g.paths,
                })

            self._scan_result = result

            self._js("scan-complete", {
                "filesScanned":  result.total_files_scanned,
                "duplicates":    result.total_duplicates,
                "groups":        len(result.duplicate_groups),
                "wastedBytes":   result.total_wasted_bytes,
                "wastedStr":     format_size(result.total_wasted_bytes),
                "elapsedSec":    round(result.elapsed_seconds, 2),
                "groupData":     groups,
                "bloom":         bloom_detail,
            })

        except Exception as exc:
            self._js("scan-error", {"message": str(exc)})

    # ── file actions ─────────────────────────────────────────────────────────
    def open_file(self, path: str) -> dict:
        if not os.path.exists(path):
            return {"ok": False, "error": "File not found"}
        try:
            os.startfile(path)
            return {"ok": True}
        except Exception as e:
            return {"ok": False, "error": str(e)}

    def reveal_in_explorer(self, path: str) -> dict:
        if not os.path.exists(path):
            return {"ok": False, "error": "File not found"}
        try:
            subprocess.run(["explorer", f"/select,{os.path.normpath(path)}"])
            return {"ok": True}
        except Exception as e:
            return {"ok": False, "error": str(e)}

    def stage_delete(self, path: str) -> dict:
        """Stage a file for deletion (returns True if exists)."""
        if not os.path.exists(path):
            return {"ok": False, "error": "File not found"}
        self._pending_delete = path
        return {"ok": True}

    def confirm_delete(self) -> dict:
        """Execute the staged deletion."""
        path = self._pending_delete
        if not path:
            return {"ok": False, "error": "No file staged"}
        if not os.path.exists(path):
            self._pending_delete = None
            return {"ok": False, "error": "File no longer exists"}
        try:
            os.remove(path)
            self._pending_delete = None
            return {"ok": True, "deleted": path}
        except Exception as e:
            return {"ok": False, "error": str(e)}

    def cancel_delete(self) -> None:
        self._pending_delete = None

    # ── export ───────────────────────────────────────────────────────────────
    def export_report(self, fmt: str = "json") -> dict:
        """Open a save-dialog and export the last scan result."""
        if not self._scan_result:
            return {"ok": False, "error": "No scan results available"}

        ext_map  = {"json": ".json", "csv": ".csv", "text": ".txt"}
        type_map = {
            "json": [("JSON Report", "*.json")],
            "csv":  [("CSV Spreadsheet", "*.csv")],
            "text": [("Text Report", "*.txt")],
        }
        ext = ext_map.get(fmt, ".json")
        ftypes = type_map.get(fmt, type_map["json"])

        root = tk.Tk()
        root.withdraw()
        root.attributes("-topmost", True)
        save_path = filedialog.asksaveasfilename(
            title="Export Duplicate Report",
            defaultextension=ext,
            filetypes=ftypes,
        )
        root.destroy()

        if not save_path:
            return {"ok": False, "error": "Cancelled"}

        try:
            reporter = ReportGenerator(self._scan_result)
            reporter.save_report(save_path, format=fmt)
            return {"ok": True, "path": save_path}
        except Exception as e:
            return {"ok": False, "error": str(e)}


# ─────────────────────────────────────────────────────────────────────────────
# ENTRY POINT
# ─────────────────────────────────────────────────────────────────────────────

def launch_gui() -> None:
    gui_html = os.path.join(os.path.dirname(os.path.abspath(__file__)), "gui.html")
    if not os.path.exists(gui_html):
        print(f"[ERROR] gui.html not found at: {gui_html}")
        sys.exit(1)

    api = Api()
    window = webview.create_window(
        title="Duplicate File Detection System — DSA Capstone",
        url=f"file:///{gui_html.replace(os.sep, '/')}",
        js_api=api,
        width=1280,
        height=800,
        min_size=(960, 640),
        background_color="#0F172A",
    )
    api.set_window(window)
    webview.start(debug=False)


if __name__ == "__main__":
    launch_gui()
