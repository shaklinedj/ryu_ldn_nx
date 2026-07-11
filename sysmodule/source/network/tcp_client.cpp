/**
 * @file tcp_client.cpp
 * @brief TCP Client Implementation for RyuLdn Protocol
 *
 * Implements the TcpClient class which provides high-level protocol
 * communication over TCP sockets.
 *
 * ## Implementation Notes
 *
 * ### Packet Buffering
 * TCP is a stream protocol - data may arrive in fragments or multiple
 * packets may arrive together. The PacketBuffer handles reassembly:
 * 1. Raw TCP data is appended to the buffer
 * 2. Buffer is checked for complete packets (header + payload)
 * 3. Complete packets are extracted and returned to caller
 *
 * ### Send Buffer
 * A fixed 2KB send buffer is used for encoding outgoing packets.
 * This is sufficient for all protocol messages (largest is ~1.3KB).
 * The buffer avoids dynamic allocation on the critical send path.
 *
 * ### Error Handling
 * - Socket errors are mapped to ClientResult for consistency
 * - Protocol errors (invalid packets) are detected during receive
 * - Connection loss is detected and reported appropriately
 *
 * @see tcp_client.hpp for the public interface
 */

#include "tcp_client.hpp"
#include "../debug/log.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>

#ifdef TEST_BUILD
#include <chrono>
static uint64_t GetCurrentTimeMs() {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}
#else
#include <switch.h>
static uint64_t GetCurrentTimeMs() {
    return armTicksToNs(armGetSystemTick()) / 1000000ULL;
}
#endif

