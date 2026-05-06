# =========================================
# NETWORK:TCP
# =========================================

echo [NETWORK] Loading tcp breakpoints...\n
# Namespace: ryu_ldn::network
dprintf ryu_ldn::network::TcpClient::connect, "[NETWORK:TCP] TcpClient::connect\n"
dprintf ryu_ldn::network::TcpClient::disconnect, "[NETWORK:TCP] TcpClient::disconnect\n"
dprintf ryu_ldn::network::TcpClient::is_connected, "[NETWORK:TCP] TcpClient::is_connected\n"
dprintf ryu_ldn::network::TcpClient::send_packet, "[NETWORK:TCP] TcpClient::send_packet\n"
dprintf ryu_ldn::network::TcpClient::send_raw, "[NETWORK:TCP] TcpClient::send_raw\n"
dprintf ryu_ldn::network::TcpClient::send_initialize, "[NETWORK:TCP] send_initialize\n"
dprintf ryu_ldn::network::TcpClient::send_passphrase, "[NETWORK:TCP] send_passphrase\n"
dprintf ryu_ldn::network::TcpClient::send_ping, "[NETWORK:TCP] send_ping\n"
dprintf ryu_ldn::network::TcpClient::send_disconnect, "[NETWORK:TCP] send_disconnect\n"
dprintf ryu_ldn::network::TcpClient::send_create_access_point, "[NETWORK:TCP] send_create_access_point\n"
dprintf ryu_ldn::network::TcpClient::send_create_access_point_private, "[NETWORK:TCP] send_create_access_point_private\n"
dprintf ryu_ldn::network::TcpClient::send_connect, "[NETWORK:TCP] send_connect\n"
dprintf ryu_ldn::network::TcpClient::send_connect_private, "[NETWORK:TCP] send_connect_private\n"
dprintf ryu_ldn::network::TcpClient::send_scan, "[NETWORK:TCP] send_scan\n"
dprintf ryu_ldn::network::TcpClient::send_proxy_data, "[NETWORK:TCP] send_proxy_data\n"
dprintf ryu_ldn::network::TcpClient::send_set_accept_policy, "[NETWORK:TCP] send_set_accept_policy\n"
dprintf ryu_ldn::network::TcpClient::send_set_advertise_data, "[NETWORK:TCP] send_set_advertise_data\n"
dprintf ryu_ldn::network::TcpClient::send_reject, "[NETWORK:TCP] send_reject\n"
dprintf ryu_ldn::network::TcpClient::receive_packet, "[NETWORK:TCP] receive_packet: timeout=%d\n", $x5

echo [NETWORK] tcp: 19 dprintf points\n
