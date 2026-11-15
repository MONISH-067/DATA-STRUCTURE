"""
Duplicate File Detection System — Main CLI Entry Point
========================================================
A command-line interface for scanning directories and detecting
duplicate files using advanced data structures and algorithms.

Usage:
    python main.py <directory> [options]

Examples:
    python main.py C:\\Users\\Me\\Documents
    python main.py ./photos --traversal bfs --top 5
    python main.py ./data --min-size 1024 --export report.json
"""

from __future__ import annotations
import argparse
import sys
import os

# Allow running as both `python main.py` and `python -m src`
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from src.detector import DuplicateDetector, format_size
from src.walker import ScanConfig
from src.report import ReportGenerator


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        prog="DupDetect",
        description="Duplicate File Detection System — DSA Capstone Project",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""\
Data Structures:  Hash Table, Trie, Bloom Filter, Binary Heap
Algorithms:       BFS, DFS, SHA-256, Merge Sort, Quick Sort

Examples:
  python main.py C:\\Users\\Me\\Documents
  python main.py ./photos --traversal bfs --top 5
  python main.py ./data --min-size 1024 --export report.json --format json
        """,
    )

    parser.add_argument(
        "directory",
        nargs="?",
        default=None,
        help="Root directory to scan for duplicates",
    )

    parser.add_argument(
        "--traversal", "-t",
        choices=["dfs", "bfs"],
        default="dfs",
        help="Directory traversal strategy (default: dfs)",
    )

    parser.add_argument(
        "--min-size",
        type=int,
        default=1,
        help="Minimum file size in bytes to consider (default: 1)",
    )

    parser.add_argument(
        "--max-size",
        type=int,
        default=None,
        help="Maximum file size in bytes to consider",
    )

    parser.add_argument(
        "--include-ext",
        nargs="*",
        help="Only include these file extensions (e.g., .jpg .png)",
    )

    parser.add_argument(
        "--exclude-ext",
        nargs="*",
        help="Exclude these file extensions (e.g., .tmp .log)",
    )

    parser.add_argument(
        "--top", "-k",
        type=int,
        default=20,
        help="Show top K duplicate groups by wasted space (default: 20)",
    )

    parser.add_argument(
        "--export", "-o",
        type=str,
        default=None,
        help="Export report to file",
    )

    parser.add_argument(
        "--format", "-f",
        choices=["text", "json", "csv"],
        default="text",
        help="Export format (default: text)",
    )

    parser.add_argument(
        "--algorithm",
        choices=["sha256", "md5", "sha1"],
        default="sha256",
        help="Hash algorithm (default: sha256)",
    )

    parser.add_argument(
        "--follow-symlinks",
        action="store_true",
        help="Follow symbolic links during traversal",
    )

    parser.add_argument(
        "--show-bloom-stats",
        action="store_true",
        help="Show Bloom filter diagnostics",
    )

    return parser.parse_args()


def main() -> None:
    args = parse_args()

    if args.directory is None:
        parse_args_parser = parse_args
        print("Duplicate File Detection System — CLI Mode\n")
        print("Usage: python main.py <directory> [options]\n")
        print("Run 'python main.py --help' for full option details.")
        sys.exit(0)

    directory = os.path.abspath(args.directory)
    if not os.path.isdir(directory):
        print(f"Error: '{directory}' is not a valid directory.", file=sys.stderr)
        sys.exit(1)

    # Build scan configuration
    include_ext = set(args.include_ext) if args.include_ext else None
    exclude_ext = set(args.exclude_ext) if args.exclude_ext else None

    config = ScanConfig(
        min_size=args.min_size,
        max_size=args.max_size,
        include_extensions=include_ext,
        exclude_extensions=exclude_ext,
        follow_symlinks=args.follow_symlinks,
    )

    # Create and run the detector
    print(f"🔍 Scanning: {directory}")
    print(f"   Traversal: {args.traversal.upper()} | Hash: {args.algorithm.upper()}")
    print(f"   Min size: {format_size(args.min_size)} | Follow symlinks: {args.follow_symlinks}")
    print()

    detector = DuplicateDetector(
        config=config,
        traversal=args.traversal,
        hash_algorithm=args.algorithm,
    )

    result = detector.scan(directory)

    # Generate and display report
    reporter = ReportGenerator(result)
    text_report = reporter.generate_text_report(top_k=args.top)
    print(text_report)

    # Bloom filter diagnostics
    if args.show_bloom_stats:
        bf = detector.bloom_filter
        print("\n── BLOOM FILTER DIAGNOSTICS ──────────────────────────────")
        print(f"  Bit array size   : {bf.size_bits:,} bits ({bf.size_bits // 8:,} bytes)")
        print(f"  Hash functions   : {bf.num_hash_functions}")
        print(f"  Items inserted   : {bf.count:,}")
        print(f"  Fill ratio       : {bf.fill_ratio():.2%}")
        print(f"  Est. FP rate     : {bf.estimated_fp_rate():.4%}")
        print()

    # Top-K using heap
    if result.duplicate_groups:
        top_groups = detector.get_top_k_groups(result, k=min(3, len(result.duplicate_groups)))
        print("\n── TOP SPACE-WASTERS (via Max-Heap) ─────────────────────")
        for i, g in enumerate(top_groups, 1):
            print(f"  #{i}: {format_size(g.wasted_bytes)} wasted "
                  f"({g.count} copies of {format_size(g.file_size)} file)")
        print()

    # Export
    if args.export:
        reporter.save_report(args.export, format=args.format)
        print(f"📄 Report exported to: {os.path.abspath(args.export)}")

    # Summary
    if result.total_duplicates == 0:
        print("✅ No duplicate files found!")
    else:
        print(f"⚠️  Found {result.total_duplicates:,} duplicate files in "
              f"{len(result.duplicate_groups):,} groups")
        print(f"   Potential space savings: {format_size(result.total_wasted_bytes)}")


if __name__ == "__main__":
    main()
