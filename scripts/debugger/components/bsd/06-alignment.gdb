# ==========================================
# BSD Component - Alignment-Sensitive Paths
# ==========================================
# SendTo, Bind, Connect entry points and IsLdnAddress detection.
# These are the critical paths for DABRT 0x101 diagnosis:
# SockAddrIn::IsLdnAddress() does __builtin_bswap32 on sin_addr
# which faults on ARM64 if the struct is misaligned (from IPC
# buffer pointer). The fix copies to an aligned stack buffer first.
# Using dprintf for automatic continue

echo [BSD] Loading alignment-sensitive path breakpoints...\n

# ==========================================
# SendTo — Command 10 / 11
# Two alignment fix points:
#   1. IsLdnAddress check on dest addr (line ~1721)
#   2. Proxy data path dest addr (line ~1773)
# ==========================================

dprintf ams::mitm::bsd::BsdMitmService::SendTo, "[BSD:ALIGN] SendTo: fd=%d flags=%d\n", $x3, $x4

# ==========================================
# Bind — Command 13
# Alignment fix: copy to aligned stack buffer before
# calling IsLdnAddress on the bind address.
# ==========================================

dprintf ams::mitm::bsd::BsdMitmService::Bind, "[BSD:ALIGN] Bind: fd=%d\n", $x2

# ==========================================
# Connect — Command 14
# Alignment fix: copy to aligned stack buffer before
# calling IsLdnAddress on the connect address.
# ==========================================

dprintf ams::mitm::bsd::BsdMitmService::Connect, "[BSD:ALIGN] Connect: fd=%d\n", $x2

# ==========================================
# SockAddrIn alignment detection
# IsLdnAddress is called after the __builtin_memcpy to
# aligned stack buffer. If it crashes here, the memcpy
# itself didn't fix the alignment properly.
# ==========================================

# Free function (ryu_ldn::bsd namespace)
dprintf ryu_ldn::bsd::IsLdnAddress, "[BSD:ALIGN] IsLdnAddress(free): ip=0x%x\n", $x0

# Struct member (packed, alignment-sensitive)
# Note: this is the one that CAN fault if called on misaligned data
dprintf ams::mitm::bsd::SockAddrIn::IsLdnAddress, "[BSD:ALIGN] SockAddrIn::IsLdnAddress: this=%p\n", $x0

dprintf ams::mitm::bsd::SockAddrIn::GetPort, "[BSD:ALIGN] SockAddrIn::GetPort\n"
dprintf ams::mitm::bsd::SockAddrIn::GetAddr, "[BSD:ALIGN] SockAddrIn::GetAddr\n"

# ==========================================
# ProxySocketManager — LDN proxy socket paths
# ==========================================

# Proxy socket creation (happens after IsLdnAddress returns true)
dprintf ams::mitm::bsd::ProxySocketManager::CreateProxySocket, "[BSD:ALIGN] CreateProxySocket: fd=%d type=%d protocol=%d\n", $x0, $x1, $x2
dprintf ams::mitm::bsd::ProxySocketManager::GetProxySocket, "[BSD:ALIGN] GetProxySocket: fd=%d\n", $x0
dprintf ams::mitm::bsd::ProxySocketManager::CloseProxySocket, "[BSD:ALIGN] CloseProxySocket: fd=%d\n", $x0

# Data routing (proxy path — happens after IsLdnAddress detected LDN addr)
dprintf ams::mitm::bsd::ProxySocketManager::RouteIncomingData, "[BSD:ALIGN] RouteIncomingData: dest_ip=0x%x dest_port=%u\n", $x3, $x4
dprintf ams::mitm::bsd::ProxySocketManager::SendProxyData, "[BSD:ALIGN] SendProxyData: src=0x%x:%u -> dst=0x%x:%u\n", $x1, $x2, $x3, $x4
dprintf ams::mitm::bsd::ProxySocketManager::SendProxyConnect, "[BSD:ALIGN] SendProxyConnect: src=0x%x:%u -> dst=0x%x:%u\n", $x1, $x2, $x3, $x4

# ==========================================
# Ephemeral port pool (used by proxy sockets)
# ==========================================

dprintf ams::mitm::bsd::EphemeralPortPool::AllocatePort, "[BSD:ALIGN] AllocatePort: protocol=%d\n", $x0
dprintf ams::mitm::bsd::EphemeralPortPool::AllocateSpecificPort, "[BSD:ALIGN] AllocateSpecificPort: port=%u protocol=%d\n", $x0, $x1
dprintf ams::mitm::bsd::EphemeralPortPool::ReleasePort, "[BSD:ALIGN] ReleasePort: port=%u protocol=%d\n", $x0, $x1

# ==========================================
# Inspection command (component-specific)
# ==========================================

# Note: GDB 'define' commands with complex logic belong in
# memory-trace/ or tools/common.gdb. This file provides
# dprintf breakpoints only.

echo [BSD] Alignment-sensitive paths: 16 dprintf points\n