namespace ryu_ldn::network {

// =============================================================================
// Constructor / Destructor
// =============================================================================

/**
 * @brief Default constructor - creates disconnected client
 *
 * Initializes internal buffers. No connection is established.
 */
TcpClient::TcpClient()
    : m_socket()
    , m_recv_buffer()
{
    // Send buffer is uninitialized - will be filled during send operations
#ifndef TEST_BUILD
    ::ams::os::InitializeConditionVariable(&m_queue_cv);
#endif
}

/**
 * @brief Destructor - ensures clean disconnection
 */
TcpClient::~TcpClient() {
    disconnect();
}

bool TcpClient::initialize() {
    SocketResult result = socket_init();
    return result == SocketResult::Success;
}

/**
 * @brief Move constructor
 *
 * Transfers ownership of socket and buffers from other client.
 *
 * @param other Client to move from
 */
TcpClient::TcpClient(TcpClient&& other) noexcept
    : m_socket(std::move(other.m_socket))
    , m_recv_buffer()  // PacketBuffer doesn't have move semantics, just reset
{
    // Copy buffer state manually if needed (for simplicity, just reset)
    other.m_recv_buffer.reset();
}

/**
 * @brief Move assignment operator
 *
 * Disconnects current connection (if any) and takes ownership from other.
 *
 * @param other Client to move from
 * @return Reference to this client
 */
TcpClient& TcpClient::operator=(TcpClient&& other) noexcept {
    if (this != &other) {
        disconnect();
        m_socket = std::move(other.m_socket);
        m_recv_buffer.reset();
        other.m_recv_buffer.reset();
    }
    return *this;
}

// =============================================================================
// Connection Management
// =============================================================================

/**
 * @brief Connect to RyuLdn server
 *
 * Establishes TCP connection and prepares for protocol communication.
 * The receive buffer is reset to ensure clean state.
 */
ClientResult TcpClient::connect(const char* host, uint16_t port, uint32_t timeout_ms, bool use_tls) {
    LOG_VERBOSE("TcpClient::connect(%s, %u, %u)", host, port, timeout_ms);

    // Check if already connected
    if (m_socket.is_connected()) {
        LOG_WARN("TcpClient already connected");
        return ClientResult::AlreadyConnected;
    }

    // Ensure any previous stale socket state is cleaned up before reconnecting
    disconnect();

    // Attempt connection
    SocketResult result = m_socket.connect(host, port, timeout_ms, use_tls);

    if (result != SocketResult::Success) {
        LOG_VERBOSE("Socket connect failed: %s", socket_result_to_string(result));
        return socket_to_client_result(result);
    }

    // Reset receive buffer for new connection
    m_recv_buffer.reset();

    // Enable TCP_NODELAY by default for lower latency
    m_socket.set_nodelay(true);
    m_socket.set_non_blocking(true);

    // Keep a reasonably large OS send buffer so gameplay bursts don't immediately
    // overflow the app queue and force packet drops.
    m_socket.set_send_buffer_size(128 * 1024);
    m_socket.set_recv_buffer_size(32 * 1024);

    // Start sender thread
    m_send_thread_running = true;
    m_send_queue.clear();
    m_send_queue_size = 0;
#ifdef TEST_BUILD
    m_send_thread = std::thread(&TcpClient::SendThreadLoop, this);
#else
    ::ams::Result rc = ::ams::os::CreateThread(
        &m_send_thread,
        SendThreadEntry,
        this,
        m_send_thread_stack,
        sizeof(m_send_thread_stack),
        6 // Priority, same as other MITM threads
    );
    if (R_SUCCEEDED(rc)) {
        ::ams::os::SetThreadNamePointer(&m_send_thread, "tcp_send");
        ::ams::os::StartThread(&m_send_thread);
    } else {
        LOG_ERROR("TcpClient: Failed to start sender thread");
        m_send_thread_running = false;
        m_socket.close();
        return ClientResult::ConnectionLost;
    }
#endif

    LOG_VERBOSE("TcpClient connected successfully");
    return ClientResult::Success;
}

/**
 * @brief Disconnect from server
 *
 * Closes socket and resets internal state.
 */
void TcpClient::disconnect() {
    LOG_INFO("TcpClient disconnecting");

    // Stop sender thread
    if (m_send_thread_running) {
        m_send_thread_running = false;
#ifdef TEST_BUILD
        m_queue_cv.notify_one();
        if (m_send_thread.joinable()) {
            m_send_thread.join();
        }
#else
        ::ams::os::SignalConditionVariable(&m_queue_cv);
        ::ams::os::WaitThread(&m_send_thread);
        ::ams::os::DestroyThread(&m_send_thread);
#endif
        m_send_queue.clear();
        m_send_queue_size = 0;
    }

    // Close socket to wake up any blocking recv
    if (m_socket.is_connected()) {
        m_socket.close();
    }
    m_recv_buffer.reset();
}

/**
 * @brief Check connection status
 */
bool TcpClient::is_connected() const {
    return m_socket.is_connected();
}

// =============================================================================
// Send Operations
// =============================================================================

/**
 * @brief Send a raw protocol packet
 *
 * Encodes the packet with header and sends over the socket.
 * Uses internal send buffer for encoding.
 */
ClientResult TcpClient::send_packet(protocol::PacketId type, const void* payload, size_t payload_size) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    // Encode packet into send buffer using encode_raw for arbitrary data
    size_t encoded_size = 0;
    protocol::EncodeResult encode_result = protocol::encode_raw(
        m_send_buffer,
        sizeof(m_send_buffer),
        type,
        static_cast<const uint8_t*>(payload),
        payload_size,
        encoded_size
    );

    if (encode_result != protocol::EncodeResult::Success) {
        return ClientResult::EncodingError;
    }

    // Send the encoded packet
    SocketResult send_result = m_socket.send_all(m_send_buffer, encoded_size);

    if (send_result != SocketResult::Success) {
        return socket_to_client_result(send_result);
    }

    return ClientResult::Success;
}

/**
 * @brief Send raw pre-encoded data
 */
ClientResult TcpClient::send_raw(const void* data, size_t size) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    if (data == nullptr || size == 0) {
        return ClientResult::EncodingError;
    }

    // Send the already-encoded data directly
    SocketResult send_result = m_socket.send_all(static_cast<const uint8_t*>(data), size);

    if (send_result != SocketResult::Success) {
        return socket_to_client_result(send_result);
    }

    return ClientResult::Success;
}

/**
 * @brief Send Initialize message
 */
ClientResult TcpClient::send_initialize(const protocol::InitializeMessage& msg) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    size_t encoded_size = 0;
    protocol::EncodeResult encode_result = protocol::encode(
        m_send_buffer, sizeof(m_send_buffer),
        protocol::PacketId::Initialize, msg, encoded_size);

    if (encode_result != protocol::EncodeResult::Success) {
        return ClientResult::EncodingError;
    }

    SocketResult send_result = m_socket.send_all(m_send_buffer, encoded_size);
    return send_result == SocketResult::Success ? ClientResult::Success : socket_to_client_result(send_result);
}

