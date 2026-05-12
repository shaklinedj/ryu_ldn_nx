/**
 * @file itcp_client.hpp
 * @brief Abstract interface for TcpClient operations
 *
 * Enables dependency injection so RyuLdnClient can be tested
 * with a mock TcpClient instead of hitting real sockets.
 *
 * This file also defines the ClientResult enum, which is shared
 * between ITcpClient and TcpClient.
 */

#pragma once

#include <cstdint>
#include <cstddef>

#include "protocol/ryu_protocol.hpp"

namespace ryu_ldn::network {

// =============================================================================
// Result Codes
// =============================================================================

/**
 * @brief Result codes for TcpClient operations
 *
 * These codes provide more context than raw socket errors by combining
 * transport-level and protocol-level error conditions.
 */
enum class ClientResult {
    Success = 0,           ///< Operation completed successfully

    // Connection errors
    NotConnected,          ///< Client is not connected to server
    AlreadyConnected,      ///< Client is already connected
    ConnectionFailed,      ///< Failed to establish connection
    ConnectionLost,        ///< Connection was lost during operation
    Timeout,               ///< Operation timed out

    // Protocol errors
    InvalidPacket,         ///< Received packet failed validation
    ProtocolError,         ///< Protocol-level error (version mismatch, etc.)
    BufferTooSmall,        ///< Provided buffer too small for data
    EncodingError,         ///< Failed to encode outgoing packet

    // Resource errors
    NotInitialized,        ///< Socket subsystem not initialized
    InternalError          ///< Unexpected internal error
};

/**
 * @brief Convert ClientResult to human-readable string
 *
 * @param result ClientResult enum value
 * @return Static string describing the result
 */
inline const char* client_result_to_string(ClientResult result) {
    switch (result) {
        case ClientResult::Success:          return "Success";
        case ClientResult::NotConnected:     return "NotConnected";
        case ClientResult::AlreadyConnected: return "AlreadyConnected";
        case ClientResult::ConnectionFailed: return "ConnectionFailed";
        case ClientResult::ConnectionLost:   return "ConnectionLost";
        case ClientResult::Timeout:          return "Timeout";
        case ClientResult::InvalidPacket:    return "InvalidPacket";
        case ClientResult::ProtocolError:    return "ProtocolError";
        case ClientResult::BufferTooSmall:   return "BufferTooSmall";
        case ClientResult::EncodingError:    return "EncodingError";
        case ClientResult::NotInitialized:   return "NotInitialized";
        case ClientResult::InternalError:    return "InternalError";
        default:                             return "Unknown";
    }
}

// =============================================================================
// ITcpClient Interface
// =============================================================================

/**
 * @brief Abstract interface for TcpClient operations
 *
 * Defines the full contract that RyuLdnClient depends on for TCP
 * communication. The concrete TcpClient implements this interface
 * for production use; test code substitutes a MockTcpClient.
 *
 * ## Dependency Injection
 *
 * RyuLdnClient accepts an ITcpClient via constructor injection.
 * In production, a TcpClient is created automatically. In tests,
 * a MockTcpClient is injected to control return values and verify
 * call counts without real sockets.
 *
 * @see TcpClient for the production implementation
 * @see MockTcpClient for the test double
 */
class ITcpClient {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~ITcpClient() = default;

    /**
     * @brief Initialize the underlying socket subsystem
     *
     * Called once before the first connect() attempt.
     * For TcpClient, this calls socket_init().
     * For MockTcpClient, this is a no-op unless configured to fail.
     *
     * @return true if initialization succeeded
     * @return false if initialization failed
     */
    virtual bool initialize() = 0;

    /**
     * @brief Connect to a remote server
     *
     * @param host Server hostname or IP address
     * @param port Server port number
     * @param timeout_ms Connection timeout in milliseconds
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult connect(const char* host, uint16_t port, uint32_t timeout_ms = 5000) = 0;

    /**
     * @brief Disconnect from the server
     *
     * Safe to call even if not connected.
     */
    virtual void disconnect() = 0;

    /**
     * @brief Check if the TCP connection is established
     *
     * @return true if connected
     */
    virtual bool is_connected() const = 0;

