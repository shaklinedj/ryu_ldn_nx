# =========================================
# NETWORK:STATE_CALLBACKS
# =========================================

echo [NETWORK] Loading state_callbacks breakpoints...\n
# Namespace: ryu_ldn::network
dprintf ryu_ldn::network::RyuLdnClient::set_state_callback, "[NETWORK:STATE_CALLBACKS] set_state_callback: cb=%p user_data=%p\n", $x1, $x2
dprintf ryu_ldn::network::RyuLdnClient::set_packet_callback, "[NETWORK:STATE_CALLBACKS] set_packet_callback: cb=%p user_data=%p\n", $x1, $x2
dprintf ryu_ldn::network::RyuLdnClient::connect, "[NETWORK:STATE_CALLBACKS] connect: entering\n"
dprintf ryu_ldn::network::RyuLdnClient::disconnect, "[NETWORK:STATE_CALLBACKS] disconnect: entering\n"
dprintf ryu_ldn::network::RyuLdnClient::update, "[NETWORK:STATE_CALLBACKS] update: state=%d tick\n", $x1
dprintf ryu_ldn::network::RyuLdnClient::send_scan, "[NETWORK:STATE_CALLBACKS] send_scan\n"
dprintf ryu_ldn::network::RyuLdnClient::send_create_access_point, "[NETWORK:STATE_CALLBACKS] send_create_access_point\n"
dprintf ryu_ldn::network::RyuLdnClient::send_connect, "[NETWORK:STATE_CALLBACKS] send_connect\n"
dprintf ryu_ldn::network::RyuLdnClient::send_proxy_data, "[NETWORK:STATE_CALLBACKS] send_proxy_data\n"
dprintf ryu_ldn::network::RyuLdnClient::send_ping, "[NETWORK:STATE_CALLBACKS] send_ping\n"
dprintf ryu_ldn::network::RyuLdnClient::send_ping_response, "[NETWORK:STATE_CALLBACKS] send_ping_response: ping_id=%u\n", $x1
dprintf ryu_ldn::network::RyuLdnClient::send_disconnect_network, "[NETWORK:STATE_CALLBACKS] send_disconnect_network\n"
dprintf ryu_ldn::network::RyuLdnClient::try_connect, "[NETWORK:STATE_CALLBACKS] try_connect: attempting TCP\n"
dprintf ryu_ldn::network::RyuLdnClient::process_packets, "[NETWORK:STATE_CALLBACKS] process_packets: draining receive buffer\n"
dprintf ryu_ldn::network::RyuLdnClient::handle_packet, "[NETWORK:STATE_CALLBACKS] handle_packet: id=%u size=%zu\n", $x1, $x2
dprintf ryu_ldn::network::RyuLdnClient::process_handshake_response, "[NETWORK:STATE_CALLBACKS] process_handshake_response: id=%u\n", $x1
dprintf ryu_ldn::network::RyuLdnClient::is_handshake_timeout, "[NETWORK:STATE_CALLBACKS] is_handshake_timeout\n"
dprintf ryu_ldn::network::RyuLdnClient::start_backoff, "[NETWORK:STATE_CALLBACKS] start_backoff: delay=%u retry=%u\n", $x0, $x1
dprintf ryu_ldn::network::RyuLdnClient::is_backoff_expired, "[NETWORK:STATE_CALLBACKS] is_backoff_expired\n"

echo [NETWORK] state_callbacks: 19 dprintf points\n