/**
 * @brief Send Passphrase message
 */
ClientResult TcpClient::send_passphrase(const protocol::PassphraseMessage& msg) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    size_t encoded_size = 0;
    protocol::EncodeResult encode_result = protocol::encode(
        m_send_buffer, sizeof(m_send_buffer),
        protocol::PacketId::Passphrase, msg, encoded_size);

    if (encode_result != protocol::EncodeResult::Success) {
        return ClientResult::EncodingError;
    }

    SocketResult send_result = m_socket.send_all(m_send_buffer, encoded_size);
    return send_result == SocketResult::Success ? ClientResult::Success : socket_to_client_result(send_result);
}

/**
 * @brief Send Passphrase message (string convenience overload)
 */
ClientResult TcpClient::send_passphrase(const char* passphrase) {
    protocol::PassphraseMessage msg{};
    std::memset(msg.passphrase, 0, sizeof(msg.passphrase));
    if (passphrase != nullptr) {
        size_t len = std::strlen(passphrase);
        if (len > 127) len = 127;
        std::memcpy(msg.passphrase, passphrase, len);
    }
    return send_passphrase(msg);
}

/**
 * @brief Send Ping message
 */
ClientResult TcpClient::send_ping(const protocol::PingMessage& msg) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    size_t encoded_size = 0;
    protocol::EncodeResult encode_result = protocol::encode(
        m_send_buffer, sizeof(m_send_buffer),
        protocol::PacketId::Ping, msg, encoded_size);

    if (encode_result != protocol::EncodeResult::Success) {
        return ClientResult::EncodingError;
    }

    SocketResult send_result = m_socket.send_all(m_send_buffer, encoded_size);
    return send_result == SocketResult::Success ? ClientResult::Success : socket_to_client_result(send_result);
}

/**
 * @brief Send Disconnect message
 */
ClientResult TcpClient::send_disconnect(const protocol::DisconnectMessage& msg) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    size_t encoded_size = 0;
    protocol::EncodeResult encode_result = protocol::encode(
        m_send_buffer, sizeof(m_send_buffer),
        protocol::PacketId::Disconnect, msg, encoded_size);

    if (encode_result != protocol::EncodeResult::Success) {
        return ClientResult::EncodingError;
    }

    SocketResult send_result = m_socket.send_all(m_send_buffer, encoded_size);
    return send_result == SocketResult::Success ? ClientResult::Success : socket_to_client_result(send_result);
}

/**
 * @brief Send CreateAccessPoint request
 */
