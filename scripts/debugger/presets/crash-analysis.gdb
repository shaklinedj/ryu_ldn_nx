# ==========================================
# Crash Analysis Preset
# ==========================================
# Full crash diagnostics with ARM64 alignment support.
# WARNING: This loads memory-trace breakpoints (malloc/free/memcpy/etc.)
# which are extremely expensive — each allocation/copy causes a GDB stop.
# Only use for diagnosing memory corruption, leaks, or alignment faults.
#
# For normal debugging, use 'load-preset crash-only' instead — crash handlers
# in debug.sh already capture full state without runtime overhead.
#
# Use: load-preset crash-analysis

echo [PRESET] Loading crash-analysis preset...\n

# Memory trace tools (expensive — breakpoints on malloc/free/memcpy/etc.)
source /workspace/scripts/debugger/tools/memory-trace/01-allocations.gdb
source /workspace/scripts/debugger/tools/memory-trace/02-crash-detection.gdb
source /workspace/scripts/debugger/tools/memory-trace/03-leak-detection.gdb
source /workspace/scripts/debugger/tools/memory-trace/04-corruption.gdb
source /workspace/scripts/debugger/tools/memory-trace/05-stack-analysis.gdb
source /workspace/scripts/debugger/tools/memory-trace/06-snapshots.gdb
source /workspace/scripts/debugger/tools/memory-trace/07-watchpoints.gdb

# BSD alignment paths — critical for DABRT 0x101 diagnosis
source /workspace/scripts/debugger/components/bsd/06-align.gdb

# LDN state machine — transition tracing
source /workspace/scripts/debugger/components/ldn/07-state.gdb

# LDN async architecture — receive thread + event handling
source /workspace/scripts/debugger/components/ldn/08-async.gdb

# Network state callbacks — connection state changes
source /workspace/scripts/debugger/components/network/06-state-callbacks.gdb

# Network lifecycle — basic client lifecycle
source /workspace/scripts/debugger/components/network/01-lifecycle.gdb

# BSD lifecycle — basic socket lifecycle
source /workspace/scripts/debugger/components/bsd/01-lifecycle.gdb

# P2P lifecycle
source /workspace/scripts/debugger/components/p2p/01-server.gdb

echo [PRESET] Crash-analysis preset loaded\n
echo [PRESET] WARNING: Memory-trace breakpoints active (malloc/free/memcpy/etc.)\n
echo [PRESET]   This is VERY expensive — expect significant slowdown.\n
echo [PRESET]   For normal crash diagnosis, use crash-only preset instead.\n
echo [PRESET] Key commands: dump-arm64-exception, check-alignment, stack-dump\n