#!/usr/bin/env python3
"""
GDB Tracepoint Code Generator
==============================

Bidirectional tool:
  1. generate: Scan @gdb{} annotations in C++ source → produce .gdb files
  2. inject:   Parse existing .gdb files → inject @gdb{} annotations into C++ headers
  3. verify:   Compare annotations vs existing .gdb files, report diffs

Annotations live in headers right above the function/method declaration:

    /// @gdb{tag="LDN:LIFECYCLE", msg="Communication service created"}
    explicit ICommunicationService(ncm::ProgramId program_id);

    /// @gdb{tag="LDN:OPS", msg="Reject: nodeId=%u", args="$x1"}
    Result Reject(u32 nodeId);

Usage:
    python3 scripts/gdb_codegen.py generate   # @gdb{} → .gdb files
    python3 scripts/gdb_codegen.py inject      # .gdb files → @gdb{} in headers
    python3 scripts/gdb_codegen.py verify      # Compare and report diffs
"""

import argparse
import os
import re
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from typing import Optional


# ---------------------------------------------------------------------------
# Annotation format
# ---------------------------------------------------------------------------

RE_ANNOTATION = re.compile(
    r'///\s*@gdb\{'
    r'tag="(?P<tag>[^"]*)"'
    r'(?:,\s*msg="(?P<msg>[^"]*)")?'
    r'(?:,\s*args="(?P<args>[^"]*)")?'
    r'(?:,\s*file="(?P<file>[^"]*)")?'
    r'\}'
)

RE_NAMESPACE = re.compile(r'\bnamespace\s+([\w:]+)\s*\{')
RE_CLASS = re.compile(r'\b(?:class|struct)\s+(?:__attribute__\s*\(\(.*?\)\)\s+)?(\w+)\s*(?:__attribute__\s*\(\(.*?\)\)\s*)?(?::.*?)?\s*\{')
RE_ENUM_CLASS = re.compile(r'\benum\s+class\s+\w+')
RE_DPRINTF = re.compile(
    r'dprintf\s+([\w:]+)\s*,\s*"([^"]*)"(?:\s*,\s*(.+))?'
)

# Namespace → source directory mapping
NAMESPACE_TO_DIR = {
    "ams::mitm::ldn": "ldn",
    "ryu_ldn::ldn": "ldn",
    "ryu_ldn::network": "network",
    "ams::mitm::bsd": "bsd",
    "ryu_ldn::bsd": "bsd",
    "ryu_ldn::config": "config",
    "ryu_ldn::ipc": "config",
    "ryu_ldn::debug": "debug",
    "ams::mitm::p2p": "p2p",
}


@dataclass
class GdbAnnotation:
    tag: str
    msg: str
    args: Optional[str] = None
    namespace: str = ""
    class_name: Optional[str] = None
    function_name: str = ""
    source_file: str = ""
    line_number: int = 0

    @property
    def symbol(self) -> str:
        parts = []
        if self.namespace:
            parts.append(self.namespace)
        if self.class_name:
            parts.append(self.class_name)
        parts.append(self.function_name)
        return "::".join(parts)

    @property
    def component(self) -> str:
        prefix = self.tag.split(":")[0].lower()
        mapping = {"ldn": "ldn", "config": "config", "p2p": "p2p", "bsd": "bsd", "debug": "debug", "network": "network"}
        return mapping.get(prefix, prefix)

    @property
    def dprintf_line(self) -> str:
        fmt = f"[{self.tag}] {self.msg}\\n"
        if self.args:
            return f'dprintf {self.symbol}, "{fmt}", {self.args}'
        else:
            return f'dprintf {self.symbol}, "{fmt}"'


# ---------------------------------------------------------------------------
# Generator (annotations → .gdb files)
# ---------------------------------------------------------------------------