ClientResult TcpClient::send_create_access_point(const protocol::CreateAccessPointRequest& request,
                                                   const uint8_t* advertise_data,
                                                   size_t advertise_size) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    // Wire layout (same as Ryujinx LdnMasterProxyClient.CreateNetwork):
    //   LdnHeader | CreateAccessPointRequest | AdvertiseData[N]
    // The server stores AdvertiseData verbatim in NetworkInfo.ldn so the
    // game sees it on join. Sending the request without the trailing
    // advertise blob made the host's lobby spin forever because
    // NetworkInfo.advertiseDataSize came back as 0.
    const size_t total_payload_size = sizeof(request) + advertise_size;
    if (total_payload_size > sizeof(m_send_buffer) - sizeof(protocol::LdnHeader)) {
        return ClientResult::BufferTooSmall;
    }

    protocol::LdnHeader header{};
    header.magic = protocol::PROTOCOL_MAGIC;
    header.version = protocol::PROTOCOL_VERSION;
    header.type = static_cast<uint8_t>(protocol::PacketId::CreateAccessPoint);
    header.data_size = static_cast<uint32_t>(total_payload_size);

    std::memcpy(m_send_buffer, &header, sizeof(header));
    size_t offset = sizeof(header);

    std::memcpy(m_send_buffer + offset, &request, sizeof(request));
    offset += sizeof(request);

    if (advertise_data && advertise_size > 0) {
        std::memcpy(m_send_buffer + offset, advertise_data, advertise_size);
        offset += advertise_size;
    }

    LOG_INFO("send_create_access_point: header=%zu, payload=%zu, advertise=%zu, total=%zu bytes",
             sizeof(protocol::LdnHeader), sizeof(request), advertise_size, offset);

    // Hex-dump the payload in 32-byte rows so we can confirm the bytes
    // the server sees on the wire match what we intend (struct order /
    // endianness / alignment). MAX_LOG_MESSAGE_LENGTH = 256 chars and one
    // 32-byte row in "%02X " form is 96 chars, so each row fits cleanly.
    //
    // Offset map of CreateAccessPoint in the buffer:
    //   [0..11]    : LdnHeader                 (magic RLDN, type=02, version=01, padding, data_size LE)
    //   [12..79]   : SecurityConfig (0x44)     (SecurityMode u16, PassphraseSize u16, Passphrase[64])
    //   [80..127]  : UserConfig (0x30)         (UserName[33], Unknown[15])
    //   [128..159] : NetworkConfig (0x20)      (IntentId[16], channel u16, NodeCountMax u8, reserved, LocalCommVersion u16, reserved[10])
    //   [160..199] : RyuNetworkConfig (0x28)
    //                  160..175 GameVersion[16]
    //                  176..191 PrivateIp[16]
    //                  192..195 AddressFamily       (uint32 LE, expect 02 00 00 00)
    //                  196..197 ExternalProxyPort   (uint16 LE, 39990 = F6 9C)
    //                  198..199 InternalProxyPort   (uint16 LE, 39990 = F6 9C)
    //   [200..]    : advertise_data
    {
        const size_t dump_len = std::min<size_t>(offset, 224);
        constexpr size_t RowBytes = 32;
        for (size_t row_start = 0; row_start < dump_len; row_start += RowBytes) {
            const size_t row_len = std::min<size_t>(RowBytes, dump_len - row_start);
            char hex[3 * RowBytes + 1] = {};
            for (size_t i = 0; i < row_len; i++) {
                std::snprintf(hex + i * 3, 4, "%02X ", m_send_buffer[row_start + i]);
            }
            LOG_INFO("send_create_access_point wire [%03zu..%03zu]: %s",
                     row_start, row_start + row_len - 1, hex);
        }
        ryu_ldn::debug::g_logger.flush();
    }

    SocketResult send_result = m_socket.send_all(m_send_buffer, offset);
    return send_result == SocketResult::Success ? ClientResult::Success : socket_to_client_result(send_result);
}

/**
 * @brief Send Connect request
 */
ClientResult TcpClient::send_connect(const protocol::ConnectRequest& request) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    size_t encoded_size = 0;
    protocol::EncodeResult encode_result = protocol::encode(
        m_send_buffer, sizeof(m_send_buffer),
        protocol::PacketId::Connect, request, encoded_size);

    if (encode_result != protocol::EncodeResult::Success) {
        return ClientResult::EncodingError;
    }

    LOG_INFO("send_connect: header=%zu, payload=%zu, total=%zu bytes",
             sizeof(protocol::LdnHeader), sizeof(request), encoded_size);

    // Dump first 32 bytes of packet for debugging
    LOG_INFO("send_connect packet[0-31]: %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X",
             m_send_buffer[0], m_send_buffer[1], m_send_buffer[2], m_send_buffer[3],
             m_send_buffer[4], m_send_buffer[5], m_send_buffer[6], m_send_buffer[7],
             m_send_buffer[8], m_send_buffer[9], m_send_buffer[10], m_send_buffer[11],
             m_send_buffer[12], m_send_buffer[13], m_send_buffer[14], m_send_buffer[15],
             m_send_buffer[16], m_send_buffer[17], m_send_buffer[18], m_send_buffer[19],
             m_send_buffer[20], m_send_buffer[21], m_send_buffer[22], m_send_buffer[23],
             m_send_buffer[24], m_send_buffer[25], m_send_buffer[26], m_send_buffer[27],
             m_send_buffer[28], m_send_buffer[29], m_send_buffer[30], m_send_buffer[31]);

    SocketResult send_result = m_socket.send_all(m_send_buffer, encoded_size);
    return send_result == SocketResult::Success ? ClientResult::Success : socket_to_client_result(send_result);
}

/**
 * @brief Send CreateAccessPointPrivate request
 */
