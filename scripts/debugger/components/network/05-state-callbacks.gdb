# ==========================================
# Network Component - State Callbacks & Transitions
# ==========================================
# RyuLdnClient m_state_callback invocations, ConnectionStateMachine
# transitions, and packet flow. Updated for async architecture
# where state callbacks fire from the receive thread.
# Using dprintf for automatic continue

echo [NETWORK] Loading state callback breakpoints...\n

# ==========================================
# ConnectionStateMachine transitions
# ==========================================

# The core state transition — fires callback
dprintf ryu_ldn::network::ConnectionStateMachine::transition_to, "[NETWORK:STATE] transition_to: old=%d new=%d event=%d\n", $x1, $x2, $x3

# State queries
dprintf ryu_ldn::network::ConnectionStateMachine::is_connected, "[NETWORK:STATE] is_connected\n"
dprintf ryu_ldn::network::ConnectionStateMachine::is_transitioning, "[NETWORK:STATE] is_transitioning\n"
dprintf ryu_ldn::network::ConnectionStateMachine::is_valid_transition, "[NETWORK:STATE] is_valid_transition\n"

# Callback registration
dprintf ryu_ldn::network::ConnectionStateMachine::set_state_change_callback, "[NETWORK:STATE] set_state_change_callback: cb=%p\n", $x0
dprintf ryu_ldn::network::ConnectionStateMachine::force_state, "[NETWORK:STATE] force_state: new=%d\n", $x1

# ==========================================
# RyuLdnClient — state callback invocations
# These fire from the receive thread (update loop) and
# from try_connect/disconnect (which can run on IPC thread
# or the thread that calls connect).
# ==========================================

# Connection — callback on state change
dprintf ryu_ldn::network::RyuLdnClient::connect, "[NETWORK:CB] connect: entering\n"

# try_connect — fires ConnectSuccess or ConnectFailed callback
dprintf ryu_ldn::network::RyuLdnClient::try_connect, "[NETWORK:CB] try_connect: attempting TCP\n"

# disconnect — fires Disconnecting → Disconnected callback
dprintf ryu_ldn::network::RyuLdnClient::disconnect, "[NETWORK:CB] disconnect: entering\n"

# update — main loop, fires callbacks on:
#   Connected → Handshaking (sendInitialize + HandshakeStarted)
#   Handshaking → Ready (process_handshake_response success)
#   Handshaking → Backoff (timeout or handshake fail)
#   Ready → ConnectionLost (ping timeout)
#   Backoff → Retrying (expired + try_connect)
#   Disconnecting → Disconnected
dprintf ryu_ldn::network::RyuLdnClient::update, "[NETWORK:CB] update: state=%d tick\n", $x1

# process_handshake_response — fires callbacks for:
#   Initialize → Ready
#   NetworkError → Backoff or Error
#   SyncNetwork → Ready
#   Disconnect → HandshakeFailed + Backoff
dprintf ryu_ldn::network::RyuLdnClient::process_handshake_response, "[NETWORK:CB] process_handshake_response: id=%u\n", $x1

# Callback registration
dprintf ryu_ldn::network::RyuLdnClient::set_state_callback, "[NETWORK:CB] set_state_callback: cb=%p user_data=%p\n", $x1, $x2
dprintf ryu_ldn::network::RyuLdnClient::set_packet_callback, "[NETWORK:CB] set_packet_callback: cb=%p user_data=%p\n", $x1, $x2

# ==========================================
# Backoff and reconnect
# ==========================================

dprintf ryu_ldn::network::RyuLdnClient::start_backoff, "[NETWORK:CB] start_backoff: delay=%u retry=%u\n", $x0, $x1
dprintf ryu_ldn::network::RyuLdnClient::is_backoff_expired, "[NETWORK:CB] is_backoff_expired\n"
dprintf ryu_ldn::network::RyuLdnClient::is_handshake_timeout, "[NETWORK:CB] is_handshake_timeout\n"

# ==========================================
# Packet processing (receive thread context)
# ==========================================

dprintf ryu_ldn::network::RyuLdnClient::process_packets, "[NETWORK:CB] process_packets: draining receive buffer\n"
dprintf ryu_ldn::network::RyuLdnClient::handle_packet, "[NETWORK:CB] handle_packet: id=%u size=%zu\n", $x1, $x2

# Send operations (can be called from either thread via m_send_mutex)
dprintf ryu_ldn::network::RyuLdnClient::send_scan, "[NETWORK:CB] send_scan\n"
dprintf ryu_ldn::network::RyuLdnClient::send_create_access_point, "[NETWORK:CB] send_create_access_point\n"
dprintf ryu_ldn::network::RyuLdnClient::send_connect, "[NETWORK:CB] send_connect\n"
dprintf ryu_ldn::network::RyuLdnClient::send_proxy_data, "[NETWORK:CB] send_proxy_data\n"
dprintf ryu_ldn::network::RyuLdnClient::send_ping, "[NETWORK:CB] send_ping\n"
dprintf ryu_ldn::network::RyuLdnClient::send_ping_response, "[NETWORK:CB] send_ping_response: ping_id=%u\n", $x1
dprintf ryu_ldn::network::RyuLdnClient::send_disconnect_network, "[NETWORK:CB] send_disconnect_network\n"

# ==========================================
# TcpClient — low-level send/receive
# ==========================================

dprintf ryu_ldn::network::TcpClient::connect, "[NETWORK:TCP] connect: host=%s port=%u\n", $x0, $x2
dprintf ryu_ldn::network::TcpClient::disconnect, "[NETWORK:TCP] disconnect\n"
dprintf ryu_ldn::network::TcpClient::is_connected, "[NETWORK:TCP] is_connected\n"
dprintf ryu_ldn::network::TcpClient::receive_packet, "[NETWORK:TCP] receive_packet: timeout=%d\n", $x5

echo [NETWORK] State callbacks: 29 dprintf points\n