COMPONENT_ORDER = {
    "ldn": ["LIFECYCLE", "CONFIG", "OPS", "PROXY", "SESSION", "DISPATCHER", "STATE", "ASYNC"],
    "config": ["PARSE", "MGR", "IPC"],
    "p2p": ["SERVER", "SESSION", "ROUTE", "CLIENT", "MSG", "NAT"],
    "bsd": ["LIFECYCLE", "SOCKET", "CONNECT", "DATA", "CONFIG", "ALIGN"],
    "debug": ["LOGGER", "LOG"],
    "network": ["LIFECYCLE", "CONNECT", "TCP", "PACKET", "STATE", "CB"],
}

TAG_TO_FILE = {}
for comp, tags in COMPONENT_ORDER.items():
    for idx, tag_suffix in enumerate(tags, start=1):
        key = f"{comp.upper()}:{tag_suffix}"
        TAG_TO_FILE[key] = f"{idx:02d}-{tag_suffix.lower().replace('_', '-')}.gdb"


class SourceScanner:
    """Scan C++ source files for @gdb{} annotations."""

    def __init__(self, source_dir: str):
        self.source_dir = source_dir
        self.annotations: list[GdbAnnotation] = []

    def scan_all(self):
        for root, _, files in os.walk(self.source_dir):
            for fname in sorted(files):
                if fname.endswith((".hpp", ".cpp", ".h", ".c")):
                    self._scan_file(os.path.join(root, fname))

    def _scan_file(self, filepath: str):
        with open(filepath, "r", encoding="utf-8", errors="replace") as f:
            lines = f.readlines()

        ns_stack: list[str] = []
        cls_stack: list[str] = []
        pending: Optional[re.Match] = None

        for i, raw in enumerate(lines):
            line = raw.strip()
            m = RE_ANNOTATION.search(line)
            if m:
                pending = m
                continue

            # Track namespaces
            nm = RE_NAMESPACE.search(line)
            if nm:
                for part in nm.group(1).split("::"):
                    if part:
                        ns_stack.append(part)
                pending = None
                continue

            # Track classes (skip enum class)
            cm = RE_CLASS.search(line)
            if cm and not RE_ENUM_CLASS.search(line):
                cls_stack.append(cm.group(1))
                pending = None
                continue

            if pending:
                # Try to extract function name
                # Destructor
                dtor = re.search(r'~(\w+)\s*\(', line)
                if dtor and cls_stack and dtor.group(1) == cls_stack[-1]:
                    self._add(pending, "~" + cls_stack[-1], ns_stack, cls_stack, filepath, i + 1)
                    pending = None
                    continue

                # Constructor
                if cls_stack:
                    ctor = re.compile(r'\b' + re.escape(cls_stack[-1]) + r'\s*\(')
                    if ctor.search(line):
                        self._add(pending, cls_stack[-1], ns_stack, cls_stack, filepath, i + 1)
                        pending = None
                        continue

                # Regular function
                func = re.search(r'\b(\w+)\s*\(', line)
                if func and func.group(1) not in ("if", "for", "while", "switch", "catch", "return", "namespace", "class", "struct"):
                    self._add(pending, func.group(1), ns_stack, cls_stack, filepath, i + 1)
                    pending = None
                    continue

                pending = None

    def _add(self, match, func_name, ns_stack, cls_stack, filepath, line_num):
        ann = GdbAnnotation(
            tag=match.group("tag"),
            msg=match.group("msg") or "",
            args=match.group("args"),
            namespace="::".join(ns_stack),
            class_name=cls_stack[-1] if cls_stack else None,
            function_name=func_name,
            source_file=os.path.relpath(filepath, self.source_dir),
            line_number=line_num,
        )
        if match.group("file"):
            ann.source_file = match.group("file")
        self.annotations.append(ann)


