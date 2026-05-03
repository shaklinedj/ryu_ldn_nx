# ==========================================
# Memory Trace - Stack Analysis
# ==========================================
# Monitor stack usage and detect overflow
# Enhanced with ARM64 exception diagnostics and alignment checks

echo [MEMORY] Loading stack analysis...\n

# Define helper commands for stack analysis

define stack-dump
    echo \n
    echo ==========================================\n
    echo STACK DUMP\n
    echo ==========================================\n
    echo \n

    echo [STACK] Stack pointer:\n
    print /x $sp
    echo \n

    echo [STACK] Frame pointer:\n
    print /x $x29
    echo \n

    echo [STACK] Link register:\n
    print /x $x30
    echo \n

    echo [STACK] Stack dump (256 bytes):\n
    x/64xg $sp
    echo \n

    echo [STACK] Backtrace:\n
    backtrace full
    echo \n

    echo ==========================================\n
end

document stack-dump
Dump complete stack information including registers and memory
end

define stack-check
    # Check if SP is aligned
    set $sp_aligned = ($sp & 0xf)
    if $sp_aligned != 0
        echo \n
        echo [STACK:WARNING] Stack pointer not 16-byte aligned!\n
        printf "SP = 0x%lx (alignment = %d)\n", $sp, $sp_aligned
        echo \n
    else
        echo [STACK:OK] Stack pointer is 16-byte aligned\n
    end
end

document stack-check
Check stack pointer alignment and validity. AAPCS64 requires 16-byte alignment.
end

# Recursion detection
define recursion-check
    echo [STACK] Checking for recursion...\n
    backtrace
end

document recursion-check
Display backtrace to check for recursive calls
end

# ==========================================
# ARM64 Exception Diagnostics
# ==========================================

# Dump ARM64 exception syndrome registers and alignment analysis.
# Designed for DABRT 0x101 (data abort) diagnosis on Switch.
# On Horizon, data aborts from misaligned ldr/str or bswap32
# arrive as SIGBUS.
define dump-arm64-exception
    echo \n
    echo ==========================================\n
    echo ARM64 EXCEPTION REGISTERS\n
    echo ==========================================\n
    echo \n
    echo [EXCEPTION] General registers x0-x7 (arguments / faulting address source):\n
    info registers x0 x1 x2 x3 x4 x5 x6 x7
    echo \n
    echo [EXCEPTION] Stack/frame/link/PC (control flow):\n
    info registers sp x29 x30 pc
    echo \n
    echo [EXCEPTION] Faulting instruction:\n
    x/4i $pc
    echo \n
    echo [EXCEPTION] Instructions leading to fault (24 bytes before PC):\n
    x/8i $pc-24
    echo \n
    echo [EXCEPTION] Stack contents (256 bytes from SP):\n
    x/32xg $sp
    echo \n
    echo [EXCEPTION] Alignment info — x0-x3 often hold addr operands:\n
    printf "  x0=0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x0, ($x0 & 3), ($x0 & 7), ($x0 & 0xf)
    printf "  x1=0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x1, ($x1 & 3), ($x1 & 7), ($x1 & 0xf)
    printf "  x2=0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x2, ($x2 & 3), ($x2 & 7), ($x2 & 0xf)
    printf "  x3=0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x3, ($x3 & 3), ($x3 & 7), ($x3 & 0xf)
    echo \n
    echo [EXCEPTION] Backtrace:\n
    backtrace 30
    echo \n
    echo ==========================================\n
end

document dump-arm64-exception
Dump ARM64 exception info: registers, faulting instruction, stack, alignment.
Specifically designed for DABRT 0x101 diagnosis — checks alignment of
x0-x3 for ldr/str/bswap faults on misaligned addresses.
end

# Check current instruction for alignment-sensitive operations
# (ldr/str with non-aligned addresses, bswap on misaligned data)
define check-alignment
    echo \n
    echo ==========================================\n
    echo ALIGNMENT CHECK\n
    echo ==========================================\n
    echo \n
    echo [ALIGN] Current instruction:\n
    x/1i $pc
    echo \n
    echo [ALIGN] Surrounding instructions (PC-16 to PC+16):\n
    x/8i $pc-16
    echo \n
    echo [ALIGN] x0-x7 (potential pointer/length operands):\n
    info registers x0 x1 x2 x3 x4 x5 x6 x7
    echo \n
    echo [ALIGN] Pointer alignment check:\n
    printf "  x0=0x%016lx  mod2=%d mod4=%d mod8=%d\n", $x0, ($x0 & 1), ($x0 & 3), ($x0 & 7)
    printf "  x1=0x%016lx  mod2=%d mod4=%d mod8=%d\n", $x1, ($x1 & 1), ($x1 & 3), ($x1 & 7)
    printf "  x2=0x%016lx  mod2=%d mod4=%d mod8=%d\n", $x2, ($x2 & 1), ($x2 & 3), ($x2 & 7)
    printf "  x3=0x%016lx  mod2=%d mod4=%d mod8=%d\n", $x3, ($x3 & 1), ($x3 & 3), ($x3 & 7)
    echo \n
    echo [ALIGN] Stack alignment (SP must be 16-byte aligned for AAPCS64):\n
    printf "  SP=0x%016lx  mod16=%d\n", $sp, ($sp & 0xf)
    echo \n
    echo [ALIGN] Backtrace:\n
    backtrace 10
    echo \n
    echo ==========================================\n
end

document check-alignment
Inspect current instruction and registers for alignment issues.
Looks for ldr/str with misaligned addresses, bswap on unaligned data,
and verifies AAPCS64 SP alignment. Essential for DABRT 0x101 diagnosis.
end

echo [MEMORY] Stack analysis commands loaded\n
echo [MEMORY] Commands available: stack-dump, stack-check, recursion-check,\n
echo [MEMORY]   dump-arm64-exception, check-alignment\n