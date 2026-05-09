# ==========================================
# Memory Trace - Crash Detection
# ==========================================
# ADVANCED crash handlers with full variable capture.
#
# WARNING: This file DUPLICATES the crash handlers in debug.sh.
# Do NOT load both — GDB would fire two catchpoints per signal.
# This file is only loaded via load-memory-tools, which is opt-in.
# The crash handlers in debug.sh are always active by default.
#
# If you loaded this by mistake, run:
#   delete catchpoints
#   source /workspace/scripts/debugger/presets/crash-analysis.gdb
#

echo [MEMORY] Loading crash detection...\n

# ==========================================
# SIGSEGV — Segmentation fault
# ==========================================
catch signal SIGSEGV
commands
    silent
    echo \n
    echo ==========================================\n
    echo SEGMENTATION FAULT (SIGSEGV)\n
    echo ==========================================\n
    echo \n

    # Register state
    echo [CRASH] Register state:\n
    info registers
    echo \n

    # Backtrace
    echo [CRASH] Call stack:\n
    backtrace full
    echo \n

    # Faulting instruction
    echo [CRASH] Faulting instruction:\n
    x/10i $pc-20
    echo \n

    # Memory around PC
    echo [CRASH] Memory around PC:\n
    x/32xb $pc-16
    echo \n

    # Stack dump
    echo [CRASH] Stack dump (256 bytes):\n
    x/64xb $sp
    echo \n

    # ARM64 alignment diagnostics
    echo [CRASH] ARM64 alignment diagnostics:\n
    printf "  x0=0x%016lx  mod4=%d mod8=%d mod16=%d\n", $x0, ($x0 & 3), ($x0 & 7), ($x0 & 0xf)
    printf "  x1=0x%016lx  mod4=%d mod8=%d mod16=%d\n", $x1, ($x1 & 3), ($x1 & 7), ($x1 & 0xf)
    printf "  x2=0x%016lx  mod4=%d mod8=%d mod16=%d\n", $x2, ($x2 & 3), ($x2 & 7), ($x2 & 0xf)
    printf "  x3=0x%016lx  mod4=%d mod8=%d mod16=%d\n", $x3, ($x3 & 3), ($x3 & 7), ($x3 & 0xf)
    printf "  SP=0x%016lx  mod16=%d (AAPCS64 requires 16-byte alignment)\n", $sp, ($sp & 0xf)
    echo \n

    # Local variables
    echo [CRASH] Local variables:\n
    info locals
    echo \n

    # Arguments
    echo [CRASH] Function arguments:\n
    info args
    echo \n

    echo ==========================================\n
    echo END CRASH REPORT\n
    echo ==========================================\n
    echo \n

    # Don't continue - let user examine
end

# ==========================================
# SIGBUS — Bus error (includes ARM64 alignment faults)
# On Switch, DABRT 0x101 (data abort from lower EL) arrives
# as SIGBUS.  The faulting instruction is typically a ldr/str
# or rev/bswap on a misaligned address from an IPC buffer.
# ==========================================
catch signal SIGBUS
commands
    silent
    echo \n
    echo ╔══════════════════════════════════════════╗\n
    echo ║     BUS ERROR / DATA ABORT (SIGBUS)     ║\n
    echo ╚══════════════════════════════════════════╝\n
    echo \n
    echo NOTE: On Switch ARM64, DABRT 0x101 (data abort)\n
    echo typically indicates an alignment fault: a ldr/str\n
    echo or bswap instruction accessing a misaligned address.\n
    echo Common cause: reinterpret_cast<SockAddrIn*> on\n
    echo unaligned IPC buffer pointer followed by bswap32.\n
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ CRASH LOCATION                            │\n
    echo └──────────────────────────────────────────┘\n
    printf "  PC  = 0x%016lx\n", $pc
    printf "  LR  = 0x%016lx\n", $x30
    printf "  SP  = 0x%016lx\n", $sp
    printf "  FP  = 0x%016lx\n", $x29
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ FAULTING INSTRUCTION                      │\n
    echo └──────────────────────────────────────────┘\n
    x/4i $pc
    echo \n
    echo Instructions leading to fault:\n
    x/8i $pc-24
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ ARM64 ALIGNMENT ANALYSIS                   │\n
    echo └──────────────────────────────────────────┘\n
    echo Checking argument registers for misalignment:\n
    printf "  x0 = 0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x0, ($x0 & 3), ($x0 & 7), ($x0 & 0xf)
    printf "  x1 = 0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x1, ($x1 & 3), ($x1 & 7), ($x1 & 0xf)
    printf "  x2 = 0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x2, ($x2 & 3), ($x2 & 7), ($x2 & 0xf)
    printf "  x3 = 0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x3, ($x3 & 3), ($x3 & 7), ($x3 & 0xf)
    printf "  SP = 0x%016lx  (mod16=%d, AAPCS64 requires 0)\n", $sp, ($sp & 0xf)
    echo \n
    echo If x0-x3 contain pointers, misalignment of mod4!=0\n
    echo suggests a bswap32 or ldr on an unaligned address.\n
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ CALL STACK                                 │\n
    echo └──────────────────────────────────────────┘\n
    backtrace 30
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ REGISTERS                                  │\n
    echo └──────────────────────────────────────────┘\n
    info registers
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ STACK MEMORY                              │\n
    echo └──────────────────────────────────────────┘\n
    x/32xg $sp
    echo \n

    echo ╔══════════════════════════════════════════╗\n
    echo ║           CRASH ANALYSIS COMPLETE          ║\n
    echo ╚══════════════════════════════════════════╝\n
    echo \n
