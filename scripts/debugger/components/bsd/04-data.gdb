# =========================================
# BSD:DATA
# =========================================

echo [BSD] Loading data breakpoints...\n
# Namespace: ams::mitm::bsd
dprintf ams::mitm::bsd::BsdMitmService::Recv, "[BSD:DATA] Recv: fd=%d flags=%d\n", $x3, $x4
dprintf ams::mitm::bsd::BsdMitmService::RecvFrom, "[BSD:DATA] RecvFrom: fd=%d flags=%d\n", $x3, $x4
dprintf ams::mitm::bsd::BsdMitmService::Send, "[BSD:DATA] Send: fd=%d flags=%d\n", $x3, $x4
dprintf ams::mitm::bsd::BsdMitmService::SendTo, "[BSD:DATA] SendTo: fd=%d flags=%d\n", $x3, $x4
dprintf ams::mitm::bsd::BsdMitmService::Write, "[BSD:DATA] Write: fd=%d\n", $x3
dprintf ams::mitm::bsd::BsdMitmService::Read, "[BSD:DATA] Read: fd=%d\n", $x3
dprintf ams::mitm::bsd::ProxySocket::Send, "[BSD:DATA] ProxySocket::Send\n"
dprintf ams::mitm::bsd::ProxySocket::Recv, "[BSD:DATA] ProxySocket::Recv\n"
dprintf ams::mitm::bsd::ProxySocket::RecvFrom, "[BSD:DATA] ProxySocket::RecvFrom\n"
dprintf ams::mitm::bsd::ProxySocket::IncomingData, "[BSD:DATA] ProxySocket::IncomingData\n"
dprintf ams::mitm::bsd::ProxySocket::IncomingConnection, "[BSD:DATA] ProxySocket::IncomingConnection\n"
dprintf ams::mitm::bsd::ProxySocket::HandleConnectResponse, "[BSD:DATA] ProxySocket::HandleConnectResponse\n"
dprintf ams::mitm::bsd::ProxySocketManager::RouteIncomingData, "[BSD:DATA] RouteIncomingData: dest_ip=0x%x dest_port=%u\n", $x3, $x4
dprintf ams::mitm::bsd::ProxySocketManager::SendProxyData, "[BSD:DATA] SendProxyData: src=0x%x:%u -> dst=0x%x:%u\n", $x1, $x2, $x3, $x4
dprintf ams::mitm::bsd::ProxySocketManager::SendProxyConnect, "[BSD:DATA] SendProxyConnect: src=0x%x:%u -> dst=0x%x:%u\n", $x1, $x2, $x3, $x4
dprintf ams::mitm::bsd::ProxySocketManager::RouteConnectResponse, "[BSD:DATA] RouteConnectResponse\n"
dprintf ams::mitm::bsd::ProxySocketManager::RouteConnectRequest, "[BSD:DATA] RouteConnectRequest\n"

echo [BSD] data: 17 dprintf points\n
