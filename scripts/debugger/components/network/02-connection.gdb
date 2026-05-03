# ==========================================
# Network Component - Connection Management
# ==========================================
# Connect, disconnect, reconnect, state queries
# Updated for async architecture (no duplicate symbols from 05)
# Using dprintf for automatic continue

echo [NETWORK] Loading connection management breakpoints...\n

# Connection operations (unique — state callback invocations are in 05)
dprintf ryu_ldn::network::RyuLdnClient::connect, "[NETWORK:CONNECT] Connect initiated\n"
dprintf ryu_ldn::network::RyuLdnClient::disconnect, "[NETWORK:CONNECT] Disconnect initiated\n"
dprintf ryu_ldn::network::RyuLdnClient::try_connect, "[NETWORK:CONNECT] Attempting TCP connection...\n"

# State queries
dprintf ryu_ldn::network::RyuLdnClient::is_connected, "[NETWORK:CONNECT] is_connected queried\n"
dprintf ryu_ldn::network::RyuLdnClient::is_ready, "[NETWORK:CONNECT] is_ready queried\n"
dprintf ryu_ldn::network::RyuLdnClient::is_transitioning, "[NETWORK:CONNECT] is_transitioning queried\n"
dprintf ryu_ldn::network::RyuLdnClient::get_state, "[NETWORK:CONNECT] get_state\n"
dprintf ryu_ldn::network::RyuLdnClient::get_retry_count, "[NETWORK:CONNECT] get_retry_count\n"
dprintf ryu_ldn::network::RyuLdnClient::get_last_error_code, "[NETWORK:CONNECT] get_last_error_code\n"
dprintf ryu_ldn::network::RyuLdnClient::get_last_rtt_ms, "[NETWORK:CONNECT] get_last_rtt_ms\n"
dprintf ryu_ldn::network::RyuLdnClient::get_config, "[NETWORK:CONNECT] get_config\n"

# Reconnection logic (unique — backoff details not in 05)
dprintf ryu_ldn::network::RyuLdnClient::start_backoff, "[NETWORK:CONNECT] Backoff started\n"
dprintf ryu_ldn::network::RyuLdnClient::is_backoff_expired, "[NETWORK:CONNECT] Checking backoff expiration\n"
dprintf ryu_ldn::network::RyuLdnClient::is_handshake_timeout, "[NETWORK:CONNECT] Checking handshake timeout\n"

# ReconnectManager
dprintf ryu_ldn::network::ReconnectManager::ReconnectManager, "[NETWORK:CONNECT] ReconnectManager created\n"
dprintf ryu_ldn::network::ReconnectManager::get_next_delay_ms, "[NETWORK:CONNECT] Getting next backoff delay\n"
dprintf ryu_ldn::network::ReconnectManager::should_retry, "[NETWORK:CONNECT] Should we retry?\n"
dprintf ryu_ldn::network::ReconnectManager::record_failure, "[NETWORK:CONNECT] Recording connection failure\n"
dprintf ryu_ldn::network::ReconnectManager::reset, "[NETWORK:CONNECT] Resetting reconnect state\n"

echo [NETWORK] Connection management: 19 dprintf points\n