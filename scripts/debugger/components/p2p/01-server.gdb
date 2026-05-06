# =========================================
# P2P:SERVER
# =========================================

echo [P2P] Loading server breakpoints...\n
# Namespace: ams::mitm::p2p
dprintf ams::mitm::p2p::P2pProxyServer::Start, "[P2P:SERVER] Server starting\n"
dprintf ams::mitm::p2p::P2pProxyServer::Stop, "[P2P:SERVER] Server stopping\n"
dprintf ams::mitm::p2p::P2pProxyServer::IsRunning, "[P2P:SERVER] Server state queried\n"
dprintf ams::mitm::p2p::P2pProxyServer::Configure, "[P2P:SERVER] Server configuration updated\n"
dprintf ams::mitm::p2p::AcceptThreadEntry, "[P2P:SERVER] Accept thread started\n"
dprintf ams::mitm::p2p::P2pProxyServer::AcceptLoop, "[P2P:SERVER] Accept loop running\n"

echo [P2P] server: 6 dprintf points\n
