# =========================================
# NETWORK:LIFECYCLE
# =========================================

echo [NETWORK] Loading lifecycle breakpoints...\n
# Namespace: ryu_ldn::network
dprintf ryu_ldn::network::RyuLdnClient::RyuLdnClient, "[NETWORK:LIFECYCLE] Client constructor\n"
dprintf ryu_ldn::network::RyuLdnClient::~RyuLdnClient, "[NETWORK:LIFECYCLE] Client destructor\n"
dprintf ryu_ldn::network::RyuLdnClient::set_config, "[NETWORK:LIFECYCLE] Configuration set\n"

echo [NETWORK] lifecycle: 3 dprintf points\n
