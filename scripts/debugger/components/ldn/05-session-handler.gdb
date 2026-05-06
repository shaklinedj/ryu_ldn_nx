# =========================================
# LDN:SESSION_HANDLER
# =========================================

echo [LDN] Loading session_handler breakpoints...\n
# Namespace: ryu_ldn::ldn
dprintf ryu_ldn::ldn::LdnSessionHandler::LdnSessionHandler, "[LDN:SESSION_HANDLER] Handler created\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::set_state_callback, "[LDN:SESSION_HANDLER] State callback registered\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::set_network_updated_callback, "[LDN:SESSION_HANDLER] Network updated callback registered\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::set_scan_result_callback, "[LDN:SESSION_HANDLER] Scan result callback registered\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::set_scan_completed_callback, "[LDN:SESSION_HANDLER] Scan completed callback registered\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::set_disconnected_callback, "[LDN:SESSION_HANDLER] Disconnected callback registered\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::set_error_callback, "[LDN:SESSION_HANDLER] Error callback registered\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::set_rejected_callback, "[LDN:SESSION_HANDLER] Rejected callback registered\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::set_accept_policy_changed_callback, "[LDN:SESSION_HANDLER] Accept policy callback registered\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::handle_initialize, "[LDN:SESSION_HANDLER] handle_initialize\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::handle_connected, "[LDN:SESSION_HANDLER] handle_connected\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::handle_sync_network, "[LDN:SESSION_HANDLER] handle_sync_network\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::handle_scan_reply, "[LDN:SESSION_HANDLER] handle_scan_reply\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::handle_scan_reply_end, "[LDN:SESSION_HANDLER] handle_scan_reply_end\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::handle_ping, "[LDN:SESSION_HANDLER] handle_ping\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::handle_disconnect, "[LDN:SESSION_HANDLER] handle_disconnect\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::handle_network_error, "[LDN:SESSION_HANDLER] handle_network_error\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::handle_reject, "[LDN:SESSION_HANDLER] handle_reject\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::handle_reject_reply, "[LDN:SESSION_HANDLER] handle_reject_reply\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::handle_set_accept_policy, "[LDN:SESSION_HANDLER] handle_set_accept_policy\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::get_state, "[LDN:SESSION_HANDLER] get_state\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::is_in_session, "[LDN:SESSION_HANDLER] is_in_session\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::is_host, "[LDN:SESSION_HANDLER] is_host\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::get_node_count, "[LDN:SESSION_HANDLER] get_node_count\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::leave_session, "[LDN:SESSION_HANDLER] leave_session\n"
dprintf ryu_ldn::ldn::LdnSessionHandler::reset, "[LDN:SESSION_HANDLER] reset\n"

echo [LDN] session_handler: 26 dprintf points\n