end

# ==========================================
# SIGILL — Illegal instruction
# ==========================================
catch signal SIGILL
commands
    silent
    echo \n
    echo ╔══════════════════════════════════════════╗\n
    echo ║      ILLEGAL INSTRUCTION (SIGILL)          ║\n
    echo ╚══════════════════════════════════════════╝\n
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ CRASH LOCATION                             │\n
    echo └──────────────────────────────────────────┘\n
    printf "  PC  = 0x%016lx\n", $pc
    printf "  LR  = 0x%016lx\n", $x30
    printf "  SP  = 0x%016lx\n", $sp
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ CALL STACK                                 │\n
    echo └──────────────────────────────────────────┘\n
    backtrace 30
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ DISASSEMBLY @ PC                           │\n
    echo └──────────────────────────────────────────┘\n
    x/10i $pc-20
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ REGISTERS                                  │\n
    echo └──────────────────────────────────────────┘\n
    info registers
    echo \n

    echo ╔══════════════════════════════════════════╗\n
    echo ║           SESSION TERMINATED                ║\n
    echo ╚══════════════════════════════════════════╝\n
    echo \n
end

# ==========================================
# SIGFPE — Floating point exception
# ==========================================
catch signal SIGFPE
commands
    silent
    echo \n
    echo ╔══════════════════════════════════════════╗\n
    echo ║    FLOATING POINT EXCEPTION (SIGFPE)       ║\n
    echo ╚══════════════════════════════════════════╝\n
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ CRASH LOCATION                             │\n
    echo └──────────────────────────────────────────┘\n
    printf "  PC  = 0x%016lx\n", $pc
    printf "  LR  = 0x%016lx\n", $x30
    printf "  SP  = 0x%016lx\n", $sp
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ CALL STACK                                 │\n
    echo └──────────────────────────────────────────┘\n
    backtrace full
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ REGISTERS                                  │\n
    echo └──────────────────────────────────────────┘\n
    info registers
    echo \n

    echo ╔══════════════════════════════════════════╗\n
    echo ║           SESSION TERMINATED                ║\n
    echo ╚══════════════════════════════════════════╝\n
    echo \n
end

# ==========================================
# SIGABRT — Abort (assert, std::terminate, etc.)
# ==========================================
catch signal SIGABRT
commands
    silent
    echo \n
    echo ╔══════════════════════════════════════════╗\n
    echo ║           ABORT (SIGABRT)                   ║\n
    echo ╚══════════════════════════════════════════╝\n
    echo \n
    echo Possible causes: assert(), std::terminate(), unhandled exception\n
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ CRASH LOCATION                             │\n
    echo └──────────────────────────────────────────┘\n
    printf "  PC  = 0x%016lx\n", $pc
    printf "  LR  = 0x%016lx\n", $x30
    printf "  SP  = 0x%016lx\n", $sp
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ CALL STACK                                 │\n
    echo └──────────────────────────────────────────┘\n
    backtrace 30
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ REGISTERS                                  │\n
    echo └──────────────────────────────────────────┘\n
    info registers
    echo \n

    echo ┌──────────────────────────────────────────┐\n
    echo │ STACK MEMORY                               │\n
    echo └──────────────────────────────────────────┘\n
    x/32xg $sp
    echo \n

    echo ╔══════════════════════════════════════════╗\n
    echo ║           SESSION TERMINATED                ║\n
    echo ╚══════════════════════════════════════════╝\n
    echo \n
end

echo [MEMORY] Crash detection: 5 signal handlers (SIGSEGV+ARM64 diagnostics, SIGBUS+alignment, SIGILL, SIGFPE, SIGABRT)\n