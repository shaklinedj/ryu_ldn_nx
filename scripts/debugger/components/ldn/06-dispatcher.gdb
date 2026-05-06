# =========================================
# LDN:DISPATCHER
# =========================================

echo [LDN] Loading dispatcher breakpoints...\n
# Namespace: ryu_ldn::ldn
dprintf ryu_ldn::ldn::PacketDispatcher::PacketDispatcher, "[LDN:DISPATCHER] Dispatcher created\n"
dprintf ryu_ldn::ldn::PacketDispatcher::~PacketDispatcher, "[LDN:DISPATCHER] Dispatcher destroyed\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_initialize_handler, "[LDN:DISPATCHER] Set initialize handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_connected_handler, "[LDN:DISPATCHER] Set connected handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_sync_network_handler, "[LDN:DISPATCHER] Set sync_network handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_scan_reply_handler, "[LDN:DISPATCHER] Set scan_reply handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_scan_reply_end_handler, "[LDN:DISPATCHER] Set scan_reply_end handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_disconnect_handler, "[LDN:DISPATCHER] Set disconnect handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_ping_handler, "[LDN:DISPATCHER] Set ping handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_network_error_handler, "[LDN:DISPATCHER] Set network_error handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_proxy_config_handler, "[LDN:DISPATCHER] Set proxy_config handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_proxy_connect_handler, "[LDN:DISPATCHER] Set proxy_connect handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_proxy_connect_reply_handler, "[LDN:DISPATCHER] Set proxy_connect_reply handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_proxy_data_handler, "[LDN:DISPATCHER] Set proxy_data handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_proxy_disconnect_handler, "[LDN:DISPATCHER] Set proxy_disconnect handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_reject_handler, "[LDN:DISPATCHER] Set reject handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_reject_reply_handler, "[LDN:DISPATCHER] Set reject_reply handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::set_accept_policy_handler, "[LDN:DISPATCHER] Set accept_policy handler\n"
dprintf ryu_ldn::ldn::PacketDispatcher::dispatch, "[LDN:DISPATCHER] Dispatching packet\n"

echo [LDN] dispatcher: 19 dprintf points\n