def generate_gdb_files(annotations: list[GdbAnnotation], output_dir: str, dry_run: bool = False):
    grouped: dict[str, dict[str, list[GdbAnnotation]]] = defaultdict(lambda: defaultdict(list))

    for ann in annotations:
        parts = ann.tag.split(":", 1)
        comp = parts[0].lower() if len(parts) > 1 else ann.component
        tag_key = ann.tag
        grouped[comp][tag_key].append(ann)

    written = []
    for comp in sorted(grouped.keys()):
        comp_dir = os.path.join(output_dir, comp)
        file_groups: dict[str, list[tuple[str, list[GdbAnnotation]]]] = defaultdict(list)

        for tag_key in sorted(grouped[comp].keys()):
            anns = grouped[comp][tag_key]
            gdb_file = TAG_TO_FILE.get(tag_key)
            if not gdb_file:
                tag_suffix = tag_key.split(":", 1)[-1] if ":" in tag_key else tag_key
                for k, v in TAG_TO_FILE.items():
                    if k.split(":", 1)[-1] == tag_suffix and k.split(":", 1)[0].lower() == comp:
                        gdb_file = v
                        break
                if not gdb_file:
                    idx = len(file_groups) + 1
                    gdb_file = f"{idx:02d}-{tag_suffix.lower().replace('_', '-')}.gdb"
            file_groups[gdb_file].append((tag_key, anns))

        for gdb_file in sorted(file_groups.keys()):
            filepath = os.path.join(comp_dir, gdb_file)
            lines = []
            total = 0

            for tag_key, anns in file_groups[gdb_file]:
                parts = tag_key.split(":", 1)
                prefix = parts[0] if len(parts) > 1 else comp.upper()
                suffix = parts[1] if len(parts) > 1 else tag_key
                lines.append(f"# {'=' * 41}")
                lines.append(f"# {prefix}:{suffix}")
                lines.append(f"# {'=' * 41}")
                lines.append("")
                lines.append(f'echo [{prefix}] Loading {suffix.lower()} breakpoints...\\n')

                by_ns: dict[str, list[GdbAnnotation]] = defaultdict(list)
                for a in anns:
                    by_ns[a.namespace or "(global)"].append(a)

                for ns in sorted(by_ns.keys()):
                    ns_anns = by_ns[ns]
                    if ns != "(global)":
                        lines.append(f"# Namespace: {ns}")
                    for a in ns_anns:
                        lines.append(a.dprintf_line)
                        total += 1
                lines.append("")

            lines.append(f'echo [{prefix}] {suffix.lower()}: {total} dprintf points\\n')
            content = "\n".join(lines) + "\n"

            if dry_run:
                print(f"[DRY RUN] Would write: {filepath} ({total} dprintfs)")
            else:
                os.makedirs(comp_dir, exist_ok=True)
                with open(filepath, "w") as f:
                    f.write(content)
                written.append(filepath)
                print(f"  ✓ {filepath} ({total} dprintfs)")

    return written


# ---------------------------------------------------------------------------
# Injector (.gdb files → @gdb{} annotations in headers)
# ---------------------------------------------------------------------------

@dataclass
class GdbDprintf:
    """A dprintf entry parsed from a .gdb file."""
    symbol: str
    format_string: str
    args: Optional[str] = None
    component: str = ""
    tag_file: str = ""

    @property
    def tag(self) -> str:
        """Extract tag from format string like [LDN:STATE] ..."""
        m = re.match(r'\[([A-Z]+:[A-Z]+)\]', self.format_string)
        return m.group(1) if m else "UNKNOWN:UNKNOWN"

    @property
    def msg(self) -> str:
        """Extract message without tag prefix and \\n."""
        m = re.match(r'\[[A-Z]+:[A-Z]+\]\s*(.*)', self.format_string)
        msg = m.group(1) if m else self.format_string
        return msg.rstrip("\\n")


def parse_gdb_files(gdb_dir: str) -> list[GdbDprintf]:
    """Parse all .gdb files and extract dprintf entries."""
    entries = []
    for root, _, files in os.walk(gdb_dir):
        for fname in sorted(files):
            if not fname.endswith(".gdb"):
                continue
            filepath = os.path.join(root, fname)
            comp = os.path.basename(root)
            with open(filepath, "r") as f:
                for line in f:
                    line = line.strip()
                    m = RE_DPRINTF.match(line)
                    if m:
                        entry = GdbDprintf(
                            symbol=m.group(1),
                            format_string=m.group(2),
                            args=m.group(3).strip() if m.group(3) else None,
                            component=comp,
                            tag_file=fname,
                        )
                        entries.append(entry)
    return entries


