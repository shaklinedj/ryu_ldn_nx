# ==========================================
# Common Helper Commands (generic GDB only)
# ==========================================
#
# This file contains ONLY generic GDB helpers and loading
# infrastructure.  Application-specific breakpoints, state
# inspection, and alignment diagnostics belong in
# components/ or memory-trace/.
#
# Memory-trace is always auto-loaded so that kernel panics
# and data aborts are caught with full diagnostics by default.
#

# Basic GDB settings
set breakpoint pending on
set pagination off
set confirm off
set auto-load safe-path /

# ==========================================
# Memory-trace: always loaded by default
# ==========================================
# Provides crash detection, allocation tracking, stack
# analysis, etc. — essential for KP / DABRT diagnosis.

define load-memory-tools
    echo \n
    echo [MEMORY] Loading memory diagnostic tools...\n
    source /workspace/scripts/debugger/tools/memory-trace/01-allocations.gdb
    source /workspace/scripts/debugger/tools/memory-trace/02-crash-detection.gdb
    source /workspace/scripts/debugger/tools/memory-trace/03-leak-detection.gdb
    source /workspace/scripts/debugger/tools/memory-trace/04-corruption.gdb
    source /workspace/scripts/debugger/tools/memory-trace/05-stack-analysis.gdb
    source /workspace/scripts/debugger/tools/memory-trace/06-snapshots.gdb
    source /workspace/scripts/debugger/tools/memory-trace/07-watchpoints.gdb
    echo [MEMORY] All tools loaded\n
    echo \n
end

document load-memory-tools
Load all memory diagnostic tools (allocations, crash detection, leaks, etc.)
This is not loaded by default for performance - call this command when needed.
end

# Memory-trace is NOT loaded by default — it is extremely expensive.
# Each breakpoint on malloc/free/memcpy/etc. causes GDB to stop and resume
# on every single call, which kills real-time performance for a sysmodule.
# On a remote target (Switch via TCP), each hit costs a full network roundtrip.
#
# To load manually in a GDB session, type:
#   load-memory-tools
#
# Or start the debugger with:  --memory

# ==========================================
# Generic GDB Helpers
# ==========================================

# Helper: List all breakpoints
define bpl
    info breakpoints
end
document bpl
List all breakpoints
end

# Helper: Delete all breakpoints
define bpd
    delete breakpoints
end
document bpd
Delete all breakpoints
end

# Helper: Continue with status
define c-status
    echo \nContinuing execution...\n
    continue
end
document c-status
Continue execution with status message
end

# ==========================================
# Dynamic Loader
# ==========================================

define load
    if $argc == 0
        echo \n
        echo [LOAD] Usage: load <component> [subcomponent]\n
        echo \n
        echo Available components:\n
        echo   bsd config debug ldn network p2p\n
        echo \n
        echo Examples:\n
        echo   load bsd               - Load all BSD component files\n
        echo   load bsd lifecycle     - Load BSD lifecycle only\n
        echo   load ldn               - Load all LDN component files\n
        echo \n
        echo Use 'list-components' to see all available files\n
        echo \n
    else
        if $argc == 1
            source /workspace/scripts/debugger/components/$arg0/*.gdb
            printf "\n[LOAD] Loaded component: %s (all files)\n\n", "$arg0"
        else
            shell ls /workspace/scripts/debugger/components/$arg0/*$arg1*.gdb 2>/dev/null | head -1 | xargs -I {} echo "source {}" > /tmp/load_component.gdb
            source /tmp/load_component.gdb
            printf "\n[LOAD] Loaded: %s/%s\n\n", "$arg0", "$arg1"
        end
    end
end
document load
Dynamically load debugger components or subcomponents.
Usage: load <component> [subcomponent]
Examples:
  load bsd              - Load all BSD files
  load ldn lifecycle    - Load LDN lifecycle only
  load network packet   - Load network packet handling
end

# Preset loader
define load-preset
    if $argc == 0
        echo \n
        echo [LOAD-PRESET] Usage: load-preset <preset-name>\n
        echo \n
        echo Use 'list-presets' to see available presets\n
        echo \n
    else
        set $preset_file = "/workspace/scripts/debugger/presets/$arg0.gdb"
        shell test -f /workspace/scripts/debugger/presets/$arg0.gdb && echo "[LOAD-PRESET] Loading preset: $arg0" || echo "[LOAD-PRESET] Preset not found: $arg0"
        source /workspace/scripts/debugger/presets/$arg0.gdb
        printf "\n[LOAD-PRESET] Loaded preset: %s\n\n", "$arg0"
    end
end
document load-preset
Load a debug preset configuration.
Usage: load-preset <preset-name>
Examples:
  load-preset crash-analysis
  load-preset network-focus
  load-preset ldn-focus
end

# List available components
define list-components
    echo \n
    echo ==========================================\n
    echo AVAILABLE DEBUG COMPONENTS\n
    echo ==========================================\n
    echo \n
    shell echo "[BSD] BSD socket MITM service:" && ls -1 /workspace/scripts/debugger/components/bsd/*.gdb | sed 's|.*/||' | sed 's/\.gdb$//' | sed 's/^/  - /'
    echo \n
    shell echo "[CONFIG] Configuration management:" && ls -1 /workspace/scripts/debugger/components/config/*.gdb | sed 's|.*/||' | sed 's/\.gdb$//' | sed 's/^/  - /'
    echo \n
    shell echo "[DEBUG] Logging utilities:" && ls -1 /workspace/scripts/debugger/components/debug/*.gdb | sed 's|.*/||' | sed 's/\.gdb$//' | sed 's/^/  - /'
    echo \n
    shell echo "[LDN] LDN service handlers (async architecture):" && ls -1 /workspace/scripts/debugger/components/ldn/*.gdb | sed 's|.*/||' | sed 's/\.gdb$//' | sed 's/^/  - /'
    echo \n
    shell echo "[NETWORK] Network client (state callbacks):" && ls -1 /workspace/scripts/debugger/components/network/*.gdb | sed 's|.*/||' | sed 's/\.gdb$//' | sed 's/^/  - /'
    echo \n
    shell echo "[P2P] P2P proxy:" && ls -1 /workspace/scripts/debugger/components/p2p/*.gdb | sed 's|.*/||' | sed 's/\.gdb$//' | sed 's/^/  - /'
    echo \n
    echo ==========================================\n
    echo \n
