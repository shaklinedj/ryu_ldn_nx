# =========================================
# NETWORK:PACKET
# =========================================

echo [NETWORK] Loading packet breakpoints...\n
# Namespace: ryu_ldn::network
dprintf ryu_ldn::network::RyuLdnClient::send_initialize, "[NETWORK:PACKET] send_initialize (handshake)\n"
dprintf ryu_ldn::network::RyuLdnClient::generate_mac_address, "[NETWORK:PACKET] Generating MAC address\n"
dprintf ryu_ldn::network::client_op_result_to_string, "[NETWORK:PACKET] client_op_result_to_string\n"

echo [NETWORK] packet: 3 dprintf points\n