ClientResult TcpClient::send_create_access_point_private(
        const protocol::CreateAccessPointPrivateRequest& request,
        const uint8_t* advertise_data,
        size_t advertise_size) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    // Encode header + request + advertise data
    size_t total_payload_size = sizeof(request) + advertise_size;
    if (total_payload_size > sizeof(m_send_buffer) - sizeof(protocol::LdnHeader)) {
        return ClientResult::BufferTooSmall;
    }

    // Build header
    protocol::LdnHeader header{};
    header.magic = protocol::PROTOCOL_MAGIC;
    header.version = protocol::PROTOCOL_VERSION;
    header.type = static_cast<uint8_t>(protocol::PacketId::CreateAccessPointPrivate);
    header.data_size = static_cast<uint32_t>(total_payload_size);

    // Copy header
    std::memcpy(m_send_buffer, &header, sizeof(header));
    size_t offset = sizeof(header);

    // Copy request
    std::memcpy(m_send_buffer + offset, &request, sizeof(request));
    offset += sizeof(request);

    // Copy advertise data if present
    if (advertise_data && advertise_size > 0) {
        std::memcpy(m_send_buffer + offset, advertise_data, advertise_size);
        offset += advertise_size;
    }

    SocketResult send_result = m_socket.send_all(m_send_buffer, offset);
    return send_result == SocketResult::Success ? ClientResult::Success : socket_to_client_result(send_result);
}

/**
 * @brief Send ConnectPrivate request
 */
ClientResult TcpClient::send_connect_private(const protocol::ConnectPrivateRequest& request) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    size_t encoded_size = 0;
    protocol::EncodeResult encode_result = protocol::encode(
        m_send_buffer, sizeof(m_send_buffer),
        protocol::PacketId::ConnectPrivate, request, encoded_size);

    if (encode_result != protocol::EncodeResult::Success) {
        return ClientResult::EncodingError;
    }

    SocketResult send_result = m_socket.send_all(m_send_buffer, encoded_size);
    return send_result == SocketResult::Success ? ClientResult::Success : socket_to_client_result(send_result);
}

/**
 * @brief Send Scan request
 */
ClientResult TcpClient::send_scan(const protocol::ScanFilterFull& filter) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    size_t encoded_size = 0;
    protocol::EncodeResult encode_result = protocol::encode(
        m_send_buffer, sizeof(m_send_buffer),
        protocol::PacketId::Scan, filter, encoded_size);

    if (encode_result != protocol::EncodeResult::Success) {
        return ClientResult::EncodingError;
    }

    SocketResult send_result = m_socket.send_all(m_send_buffer, encoded_size);
    return send_result == SocketResult::Success ? ClientResult::Success : socket_to_client_result(send_result);
}

/**
 * @brief Send proxy data
 *
 * Proxy data packets are special - they combine header + variable data.
 * We need to encode them together in the send buffer.
 */
ClientResult TcpClient::send_proxy_data(const protocol::ProxyDataHeader& header,
                                         const uint8_t* data, size_t data_size) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    size_t encoded_size = 0;
    protocol::EncodeResult encode_result = protocol::encode_proxy_data(
        m_send_buffer, sizeof(m_send_buffer),
        header.info, data, data_size, encoded_size);

    if (encode_result != protocol::EncodeResult::Success) {
        return ClientResult::EncodingError;
    }

    // Push to background send queue
    {
#ifdef TEST_BUILD
        std::lock_guard<std::mutex> lock(m_queue_mutex);
#else
        std::scoped_lock lock(m_queue_mutex);
#endif

        // If queue is full, drop the OLDEST packets (head-drop) to make room for the NEW packet.
        // This ensures Ryujinx always gets the freshest drivedata after a lag spike,
        // preventing the 'ReceiveOldDrivedata' disconnection error.
        while (m_send_queue_size + encoded_size > kMaxSendQueueSize && !m_send_queue.empty()) {
            m_send_queue_size -= m_send_queue.front().data.size();
            m_send_queue.pop_front();
            LOG_WARN("TcpClient::send_proxy_data: Queue full (size=%zu), dropping OLDEST UDP packet", m_send_queue_size);
        }

        std::vector<uint8_t> packet(m_send_buffer, m_send_buffer + encoded_size);
        m_send_queue.push_back({GetCurrentTimeMs(), std::move(packet)});
        m_send_queue_size += encoded_size;
    }

#ifdef TEST_BUILD
    m_queue_cv.notify_one();
#else
    ::ams::os::SignalConditionVariable(&m_queue_cv);
#endif

    return ClientResult::Success;
}

/**
 * @brief Send SetAcceptPolicy request
 */
