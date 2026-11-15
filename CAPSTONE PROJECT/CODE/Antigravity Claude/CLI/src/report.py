"""
Report Generator — Formatted Output for Duplicate Scan Results
================================================================
Generates human-readable text reports, JSON exports, and CSV exports
from ScanResult data.
"""

from __future__ import annotations
import json
import csv
import io
import os
from datetime import datetime
from typing import Optional

from .detector import ScanResult, DuplicateGroup, format_size


class ReportGenerator:
    """Generate reports in various formats from scan results."""

    def __init__(self, result: ScanResult):
        self.result = result

    # ------------------------------------------------------------------ #
    #  Text report                                                         #
    # ------------------------------------------------------------------ #
    def generate_text_report(self, top_k: Optional[int] = None) -> str:
        """Generate a detailed text report."""
        r = self.result
        lines = []

        lines.append("=" * 72)
        lines.append("  DUPLICATE FILE DETECTION REPORT")
        lines.append(f"  Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        lines.append("=" * 72)

        # Summary
        lines.append("")
        lines.append("── SCAN SUMMARY ──────────────────────────────────────────")
        lines.append(f"  Files scanned       : {r.total_files_scanned:,}")
        lines.append(f"  Directories scanned : {r.total_dirs_scanned:,}")
        lines.append(f"  Files skipped       : {r.total_files_skipped:,}")
        lines.append(f"  Unique files        : {r.total_unique_files:,}")
        lines.append(f"  Duplicate files     : {r.total_duplicates:,}")
        lines.append(f"  Duplicate groups    : {len(r.duplicate_groups):,}")
        lines.append(f"  Wasted disk space   : {format_size(r.total_wasted_bytes)}")
        lines.append(f"  Scan time           : {r.elapsed_seconds:.2f} seconds")
        lines.append(f"  Scan errors         : {len(r.scan_errors)}")

        # Pipeline statistics
        lines.append("")
        lines.append("── PIPELINE STATISTICS ───────────────────────────────────")
        lines.append(f"  Phase 1 — Size groups (≥2 files) : {r.phase1_size_groups}")
        lines.append(f"  Phase 2 — Bloom filter candidates : {r.phase2_bloom_candidates}")
        lines.append(f"  Phase 3 — Quick-hash groups       : {r.phase3_quick_hash_groups}")
        lines.append(f"  Phase 4 — Full-hash groups (dupes): {r.phase4_full_hash_groups}")

        # Duplicate groups
        groups_to_show = r.duplicate_groups
        if top_k is not None:
            groups_to_show = groups_to_show[:top_k]

        lines.append("")
        title = f"── DUPLICATE GROUPS (Top {len(groups_to_show)}) "
        lines.append(title + "─" * (56 - len(title)))

        for i, group in enumerate(groups_to_show, 1):
            lines.append("")
            lines.append(f"  Group {i}: {group.count} copies │ "
                         f"Size: {format_size(group.file_size)} │ "
                         f"Wasted: {format_size(group.wasted_bytes)}")
            lines.append(f"  Hash: {group.hash_digest}")
            for path in group.paths:
                lines.append(f"    • {path}")

        # Errors
        if r.scan_errors:
            lines.append("")
            lines.append("── ERRORS ───────────────────────────────────────────────")
            for err in r.scan_errors[:20]:
                lines.append(f"  ⚠ {err}")
            if len(r.scan_errors) > 20:
                lines.append(f"  ... and {len(r.scan_errors) - 20} more errors")

        lines.append("")
        lines.append("=" * 72)
        lines.append("  End of report")
        lines.append("=" * 72)

        return "\n".join(lines)

    # ------------------------------------------------------------------ #
    #  JSON export                                                         #
    # ------------------------------------------------------------------ #
    def generate_json_report(self) -> str:
        """Export scan results as JSON."""
        r = self.result
        data = {
            "report_date": datetime.now().isoformat(),
            "summary": {
                "files_scanned": r.total_files_scanned,
                "dirs_scanned": r.total_dirs_scanned,
                "files_skipped": r.total_files_skipped,
                "unique_files": r.total_unique_files,
                "duplicate_files": r.total_duplicates,
                "duplicate_groups": len(r.duplicate_groups),
                "wasted_bytes": r.total_wasted_bytes,
                "wasted_human": format_size(r.total_wasted_bytes),
                "elapsed_seconds": round(r.elapsed_seconds, 3),
            },
            "pipeline": {
                "phase1_size_groups": r.phase1_size_groups,
                "phase2_bloom_candidates": r.phase2_bloom_candidates,
                "phase3_quick_hash_groups": r.phase3_quick_hash_groups,
                "phase4_full_hash_groups": r.phase4_full_hash_groups,
            },
            "duplicate_groups": [
                {
                    "hash": g.hash_digest,
                    "file_size": g.file_size,
                    "file_size_human": format_size(g.file_size),
                    "copies": g.count,
                    "wasted_bytes": g.wasted_bytes,
                    "paths": g.paths,
                }
                for g in r.duplicate_groups
            ],
        }
        return json.dumps(data, indent=2)

    # ------------------------------------------------------------------ #
    #  CSV export                                                          #
    # ------------------------------------------------------------------ #
    def generate_csv_report(self) -> str:
        """Export duplicate groups as CSV."""
        output = io.StringIO()
        writer = csv.writer(output)
        writer.writerow([
            "Group", "Hash", "File Size (bytes)", "File Size",
            "Copies", "Wasted (bytes)", "Wasted", "File Path"
        ])
        for i, group in enumerate(self.result.duplicate_groups, 1):
            for path in group.paths:
                writer.writerow([
                    i,
                    group.hash_digest,
                    group.file_size,
                    format_size(group.file_size),
                    group.count,
                    group.wasted_bytes,
                    format_size(group.wasted_bytes),
                    path,
                ])
        return output.getvalue()

    # ------------------------------------------------------------------ #
    #  Save to file                                                        #
    # ------------------------------------------------------------------ #
    def save_report(self, path: str, format: str = "text") -> None:
        """Save the report to a file."""
        if format == "text":
            content = self.generate_text_report()
        elif format == "json":
            content = self.generate_json_report()
        elif format == "csv":
            content = self.generate_csv_report()
        else:
            raise ValueError(f"Unknown format: {format}")

        os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
        with open(path, "w", encoding="utf-8") as f:
            f.write(content)
