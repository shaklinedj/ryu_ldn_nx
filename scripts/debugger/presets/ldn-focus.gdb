# ==========================================
# LDN Focus Preset
# ==========================================
# All LDN component files including the async architecture module.
# Use: load-preset ldn-focus

echo [PRESET] Loading ldn-focus preset...\n

# LDN component (all 8 files including async architecture)
source /workspace/scripts/debugger/components/ldn/01-lifecycle.gdb
source /workspace/scripts/debugger/components/ldn/02-config.gdb
source /workspace/scripts/debugger/components/ldn/03-ops.gdb
source /workspace/scripts/debugger/components/ldn/04-proxy-handler.gdb
source /workspace/scripts/debugger/components/ldn/05-session-handler.gdb
source /workspace/scripts/debugger/components/ldn/06-dispatcher.gdb
source /workspace/scripts/debugger/components/ldn/07-state.gdb
source /workspace/scripts/debugger/components/ldn/08-async.gdb

echo [PRESET] LDN-focus preset loaded (8 files)\n
echo [PRESET] Tracking: lifecycle, config, operations, proxy, session,\n
echo [PRESET]   dispatch, state machine, async architecture\n