# =========================================
# NETWORK:CONNECTION
# =========================================

echo [NETWORK] Loading connection breakpoints...\n
# Namespace: ryu_ldn::network
dprintf ryu_ldn::network::RyuLdnClient::get_config, "[NETWORK:CONNECTION] get_config\n"
dprintf ryu_ldn::network::RyuLdnClient::connect, "[NETWORK:CONNECTION] Connect initiated\n"
dprintf ryu_ldn::network::RyuLdnClient::get_state, "[NETWORK:CONNECTION] get_state\n"
dprintf ryu_ldn::network::RyuLdnClient::is_connected, "[NETWORK:CONNECTION] is_connected queried\n"
dprintf ryu_ldn::network::RyuLdnClient::is_ready, "[NETWORK:CONNECTION] is_ready queried\n"
dprintf ryu_ldn::network::RyuLdnClient::is_transitioning, "[NETWORK:CONNECTION] is_transitioning queried\n"
dprintf ryu_ldn::network::RyuLdnClient::get_retry_count, "[NETWORK:CONNECTION] get_retry_count\n"
dprintf ryu_ldn::network::RyuLdnClient::get_last_error_code, "[NETWORK:CONNECTION] get_last_error_code\n"
dprintf ryu_ldn::network::RyuLdnClient::get_last_rtt_ms, "[NETWORK:CONNECTION] get_last_rtt_ms\n"
dprintf ryu_ldn::network::ReconnectManager::ReconnectManager, "[NETWORK:CONNECTION] ReconnectManager created\n"
dprintf ryu_ldn::network::ReconnectManager::get_next_delay_ms, "[NETWORK:CONNECTION] Getting next backoff delay\n"
dprintf ryu_ldn::network::ReconnectManager::should_retry, "[NETWORK:CONNECTION] Should we retry?\n"
dprintf ryu_ldn::network::ReconnectManager::record_failure, "[NETWORK:CONNECTION] Recording connection failure\n"
dprintf ryu_ldn::network::ReconnectManager::reset, "[NETWORK:CONNECTION] Resetting reconnect state\n"

echo [NETWORK] connection: 14 dprintf points\n
