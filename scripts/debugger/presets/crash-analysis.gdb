# ==========================================
# Crash Analysis Preset
# ==========================================
# Full crash diagnostics with ARM64 alignment support.
# Uses memory-trace tools + lifecycle breakpoints for context.
# Use: load-preset crash-analysis

echo [PRESET] Loading crash-analysis preset...\n

# Memory trace tools (always available via common.gdb default load,
# but explicit here for clarity when loading this preset standalone)
source /workspace/scripts/debugger/tools/memory-trace/01-allocations.gdb
source /workspace/scripts/debugger/tools/memory-trace/02-crash-detection.gdb
source /workspace/scripts/debugger/tools/memory-trace/03-leak-detection.gdb
source /workspace/scripts/debugger/tools/memory-trace/04-corruption.gdb
source /workspace/scripts/debugger/tools/memory-trace/05-stack-analysis.gdb
source /workspace/scripts/debugger/tools/memory-trace/06-snapshots.gdb
source /workspace/scripts/debugger/tools/memory-trace/07-watchpoints.gdb

# BSD alignment paths — critical for DABRT 0x101 diagnosis
source /workspace/scripts/debugger/components/bsd/06-alignment.gdb

# LDN async architecture — receive thread + event handling
source /workspace/scripts/debugger/components/ldn/08-async-architecture.gdb

# LDN state machine — transition tracing
source /workspace/scripts/debugger/components/ldn/07-state-communication.gdb

# Network state callbacks — connection state changes
source /workspace/scripts/debugger/components/network/05-state-callbacks.gdb

# Network lifecycle — basic client lifecycle
source /workspace/scripts/debugger/components/network/01-lifecycle.gdb

# BSD lifecycle — basic socket lifecycle
source /workspace/scripts/debugger/components/bsd/01-lifecycle.gdb

# P2P lifecycle
source /workspace/scripts/debugger/components/p2p/01-server-lifecycle.gdb

echo [PRESET] Crash-analysis preset loaded\n
echo [PRESET] Tools: allocations, crash-detection (ARM64+alignment), leaks, corruption\n
echo [PRESET]   stack (ARM64 exception diagnostics), snapshots, watchpoints\n
echo [PRESET] Components: BSD alignment, LDN async+state, NET callbacks+state\n
echo [PRESET] Key commands: dump-arm64-exception, check-alignment, stack-dump\n