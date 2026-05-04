# GDB Tracepoint Annotations

## Overview

GDB tracepoints (`dprintf`) for debugging on Switch hardware are defined **inline in C++ source files** using `@gdb{}` annotations in Doxygen comments. A Python code generator (`scripts/gdb_codegen.py`) extracts these annotations and produces the `.gdb` files that `debug.sh` loads.

This keeps GDB tracepoints co-located with the code they trace, making them easy to discover and maintain.

## Annotation Format

Place a `/// @gdb{}` comment directly above the function/method declaration:

```cpp
/// @gdb{tag="LDN:LIFECYCLE", msg="Communication service created"}
explicit ICommunicationService(ncm::ProgramId program_id);

/// @gdb{tag="LDN:OPS", msg="Reject: nodeId=%u", args="$x1"}
Result Reject(u32 nodeId);
```

### Fields

| Field  | Required | Description |
|--------|----------|-------------|
| `tag`  | Yes      | Hierarchical tag like `LDN:LIFECYCLE`. Maps to component + sub-file (see below). |
| `msg`  | Yes      | Printf-style format string **without** the `[TAG]` prefix or `\n` — the generator adds both. |
| `args` | No       | GDB register arguments (e.g., `$x0`, `$x1`, `$x2`). ARM64 calling convention. |

### Tag to Component Mapping

| Tag Prefix | Component Directory | Sub-files |
|------------|-------------------|-----------|
| `LDN` | `ldn/` | LIFECYCLE, CONFIG, OPS, PROXY, SESSION, DISPATCHER, STATE, ASYNC |
| `CONFIG` | `config/` | PARSE, MGR, IPC |
| `P2P` | `p2p/` | SERVER, SESSION, ROUTE, CLIENT, MSG, NAT |
| `BSD` | `bsd/` | LIFECYCLE, SOCKET, CONNECT, DATA, CONFIG, ALIGN |
| `DEBUG` | `debug/` | LOGGER, LOG |
| `NETWORK` | `network/` | LIFECYCLE, CONNECT, TCP, PACKET, STATE, CB |

### Symbol Resolution

The generator reconstructs fully-qualified symbols from:
1. Enclosing `namespace` declarations (e.g., `ams::mitm::ldn`)
2. Enclosing `class`/`struct` declarations (e.g., `ICommunicationService`)
3. The function name on the line following the annotation

For destructors, use the `~ClassName` pattern. For constructors, the generator matches the class name.

## Commands

```bash
# Generate .gdb files from annotations
python3 scripts/gdb_codegen.py generate

# Dry-run (show what would be generated)
python3 scripts/gdb_codegen.py generate --dry-run

# Compare annotations against existing .gdb files
python3 scripts/gdb_codegen.py verify

# Inject annotations from existing .gdb files into source headers
python3 scripts/gdb_codegen.py inject

# Inject with dry-run
python3 scripts/gdb_codegen.py inject --dry-run
```

## Workflow

1. **Add/edit `@gdb{}` annotations** in `.hpp` files next to function declarations
2. **Run `python3 scripts/gdb_codegen.py generate`** to regenerate `.gdb` files
3. **Use `debug.sh`** as before to load tracepoints on Switch

### Adding a new tracepoint

```cpp
// In ldn_icommunication.hpp:
/// @gdb{tag="LDN:OPS", msg="Scan started: channel=%d", args="$x2"}
Result Scan(ams::sf::Out<u32> count, ...);
```

Then run `python3 scripts/gdb_codegen.py generate` — it will add the dprintf to the appropriate `.gdb` file.

### Adding a new tag category

1. Add annotations in source with the new tag (e.g., `tag="FOO:BAR"`)
2. Edit `scripts/gdb_codegen.py` — add the component and tag suffix to `COMPONENT_ORDER`
3. Re-run `generate`

### Free functions and .cpp annotations

For free functions or functions defined in `.cpp` files without a header declaration, add the annotation directly in the `.cpp` file above the function definition. The generator scans both `.hpp` and `.cpp` files.

## What Stays as Standalone .gdb

The following GDB scripts remain standalone (not generated from annotations):
- `tools/common.gdb` — GDB settings, helper commands, dprintf loader
- `tools/memory-trace/*.gdb` — Crash detection, leak detection, watchpoints
- `presets/*.gdb` — Curated component bundles

## Design Rationale

**Why inline annotations instead of separate files?**

| Aspect | Before (separate .gdb) | After (inline @gdb{}) |
|--------|------------------------|----------------------|
| Discovery | Search 30 files in 6 directories | Visible in each source file |
| Maintenance | Manual sync when renaming/moving | Moves with the code |
| Consistency | Tag format must match by convention | Generator enforces format |
| Runtime | Direct dprintf commands | Identical (generated .gdb files) |
| IDE visibility | None | Doxygen comments show in hover |