/**
 * @file proxy_socket_manager.cpp
 * @brief Implementation of the Proxy Socket Manager
 *
 * This file implements the central registry for proxy sockets.
 * See proxy_socket_manager.hpp for design documentation.
 *
 * @copyright Copyright (c) 2026 ryu_ldn_nx contributors
 * @license GPL-2.0-or-later
 */

#include "proxy_socket_manager.hpp"
#include "../debug/log.hpp"

namespace ams::mitm::bsd {

namespace {

uint64_t GetCurrentTimeMs() {
    return armTicksToNs(armGetSystemTick()) / 1000000ULL;
}

bool IsBroadcastDestinationIp(uint32_t ip) {
    return ((ip & 0x000000FF) == 0x000000FF) ||
           ((ip & 0x0000FFFF) == 0x0000FFFF);
}

bool PacketMatchesSocketDestination(const ProxySocket* socket, uint32_t dest_ip) {
    if (socket == nullptr) {
        return false;
    }

    const auto& local_addr = socket->GetLocalAddr();
    const uint32_t local_ip = local_addr.GetAddr();

    if (local_ip == 0 || local_ip == dest_ip) {
        return true;
    }

    if (IsBroadcastDestinationIp(dest_ip)) {
        return (local_ip & 0xFFFF0000) == (dest_ip & 0xFFFF0000);
    }

    return false;
}

} // namespace

// =============================================================================
// Singleton
// =============================================================================

ProxySocketManager& ProxySocketManager::GetInstance() {
    static ProxySocketManager instance;
    return instance;
}

// =============================================================================
// Socket Management
// =============================================================================

ProxySocket* ProxySocketManager::CreateProxySocket(s32 fd, ryu_ldn::bsd::SocketType type,
                                                    ryu_ldn::bsd::ProtocolType protocol) {
    std::scoped_lock lock(m_mutex);

    // Check if fd already has a proxy socket
    if (m_sockets.find(fd) != m_sockets.end()) {
        // Already exists - return existing
        return m_sockets[fd].get();
    }

    // Check limit
    if (m_sockets.size() >= MAX_PROXY_SOCKETS) {
        return nullptr;
    }

    // Create new proxy socket
    auto socket = std::make_unique<ProxySocket>(type, protocol);
    ProxySocket* result = socket.get();

    // Add to registry
    m_sockets[fd] = std::move(socket);

    return result;
}

ProxySocket* ProxySocketManager::GetProxySocket(s32 fd) {
    std::scoped_lock lock(m_mutex);

    auto it = m_sockets.find(fd);
    if (it != m_sockets.end()) {
        return it->second.get();
    }

    return nullptr;
}

bool ProxySocketManager::IsProxySocket(s32 fd) const {
    std::scoped_lock lock(m_mutex);
    return m_sockets.find(fd) != m_sockets.end();
}

bool ProxySocketManager::CloseProxySocket(s32 fd) {
    std::scoped_lock lock(m_mutex);

    auto it = m_sockets.find(fd);
    if (it == m_sockets.end()) {
        return false;
    }

    // Get socket info before closing
    ProxySocket* socket = it->second.get();
    uint16_t closed_port = 0;
    ryu_ldn::bsd::ProtocolType closed_protocol = ryu_ldn::bsd::ProtocolType::Udp;
    if (socket != nullptr) {
        closed_protocol = socket->GetProtocol();

        // Release the port
        const auto& local_addr = socket->GetLocalAddr();
        closed_port = local_addr.GetPort();
        if (closed_port != 0) {
            m_port_pool.ReleasePort(closed_port, closed_protocol);
        }

        // Close the socket
        Result close_result = socket->Close();
        if (R_FAILED(close_result)) {
            // Log but continue cleanup - socket will be destroyed regardless
            AMS_UNUSED(close_result);
        }
    }

    // If this was the last socket using this local port/protocol, purge any
    // queued pre-bind packets for that endpoint so stale traffic from a previous
    // phase (e.g. lobby) doesn't get delivered after fast port reuse in race.
    if (closed_port != 0 && m_pending_count > 0) {
        bool has_same_endpoint_socket = false;
        for (const auto& [other_fd, other_socket] : m_sockets) {
            if (other_fd == fd || other_socket == nullptr) {
                continue;
            }
            if (other_socket->GetProtocol() != closed_protocol) {
                continue;
            }
            if (other_socket->GetLocalAddr().GetPort() == closed_port) {
                has_same_endpoint_socket = true;
                break;
            }
        }

        if (!has_same_endpoint_socket) {
            size_t read_idx = m_pending_head;
            size_t write_idx = m_pending_head;
            size_t remaining = m_pending_count;
            size_t kept = 0;

            while (remaining > 0) {
                const PendingPacket& pkt = m_pending_ring[read_idx];
                const bool should_purge = (pkt.protocol == closed_protocol &&
                                           pkt.dest_port == closed_port);

                if (!should_purge) {
                    if (write_idx != read_idx) {
                        m_pending_ring[write_idx] = pkt;
                    }
                    write_idx = (write_idx + 1) % MaxPendingPackets;
                    kept++;
                }

                read_idx = (read_idx + 1) % MaxPendingPackets;
                remaining--;
            }

            m_pending_tail = write_idx;
            m_pending_count = kept;
        }
    }

    // Remove from registry
    m_sockets.erase(it);

    return true;
}

void ProxySocketManager::CloseAllProxySockets() {
    std::scoped_lock lock(m_mutex);

    // Close all sockets
    for (auto& [fd, socket] : m_sockets) {
        if (socket != nullptr) {
            Result close_result = socket->Close();
            if (R_FAILED(close_result)) {
                // Log but continue cleanup - socket will be destroyed regardless
                AMS_UNUSED(close_result);
            }
        }
    }

    // Clear registry
    m_sockets.clear();

    // Release all ports
    m_port_pool.ReleaseAll();
}

void ProxySocketManager::Reset() {
    std::scoped_lock lock(m_mutex);

    // Close all proxy sockets
    for (auto& [fd, socket] : m_sockets) {
        if (socket != nullptr) {
            Result close_result = socket->Close();
            AMS_UNUSED(close_result);
        }
    }
    m_sockets.clear();

    // Release all ports
    m_port_pool.ReleaseAll();

    // Clear pending packets ring (no dealloc — just reset indices)
    m_pending_head = 0;
    m_pending_tail = 0;
    m_pending_count = 0;

    // Reset local IP
    m_local_ip = 0;

    // Clear callbacks
    m_send_callback = nullptr;
    m_proxy_connect_callback = nullptr;
    m_proxy_connect_reply_callback = nullptr;
    m_proxy_disconnect_callback = nullptr;
}

// =============================================================================
// Port Management
// =============================================================================

uint16_t ProxySocketManager::AllocatePort(ryu_ldn::bsd::ProtocolType protocol) {
    return m_port_pool.AllocatePort(protocol);
}

bool ProxySocketManager::ReservePort(uint16_t port, ryu_ldn::bsd::ProtocolType protocol) {
    return m_port_pool.AllocateSpecificPort(port, protocol);
}

void ProxySocketManager::ReleasePort(uint16_t port, ryu_ldn::bsd::ProtocolType protocol) {
    m_port_pool.ReleasePort(port, protocol);
}

// =============================================================================
// Outgoing Data
// =============================================================================

void ProxySocketManager::SetSendCallback(SendProxyDataCallback callback) {
    std::scoped_lock lock(m_mutex);
    m_send_callback = callback;
}

bool ProxySocketManager::SendProxyData(uint32_t source_ip, uint16_t source_port,
                                        uint32_t dest_ip, uint16_t dest_port,
                                        ryu_ldn::bsd::ProtocolType protocol,
                                        const void* data, size_t data_len) {
    SendProxyDataCallback callback;
    {
        std::scoped_lock lock(m_mutex);
        callback = m_send_callback;
    }

    if (callback == nullptr) {
        LOG_WARN("ProxySocketManager::SendProxyData: m_send_callback=nullptr (src=0x%08X:%u dst=0x%08X:%u)",
                 source_ip, source_port, dest_ip, dest_port);
        return false;
    }

    bool ok = callback(source_ip, source_port, dest_ip, dest_port, protocol, data, data_len);
    if (!ok) {
        LOG_WARN("ProxySocketManager::SendProxyData: callback returned false (src=0x%08X:%u dst=0x%08X:%u len=%zu)",
                 source_ip, source_port, dest_ip, dest_port, data_len);
    }
    return ok;
}

void ProxySocketManager::SetProxyConnectCallback(SendProxyConnectCallback callback) {
    std::scoped_lock lock(m_mutex);
    m_proxy_connect_callback = callback;
}

bool ProxySocketManager::SendProxyConnect(uint32_t source_ip, uint16_t source_port,
                                           uint32_t dest_ip, uint16_t dest_port,
                                           ryu_ldn::bsd::ProtocolType protocol) {
    SendProxyConnectCallback callback;
    {
        std::scoped_lock lock(m_mutex);
        callback = m_proxy_connect_callback;
    }

    if (callback == nullptr) {
        return false;
    }

    return callback(source_ip, source_port, dest_ip, dest_port, protocol);
}

void ProxySocketManager::SetProxyConnectReplyCallback(SendProxyConnectReplyCallback callback) {
    std::scoped_lock lock(m_mutex);
    m_proxy_connect_reply_callback = callback;
}

bool ProxySocketManager::SendProxyConnectReply(uint32_t source_ip, uint16_t source_port,
                                                uint32_t dest_ip, uint16_t dest_port,
                                                ryu_ldn::bsd::ProtocolType protocol) {
    SendProxyConnectReplyCallback callback;
    {
        std::scoped_lock lock(m_mutex);
        callback = m_proxy_connect_reply_callback;
    }

    if (callback == nullptr) {
        return false;
    }

    return callback(source_ip, source_port, dest_ip, dest_port, protocol);
}

void ProxySocketManager::SetProxyDisconnectCallback(SendProxyDisconnectCallback callback) {
    std::scoped_lock lock(m_mutex);
    m_proxy_disconnect_callback = callback;
}

bool ProxySocketManager::SendProxyDisconnect(uint32_t source_ip, uint16_t source_port,
                                              uint32_t dest_ip, uint16_t dest_port,
                                              ryu_ldn::bsd::ProtocolType protocol) {
    SendProxyDisconnectCallback callback;
    {
        std::scoped_lock lock(m_mutex);
        callback = m_proxy_disconnect_callback;
    }

    if (callback == nullptr) {
        return false;
    }

    return callback(source_ip, source_port, dest_ip, dest_port, protocol);
}

bool ProxySocketManager::RouteConnectResponse(const ryu_ldn::protocol::ProxyConnectResponse& response) {
    std::scoped_lock lock(m_mutex);

    // Find socket in Connecting state that matches the destination
    uint32_t dest_ip = response.info.source_ipv4;  // Response comes back to our source
    uint16_t dest_port = response.info.source_port;

    for (auto& [fd, socket] : m_sockets) {
        if (socket == nullptr) {
            continue;
        }

        // Check if socket is connecting
        if (socket->GetState() != ProxySocketState::Connecting) {
            continue;
        }

        // Check local address matches
        const auto& local_addr = socket->GetLocalAddr();
        if (local_addr.GetAddr() != dest_ip || local_addr.GetPort() != dest_port) {
            continue;
        }

        // Found matching socket - deliver response
        socket->HandleConnectResponse(response);
        return true;
    }

    return false;
}

bool ProxySocketManager::RouteConnectRequest(const ryu_ldn::protocol::ProxyConnectRequest& request) {
    std::scoped_lock lock(m_mutex);

    // Find listening socket that matches the destination
    uint32_t dest_ip = request.info.dest_ipv4;
    uint16_t dest_port = request.info.dest_port;

    for (auto& [fd, socket] : m_sockets) {
        if (socket == nullptr) {
            continue;
        }

        // Check if socket is listening
        if (socket->GetState() != ProxySocketState::Listening) {
            continue;
        }

        // Check protocol matches (TCP)
        if (socket->GetProtocol() != ryu_ldn::bsd::ProtocolType::Tcp) {
            continue;
        }

        // Check local address matches destination
        const auto& local_addr = socket->GetLocalAddr();

        // Port must match
        if (local_addr.GetPort() != dest_port) {
            continue;
        }

        // IP can be exact match or INADDR_ANY
        uint32_t local_ip = local_addr.GetAddr();
        if (local_ip != 0 && local_ip != dest_ip) {
            continue;
        }

        // Found matching listener - queue the connection
        socket->IncomingConnection(request);
        return true;
    }

    return false;
}

// =============================================================================
// Data Routing
// =============================================================================

bool ProxySocketManager::RouteIncomingData(uint32_t source_ip, uint16_t source_port,
                                            uint32_t dest_ip, uint16_t dest_port,
                                            ryu_ldn::bsd::ProtocolType protocol,
                                            const void* data, size_t data_len) {
    std::scoped_lock lock(m_mutex);

    // Filter out loopback packets (packets sent by ourselves that the server echoed back).
    // The game's network stack does not expect to receive its own sent packets,
    // and processing them causes duplicate packet conflicts in Nintendo's PIA mesh.
    if (m_local_ip != 0 && source_ip == m_local_ip) {
        LOG_VERBOSE("ProxyData: dropped loopback packet from ourselves (src=0x%08X)", source_ip);
        // Consider this packet handled so callers don't route it into legacy
        // fallback buffers (which can accumulate stale self-echo traffic).
        return true;
    }

    // For broadcast/multicast packets, deliver to ALL matching sockets.
    // PIA mesh discovery relies on broadcast UDP reaching every listener
    // on the same port. For unicast, only one socket matches.
    constexpr size_t kMaxSockets = 8;
    ProxySocket* targets[kMaxSockets];
    size_t match_count = FindAllSocketsByDestination(dest_ip, dest_port, protocol,
                                                      targets, kMaxSockets);

    if (match_count == 0) {
        // For UDP gameplay traffic, buffering unmatched packets can re-inject stale
        // data into a later socket lifecycle phase (lobby/race) and trigger
        // protocol-level desyncs. Drop unmatched UDP immediately.
        if (protocol == ryu_ldn::bsd::ProtocolType::Udp) {
            return false;
        }

        // For non-UDP traffic, keep fixed-size post-bind buffering behavior.
        if (m_pending_count >= MaxPendingPackets) {
            // Drop oldest
            m_pending_head = (m_pending_head + 1) % MaxPendingPackets;
            m_pending_count--;
        }
        PendingPacket& slot = m_pending_ring[m_pending_tail];
        slot.source_ip = source_ip;
        slot.source_port = source_port;
        slot.dest_ip = dest_ip;
        slot.dest_port = dest_port;
        slot.protocol = protocol;
        slot.enqueue_time_ms = GetCurrentTimeMs();
        size_t copy_len = std::min(data_len, static_cast<size_t>(PendingPayloadMax));
        slot.len = static_cast<uint16_t>(copy_len);
        if (copy_len > 0 && data != nullptr) {
            std::memcpy(slot.data, data, copy_len);
        }
        m_pending_tail = (m_pending_tail + 1) % MaxPendingPackets;
        m_pending_count++;
        return false;
    }

    // Build source address for RecvFrom
    // Note: Nintendo Switch uses BSD-style sockaddr with sin_len field
    // sin_len must be sizeof(SockAddrIn) = 16 for the game to accept the address
    //
    // CRITICAL: Ryujinx stores IPs as uint in "big-endian read" format:
    // For 10.114.0.1, Ryujinx stores 0x0A720001.
    // BUT in BsdSockAddr, sin_addr must be in NETWORK BYTE ORDER (standard BSD).
    // Ryujinx does Array.Reverse() when converting ProxyInfo.SourceIpV4 to IPEndPoint,
    // then copies bytes directly to sin_addr, resulting in network byte order.
    // So we must bswap32 to convert Ryujinx format -> network byte order.
    // For 10.114.0.1: 0x0A720001 -> bswap32 -> 0x0100720A (network order)
    ryu_ldn::bsd::SockAddrIn from_addr{};
    from_addr.sin_len = sizeof(ryu_ldn::bsd::SockAddrIn);
    from_addr.sin_family = static_cast<uint8_t>(ryu_ldn::bsd::AddressFamily::Inet);
    from_addr.sin_port = __builtin_bswap16(source_port);
    from_addr.sin_addr = __builtin_bswap32(source_ip);  // Convert Ryujinx format to network byte order

    // Deliver to all matching sockets (critical for broadcast UDP)
    for (size_t i = 0; i < match_count; i++) {
        targets[i]->IncomingData(data, data_len, from_addr);
    }

    return true;
}

ProxySocket* ProxySocketManager::FindSocketByDestination(uint32_t dest_ip, uint16_t dest_port,
                                                          ryu_ldn::bsd::ProtocolType protocol) {
    // Caller must hold m_mutex

    // Check if dest_ip is a broadcast address (ends in .255 or .255.255)
    // LDN subnet is 10.114.x.x with mask 255.255.0.0, so broadcast is 10.114.255.255
    // CRITICAL: dest_ip is in Ryujinx format (Big Endian read as uint32)
    // For 10.114.255.255: first octet (10) in HIGH byte, last octet (255) in LOW byte
    // So 10.114.255.255 = 0x0A72FFFF where:
    //   - 0x0A = 10 (first octet, bits 24-31)
    //   - 0x72 = 114 (second octet, bits 16-23)
    //   - 0xFF = 255 (third octet, bits 8-15)
    //   - 0xFF = 255 (fourth octet, bits 0-7)
    // To check .255 (last octet): mask 0x000000FF
    // To check .255.255 (last two octets): mask 0x0000FFFF
    bool is_broadcast = ((dest_ip & 0x000000FF) == 0x000000FF) ||   // x.x.x.255 in Ryujinx format
                        ((dest_ip & 0x0000FFFF) == 0x0000FFFF);     // x.x.255.255 in Ryujinx format

    for (auto& [fd, socket] : m_sockets) {
        if (socket == nullptr) {
            continue;
        }

        // Check protocol matches
        if (socket->GetProtocol() != protocol) {
            continue;
        }

        // Check local address matches destination
        const auto& local_addr = socket->GetLocalAddr();

        // Port must match (GetPort does bswap16, but dest_port is in host order)
        if (local_addr.GetPort() != dest_port) {
            continue;
        }

        // IP matching:
        // Use GetAddr() to convert sin_addr from network byte order to host byte order
        // to properly match against dest_ip which is in Ryujinx host format (0x0A72xxxx).
        // 1. INADDR_ANY (bound to 0.0.0.0 - accepts any destination)
        // 2. Exact match (bound to specific IP)
        // 3. Broadcast: any socket on the same port receives broadcast packets
        uint32_t local_ip = local_addr.GetAddr();  // Host byte order
        if (local_ip == 0) {
            // Bound to INADDR_ANY - accepts all
            return socket.get();
        }
        if (local_ip == dest_ip) {
            // Exact match
            return socket.get();
        }
        if (is_broadcast) {
            // Broadcast packet - deliver to any socket bound on this port
            // Check if socket is in the same subnet (10.114.x.x)
            // In Ryujinx format: 0x0A72xxxx where 0x0A72 is the subnet (10.114)
            // Subnet is in HIGH 16 bits, so mask is 0xFFFF0000
            if ((local_ip & 0xFFFF0000) == (dest_ip & 0xFFFF0000)) {
                return socket.get();
            }
        }
    }

    return nullptr;
}

size_t ProxySocketManager::FindAllSocketsByDestination(uint32_t dest_ip, uint16_t dest_port,
                                                          ryu_ldn::bsd::ProtocolType protocol,
                                                          ProxySocket* out_sockets[], size_t max_sockets) {
    // Caller must hold m_mutex

    if (max_sockets == 0) {
        return 0;
    }

    // Identify if the destination is a broadcast or multicast IP.
    // 1. Limited broadcast: 255.255.255.255 = 0xFFFFFFFF
    // 2. Subnet broadcast: ends with .255 or .255.255 in Ryujinx format (Big-Endian read)
    //    For 10.114.255.255: first octet 10 (0x0A), second octet 114 (0x72), last two octets 0xFFFF.
    //    So check if the last octet is 0xFF, or the last two octets are 0xFFFF.
    // 3. Multicast: 224.0.0.0/4 (0xE0000000 to 0xEFFFFFFF)
    bool is_multicast = (dest_ip & 0xF0000000) == 0xE0000000;
    bool is_broadcast = (dest_ip == 0xFFFFFFFF) ||
                        ((dest_ip & 0x000000FF) == 0x000000FF) ||
                        ((dest_ip & 0x0000FFFF) == 0x0000FFFF) ||
                        is_multicast;

    if (!is_broadcast) {
        // Unicast routing: We only deliver to the single most specific socket matching the port.
        // 1. Exact match (bound to the specific destination IP)
        // 2. Wildcard match (bound to INADDR_ANY)
        ProxySocket* exact_match = nullptr;
        ProxySocket* wildcard_match = nullptr;

        for (auto& [fd, socket] : m_sockets) {
            if (socket == nullptr) continue;
            if (socket->GetProtocol() != protocol) continue;

            const auto& local_addr = socket->GetLocalAddr();
            if (local_addr.GetPort() != dest_port) continue;

            uint32_t local_ip = local_addr.GetAddr();
            if (local_ip == dest_ip) {
                exact_match = socket.get();
                break; // Exact match found, we can stop searching.
            } else if (local_ip == 0) {
                if (wildcard_match == nullptr) {
                    wildcard_match = socket.get();
                }
            }
        }

        if (exact_match != nullptr) {
            out_sockets[0] = exact_match;
            return 1;
        } else if (wildcard_match != nullptr) {
            out_sockets[0] = wildcard_match;
            return 1;
        }
        return 0;
    }

    // Broadcast/Multicast routing: Deliver to ALL matching sockets.
    size_t count = 0;
    for (auto& [fd, socket] : m_sockets) {
        if (socket == nullptr) {
            continue;
        }
        if (count >= max_sockets) {
            break;
        }

        if (socket->GetProtocol() != protocol) {
            continue;
        }

        const auto& local_addr = socket->GetLocalAddr();
        if (local_addr.GetPort() != dest_port) {
            continue;
        }

        uint32_t local_ip = local_addr.GetAddr();

        // 1. Sockets bound to INADDR_ANY (0.0.0.0) receive all broadcast/multicast.
        if (local_ip == 0) {
            out_sockets[count++] = socket.get();
            continue;
        }

        // 2. Global broadcast or multicast: all sockets bound to this port receive it.
        if (dest_ip == 0xFFFFFFFF || is_multicast) {
            out_sockets[count++] = socket.get();
            continue;
        }

        // 3. Sockets bound to exact destination IP receive it.
        if (local_ip == dest_ip) {
            out_sockets[count++] = socket.get();
            continue;
        }

        // 4. Subnet broadcast: Check if the socket's bound IP is in the destination's subnet.
        // LDN subnet is 10.114.x.x (mask 255.255.0.0). High 16 bits must match (0xFFFF0000).
        if ((local_ip & 0xFFFF0000) == (dest_ip & 0xFFFF0000)) {
            out_sockets[count++] = socket.get();
            continue;
        }
    }

    return count;
}

// =============================================================================
// LDN Network Configuration
// =============================================================================

void ProxySocketManager::SetLocalIp(uint32_t ip) {
    std::scoped_lock lock(m_mutex);
    if (m_local_ip != 0 && m_local_ip != ip && m_pending_count > 0) {
        m_pending_head = 0;
        m_pending_tail = 0;
        m_pending_count = 0;
        LOG_INFO("ProxySocketManager: cleared pending packet ring due to local IP change (0x%08X -> 0x%08X)",
                 m_local_ip, ip);
    }
    m_local_ip = ip;
}

uint32_t ProxySocketManager::GetLocalIp() const {
    std::scoped_lock lock(m_mutex);
    return m_local_ip;
}

// =============================================================================
// Statistics
// =============================================================================

size_t ProxySocketManager::GetActiveSocketCount() const {
    std::scoped_lock lock(m_mutex);
    return m_sockets.size();
}

size_t ProxySocketManager::GetAvailablePortCount(ryu_ldn::bsd::ProtocolType protocol) const {
    return m_port_pool.GetAvailableCount(protocol);
}

// =============================================================================
// Pending Packet Delivery
// =============================================================================

void ProxySocketManager::DeliverPendingPackets(ProxySocket* socket, uint16_t port,
                                                ryu_ldn::bsd::ProtocolType protocol) {
    // Called after bind - deliver any buffered packets matching this socket
    // Caller must NOT hold m_mutex (we take it here)
    std::scoped_lock lock(m_mutex);

    if (socket == nullptr || m_pending_count == 0) {
        return;
    }

    // Walk the ring in arrival order, delivering matches and compacting in place.
    size_t read_idx = m_pending_head;
    size_t write_idx = m_pending_head;
    size_t remaining = m_pending_count;
    size_t kept = 0;
    const uint64_t now_ms = GetCurrentTimeMs();

    while (remaining > 0) {
        const PendingPacket& pkt = m_pending_ring[read_idx];
        const bool is_stale = (now_ms > pkt.enqueue_time_ms) &&
                              ((now_ms - pkt.enqueue_time_ms) > MaxPendingPacketAgeMs);

        if (!is_stale &&
            pkt.protocol == protocol &&
            pkt.dest_port == port &&
            PacketMatchesSocketDestination(socket, pkt.dest_ip)) {
            ryu_ldn::bsd::SockAddrIn from_addr{};
            from_addr.sin_len = sizeof(ryu_ldn::bsd::SockAddrIn);
            from_addr.sin_family = static_cast<uint8_t>(ryu_ldn::bsd::AddressFamily::Inet);
            from_addr.sin_port = __builtin_bswap16(pkt.source_port);
            from_addr.sin_addr = __builtin_bswap32(pkt.source_ip);  // Ryujinx -> network byte order

            socket->IncomingData(pkt.data, pkt.len, from_addr);
            // Drop this slot by not copying it forward
        } else {
            if (!is_stale) {
                if (write_idx != read_idx) {
                    m_pending_ring[write_idx] = pkt;
                }
                write_idx = (write_idx + 1) % MaxPendingPackets;
                kept++;
            }
        }

        read_idx = (read_idx + 1) % MaxPendingPackets;
        remaining--;
    }

    m_pending_tail = write_idx;
    m_pending_count = kept;
}

} // namespace ams::mitm::bsd
