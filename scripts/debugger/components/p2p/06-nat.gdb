# =========================================
# P2P:NAT
# =========================================

echo [P2P] Loading nat breakpoints...\n
# Namespace: ams::mitm::p2p
dprintf ams::mitm::p2p::P2pProxyServer::ReleaseNatPunch, "[P2P:NAT] Releasing NAT punch\n"
dprintf ams::mitm::p2p::LeaseThreadEntry, "[P2P:NAT] Lease thread started\n"
dprintf ams::mitm::p2p::P2pProxyServer::LeaseRenewalLoop, "[P2P:NAT] Lease renewal loop\n"
dprintf ams::mitm::p2p::P2pProxyServer::StartLeaseRenewal, "[P2P:NAT] Starting lease renewal\n"
dprintf ams::mitm::p2p::UpnpPortMapper::Discover, "[P2P:NAT] UPnP discovery initiated\n"
dprintf ams::mitm::p2p::UpnpPortMapper::IsAvailable, "[P2P:NAT] Checking UPnP availability\n"
dprintf ams::mitm::p2p::UpnpPortMapper::AddPortMapping, "[P2P:NAT] Adding port mapping\n"
dprintf ams::mitm::p2p::UpnpPortMapper::DeletePortMapping, "[P2P:NAT] Deleting port mapping\n"
dprintf ams::mitm::p2p::UpnpPortMapper::RefreshPortMapping, "[P2P:NAT] Refreshing port mapping\n"
dprintf ams::mitm::p2p::UpnpPortMapper::GetExternalIPAddress, "[P2P:NAT] Getting external IP\n"
dprintf ams::mitm::p2p::UpnpPortMapper::GetLocalIPAddress, "[P2P:NAT] Getting local IP\n"
dprintf ams::mitm::p2p::UpnpPortMapper::Cleanup, "[P2P:NAT] UPnP cleanup\n"

echo [P2P] nat: 12 dprintf points\n
