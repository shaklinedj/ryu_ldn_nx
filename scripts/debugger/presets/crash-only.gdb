# ==========================================
# Crash-Only Preset
# ==========================================
# Zero-overhead crash diagnostics.
# No breakpoints, no memory tracing — only crash handlers (which are
# always active via debug.sh). Use this when you want the debugger
# attached but with minimal runtime impact.
#
# The crash handlers in debug.sh already capture:
#   - backtrace full (with local variables)
#   - info locals + info args
#   - Disassembly, registers, stack dump, thread list
#
# Use: load-preset crash-only

echo [PRESET] Crash-only preset: no component breakpoints.\n
echo [PRESET] Crash handlers are always active (from debug.sh).\n
echo [PRESET] Zero runtime overhead. Attach 'load-preset crash-analysis'\n
echo [PRESET]   or 'load-memory-tools' if you need memory tracing.\n