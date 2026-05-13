/**
 * @file mock_tcp_client.hpp
 * @brief Mock TCP client for unit testing RyuLdnClient
 *
 * Provides full control over connect/disconnect/send/receive behavior
 * to exercise all code paths in RyuLdnClient without real sockets.
 */

#pragma once

#include "network/itcp_client.hpp"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <queue>
#include <vector>

namespace ryu_ldn::network::test {

struct MockPacket {
    protocol::PacketId id;
    std::vector<uint8_t> data;
};

class MockTcpClient : public ITcpClient {
public:
    bool connected = false;
    bool initialized = false;
    bool initialize_should_fail = false;

    ClientResult next_connect_result = ClientResult::Success;
    ClientResult next_send_result = ClientResult::Success;
    ClientResult next_recv_result = ClientResult::Timeout;

    std::queue<MockPacket> recv_queue;

    uint32_t connect_call_count = 0;
    uint32_t disconnect_call_count = 0;
    uint32_t send_packet_call_count = 0;
    uint32_t send_raw_call_count = 0;
    uint32_t send_initialize_call_count = 0;
    uint32_t send_passphrase_msg_call_count = 0;
    uint32_t send_passphrase_str_call_count = 0;
    uint32_t send_ping_call_count = 0;
    uint32_t send_disconnect_call_count = 0;
    uint32_t send_create_access_point_call_count = 0;
    uint32_t send_create_access_point_private_call_count = 0;
    uint32_t send_connect_call_count = 0;
    uint32_t send_connect_private_call_count = 0;
    uint32_t send_scan_call_count = 0;
    uint32_t send_proxy_data_call_count = 0;
    uint32_t send_set_accept_policy_call_count = 0;
    uint32_t send_set_advertise_data_call_count = 0;
    uint32_t send_reject_call_count = 0;
    uint32_t recv_call_count = 0;

    std::string last_connect_host;
    uint16_t last_connect_port = 0;
    uint32_t last_connect_timeout_ms = 0;

    bool initialize() override {
        if (initialize_should_fail) return false;
        initialized = true;
        return true;
    }

    ClientResult connect(const char* host, uint16_t port, uint32_t timeout_ms) override {
        connect_call_count++;
        last_connect_host = host ? host : "";
        last_connect_port = port;
        last_connect_timeout_ms = timeout_ms;
        if (next_connect_result == ClientResult::Success) {
            connected = true;
        }
        return next_connect_result;
    }

    void disconnect() override {
        disconnect_call_count++;
        connected = false;
    }

    bool is_connected() const override {
        return connected;
    }

    ClientResult send_packet(protocol::PacketId type, const void* payload, size_t payload_size) override {
        send_packet_call_count++;
        return next_send_result;
    }

    ClientResult send_raw(const void* data, size_t size) override {
        send_raw_call_count++;
        return next_send_result;
    }

    ClientResult send_initialize(const protocol::InitializeMessage& msg) override {
        send_initialize_call_count++;
        return next_send_result;
    }

    ClientResult send_passphrase(const protocol::PassphraseMessage& msg) override {
        send_passphrase_msg_call_count++;
        return next_send_result;
    }

    ClientResult send_passphrase(const char* passphrase) override {
        send_passphrase_str_call_count++;
        return next_send_result;
    }

    ClientResult send_ping(const protocol::PingMessage& msg) override {
        send_ping_call_count++;
        return next_send_result;
    }

    ClientResult send_disconnect(const protocol::DisconnectMessage& msg) override {
        send_disconnect_call_count++;
        return next_send_result;
    }

    ClientResult send_create_access_point(const protocol::CreateAccessPointRequest& request,
                                            const uint8_t* advertise_data,
                                            size_t advertise_size) override {
        send_create_access_point_call_count++;
        return next_send_result;
    }

    ClientResult send_create_access_point_private(const protocol::CreateAccessPointPrivateRequest& request,
                                                    const uint8_t* advertise_data,
                                                    size_t advertise_size) override {
        send_create_access_point_private_call_count++;
        return next_send_result;
    }

    ClientResult send_connect(const protocol::ConnectRequest& request) override {
        send_connect_call_count++;
        return next_send_result;
    }

    ClientResult send_connect_private(const protocol::ConnectPrivateRequest& request) override {
        send_connect_private_call_count++;
        return next_send_result;
    }

    ClientResult send_scan(const protocol::ScanFilterFull& filter) override {
        send_scan_call_count++;
        return next_send_result;
    }

    ClientResult send_proxy_data(const protocol::ProxyDataHeader& header,
                                  const uint8_t* data, size_t data_size) override {
        send_proxy_data_call_count++;
        return next_send_result;
    }

    ClientResult send_set_accept_policy(const protocol::SetAcceptPolicyRequest& request) override {
        send_set_accept_policy_call_count++;
        return next_send_result;
    }

    ClientResult send_set_advertise_data(const uint8_t* data, size_t size) override {
        send_set_advertise_data_call_count++;
        return next_send_result;
    }

    ClientResult send_reject(const protocol::RejectRequest& request) override {
        send_reject_call_count++;
        return next_send_result;
    }

    ClientResult receive_packet(protocol::PacketId& type,
                                 void* payload, size_t payload_buffer_size,
                                 size_t& payload_size,
                                 int32_t /*timeout_ms*/) override {
        recv_call_count++;
        if (recv_queue.empty()) {
            // When queue is empty, never return Success — that causes infinite loops
            // in process_packets() which loops until recv returns non-Success.
            // Return next_recv_result only if it's an error; otherwise return Timeout.
            type = static_cast<protocol::PacketId>(0xFF);
            payload_size = 0;
            if (next_recv_result == ClientResult::Success) {
                return ClientResult::Timeout;
            }
            return next_recv_result;
        }
        MockPacket pkt = std::move(recv_queue.front());
        recv_queue.pop();
        type = pkt.id;
        payload_size = pkt.data.size();
        if (payload_size > payload_buffer_size) {
            return ClientResult::BufferTooSmall;
        }
        if (payload_size > 0 && payload != nullptr) {
            std::memcpy(payload, pkt.data.data(), payload_size);
        }
        return ClientResult::Success;
    }

    /** Reset all counters to zero. */
    void reset_counters() {
        connect_call_count = 0;
        disconnect_call_count = 0;
        send_packet_call_count = 0;
        send_raw_call_count = 0;
        send_initialize_call_count = 0;
        send_passphrase_msg_call_count = 0;
        send_passphrase_str_call_count = 0;
        send_ping_call_count = 0;
        send_disconnect_call_count = 0;
        send_create_access_point_call_count = 0;
        send_create_access_point_private_call_count = 0;
        send_connect_call_count = 0;
        send_connect_private_call_count = 0;
        send_scan_call_count = 0;
        send_proxy_data_call_count = 0;
        send_set_accept_policy_call_count = 0;
        send_set_advertise_data_call_count = 0;
        send_reject_call_count = 0;
        recv_call_count = 0;
    }

    bool has_packet_available() const override {
        return !recv_queue.empty();
    }

    ClientResult set_nodelay(bool /*enable*/) override {
        return ClientResult::Success;
    }
};

} // namespace ryu_ldn::network::test