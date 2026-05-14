#!/usr/bin/env python3
"""Check that all source files meet a minimum line-coverage threshold.

Usage:
    python3 scripts/check-coverage.py [--min 80] [--root .]

Reads coverage data via gcovr and reports per-file coverage.
Exits with code 1 if any file with >= 10 source lines falls below
the minimum threshold.
"""

import argparse
import json
import subprocess
import sys


def compute_file_coverage(file_entry):
    """Compute line coverage from a gcovr JSON file entry.

    Handles both old format (filename/line_rate/summary) and new format
    (file/lines with per-line count).
    """
    filename = file_entry.get("filename") or file_entry.get("file", "???")

    # Old format: top-level line_rate and summary
    if "line_rate" in file_entry:
        line_rate = file_entry["line_rate"] * 100.0
        num_lines = file_entry.get("summary", {}).get("num_lines", 0)
        return filename, line_rate, num_lines

    # New format: compute from lines array
    lines = file_entry.get("lines", [])
    if not lines:
        return filename, 0.0, 0

    num_lines = len(lines)
    covered = sum(1 for line in lines if line.get("count", 0) > 0)
    line_rate = (covered / num_lines * 100.0) if num_lines > 0 else 0.0
    return filename, line_rate, num_lines


def main():
    parser = argparse.ArgumentParser(description="Check per-file coverage threshold")
    parser.add_argument("--min", type=float, default=80.0,
                        help="Minimum coverage percentage (default: 80)")
    parser.add_argument("--root", default=".",
                        help="Project root directory (default: .)")
    args = parser.parse_args()

    cmd = [
        "gcovr",
        "--merge-mode-functions=merge-use-line-max",
        f"--root={args.root}",
        "--gcov-ignore-errors=no_working_dir_found",
        "--json",
    ]

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"ERROR: gcovr failed: {result.stderr}", file=sys.stderr)
        sys.exit(2)

    try:
        data = json.loads(result.stdout)
    except json.JSONDecodeError:
        print("ERROR: could not parse gcovr JSON output", file=sys.stderr)
        sys.exit(2)

    failed = False
    files_checked = 0

    for f in data.get("files", []):
        filename, line_rate, num_lines = compute_file_coverage(f)

        # Only check files under sysmodule/source/
        if "sysmodule/source/" not in filename:
            continue

        if num_lines < 10:
            continue

        files_checked += 1

        if line_rate < args.min:
            print(f"FAIL: {filename} ({line_rate:.1f}% on {num_lines} lines, need {args.min:.0f}%)")
            failed = True
        else:
            print(f"  OK: {filename} ({line_rate:.1f}%)")

    print(f"\nChecked {files_checked} source files against {args.min:.0f}% threshold")

    if failed:
        print(f"ERROR: Coverage below {args.min:.0f}% for one or more files")
        sys.exit(1)
    else:
        print(f"All files meet {args.min:.0f}% coverage threshold")
        sys.exit(0)


if __name__ == "__main__":
    main()