def inject_annotations(source_dir: str, dprintfs: list[GdbDprintf], dry_run: bool = False):
    """Inject @gdb{} annotations into C++ source files based on parsed dprintf entries."""
    # Group by (namespace, class) → function → list of dprintfs
    # We need to find the right source file and line to inject

    # First, parse all source files to build a symbol → (file, line) mapping
    symbol_map = build_symbol_map(source_dir)

    # Group annotations by target file
    file_annotations: dict[str, list[tuple[int, str, GdbDprintf]]] = defaultdict(list)

    for dp in dprintfs:
        symbol = dp.symbol
        if symbol in symbol_map:
            filepath, line_num, ns, cls, func = symbol_map[symbol]
            # Build annotation line
            ann_line = f'/// @gdb{{tag="{dp.tag}", msg="{dp.msg}"'
            if dp.args:
                ann_line += f', args="{dp.args}"'
            ann_line += '}'
            file_annotations[filepath].append((line_num, ann_line, dp))
        else:
            print(f"  ⚠ Symbol not found in sources: {symbol}")

    # Inject annotations into each file
    for filepath in sorted(file_annotations.keys()):
        annotations = sorted(file_annotations[filepath], key=lambda x: x[0])

        if dry_run:
            print(f"[DRY RUN] Would inject {len(annotations)} annotations into {filepath}")
            for _, ann_line, dp in annotations:
                print(f"    L{dp.symbol}: {ann_line}")
            continue

        with open(filepath, "r", encoding="utf-8") as f:
            lines = f.readlines()

        # Insert annotations in reverse order to preserve line numbers
        insertions = {}
        for line_num, ann_line, dp in annotations:
            # Find the declaration line (the one with the function name)
            target_line = line_num - 1  # Convert to 0-based
            # Insert annotation before the declaration
            insertions.setdefault(target_line, []).append(ann_line)

        # Check if annotation already exists on the line above
        new_lines = []
        for i, line in enumerate(lines):
            # Check if there's an annotation to insert before this line
            if i in insertions:
                for ann_line in reversed(insertions[i]):
                    # Check if previous line already has this annotation
                    if i > 0 and RE_ANNOTATION.search(lines[i - 1]):
                        continue  # Skip — already annotated
                    new_lines.append(ann_line + "\n")
            new_lines.append(line)

        with open(filepath, "w", encoding="utf-8") as f:
            f.writelines(new_lines)

        print(f"  ✓ Injected {len(annotations)} annotations into {filepath}")


def build_symbol_map(source_dir: str) -> dict[str, tuple]:
    """Build a mapping from fully-qualified symbol names to (file, line, namespace, class, function)."""
    symbol_map = {}

    for root, _, files in os.walk(source_dir):
        for fname in sorted(files):
            if not fname.endswith((".hpp", ".h")):
                continue
            filepath = os.path.join(root, fname)
            with open(filepath, "r", encoding="utf-8", errors="replace") as f:
                lines = f.readlines()

            ns_stack = []
            cls_stack = []

            for i, raw in enumerate(lines):
                line = raw.strip()

                # Check for existing @gdb annotation — skip if present
                if RE_ANNOTATION.search(line):
                    continue

                nm = RE_NAMESPACE.search(line)
                if nm:
                    for part in nm.group(1).split("::"):
                        if part:
                            ns_stack.append(part)
                    continue

                cm = RE_CLASS.search(line)
                if cm:
                    cls_stack.append(cm.group(1))
                    continue

                # Functions
                dtor = re.search(r'~(\w+)\s*\(', line)
                if dtor and cls_stack and dtor.group(1) == cls_stack[-1]:
                    sym = "::".join(ns_stack + cls_stack + ["~" + cls_stack[-1]])
                    symbol_map[sym] = (filepath, i + 1, "::".join(ns_stack), cls_stack[-1], "~" + cls_stack[-1])
                    continue

                if cls_stack:
                    ctor = re.compile(r'\b' + re.escape(cls_stack[-1]) + r'\s*\(')
                    if ctor.search(line):
                        sym = "::".join(ns_stack + cls_stack + [cls_stack[-1]])
                        symbol_map[sym] = (filepath, i + 1, "::".join(ns_stack), cls_stack[-1], cls_stack[-1])
                        continue

                # Free functions or static methods
                func = re.search(r'\b(\w+)\s*\(', line)
                if func and func.group(1) not in ("if", "for", "while", "switch", "catch", "return", "namespace", "class", "struct"):
                    name = func.group(1)
                    parts = ns_stack + cls_stack + [name] if cls_stack else ns_stack + [name]
                    sym = "::".join(parts)
                    symbol_map[sym] = (filepath, i + 1, "::".join(ns_stack), cls_stack[-1] if cls_stack else None, name)

    return symbol_map


