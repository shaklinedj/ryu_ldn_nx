# ==========================================
# LDN Component - Async Architecture
# ==========================================
# Receive thread, event-driven IPC, MultiWait, HandleServerPacket,
# ConnectToServer, WaitForResponse — the core of the async model
# that replaced the old polling architecture.
# Using dprintf for automatic continue

echo [LDN] Loading async architecture breakpoints...\n

# ==========================================
# Receive Thread
# ==========================================

# Receive thread entry and main loop
dprintf ams::mitm::ldn::ICommunicationService::ReceiveThreadEntry, "[LDN:ASYNC] ReceiveThreadEntry: thread started arg=%p\n", $x0
dprintf ams::mitm::ldn::ICommunicationService::ReceiveThreadFunc, "[LDN:ASYNC] ReceiveThreadFunc: loop iteration\n"

# ==========================================
# Connection to server (event-driven handshake)
# ==========================================

# ConnectToServer — uses os::MultiWait on m_handshake_event
dprintf ams::mitm::ldn::ICommunicationService::ConnectToServer, "[LDN:ASYNC] ConnectToServer: entering\n"

# DisconnectFromServer
dprintf ams::mitm::ldn::ICommunicationService::DisconnectFromServer, "[LDN:ASYNC] DisconnectFromServer: entering\n"

# IsServerConnected check
dprintf ams::mitm::ldn::ICommunicationService::IsServerConnected, "[LDN:ASYNC] IsServerConnected: queried\n"

# ==========================================
# HandleServerPacket — packet dispatch on receive thread
# ==========================================

dprintf ams::mitm::ldn::ICommunicationService::HandleServerPacket, "[LDN:ASYNC] HandleServerPacket: id=%u size=%zu\n", $x2, $x3

# ==========================================
# WaitForResponse — IPC handler event wait
# ==========================================

dprintf ams::mitm::ldn::ICommunicationService::WaitForResponse, "[LDN:ASYNC] WaitForResponse: expected_id=%u timeout_ms=%lu\n", $x1, $x2

# ==========================================
# Synchronization primitives
# ==========================================

# Event signaling points (these are methods on os::Event but we can
# trace the signaling by watching when ICommunicationService signals
# them — the key ones are in ConnectToServer, HandleServerPacket, etc.)
# Since os::Event::Signal is a Stratosphere inline, we trace the callers.

# m_shared_mutex — locked/unlocked by IPC and receive threads
# Can't dprintf on mutex lock/unlock directly, but we can observe
# the key critical sections via the functions that hold them.

# ==========================================
# P2P connect thread (separate async path)
# ==========================================

dprintf ams::mitm::ldn::ICommunicationService::P2pConnectThreadEntry, "[LDN:ASYNC] P2pConnectThreadEntry: thread started arg=%p\n", $x0

# ==========================================
# Key IPC handlers that wait on events
# ==========================================

# CreateNetwork — waits for SyncNetwork via m_response_event
dprintf ams::mitm::ldn::ICommunicationService::CreateNetwork, "[LDN:ASYNC] CreateNetwork: entering\n"
dprintf ams::mitm::ldn::ICommunicationService::DestroyNetwork, "[LDN:ASYNC] DestroyNetwork: entering\n"

# Scan — waits for ScanReplyEnd via m_scan_event
dprintf ams::mitm::ldn::ICommunicationService::Scan, "[LDN:ASYNC] Scan: entering\n"
dprintf ams::mitm::ldn::ICommunicationService::ScanPrivate, "[LDN:ASYNC] ScanPrivate: entering\n"

# Connect — waits for SyncNetwork via m_response_event
dprintf ams::mitm::ldn::ICommunicationService::Connect, "[LDN:ASYNC] Connect: entering\n"
dprintf ams::mitm::ldn::ICommunicationService::ConnectPrivate, "[LDN:ASYNC] ConnectPrivate: entering\n"

# Disconnect
dprintf ams::mitm::ldn::ICommunicationService::Disconnect, "[LDN:ASYNC] Disconnect: entering\n"

# Reconnect
dprintf ams::mitm::ldn::LdnConfigService::ForceReconnect, "[LDN:ASYNC] ForceReconnect\n"

echo [LDN] Async architecture: 16 dprintf points\n