end
document list-components
List all available debug components and subcomponents
end

# List available presets
define list-presets
    echo \n
    echo ==========================================\n
    echo AVAILABLE DEBUG PRESETS\n
    echo ==========================================\n
    echo \n
    shell ls /workspace/scripts/debugger/presets/*.gdb 2>/dev/null | sed 's|.*/||' | sed 's/\.gdb$//' | sed 's/^/  - /' || echo "  (No presets available)"
    echo \n
    echo ==========================================\n
    echo \n
end
document list-presets
List all available debug presets
end

define ryu-help
    echo \n
    echo ==========================================\n
    echo ryu_ldn_nx GDB DEBUGGER HELP\n
    echo ==========================================\n
    echo \n
    echo [DYNAMIC LOADING] Load components on demand:\n
    echo   load <component>             : Load all files from a component\n
    echo   load <component> <sub>       : Load specific subcomponent\n
    echo   load-preset <name>           : Load a preset configuration\n
    echo   list-components              : List all available components\n
    echo   list-presets                 : List all available presets\n
    echo \n
    echo [MEMORY TOOLS] Always loaded by default:\n
    echo   memory-snapshot      : Capture full memory state\n
    echo   memory-snapshot-lite : Lightweight snapshot (registers + backtrace)\n
    echo   memory-leak-report   : Generate memory leak analysis\n
    echo   memory-usage         : Display memory mappings\n
    echo   stack-dump           : Dump complete stack information\n
    echo   stack-check          : Check stack alignment\n
    echo   recursion-check      : Display backtrace for recursion detection\n
    echo   dump-arm64-exception : Dump ARM64 exception regs + alignment (05-stack)\n
    echo   check-alignment      : Inspect instruction + register alignment (05-stack)\n
    echo \n
    echo [WATCHPOINTS] State monitoring commands (07-watchpoints):\n
    echo   watch-client-state   : Watch P2P client state changes\n
    echo   watch-network-state  : Watch network connection state\n
    echo   watch-connection     : Watch connection establishment\n
    echo   watch-cond <expr>    : Set conditional watchpoint\n
    echo \n
    echo [COMPONENT COMMANDS] Per-component inspection (loaded with component):\n
    echo   inspect-recv-thread  : Show receive thread state (LDN 08-async)\n
    echo   inspect-ldn-state    : Show LDN state machine + async arch (LDN 08-async)\n
    echo   inspect-bsd-alignment: BSD alignment path info (BSD 06-alignment)\n
    echo   inspect-client-state : Network client state (NET 05-callbacks)\n
    echo \n
    echo [BREAKPOINT HELPERS] Breakpoint management:\n
    echo   bpl                  : List all breakpoints\n
    echo   bpd                  : Delete all breakpoints\n
    echo   info breakpoints     : Detailed breakpoint info\n
    echo \n
    echo [NAVIGATION] Execution control:\n
    echo   c / continue         : Continue execution\n
    echo   c-status             : Continue with status message\n
    echo   s / step             : Step into\n
    echo   n / next             : Step over\n
    echo   finish               : Step out\n
    echo \n
    echo [INFO] Session information:\n
    echo   info threads         : List all threads\n
    echo   info registers       : Show all registers\n
    echo   backtrace / bt       : Show call stack\n
    echo \n
    echo Type 'help <command>' for detailed help on any command\n
    echo ==========================================\n
    echo \n
end
document ryu-help
Display ryu_ldn_nx debugger help and available commands
end