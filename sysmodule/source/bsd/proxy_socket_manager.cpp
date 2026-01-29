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

namespace ams::mitm::bsd {

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
    if (socket != nullptr) {
        // Release the port
        const auto& local_addr = socket->GetLocalAddr();
        if (local_addr.GetPort() != 0) {
            m_port_pool.ReleasePort(local_addr.GetPort(), socket->GetProtocol());
        }

        // Close the socket
        Result close_result = socket->Close();
        if (R_FAILED(close_result)) {
            // Log but continue cleanup - socket will be destroyed regardless
            AMS_UNUSED(close_result);
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

    // Clear pending packets buffer
    m_pending_packets.clear();

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
        return false;
    }

    return callback(source_ip, source_port, dest_ip, dest_port, protocol, data, data_len);
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

    // Find socket matching destination
    ProxySocket* socket = FindSocketByDestination(dest_ip, dest_port, protocol);
    if (socket == nullptr) {
        // No matching socket - buffer the packet for later delivery
        if (m_pending_packets.size() >= MaxPendingPackets) {
            m_pending_packets.pop_front();  // Drop oldest
        }
        PendingPacket pkt;
        pkt.source_ip = source_ip;
        pkt.source_port = source_port;
        pkt.dest_ip = dest_ip;
        pkt.dest_port = dest_port;
        pkt.protocol = protocol;
        pkt.data.assign(static_cast<const uint8_t*>(data),
                       static_cast<const uint8_t*>(data) + data_len);
        m_pending_packets.push_back(std::move(pkt));
        return false;
    }

    // Build source address for RecvFrom
    // Note: sin_len is set to 0 to match Ryujinx's BsdSockAddr.FromIPEndPoint behavior
    ryu_ldn::bsd::SockAddrIn from_addr{};
    from_addr.sin_len = 0;
    from_addr.sin_family = static_cast<uint8_t>(ryu_ldn::bsd::AddressFamily::Inet);
    from_addr.sin_port = __builtin_bswap16(source_port);
    from_addr.sin_addr = __builtin_bswap32(source_ip);

    // Queue data to socket
    socket->IncomingData(data, data_len, from_addr);

    return true;
}

ProxySocket* ProxySocketManager::FindSocketByDestination(uint32_t dest_ip, uint16_t dest_port,
                                                          ryu_ldn::bsd::ProtocolType protocol) {
    // Caller must hold m_mutex

    // Check if dest_ip is a broadcast address (ends in .255 or .255.255)
    // LDN subnet is 10.114.x.x with mask 255.255.0.0, so broadcast is 10.114.255.255
    bool is_broadcast = ((dest_ip & 0xFF) == 0xFF) ||     // x.x.x.255
                        ((dest_ip & 0xFFFF) == 0xFFFF);   // x.x.255.255

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

        // Port must match
        if (local_addr.GetPort() != dest_port) {
            continue;
        }

        // IP matching:
        // 1. INADDR_ANY (bound to 0.0.0.0 - accepts any destination)
        // 2. Exact match (bound to specific IP)
        // 3. Broadcast: any socket on the same port receives broadcast packets
        uint32_t local_ip = local_addr.GetAddr();
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
            if ((local_ip & 0xFFFF0000) == (dest_ip & 0xFFFF0000)) {
                return socket.get();
            }
        }
    }

    return nullptr;
}

// =============================================================================
// LDN Network Configuration
// =============================================================================

void ProxySocketManager::SetLocalIp(uint32_t ip) {
    std::scoped_lock lock(m_mutex);
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

    if (socket == nullptr || m_pending_packets.empty()) {
        return;
    }

    auto it = m_pending_packets.begin();
    while (it != m_pending_packets.end()) {
        const auto& pkt = *it;

        // Check if packet matches this socket
        if (pkt.protocol == protocol && pkt.dest_port == port) {
            // Build source address
            // Note: sin_len is set to 0 to match Ryujinx's BsdSockAddr.FromIPEndPoint behavior
            ryu_ldn::bsd::SockAddrIn from_addr{};
            from_addr.sin_len = 0;
            from_addr.sin_family = static_cast<uint8_t>(ryu_ldn::bsd::AddressFamily::Inet);
            from_addr.sin_port = __builtin_bswap16(pkt.source_port);
            from_addr.sin_addr = __builtin_bswap32(pkt.source_ip);

            // Deliver to socket
            socket->IncomingData(pkt.data.data(), pkt.data.size(), from_addr);

            // Remove from pending
            it = m_pending_packets.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace ams::mitm::bsd