ClientResult TcpClient::send_set_accept_policy(const protocol::SetAcceptPolicyRequest& request) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    size_t encoded_size = 0;
    protocol::EncodeResult encode_result = protocol::encode(
        m_send_buffer, sizeof(m_send_buffer),
        protocol::PacketId::SetAcceptPolicy, request, encoded_size);

    if (encode_result != protocol::EncodeResult::Success) {
        return ClientResult::EncodingError;
    }

    SocketResult send_result = m_socket.send_all(m_send_buffer, encoded_size);
    return send_result == SocketResult::Success ? ClientResult::Success : socket_to_client_result(send_result);
}

/**
 * @brief Send SetAdvertiseData request
 */
ClientResult TcpClient::send_set_advertise_data(const uint8_t* data, size_t size) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    // Limit to max advertise data size (384 bytes as per protocol)
    if (size > 384) {
        size = 384;
    }

    // Build header manually for variable-size payload
    protocol::LdnHeader header{};
    header.magic = protocol::PROTOCOL_MAGIC;
    header.version = protocol::PROTOCOL_VERSION;
    header.type = static_cast<uint8_t>(protocol::PacketId::SetAdvertiseData);
    header.data_size = static_cast<int32_t>(size);

    // Encode header
    std::memcpy(m_send_buffer, &header, sizeof(header));
    // Append data
    if (data && size > 0) {
        std::memcpy(m_send_buffer + sizeof(header), data, size);
    }

    size_t total_size = sizeof(header) + size;
    SocketResult send_result = m_socket.send_all(m_send_buffer, total_size);
    return send_result == SocketResult::Success ? ClientResult::Success : socket_to_client_result(send_result);
}

/**
 * @brief Send Reject request
 */
ClientResult TcpClient::send_reject(const protocol::RejectRequest& request) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    std::scoped_lock send_lock(m_send_mutex);

    size_t encoded_size = 0;
    protocol::EncodeResult encode_result = protocol::encode(
        m_send_buffer, sizeof(m_send_buffer),
        protocol::PacketId::Reject, request, encoded_size);

    if (encode_result != protocol::EncodeResult::Success) {
        return ClientResult::EncodingError;
    }

    SocketResult send_result = m_socket.send_all(m_send_buffer, encoded_size);
    return send_result == SocketResult::Success ? ClientResult::Success : socket_to_client_result(send_result);
}

// =============================================================================
// Receive Operations
// =============================================================================

/**
 * @brief Receive next protocol packet
 *
 * This function handles the complexity of TCP stream reassembly:
 * 1. Check if a complete packet is already buffered
 * 2. If not, receive more data from socket
 * 3. Repeat until complete packet or timeout
 * 4. Extract and return the packet
 */
ClientResult TcpClient::receive_packet(protocol::PacketId& type,
                                        void* payload, size_t payload_buffer_size,
                                        size_t& payload_size,
                                        int32_t timeout_ms) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    // Serialize receivers — m_recv_buffer state (partial-packet reassembly)
    // is not safe for concurrent access. receive_into_buffer is called only
    // from here so no nested locking needed.
    std::scoped_lock recv_lock(m_recv_mutex);

    payload_size = 0;

    // Loop until we have a complete packet or error/timeout
    while (!m_recv_buffer.has_complete_packet()) {
        ClientResult recv_result = receive_into_buffer(timeout_ms);

        if (recv_result != ClientResult::Success) {
            return recv_result;
        }
    }

    // We have a complete packet - get packet info
    size_t packet_size = 0;
    protocol::BufferResult peek_result = m_recv_buffer.peek_packet_info(packet_size);

    if (peek_result != protocol::BufferResult::Success) {
        return ClientResult::InvalidPacket;
    }

    // Decode header to get type and payload size
    protocol::LdnHeader header;
    protocol::DecodeResult decode_result = protocol::decode_header(
        m_recv_buffer.data(), m_recv_buffer.size(), header);

    if (decode_result != protocol::DecodeResult::Success) {
        return ClientResult::InvalidPacket;
    }

    // Check if payload fits in user buffer
    size_t packet_payload_size = static_cast<size_t>(header.data_size);
    payload_size = packet_payload_size;

    if (packet_payload_size > payload_buffer_size) {
        // Don't consume the packet - let caller provide larger buffer
        return ClientResult::BufferTooSmall;
    }

    // Extract the payload (skip header)
    if (packet_payload_size > 0) {
        std::memcpy(payload, m_recv_buffer.data() + sizeof(protocol::LdnHeader), packet_payload_size);
    }

    // Consume the packet from buffer
    m_recv_buffer.consume(packet_size);

    type = static_cast<protocol::PacketId>(header.type);
    return ClientResult::Success;
}

