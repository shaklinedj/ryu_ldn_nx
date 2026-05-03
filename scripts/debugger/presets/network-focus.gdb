# ==========================================
# Network Focus Preset
# ==========================================
# Network + BSD connection/data/alignment + P2P client/messages
# Updated with alignment path and state callback coverage.
# Use: load-preset network-focus

echo [PRESET] Loading network-focus preset...\n

# Network component (all files including state callbacks)
source /workspace/scripts/debugger/components/network/01-lifecycle.gdb
source /workspace/scripts/debugger/components/network/02-connection.gdb
source /workspace/scripts/debugger/components/network/03-packet.gdb
source /workspace/scripts/debugger/components/network/04-state-machine.gdb
source /workspace/scripts/debugger/components/network/05-state-callbacks.gdb

# BSD lifecycle + connection + data transfer + alignment paths
source /workspace/scripts/debugger/components/bsd/01-lifecycle.gdb
source /workspace/scripts/debugger/components/bsd/03-connection.gdb
source /workspace/scripts/debugger/components/bsd/04-data-transfer.gdb
source /workspace/scripts/debugger/components/bsd/06-alignment.gdb

# P2P client/server
source /workspace/scripts/debugger/components/p2p/01-server-lifecycle.gdb
source /workspace/scripts/debugger/components/p2p/04-client.gdb
source /workspace/scripts/debugger/components/p2p/05-messages.gdb

# LDN async architecture (receive thread drives network)
source /workspace/scripts/debugger/components/ldn/08-async-architecture.gdb

echo [PRESET] Network-focus preset loaded (12 files)\n
echo [PRESET] Tracking: Network (all), BSD (lifecycle+connection+data+alignment),\n
echo [PRESET]   P2P (client+messages), LDN async\n