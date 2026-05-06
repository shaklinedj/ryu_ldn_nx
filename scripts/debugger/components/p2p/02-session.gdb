# =========================================
# P2P:SESSION
# =========================================

echo [P2P] Loading session breakpoints...\n
# Namespace: ams::mitm::p2p
dprintf ams::mitm::p2p::P2pProxyServer::AddWaitingToken, "[P2P:SESSION] Token added to waiting list\n"
dprintf ams::mitm::p2p::P2pProxyServer::TryRegisterUser, "[P2P:SESSION] User registration attempt\n"
dprintf ams::mitm::p2p::P2pProxyServer::OnSessionDisconnected, "[P2P:SESSION] Session disconnected event\n"
dprintf ams::mitm::p2p::P2pProxyServer::NotifyMasterDisconnect, "[P2P:SESSION] Master disconnect notification\n"
dprintf ams::mitm::p2p::P2pProxySession::Start, "[P2P:SESSION] Session started\n"
dprintf ams::mitm::p2p::P2pProxySession::Disconnect, "[P2P:SESSION] Session disconnecting\n"
dprintf ams::mitm::p2p::SessionRecvThreadEntry, "[P2P:SESSION] Session thread started\n"
dprintf ams::mitm::p2p::P2pProxySession::ReceiveLoop, "[P2P:SESSION] Session receive loop\n"

echo [P2P] session: 8 dprintf points\n