/**
 * @brief Check if a complete packet is available
 */
bool TcpClient::has_packet_available() const {
    return m_recv_buffer.has_complete_packet();
}

// =============================================================================
// Configuration
// =============================================================================

/**
 * @brief Enable/disable TCP_NODELAY
 */
ClientResult TcpClient::set_nodelay(bool enable) {
    if (!m_socket.is_connected()) {
        return ClientResult::NotConnected;
    }

    // setsockopt is thread-safe at the kernel level and does not touch
    // m_send_buffer, so no lock needed.
    SocketResult result = m_socket.set_nodelay(enable);
    return socket_to_client_result(result);
}

// =============================================================================
// Private Helper Functions
// =============================================================================

/**
 * @brief Convert SocketResult to ClientResult
 *
 * Maps low-level socket errors to appropriate client-level results.
 * WouldBlock is mapped to Success because in non-blocking mode a
 * recv/send returning EAGAIN/EWOULDBLOCK means "no data yet, try
 * later" — the caller's polling loop treats this the same as a
 * successful zero-byte read and retries on the next iteration.
 */
ClientResult TcpClient::socket_to_client_result(SocketResult socket_result) {
    switch (socket_result) {
        case SocketResult::Success:
            return ClientResult::Success;

        case SocketResult::WouldBlock:
            return ClientResult::Success;  // Not an error in async context

        case SocketResult::Timeout:
            return ClientResult::Timeout;

        case SocketResult::NotConnected:
            return ClientResult::NotConnected;

        case SocketResult::AlreadyConnected:
            return ClientResult::AlreadyConnected;

        case SocketResult::NotInitialized:
            return ClientResult::NotInitialized;

        case SocketResult::Closed:
        case SocketResult::ConnectionReset:
            return ClientResult::ConnectionLost;

        case SocketResult::ConnectionRefused:
        case SocketResult::HostUnreachable:
        case SocketResult::NetworkDown:
        case SocketResult::InvalidAddress:
            return ClientResult::ConnectionFailed;

        default:
            return ClientResult::InternalError;
    }
}

/**
 * @brief Receive data into packet buffer
 *
 * Reads available data from socket and appends to the receive buffer.
 * Handles WouldBlock gracefully for non-blocking operation.
 */
ClientResult TcpClient::receive_into_buffer(int32_t timeout_ms) {
    // Temporary buffer for receiving
    uint8_t temp_buffer[4096];
    size_t received = 0;

    SocketResult recv_result = m_socket.recv(temp_buffer, sizeof(temp_buffer), received, timeout_ms);

    if (recv_result == SocketResult::WouldBlock) {
        // No data available right now - not an error
        return ClientResult::Timeout;
    }

    if (recv_result == SocketResult::Timeout) {
        return ClientResult::Timeout;
    }

    if (recv_result == SocketResult::Closed) {
        LOG_INFO("recv: SocketResult::Closed (server sent FIN)");
        return ClientResult::ConnectionLost;
    }

    if (recv_result != SocketResult::Success) {
        LOG_INFO("recv: non-success socket result %d", static_cast<int>(recv_result));
        return socket_to_client_result(recv_result);
    }

    if (received == 0) {
        // Zero bytes with Success means connection closed gracefully
        LOG_INFO("recv: 0 bytes with Success (graceful close by peer)");
        return ClientResult::ConnectionLost;
    }

    // Append received data to packet buffer
    protocol::BufferResult append_result = m_recv_buffer.append(temp_buffer, received);
    if (append_result != protocol::BufferResult::Success) {
        LOG_WARN("recv: recv_buffer.append failed (overflow, %zu bytes)", received);
        return ClientResult::InvalidPacket;
    }

    return ClientResult::Success;
}

// =============================================================================
// Background Sender Thread
// =============================================================================

void TcpClient::SendThreadEntry(void* arg) {
    auto* client = static_cast<TcpClient*>(arg);
    client->SendThreadLoop();
}

