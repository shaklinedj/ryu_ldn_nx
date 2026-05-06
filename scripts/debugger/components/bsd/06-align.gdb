# =========================================
# BSD:ALIGN
# =========================================

echo [BSD] Loading align breakpoints...\n
# Namespace: ams::mitm::bsd
dprintf ams::mitm::bsd::ProxySocket::ProxySocket, "[BSD:ALIGN] ProxySocket constructed: type=%d protocol=%d\n", $x0, $x1
dprintf ams::mitm::bsd::ProxySocket::~ProxySocket, "[BSD:ALIGN] ProxySocket destroyed\n"
dprintf ams::mitm::bsd::ProxySocket::Bind, "[BSD:ALIGN] ProxySocket::Bind\n"
dprintf ams::mitm::bsd::ProxySocket::Connect, "[BSD:ALIGN] ProxySocket::Connect\n"
dprintf ams::mitm::bsd::ProxySocket::SendTo, "[BSD:ALIGN] ProxySocket::SendTo\n"
# Namespace: ryu_ldn::bsd
dprintf ryu_ldn::bsd::IsLdnAddress, "[BSD:ALIGN] IsLdnAddress(free): ip=0x%x\n", $x0
dprintf ryu_ldn::bsd::SockAddrIn::IsLdnAddress, "[BSD:ALIGN] SockAddrIn::IsLdnAddress: this=%p\n", $x0
dprintf ryu_ldn::bsd::SockAddrIn::GetPort, "[BSD:ALIGN] SockAddrIn::GetPort\n"
dprintf ryu_ldn::bsd::SockAddrIn::GetAddr, "[BSD:ALIGN] SockAddrIn::GetAddr\n"

echo [BSD] align: 9 dprintf points\n
