# =========================================
# BSD:CONFIG
# =========================================

echo [BSD] Loading config breakpoints...\n
# Namespace: ams::mitm::bsd
dprintf ams::mitm::bsd::BsdMitmService::Select, "[BSD:CONFIG] Select: nfds=%d\n", $x3
dprintf ams::mitm::bsd::BsdMitmService::Poll, "[BSD:CONFIG] Poll: nfds=%d timeout=%d\n", $x4, $x5
dprintf ams::mitm::bsd::BsdMitmService::Sysctl, "[BSD:CONFIG] Sysctl\n"
dprintf ams::mitm::bsd::BsdMitmService::GetSockOpt, "[BSD:CONFIG] GetSockOpt: fd=%d level=%d optname=%d\n", $x2, $x3, $x4
dprintf ams::mitm::bsd::BsdMitmService::Ioctl, "[BSD:CONFIG] Ioctl: fd=%d request=0x%x\n", $x3, $x4
dprintf ams::mitm::bsd::BsdMitmService::Fcntl, "[BSD:CONFIG] Fcntl: fd=%d cmd=%d arg=%d\n", $x3, $x4, $x5
dprintf ams::mitm::bsd::BsdMitmService::SetSockOpt, "[BSD:CONFIG] SetSockOpt: fd=%d level=%d optname=%d\n", $x2, $x3, $x4
dprintf ams::mitm::bsd::ProxySocket::SetSockOpt, "[BSD:CONFIG] ProxySocket::SetSockOpt\n"
dprintf ams::mitm::bsd::ProxySocket::GetSockOpt, "[BSD:CONFIG] ProxySocket::GetSockOpt\n"
dprintf ams::mitm::bsd::ProxySocketManager::SetSendCallback, "[BSD:CONFIG] SetSendCallback\n"
dprintf ams::mitm::bsd::ProxySocketManager::SetProxyConnectCallback, "[BSD:CONFIG] SetProxyConnectCallback\n"
dprintf ams::mitm::bsd::ProxySocketManager::SetLocalIp, "[BSD:CONFIG] SetLocalIp: ip=0x%x\n", $x1

echo [BSD] config: 12 dprintf points\n