void TcpClient::SendThreadLoop() {
    size_t send_offset = 0;

    while (m_send_thread_running) {
        // Step 1: Wait for data to be available in the queue (if we don't have a partially sent packet)
        if (send_offset == 0) {
#ifdef TEST_BUILD
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_queue_cv.wait(lock, [this]() {
                return !m_send_queue.empty() || !m_send_thread_running;
            });
            if (!m_send_thread_running) {
                break;
            }
#else
            m_queue_mutex.Lock();
            while (m_send_queue.empty() && m_send_thread_running) {
                ::ams::os::WaitConditionVariable(&m_queue_cv, m_queue_mutex.GetBase());
            }
            if (!m_send_thread_running) {
                m_queue_mutex.Unlock();
                break;
            }
#endif
            // Check age of the packet at the front of the queue
            uint64_t now_ms = GetCurrentTimeMs();
            if (now_ms > m_send_queue.front().timestamp_ms &&
                (now_ms - m_send_queue.front().timestamp_ms) > kMaxQueuedPacketAgeMs) {
                // Drop stale packet!
                LOG_WARN("TcpClient::SendThreadLoop: Dropping stale packet (age=%llu ms)", now_ms - m_send_queue.front().timestamp_ms);
                m_send_queue_size -= m_send_queue.front().data.size();
                m_send_queue.pop_front();
#ifdef TEST_BUILD
                lock.unlock();
#else
                m_queue_mutex.Unlock();
#endif
                continue; // Loop back and check next packet
            }
#ifdef TEST_BUILD
            lock.unlock();
#else
            m_queue_mutex.Unlock();
#endif
        }

        // Step 2: Get the packet details (front of the queue) safely
        std::vector<uint8_t> temp_data;
        bool has_data = false;
        
#ifdef TEST_BUILD
        std::unique_lock<std::mutex> lock(m_queue_mutex);
#else
        m_queue_mutex.Lock();
#endif
        if (!m_send_queue.empty()) {
            // Check age again in case it aged while we were in a non-blocking retry cycle
            if (send_offset == 0) {
                uint64_t now_ms = GetCurrentTimeMs();
                if (now_ms > m_send_queue.front().timestamp_ms &&
                    (now_ms - m_send_queue.front().timestamp_ms) > kMaxQueuedPacketAgeMs) {
                    LOG_WARN("TcpClient::SendThreadLoop: Dropping stale packet during retry (age=%llu ms)", now_ms - m_send_queue.front().timestamp_ms);
                    m_send_queue_size -= m_send_queue.front().data.size();
                    m_send_queue.pop_front();
#ifdef TEST_BUILD
                    lock.unlock();
#else
                    m_queue_mutex.Unlock();
#endif
                    continue;
                }
            }
            temp_data = m_send_queue.front().data;
            has_data = true;
        }
#ifdef TEST_BUILD
        lock.unlock();
#else
        m_queue_mutex.Unlock();
#endif

        if (!has_data) {
            // Queue became empty
            send_offset = 0;
            continue;
        }

        // Step 3: Try to send
        size_t to_send = temp_data.size() - send_offset;
        size_t sent = 0;
        
        SocketResult res;
        {
            std::scoped_lock send_lock(m_send_mutex);
            res = m_socket.send(temp_data.data() + send_offset, to_send, sent);
        }
        
        if (res == SocketResult::Success) {
            send_offset += sent;
            if (send_offset >= temp_data.size()) {
                // Packet fully sent! Pop it from the queue.
#ifdef TEST_BUILD
                lock.lock();
#else
                m_queue_mutex.Lock();
#endif
                if (!m_send_queue.empty()) {
                    m_send_queue_size -= m_send_queue.front().data.size();
                    m_send_queue.pop_front();
                }
#ifdef TEST_BUILD
                lock.unlock();
#else
                m_queue_mutex.Unlock();
#endif
                send_offset = 0;
            }
        } else if (res == SocketResult::WouldBlock) {
            // Socket buffer full. Sleep for 1ms and retry later.
            // This yields CPU and avoids blocking on concurrently-unsafe poll() calls!
#ifdef TEST_BUILD
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
#else
            ::ams::os::SleepThread(::ams::TimeSpan::FromMilliSeconds(1)); // 1ms
#endif
        } else {
            // Socket error
            LOG_WARN("TcpClient::SendThreadLoop: send failed: %s", socket_result_to_string(res));
            break;
        }
    }
}

} // namespace ryu_ldn::network
