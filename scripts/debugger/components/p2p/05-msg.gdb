# =========================================
# P2P:MSG
# =========================================

echo [P2P] Loading msg breakpoints...\n
# Namespace: ams::mitm::p2p
dprintf ams::mitm::p2p::P2pProxyClient::SendProxyData, "[P2P:MSG] Client sending proxy data\n"
dprintf ams::mitm::p2p::P2pProxyClient::SendProxyConnect, "[P2P:MSG] Client sending proxy connect\n"
dprintf ams::mitm::p2p::P2pProxyClient::SendProxyConnectReply, "[P2P:MSG] Client sending connect reply\n"
dprintf ams::mitm::p2p::P2pProxyClient::SendProxyDisconnect, "[P2P:MSG] Client sending disconnect\n"
dprintf ams::mitm::p2p::P2pProxyClient::HandleProxyConfig, "[P2P:MSG] Client handling proxy config\n"
dprintf ams::mitm::p2p::P2pProxyClient::HandleProxyData, "[P2P:MSG] Client handling proxy data\n"
dprintf ams::mitm::p2p::P2pProxyClient::HandleProxyConnect, "[P2P:MSG] Client handling proxy connect\n"
dprintf ams::mitm::p2p::P2pProxyClient::HandleProxyConnectReply, "[P2P:MSG] Client handling connect reply\n"
dprintf ams::mitm::p2p::P2pProxyClient::HandleProxyDisconnect, "[P2P:MSG] Client handling disconnect\n"

echo [P2P] msg: 9 dprintf points\n
