# =========================================
# LDN:PROXY_HANDLER
# =========================================

echo [LDN] Loading proxy_handler breakpoints...\n
# Namespace: ryu_ldn::ldn
dprintf ryu_ldn::ldn::LdnProxyHandler::LdnProxyHandler, "[LDN:PROXY_HANDLER] Handler created\n"
dprintf ryu_ldn::ldn::LdnProxyHandler::set_config_callback, "[LDN:PROXY_HANDLER] Config callback registered\n"
dprintf ryu_ldn::ldn::LdnProxyHandler::set_connect_callback, "[LDN:PROXY_HANDLER] Connect callback registered\n"
dprintf ryu_ldn::ldn::LdnProxyHandler::set_connect_reply_callback, "[LDN:PROXY_HANDLER] Connect reply callback registered\n"
dprintf ryu_ldn::ldn::LdnProxyHandler::set_data_callback, "[LDN:PROXY_HANDLER] Data callback registered\n"
dprintf ryu_ldn::ldn::LdnProxyHandler::set_disconnect_callback, "[LDN:PROXY_HANDLER] Disconnect callback registered\n"
dprintf ryu_ldn::ldn::LdnProxyHandler::handle_proxy_config, "[LDN:PROXY_HANDLER] Handling proxy config\n"
dprintf ryu_ldn::ldn::LdnProxyHandler::handle_proxy_connect, "[LDN:PROXY_HANDLER] Handling proxy connect\n"
dprintf ryu_ldn::ldn::LdnProxyHandler::handle_proxy_connect_reply, "[LDN:PROXY_HANDLER] Handling proxy connect reply\n"
dprintf ryu_ldn::ldn::LdnProxyHandler::handle_proxy_data, "[LDN:PROXY_HANDLER] Handling proxy data\n"
dprintf ryu_ldn::ldn::LdnProxyHandler::handle_proxy_disconnect, "[LDN:PROXY_HANDLER] Handling proxy disconnect\n"
dprintf ryu_ldn::ldn::LdnProxyHandler::reset, "[LDN:PROXY_HANDLER] Reset handler\n"

echo [LDN] proxy_handler: 12 dprintf points\n