    /**
     * @brief Send a raw protocol packet with header encoding
     *
     * @param type Packet type identifier
     * @param payload Packet payload data
     * @param payload_size Size of payload in bytes
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_packet(protocol::PacketId type, const void* payload, size_t payload_size) = 0;

    /**
     * @brief Send pre-encoded data (used by P2P components)
     *
     * @param data Encoded packet data
     * @param size Size of data in bytes
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_raw(const void* data, size_t size) = 0;

    /**
     * @brief Send Initialize handshake message
     *
     * @param msg Initialize message with session ID and MAC
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_initialize(const protocol::InitializeMessage& msg) = 0;

    /**
     * @brief Send Passphrase message (struct overload)
     *
     * @param msg Passphrase message with room password
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_passphrase(const protocol::PassphraseMessage& msg) = 0;

    /**
     * @brief Send Passphrase message (string overload)
     *
     * @param passphrase Null-terminated passphrase string
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_passphrase(const char* passphrase) = 0;

    /**
     * @brief Send Ping keepalive message
     *
     * @param msg Ping message with requester and ID
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_ping(const protocol::PingMessage& msg) = 0;

    /**
     * @brief Send Disconnect notification to server
     *
     * @param msg Disconnect message with reason code
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_disconnect(const protocol::DisconnectMessage& msg) = 0;

    /**
     * @brief Send CreateAccessPoint request (host mode)
     *
     * @param request Access point configuration
     * @param advertise_data Optional advertise data buffer
     * @param advertise_size Size of advertise data
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_create_access_point(const protocol::CreateAccessPointRequest& request,
                                                    const uint8_t* advertise_data = nullptr,
                                                    size_t advertise_size = 0) = 0;

    /**
     * @brief Send CreateAccessPointPrivate request (private host mode)
     *
     * @param request Private access point configuration
     * @param advertise_data Optional advertise data buffer
     * @param advertise_size Size of advertise data
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_create_access_point_private(const protocol::CreateAccessPointPrivateRequest& request,
                                                            const uint8_t* advertise_data = nullptr,
                                                            size_t advertise_size = 0) = 0;

    /**
     * @brief Send Connect request (join a network)
     *
     * @param request Connection request with target network info
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_connect(const protocol::ConnectRequest& request) = 0;

    /**
     * @brief Send ConnectPrivate request (join a private network)
     *
     * @param request Private connection request
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_connect_private(const protocol::ConnectPrivateRequest& request) = 0;

    /**
     * @brief Send Scan request (find available networks)
     *
     * @param filter Scan filter criteria
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_scan(const protocol::ScanFilterFull& filter) = 0;

    /**
     * @brief Send proxy data (game traffic relay)
     *
     * @param header Proxy header with source/destination nodes
     * @param data Game data to send
     * @param data_size Size of game data
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_proxy_data(const protocol::ProxyDataHeader& header,
                                          const uint8_t* data, size_t data_size) = 0;

    /**
     * @brief Send SetAcceptPolicy request (host-only)
     *
     * @param request Accept policy request
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_set_accept_policy(const protocol::SetAcceptPolicyRequest& request) = 0;

    /**
     * @brief Send SetAdvertiseData request (host-only)
     *
     * @param data Advertise data buffer
     * @param size Size of advertise data
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_set_advertise_data(const uint8_t* data, size_t size) = 0;

    /**
     * @brief Send Reject request (host-only, kick a player)
     *
     * @param request Reject request with node ID and reason
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult send_reject(const protocol::RejectRequest& request) = 0;

    /**
     * @brief Receive the next complete protocol packet
     *
     * Handles TCP stream reassembly internally.
     *
     * @param[out] type Packet type of received packet
     * @param[out] payload Buffer to receive payload data
     * @param payload_buffer_size Size of payload buffer
     * @param[out] payload_size Actual size of received payload
     * @param timeout_ms Receive timeout in milliseconds (-1 = blocking)
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult receive_packet(protocol::PacketId& type,
                                         void* payload, size_t payload_buffer_size,
                                         size_t& payload_size,
                                         int32_t timeout_ms = -1) = 0;

    /**
     * @brief Check if a complete packet is buffered and available
     *
     * @return true if at least one complete packet is available
     */
    virtual bool has_packet_available() const = 0;

    /**
     * @brief Enable or disable TCP_NODELAY
     *
     * @param enable true to disable Nagle's algorithm
     * @return ClientResult indicating success or failure
     */
    virtual ClientResult set_nodelay(bool enable) = 0;
};

} // namespace ryu_ldn::network