# =========================================
# BSD:CONNECT
# =========================================

echo [BSD] Loading connect breakpoints...\n
# Namespace: ams::mitm::bsd
dprintf ams::mitm::bsd::BsdMitmService::Accept, "[BSD:CONNECT] Accept: fd=%d\n", $x3
dprintf ams::mitm::bsd::BsdMitmService::Bind, "[BSD:CONNECT] Bind: fd=%d\n", $x2
dprintf ams::mitm::bsd::BsdMitmService::Connect, "[BSD:CONNECT] Connect: fd=%d\n", $x2
dprintf ams::mitm::bsd::BsdMitmService::GetPeerName, "[BSD:CONNECT] GetPeerName: fd=%d\n", $x2
dprintf ams::mitm::bsd::BsdMitmService::GetSockName, "[BSD:CONNECT] GetSockName: fd=%d\n", $x2
dprintf ams::mitm::bsd::BsdMitmService::Listen, "[BSD:CONNECT] Listen: fd=%d backlog=%d\n", $x2, $x3
dprintf ams::mitm::bsd::BsdMitmService::Shutdown, "[BSD:CONNECT] Shutdown: fd=%d how=%d\n", $x2, $x3
dprintf ams::mitm::bsd::BsdMitmService::ShutdownAllSockets, "[BSD:CONNECT] ShutdownAllSockets: pid=%lu how=%d\n", $x2, $x3
dprintf ams::mitm::bsd::ProxySocket::Listen, "[BSD:CONNECT] ProxySocket::Listen\n"
dprintf ams::mitm::bsd::ProxySocket::Accept, "[BSD:CONNECT] ProxySocket::Accept\n"
dprintf ams::mitm::bsd::ProxySocket::Shutdown, "[BSD:CONNECT] ProxySocket::Shutdown\n"

echo [BSD] connect: 11 dprintf points\n