# ---------------------------------------------------------------------------
# Verification
# ---------------------------------------------------------------------------

def verify(annotations: list[GdbAnnotation], existing_dir: str):
    """Compare generated annotations against existing .gdb files."""
    existing_dprintfs: dict[str, str] = {}
    for root, _, files in os.walk(existing_dir):
        for fname in sorted(files):
            if not fname.endswith(".gdb"):
                continue
            with open(os.path.join(root, fname), "r") as f:
                for line in f:
                    m = RE_DPRINTF.match(line.strip())
                    if m:
                        existing_dprintfs[m.group(1)] = m.group(2)

    gen_dprintfs: dict[str, str] = {}
    for ann in annotations:
        fmt = f"[{ann.tag}] {ann.msg}\\n"
        gen_dprintfs[ann.symbol] = fmt

    existing_syms = set(existing_dprintfs.keys())
    gen_syms = set(gen_dprintfs.keys())

    missing = existing_syms - gen_syms
    extra = gen_syms - existing_syms
    common = existing_syms & gen_syms
    changed = {s for s in common if existing_dprintfs[s] != gen_dprintfs[s]}

    print(f"\n=== Verification Report ===")
    print(f"Existing symbols: {len(existing_syms)}")
    print(f"Generated symbols: {len(gen_syms)}")
    print(f"Matched: {len(common)}")
    print(f"Missing (in source, not annotated): {len(missing)}")
    print(f"Extra (annotated, not in existing): {len(extra)}")
    print(f"Format string changes: {len(changed)}")

    if missing:
        print(f"\n  Missing symbols (need @gdb annotation):")
        for s in sorted(missing):
            print(f"    - {s}")

    return len(missing) == 0 and len(changed) == 0


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="GDB tracepoint code generator/injector")
    parser.add_argument("--source-dir", default="sysmodule/source",
                        help="Root directory to scan for source files")
    parser.add_argument("--gdb-dir", default="scripts/debugger/components",
                        help="Directory with existing .gdb files")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print what would be done without writing")
    parser.add_argument("command", choices=["generate", "inject", "verify"],
                        help="Command: generate (.gdb from annotations), "
                             "inject (annotations from .gdb), verify (compare)")
    args = parser.parse_args()

    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    source_dir = os.path.join(repo_root, args.source_dir)
    gdb_dir = os.path.join(repo_root, args.gdb_dir)

    if args.command == "generate":
        print(f"Scanning: {source_dir}")
        scanner = SourceScanner(source_dir)
        scanner.scan_all()
        print(f"Found {len(scanner.annotations)} @gdb annotations\n")
        generate_gdb_files(scanner.annotations, gdb_dir, dry_run=args.dry_run)

    elif args.command == "inject":
        print(f"Parsing: {gdb_dir}")
        dprintfs = parse_gdb_files(gdb_dir)
        print(f"Found {len(dprintfs)} dprintf entries\n")
        inject_annotations(source_dir, dprintfs, dry_run=args.dry_run)

    elif args.command == "verify":
        print(f"Scanning: {source_dir}")
        scanner = SourceScanner(source_dir)
        scanner.scan_all()
        print(f"Found {len(scanner.annotations)} @gdb annotations\n")
        ok = verify(scanner.annotations, gdb_dir)
        sys.exit(0 if ok else 1)

    print("\nDone.")


if __name__ == "__main__":
    main()