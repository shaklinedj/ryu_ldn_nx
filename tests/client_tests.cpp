/**
 * @file client_tests.cpp
 * @brief Comprehensive unit tests for RyuLdnClient with mock TCP
 *
 * Uses MockTcpClient to exercise all state transitions, callback paths,
 * and error handling without real sockets. Targets high line coverage.
 *
 * @copyright Copyright (c) 2026 ryu_ldn_nx contributors
 * @license GPL-2.0-or-later
 *
 * @section Test Categories
 *
 * ### Configuration Tests
 * Verify default and custom config values.
 *
 * ### Construction / Move Tests
 * Verify constructors, move constructor, move assignment, DI constructor.
 *
 * ### Disconnect (No-op) Tests
 * Test disconnect when already disconnected.
 *
 * ### Send (Not Ready) Tests
 * Test that all 13 send operations return NotReady when disconnected.
 *
 * ### Callback Tests
 * Test state and packet callback registration and invocation.
 *
 * ### Connect Path Tests
 * Test connect success, fail, backoff, already-connected, null host, DI failure.
 *
 * ### Handshake / Update Path Tests
 * Test handshake send, timeout, success, NetworkError, VersionMismatch,
 * SyncNetwork, server disconnect, ConnectionLost during handshake.
 *
 * ### Ready / process_packets Tests
 * Test ConnectionLost callback during packet processing.
 *
 * ### Backoff Tests
 * Test backoff expiry and retry, no-auto-reconnect stays in Backoff.
 *
 * ### State Machine Edge Case Tests
 * Test Disconnecting transition, Error state no-op, send with ConnectionLost callback.
 *
 * ### String Conversion Tests
 * Test ClientOpResult and NetworkErrorCode to-string functions.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

#include "network/client.hpp"
#include "network/socket.hpp"
#include "mock_tcp_client.hpp"

using namespace ryu_ldn;
using namespace ryu_ldn::network;
using namespace ryu_ldn::network::test;
using namespace ryu_ldn::protocol;

// ============================================================================
// Test Framework (Minimal)
// ============================================================================

/** @brief Global pass counter */
static int g_tests_passed = 0;

/** @brief Global fail counter */
static int g_tests_failed = 0;

/**
 * @brief Assert macro with detailed failure message
 * @param cond Condition to evaluate
 */
#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            printf("    FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            return false; \
        } \
    } while(0)

/**
 * @brief Assert equality macro
 * @param a Left operand
 * @param b Right operand
 */
#define ASSERT_EQ(a, b) \
    do { \
        auto _a = static_cast<long long>(a); \
        auto _b = static_cast<long long>(b); \
        if (_a != _b) { \
            printf("    FAIL: %s:%d: %s == %s (%lld != %lld)\n", \
                   __FILE__, __LINE__, #a, #b, _a, _b); \
            return false; \
        } \
    } while(0)

/**
 * @brief Assert string equality macro
 * @param a First string
 * @param b Second string
 */
#define ASSERT_STREQ(a, b) \
    do { \
        if (std::strcmp((a), (b)) != 0) { \
            printf("    FAIL: %s:%d: \"%s\" != \"%s\"\n", \
                   __FILE__, __LINE__, (a), (b)); \
            return false; \
        } \
    } while(0)

/**
 * @brief Test runner macro
 * @param test_func Function name to run
 */
#define RUN_TEST(test_func) \
    do { \
        printf("  [TEST] %s... ", #test_func); \
        if (test_func()) { \
            printf("PASS\n"); \
            g_tests_passed++; \
        } else { \
            g_tests_failed++; \
        } \
    } while(0)

// ============================================================================
// Helpers
// ============================================================================

/**
 * @brief Create a RyuLdnClient with a MockTcpClient and auto-reconnect enabled
 *
 * @param[out] mock_out Pointer to the injected MockTcpClient
 * @return Owning pointer to the constructed RyuLdnClient
 */
static std::unique_ptr<RyuLdnClient> make_client(MockTcpClient*& mock_out) {
    auto mock = std::make_unique<MockTcpClient>();
    mock_out = mock.get();
    RyuLdnClientConfig cfg;
    cfg.auto_reconnect = true;
    return std::make_unique<RyuLdnClient>(cfg, std::move(mock));
}

/**
 * @brief Create a RyuLdnClient with auto-reconnect disabled
 *
 * @param[out] mock_out Pointer to the injected MockTcpClient
 * @return Owning pointer to the constructed RyuLdnClient
 */
static std::unique_ptr<RyuLdnClient> make_client_no_reconnect(MockTcpClient*& mock_out) {
    auto mock = std::make_unique<MockTcpClient>();
    mock_out = mock.get();
    RyuLdnClientConfig cfg;
    cfg.auto_reconnect = false;
    return std::make_unique<RyuLdnClient>(cfg, std::move(mock));
}

/** @brief Recorded state transitions for callback verification */
struct StateChange {
    ConnectionState old_state;
    ConnectionState new_state;
};

/** @brief Accumulated state changes from callbacks */
static std::vector<StateChange> g_state_changes;

/** @brief Last user_data pointer received by callback */
static void* g_callback_ud = nullptr;

/**
 * @brief State change callback that records transitions
 *
 * @param old_state Previous connection state
 * @param new_state New connection state
 * @param ud User data pointer
 */
static void state_callback(ConnectionState old_state, ConnectionState new_state, void* ud) {
    g_state_changes.push_back({old_state, new_state});
    g_callback_ud = ud;
}

/**
 * @brief Packet callback (no-op, for registration tests only)
 *
 * @param id Packet type
 * @param data Packet data
 * @param size Data size
 * @param ud User data pointer
 */
static void packet_callback(PacketId id, const uint8_t* data, size_t size, void* ud) {
    (void)id; (void)data; (void)size; (void)ud;
}

/** @brief Clear accumulated state changes */
static void clear_state_changes() {
    g_state_changes.clear();
    g_callback_ud = nullptr;
}

/**
 * @brief Drive a client through connect → handshake → Ready using a mock
 *
 * Puts the mock into success mode, calls connect(), then two update()
 * cycles to reach Ready state.
 *
 * @param client The RyuLdnClient to drive
 * @param mock The associated MockTcpClient
 * @param start_time Starting time value for update() calls
 */
static void drive_to_ready(RyuLdnClient* client, MockTcpClient* mock, uint64_t start_time) {
    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);

    // Connected state: send handshake
    mock->next_send_result = ClientResult::Success;
    client->update(start_time);

    // Handshaking state: receive Initialize response
    MockPacket init_pkt;
    init_pkt.id = PacketId::Initialize;
    init_pkt.data.resize(sizeof(InitializeMessage));
    mock->recv_queue.push(std::move(init_pkt));
    mock->next_recv_result = ClientResult::Success;
    client->update(start_time + 500);
}

// ============================================================================
// Configuration Tests
// ============================================================================

/**
 * @brief Test RyuLdnClientConfig default values
 *
 * Verifies that default host is "127.0.0.1", port is 30456,
 * recv_timeout is 20ms, ping_interval is 0 (disabled), and
 * auto_reconnect is true.
 */
bool test_config_defaults() {
    RyuLdnClientConfig cfg;
    ASSERT_STREQ(cfg.host, "127.0.0.1");
    ASSERT_EQ(cfg.port, 30456);
    ASSERT_EQ(cfg.connect_timeout_ms, 5000);
    ASSERT_EQ(cfg.recv_timeout_ms, 20);
    ASSERT_EQ(cfg.ping_interval_ms, 0);
    ASSERT_TRUE(cfg.auto_reconnect);
    return true;
}

/**
 * @brief Test RyuLdnClientConfig constructed from Config struct
 *
 * Verifies that values from a Config struct are properly copied
 * and that ping_interval is forced to 0 regardless of config.
 */
bool test_config_from_app_config() {
    config::Config app_cfg = config::get_default_config();
    std::strncpy(app_cfg.server.host, "192.168.1.100", sizeof(app_cfg.server.host));
    app_cfg.server.port = 12345;
    app_cfg.network.connect_timeout_ms = 10000;
    app_cfg.network.max_reconnect_attempts = 0;
    app_cfg.network.reconnect_delay_ms = 2000;

    RyuLdnClientConfig cfg(app_cfg);
    ASSERT_STREQ(cfg.host, "192.168.1.100");
    ASSERT_EQ(cfg.port, 12345);
    ASSERT_EQ(cfg.connect_timeout_ms, 10000);
    ASSERT_EQ(cfg.ping_interval_ms, 0);  // ping_interval is forced to 0
    ASSERT_EQ(cfg.reconnect.initial_delay_ms, 2000);
    return true;
}

// ============================================================================
// Construction / Move Tests
// ============================================================================

/**
 * @brief Test default constructor creates client in Disconnected state
 */
bool test_default_construction() {
    RyuLdnClient client;
    ASSERT_EQ(client.get_state(), ConnectionState::Disconnected);
    ASSERT_TRUE(!client.is_connected());
    ASSERT_TRUE(!client.is_ready());
    ASSERT_TRUE(!client.is_transitioning());
    ASSERT_EQ(client.get_retry_count(), 0);
    return true;
}

/**
 * @brief Test constructor with custom config
 */
bool test_construction_with_config() {
    RyuLdnClientConfig cfg;
    cfg.port = 9999;
    cfg.ping_interval_ms = 5000;
    RyuLdnClient client(cfg);
    ASSERT_EQ(client.get_config().port, 9999);
    ASSERT_EQ(client.get_config().ping_interval_ms, 5000);
    ASSERT_EQ(client.get_state(), ConnectionState::Disconnected);
    return true;
}

/**
 * @brief Test constructor with injected ITcpClient (dependency injection)
 */
bool test_construction_with_mock() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    ASSERT_EQ(client->get_state(), ConnectionState::Disconnected);
    ASSERT_TRUE(!client->is_connected());
    ASSERT_TRUE(!client->is_ready());
    return true;
}

/**
 * @brief Test that multiple client instances coexist safely
 */
bool test_multiple_clients() {
    RyuLdnClient c1, c2, c3;
    ASSERT_EQ(c1.get_state(), ConnectionState::Disconnected);
    ASSERT_EQ(c2.get_state(), ConnectionState::Disconnected);
    ASSERT_EQ(c3.get_state(), ConnectionState::Disconnected);
    return true;
}

/**
 * @brief Test set_config updates configuration and reconnect manager
 */
bool test_set_config() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    RyuLdnClientConfig new_cfg;
    std::strncpy(new_cfg.host, "10.0.0.1", sizeof(new_cfg.host));
    new_cfg.port = 8888;
    new_cfg.ping_interval_ms = 1000;
    client->set_config(new_cfg);
    ASSERT_STREQ(client->get_config().host, "10.0.0.1");
    ASSERT_EQ(client->get_config().port, 8888);
    ASSERT_EQ(client->get_config().ping_interval_ms, 1000);
    return true;
}

/**
 * @brief Test move constructor transfers state correctly
 */
bool test_move_constructor() {
    RyuLdnClientConfig cfg;
    cfg.port = 7777;
    RyuLdnClient c1(cfg);
    RyuLdnClient c2(std::move(c1));
    ASSERT_EQ(c2.get_config().port, 7777);
    ASSERT_EQ(c2.get_state(), ConnectionState::Disconnected);
    return true;
}

/**
 * @brief Test move assignment transfers state correctly
 */
bool test_move_assignment() {
    RyuLdnClientConfig cfg;
    cfg.port = 6666;
    RyuLdnClient c1(cfg);
    RyuLdnClient c2;
    c2 = std::move(c1);
    ASSERT_EQ(c2.get_config().port, 6666);
    ASSERT_EQ(c2.get_state(), ConnectionState::Disconnected);
    return true;
}

/**
 * @brief Test self-move-assignment is safe (guarded by if)
 */
bool test_self_move_assignment() {
    RyuLdnClientConfig cfg;
    cfg.port = 5555;
    RyuLdnClient client(cfg);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
    client = std::move(client);
#pragma GCC diagnostic pop
    ASSERT_EQ(client.get_config().port, 5555);
    return true;
}

// ============================================================================
// State Query Tests (Disconnected)
// ============================================================================

/**
 * @brief Test is_connected() returns false when disconnected
 */
bool test_is_connected_when_disconnected() {
    RyuLdnClient client;
    ASSERT_TRUE(!client.is_connected());
    return true;
}

/**
 * @brief Test is_ready() returns false when disconnected
 */
bool test_is_ready_when_disconnected() {
    RyuLdnClient client;
    ASSERT_TRUE(!client.is_ready());
    return true;
}

/**
 * @brief Test is_transitioning() returns false when disconnected
 */
bool test_is_transitioning_when_disconnected() {
    RyuLdnClient client;
    ASSERT_TRUE(!client.is_transitioning());
    return true;
}

/**
 * @brief Test get_retry_count() returns 0 initially
 */
bool test_get_retry_count_initial() {
    RyuLdnClient client;
    ASSERT_EQ(client.get_retry_count(), 0);
    return true;
}

// ============================================================================
// Disconnect (No-op) Tests
// ============================================================================

/**
 * @brief Test disconnect() is safe when already disconnected
 */
bool test_disconnect_when_disconnected() {
    RyuLdnClient client;
    client.disconnect();
    ASSERT_EQ(client.get_state(), ConnectionState::Disconnected);
    return true;
}

/**
 * @brief Test multiple disconnect() calls are safe
 */
bool test_multiple_disconnect_calls() {
    RyuLdnClient client;
    client.disconnect();
    client.disconnect();
    client.disconnect();
    ASSERT_EQ(client.get_state(), ConnectionState::Disconnected);
    return true;
}

// ============================================================================
// Send (Not Ready) — all 13 methods
// ============================================================================

/**
 * @brief Test send_scan returns NotReady when disconnected
 */
bool test_send_scan_not_ready() {
    RyuLdnClient client;
    ScanFilterFull filter{};
    ASSERT_EQ(client.send_scan(filter), ClientOpResult::NotReady);
    return true;
}

/**
 * @brief Test send_create_access_point returns NotReady when disconnected
 */
bool test_send_create_access_point_not_ready() {
    RyuLdnClient client;
    CreateAccessPointRequest req{};
    ASSERT_EQ(client.send_create_access_point(req), ClientOpResult::NotReady);
    return true;
}

/**
 * @brief Test send_connect returns NotReady when disconnected
 */
bool test_send_connect_not_ready() {
    RyuLdnClient client;
    ConnectRequest req{};
    ASSERT_EQ(client.send_connect(req), ClientOpResult::NotReady);
    return true;
}

/**
 * @brief Test send_create_access_point_private returns NotReady when disconnected
 */
bool test_send_create_access_point_private_not_ready() {
    RyuLdnClient client;
    CreateAccessPointPrivateRequest req{};
    ASSERT_EQ(client.send_create_access_point_private(req), ClientOpResult::NotReady);
    return true;
}

/**
 * @brief Test send_connect_private returns NotReady when disconnected
 */
bool test_send_connect_private_not_ready() {
    RyuLdnClient client;
    ConnectPrivateRequest req{};
    ASSERT_EQ(client.send_connect_private(req), ClientOpResult::NotReady);
    return true;
}

/**
 * @brief Test send_proxy_data returns NotReady when disconnected
 */
bool test_send_proxy_data_not_ready() {
    RyuLdnClient client;
    ProxyDataHeader hdr{};
    uint8_t data[10] = {0};
    ASSERT_EQ(client.send_proxy_data(hdr, data, sizeof(data)), ClientOpResult::NotReady);
    return true;
}

/**
 * @brief Test send_ping returns NotReady when disconnected
 */
bool test_send_ping_not_ready() {
    RyuLdnClient client;
    ASSERT_EQ(client.send_ping(), ClientOpResult::NotReady);
    return true;
}

/**
 * @brief Test send_ping_response returns NotReady when disconnected
 */
bool test_send_ping_response_not_ready() {
    RyuLdnClient client;
    ASSERT_EQ(client.send_ping_response(1), ClientOpResult::NotReady);
    return true;
}

/**
 * @brief Test send_disconnect_network returns NotReady when disconnected
 */
bool test_send_disconnect_network_not_ready() {
    RyuLdnClient client;
    ASSERT_EQ(client.send_disconnect_network(), ClientOpResult::NotReady);
    return true;
}

/**
 * @brief Test send_set_accept_policy returns NotReady when disconnected
 */
bool test_send_set_accept_policy_not_ready() {
    RyuLdnClient client;
    ASSERT_EQ(client.send_set_accept_policy(AcceptPolicy::AcceptAll), ClientOpResult::NotReady);
    return true;
}

/**
 * @brief Test send_set_advertise_data returns NotReady when disconnected
 */
bool test_send_set_advertise_data_not_ready() {
    RyuLdnClient client;
    uint8_t data[] = {0x01, 0x02};
    ASSERT_EQ(client.send_set_advertise_data(data, sizeof(data)), ClientOpResult::NotReady);
    return true;
}

/**
 * @brief Test send_reject returns NotReady when disconnected
 */
bool test_send_reject_not_ready() {
    RyuLdnClient client;
    ASSERT_EQ(client.send_reject(1, DisconnectReason::Rejected), ClientOpResult::NotReady);
    return true;
}

/**
 * @brief Test send_raw_packet returns NotReady when disconnected
 */
bool test_send_raw_packet_not_ready() {
    RyuLdnClient client;
    uint8_t data[] = {0x01};
    ASSERT_EQ(client.send_raw_packet(data, sizeof(data)), ClientOpResult::NotReady);
    return true;
}

// ============================================================================
// Callback Tests
// ============================================================================

/**
 * @brief Test set_state_callback(nullptr) is safe (no-op)
 */
bool test_set_state_callback_null() {
    RyuLdnClient client;
    client.set_state_callback(nullptr);
    return true;
}

/**
 * @brief Test set_packet_callback(nullptr) is safe (no-op)
 */
bool test_set_packet_callback_null() {
    RyuLdnClient client;
    client.set_packet_callback(nullptr);
    return true;
}

/**
 * @brief Test state callback is invoked on successful connect
 *
 * After connect() with a succeeding mock, the state callback
 * should receive at least one transition.
 */
bool test_state_callback_on_connect_success() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    mock->next_connect_result = ClientResult::Success;
    ClientOpResult result = client->connect("127.0.0.1", 30456);
    ASSERT_EQ(result, ClientOpResult::Success);
    ASSERT_TRUE(g_state_changes.size() >= 1);
    return true;
}

/**
 * @brief Test state callback is invoked on failed connect (backoff)
 *
 * After connect() with a failing mock and auto_reconnect,
 * the state callback should record Disconnected → Connecting → Backoff.
 */
bool test_state_callback_on_connect_fail() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    mock->next_connect_result = ClientResult::ConnectionFailed;
    ClientOpResult result = client->connect("127.0.0.1", 30456);
    ASSERT_EQ(result, ClientOpResult::Success);
    ASSERT_TRUE(g_state_changes.size() >= 1);
    return true;
}

/**
 * @brief Test that user_data pointer is passed through to state callback
 */
bool test_state_callback_user_data() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    int ud = 42;
    client->set_state_callback(state_callback, &ud);

    clear_state_changes();
    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);
    ASSERT_TRUE(g_callback_ud == &ud);
    return true;
}

/**
 * @brief Test setting and clearing packet callback
 */
bool test_packet_callback_set_and_clear() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    client->set_packet_callback(packet_callback);
    client->set_packet_callback(nullptr);
    return true;
}

// ============================================================================
// Connect Path Tests
// ============================================================================

/**
 * @brief Test successful connect via mock TCP client
 *
 * Verifies connect() returns Success, the mock receives the host/port,
 * and the client enters Connected state.
 */
bool test_connect_success_via_mock() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    mock->next_connect_result = ClientResult::Success;
    ClientOpResult result = client->connect("127.0.0.1", 30456);

    ASSERT_EQ(result, ClientOpResult::Success);
    ASSERT_EQ(mock->connect_call_count, 1);
    ASSERT_STREQ(mock->last_connect_host.c_str(), "127.0.0.1");
    ASSERT_EQ(mock->last_connect_port, 30456);
    ASSERT_TRUE(client->is_connected());
    return true;
}

/**
 * @brief Test connect failure leads to Backoff state (auto_reconnect=true)
 */
bool test_connect_fail_backoff() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    mock->next_connect_result = ClientResult::ConnectionFailed;
    ClientOpResult result = client->connect("127.0.0.1", 30456);
    ASSERT_EQ(result, ClientOpResult::Success);
    ASSERT_EQ(mock->connect_call_count, 1);
    ASSERT_EQ(client->get_state(), ConnectionState::Backoff);
    return true;
}

/**
 * @brief Test connect failure with auto_reconnect=false stays in Backoff
 */
bool test_connect_fail_no_reconnect() {
    MockTcpClient* mock;
    auto client = make_client_no_reconnect(mock);

    mock->next_connect_result = ClientResult::ConnectionFailed;
    client->connect("127.0.0.1", 30456);
    ASSERT_EQ(client->get_state(), ConnectionState::Backoff);
    return true;
}

/**
 * @brief Test connect() returns AlreadyConnected when already connected
 */
bool test_connect_already_connected() {
    MockTcpClient* mock;
    auto client = make_client(mock);

    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);

    ClientOpResult result = client->connect("127.0.0.1", 30456);
    ASSERT_EQ(result, ClientOpResult::AlreadyConnected);
    return true;
}

/**
 * @brief Test connect() with null host uses config default
 */
bool test_connect_null_host() {
    MockTcpClient* mock;
    auto client = make_client(mock);

    mock->next_connect_result = ClientResult::Success;
    ClientOpResult result = client->connect(nullptr, 30456);
    ASSERT_EQ(result, ClientOpResult::Success);
    return true;
}

/**
 * @brief Test connect() with explicit host/port updates config
 */
bool test_connect_with_host_and_port() {
    MockTcpClient* mock;
    auto client = make_client(mock);

    mock->next_connect_result = ClientResult::Success;
    ClientOpResult result = client->connect("192.168.1.50", 9999);
    ASSERT_EQ(result, ClientOpResult::Success);
    ASSERT_STREQ(mock->last_connect_host.c_str(), "192.168.1.50");
    ASSERT_EQ(mock->last_connect_port, 9999);
    ASSERT_STREQ(client->get_config().host, "192.168.1.50");
    ASSERT_EQ(client->get_config().port, 9999);
    return true;
}

/**
 * @brief Test connect() returns InternalError when initialize() fails
 */
bool test_connect_initialize_error() {
    MockTcpClient* mock;
    auto client = make_client(mock);

    mock->initialize_should_fail = true;
    ClientOpResult result = client->connect("127.0.0.1", 30456);
    ASSERT_EQ(result, ClientOpResult::InternalError);
    return true;
}

// ============================================================================
// Disconnect with Callback
// ============================================================================

/**
 * @brief Test disconnect() fires state callback and reaches Disconnected
 */
bool test_disconnect_with_callback() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);

    client->disconnect();
    ASSERT_EQ(client->get_state(), ConnectionState::Disconnected);
    ASSERT_EQ(mock->disconnect_call_count, 1);
    ASSERT_TRUE(g_state_changes.size() >= 1);
    return true;
}

// ============================================================================
// Handshake / Update Path Tests
// ============================================================================

/**
 * @brief Test update() in Connected state sends passphrase and initialize
 *
 * After connect(), the first update() in Connected state should call
 * send_passphrase and send_initialize on the mock.
 */
bool test_update_connected_sends_handshake() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);
    ASSERT_TRUE(client->is_connected());

    mock->next_send_result = ClientResult::Success;
    client->update(1000);

    ASSERT_EQ(mock->send_passphrase_str_call_count, 1);
    ASSERT_EQ(mock->send_initialize_call_count, 1);
    ASSERT_EQ(client->get_state(), ConnectionState::Handshaking);
    return true;
}

/**
 * @brief Test update() in Connected state with handshake send failure
 *
 * When send_initialize() fails (ConnectionLost), the client should
 * transition to Backoff and fire the state callback.
 */
bool test_update_connected_handshake_fail() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);

    mock->next_send_result = ClientResult::ConnectionLost;
    client->update(1000);

    ASSERT_EQ(client->get_state(), ConnectionState::Backoff);
    ASSERT_TRUE(g_state_changes.size() >= 1);
    return true;
}

/**
 * @brief Test handshake timeout transitions to Backoff with callback
 *
 * After entering Handshaking, advancing time past the 5000ms timeout
 * should transition to Backoff.
 */
bool test_update_handshaking_timeout() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);
    mock->next_send_result = ClientResult::Success;
    client->update(1000);
    ASSERT_EQ(client->get_state(), ConnectionState::Handshaking);

    mock->next_recv_result = ClientResult::Timeout;
    client->update(7000);

    ASSERT_EQ(client->get_state(), ConnectionState::Backoff);
    ASSERT_TRUE(g_state_changes.size() >= 1);
    return true;
}

/**
 * @brief Test successful handshake via Initialize response
 *
 * After receiving an Initialize packet from the server, the client
 * should transition from Handshaking to Ready.
 */
bool test_update_handshaking_recv_init_response() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    drive_to_ready(client.get(), mock, 1000);
    ASSERT_EQ(client->get_state(), ConnectionState::Ready);
    return true;
}

/**
 * @brief Test ConnectionLost during handshake triggers callback and Backoff
 */
bool test_update_handshaking_recv_connection_lost() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);
    mock->next_send_result = ClientResult::Success;
    client->update(1000);

    mock->next_recv_result = ClientResult::ConnectionLost;
    client->update(1500);

    ASSERT_EQ(client->get_state(), ConnectionState::Backoff);
    return true;
}

/**
 * @brief Test NetworkError response (non-fatal) triggers Backoff
 *
 * Server sends a ConnectionRejected error during handshake → Backoff.
 */
bool test_update_handshaking_recv_network_error() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);
    mock->next_send_result = ClientResult::Success;
    client->update(1000);

    MockPacket err_pkt;
    err_pkt.id = PacketId::NetworkError;
    err_pkt.data.resize(sizeof(NetworkErrorMessage));
    auto* msg = reinterpret_cast<NetworkErrorMessage*>(err_pkt.data.data());
    msg->error_code = static_cast<uint32_t>(NetworkErrorCode::ConnectionRejected);
    mock->recv_queue.push(std::move(err_pkt));
    mock->next_recv_result = ClientResult::Success;

    client->update(1500);
    ASSERT_EQ(client->get_state(), ConnectionState::Backoff);
    return true;
}

/**
 * @brief Test VersionMismatch error triggers fatal Error state
 *
 * Server sends VersionMismatch → Error (no retry).
 */
bool test_update_handshaking_recv_network_error_version_mismatch() {
    MockTcpClient* mock;
    auto client = make_client(mock);

    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);
    mock->next_send_result = ClientResult::Success;
    client->update(1000);

    MockPacket err_pkt;
    err_pkt.id = PacketId::NetworkError;
    err_pkt.data.resize(sizeof(NetworkErrorMessage));
    auto* msg = reinterpret_cast<NetworkErrorMessage*>(err_pkt.data.data());
    msg->error_code = static_cast<uint32_t>(NetworkErrorCode::VersionMismatch);
    mock->recv_queue.push(std::move(err_pkt));
    mock->next_recv_result = ClientResult::Success;

    client->update(1500);
    ASSERT_EQ(client->get_state(), ConnectionState::Error);
    return true;
}

/**
 * @brief Test SyncNetwork response as alternative handshake success
 */
bool test_update_handshaking_recv_sync_network() {
    MockTcpClient* mock;
    auto client = make_client(mock);

    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);
    mock->next_send_result = ClientResult::Success;
    client->update(1000);

    MockPacket sync_pkt;
    sync_pkt.id = PacketId::SyncNetwork;
    sync_pkt.data.resize(10);
    mock->recv_queue.push(std::move(sync_pkt));
    mock->next_recv_result = ClientResult::Success;

    client->update(1500);
    ASSERT_EQ(client->get_state(), ConnectionState::Ready);
    return true;
}

/**
 * @brief Test server Disconnect during handshake triggers Backoff
 */
bool test_update_handshaking_recv_disconnect() {
    MockTcpClient* mock;
    auto client = make_client(mock);

    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);
    mock->next_send_result = ClientResult::Success;
    client->update(1000);

    MockPacket disc_pkt;
    disc_pkt.id = PacketId::Disconnect;
    disc_pkt.data.resize(sizeof(DisconnectMessage));
    mock->recv_queue.push(std::move(disc_pkt));
    mock->next_recv_result = ClientResult::Success;

    client->update(1500);
    ASSERT_EQ(client->get_state(), ConnectionState::Backoff);
    return true;
}

/**
 * @brief Test handshake timeout with auto_reconnect=false still transitions to Backoff
 */
bool test_handshake_timeout_no_reconnect() {
    MockTcpClient* mock;
    auto client = make_client_no_reconnect(mock);

    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);
    mock->next_send_result = ClientResult::Success;
    client->update(1000);

    mock->next_recv_result = ClientResult::Timeout;
    client->update(7000);

    ASSERT_EQ(client->get_state(), ConnectionState::Backoff);
    return true;
}

/**
 * @brief Test that passphrase is sent during handshake
 *
 * When config has a non-empty passphrase, send_initialize should
 * send it first, then the Initialize message.
 */
bool test_handshake_passphrase_with_value() {
    auto mock = std::make_unique<MockTcpClient>();
    MockTcpClient* mock_ptr = mock.get();
    RyuLdnClientConfig cfg;
    cfg.auto_reconnect = true;
    std::strncpy(cfg.passphrase, "Ryujinx-abcdef12", sizeof(cfg.passphrase));
    RyuLdnClient client(cfg, std::move(mock));

    clear_state_changes();
    client.set_state_callback(state_callback);

    mock_ptr->next_connect_result = ClientResult::Success;
    client.connect("127.0.0.1", 30456);
    mock_ptr->next_send_result = ClientResult::Success;
    client.update(1000);

    ASSERT_EQ(mock_ptr->send_passphrase_str_call_count, 1);
    ASSERT_EQ(mock_ptr->send_initialize_call_count, 1);
    return true;
}

// ============================================================================
// Ready / process_packets Tests
// ============================================================================

/**
 * @brief Test ConnectionLost during packet processing in Ready state
 *
 * When receive_packet returns ConnectionLost, the client should
 * transition to Backoff and fire the state callback.
 */
bool test_update_ready_process_packets_connection_lost() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    drive_to_ready(client.get(), mock, 1000);
    ASSERT_EQ(client->get_state(), ConnectionState::Ready);

    mock->next_recv_result = ClientResult::ConnectionLost;
    client->update(2000);

    ASSERT_EQ(client->get_state(), ConnectionState::Backoff);
    return true;
}

// ============================================================================
// Backoff Tests
// ============================================================================

/**
 * @brief Test backoff expiry triggers retry and connect
 *
 * After connect failure → Backoff, advancing time past the backoff
 * delay should trigger a retry that connects successfully.
 */
bool test_update_backoff_expiry_and_retry() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    mock->next_connect_result = ClientResult::ConnectionFailed;
    client->connect("127.0.0.1", 30456);
    ASSERT_EQ(client->get_state(), ConnectionState::Backoff);

    // After connect() fails, start_backoff() sets m_backoff_start_time_ms = 0
    // (because no update() has been called yet). On the first update(),
    // is_backoff_expired() captures the time and returns false.
    // On the second update(), after the backoff delay has elapsed, it returns true.
    //
    // Backoff delay: record_failure() incremented retry_count to 1, which is
    // >= fast_retries (1), so get_next_delay_ms() returns initial_delay_ms (1000).
    // With ±10% jitter, the actual delay is between 900-1100ms.
    // Use a time jump well past the maximum possible jittered delay.

    mock->next_connect_result = ClientResult::Success;
    client->update(50000);  // Captures 50000 as start time, no retry yet
    ASSERT_EQ(mock->connect_call_count, 1);  // Only the initial connect() call

    client->update(53000);  // 3000ms later, past 1100ms max jittered delay
    ASSERT_EQ(mock->connect_call_count, 2);
    ASSERT_TRUE(client->is_connected());
    return true;
}

/**
 * @brief Test that with auto_reconnect=false, Backoff state persists
 *
 * The state machine still transitions to Backoff, but no retry timer
 * is started, so the client stays in Backoff indefinitely.
 */
bool test_update_backoff_no_reconnect() {
    MockTcpClient* mock;
    auto client = make_client_no_reconnect(mock);

    mock->next_connect_result = ClientResult::ConnectionFailed;
    client->connect("127.0.0.1", 30456);
    ASSERT_EQ(client->get_state(), ConnectionState::Backoff);

    client->update(50000);
    ASSERT_EQ(client->get_state(), ConnectionState::Backoff);
    return true;
}

// ============================================================================
// State Machine Edge Cases
// ============================================================================

/**
 * @brief Test disconnect() transitions to Disconnected via Disconnecting
 */
bool test_update_disconnecting() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);
    client->disconnect();
    ASSERT_EQ(client->get_state(), ConnectionState::Disconnected);
    return true;
}

/**
 * @brief Test Error state is inert under update()
 *
 * VersionMismatch causes Error state. Subsequent update() calls
 * should be no-ops.
 */
bool test_update_error_state_noop() {
    MockTcpClient* mock;
    auto client = make_client(mock);

    mock->next_connect_result = ClientResult::Success;
    client->connect("127.0.0.1", 30456);
    mock->next_send_result = ClientResult::Success;
    client->update(1000);

    MockPacket err_pkt;
    err_pkt.id = PacketId::NetworkError;
    err_pkt.data.resize(sizeof(NetworkErrorMessage));
    auto* msg = reinterpret_cast<NetworkErrorMessage*>(err_pkt.data.data());
    msg->error_code = static_cast<uint32_t>(NetworkErrorCode::VersionMismatch);
    mock->recv_queue.push(std::move(err_pkt));
    mock->next_recv_result = ClientResult::Success;
    client->update(1500);

    ASSERT_EQ(client->get_state(), ConnectionState::Error);
    client->update(2000);
    ASSERT_EQ(client->get_state(), ConnectionState::Error);
    return true;
}

/**
 * @brief Test send_scan with ConnectionLost fires state callback
 *
 * When in Ready state and the mock returns ConnectionLost for send_scan,
 * the client should transition to Backoff and invoke the state callback.
 */
bool test_send_scan_connection_lost_callback() {
    MockTcpClient* mock;
    auto client = make_client(mock);
    clear_state_changes();
    client->set_state_callback(state_callback);

    drive_to_ready(client.get(), mock, 1000);
    ASSERT_EQ(client->get_state(), ConnectionState::Ready);

    clear_state_changes();
    mock->next_send_result = ClientResult::ConnectionLost;
    ScanFilterFull filter{};
    ClientOpResult result = client->send_scan(filter);

    ASSERT_EQ(result, ClientOpResult::SendFailed);
    ASSERT_EQ(client->get_state(), ConnectionState::Backoff);
    ASSERT_TRUE(g_state_changes.size() >= 1);
    return true;
}

// ============================================================================
// String Conversion
// ============================================================================

/**
 * @brief Test client_op_result_to_string for all enum values and unknown
 */
bool test_client_op_result_all_to_string() {
    ASSERT_STREQ(client_op_result_to_string(ClientOpResult::Success), "Success");
    ASSERT_STREQ(client_op_result_to_string(ClientOpResult::NotConnected), "NotConnected");
    ASSERT_STREQ(client_op_result_to_string(ClientOpResult::NotReady), "NotReady");
    ASSERT_STREQ(client_op_result_to_string(ClientOpResult::AlreadyConnected), "AlreadyConnected");
    ASSERT_STREQ(client_op_result_to_string(ClientOpResult::ConnectionFailed), "ConnectionFailed");
    ASSERT_STREQ(client_op_result_to_string(ClientOpResult::SendFailed), "SendFailed");
    ASSERT_STREQ(client_op_result_to_string(ClientOpResult::InvalidState), "InvalidState");
    ASSERT_STREQ(client_op_result_to_string(ClientOpResult::Timeout), "Timeout");
    ASSERT_STREQ(client_op_result_to_string(ClientOpResult::ProtocolError), "ProtocolError");
    ASSERT_STREQ(client_op_result_to_string(ClientOpResult::InternalError), "InternalError");
    ASSERT_STREQ(client_op_result_to_string(static_cast<ClientOpResult>(99)), "Unknown");
    return true;
}

/**
 * @brief Test NetworkErrorCode enum values
 */
bool test_error_code_types() {
    ASSERT_EQ(static_cast<uint32_t>(NetworkErrorCode::None), 0);
    ASSERT_EQ(static_cast<uint32_t>(NetworkErrorCode::VersionMismatch), 1);
    ASSERT_EQ(static_cast<uint32_t>(NetworkErrorCode::InvalidMagic), 2);
    ASSERT_EQ(static_cast<uint32_t>(NetworkErrorCode::InvalidSessionId), 3);
    ASSERT_EQ(static_cast<uint32_t>(NetworkErrorCode::HandshakeTimeout), 4);
    ASSERT_EQ(static_cast<uint32_t>(NetworkErrorCode::AlreadyInitialized), 5);
    ASSERT_EQ(static_cast<uint32_t>(NetworkErrorCode::SessionNotFound), 100);
    ASSERT_EQ(static_cast<uint32_t>(NetworkErrorCode::SessionFull), 101);
    ASSERT_EQ(static_cast<uint32_t>(NetworkErrorCode::NetworkNotFound), 200);
    ASSERT_EQ(static_cast<uint32_t>(NetworkErrorCode::ConnectionRejected), 202);
    ASSERT_EQ(static_cast<uint32_t>(NetworkErrorCode::InternalError), 900);
    return true;
}

/**
 * @brief Test get_last_rtt_ms returns 0 initially
 */
bool test_get_last_rtt_initial() {
    RyuLdnClient client;
    ASSERT_EQ(client.get_last_rtt_ms(), 0);
    return true;
}

/**
 * @brief Test get_last_error_code returns None initially
 */
bool test_get_last_error_code_initial() {
    RyuLdnClient client;
    ASSERT_EQ(static_cast<int>(client.get_last_error_code()),
              static_cast<int>(NetworkErrorCode::None));
    return true;
}

// ============================================================================
// Main
// ============================================================================

/**
 * @brief Run all RyuLdnClient unit tests
 */
int main() {
    printf("\n========================================\n");
    printf("  RyuLdnClient Tests - ryu_ldn_nx\n");
    printf("========================================\n\n");

    printf("Configuration:\n");
    RUN_TEST(test_config_defaults);
    RUN_TEST(test_config_from_app_config);

    printf("\nConstruction:\n");
    RUN_TEST(test_default_construction);
    RUN_TEST(test_construction_with_config);
    RUN_TEST(test_construction_with_mock);
    RUN_TEST(test_multiple_clients);
    RUN_TEST(test_set_config);

    printf("\nMove Semantics:\n");
    RUN_TEST(test_move_constructor);
    RUN_TEST(test_move_assignment);
    RUN_TEST(test_self_move_assignment);

    printf("\nState Queries (Disconnected):\n");
    RUN_TEST(test_is_connected_when_disconnected);
    RUN_TEST(test_is_ready_when_disconnected);
    RUN_TEST(test_is_transitioning_when_disconnected);
    RUN_TEST(test_get_retry_count_initial);

    printf("\nDisconnect (No-op):\n");
    RUN_TEST(test_disconnect_when_disconnected);
    RUN_TEST(test_multiple_disconnect_calls);

    printf("\nSend (Not Ready):\n");
    RUN_TEST(test_send_scan_not_ready);
    RUN_TEST(test_send_create_access_point_not_ready);
    RUN_TEST(test_send_connect_not_ready);
    RUN_TEST(test_send_create_access_point_private_not_ready);
    RUN_TEST(test_send_connect_private_not_ready);
    RUN_TEST(test_send_proxy_data_not_ready);
    RUN_TEST(test_send_ping_not_ready);
    RUN_TEST(test_send_ping_response_not_ready);
    RUN_TEST(test_send_disconnect_network_not_ready);
    RUN_TEST(test_send_set_accept_policy_not_ready);
    RUN_TEST(test_send_set_advertise_data_not_ready);
    RUN_TEST(test_send_reject_not_ready);
    RUN_TEST(test_send_raw_packet_not_ready);

    printf("\nCallbacks:\n");
    RUN_TEST(test_set_state_callback_null);
    RUN_TEST(test_set_packet_callback_null);
    RUN_TEST(test_state_callback_on_connect_success);
    RUN_TEST(test_state_callback_on_connect_fail);
    RUN_TEST(test_state_callback_user_data);
    RUN_TEST(test_packet_callback_set_and_clear);

    printf("\nConnect paths:\n");
    RUN_TEST(test_connect_success_via_mock);
    RUN_TEST(test_connect_fail_backoff);
    RUN_TEST(test_connect_fail_no_reconnect);
    RUN_TEST(test_connect_already_connected);
    RUN_TEST(test_connect_null_host);
    RUN_TEST(test_connect_with_host_and_port);
    RUN_TEST(test_connect_initialize_error);

    printf("\nDisconnect:\n");
    RUN_TEST(test_disconnect_with_callback);

    printf("\nHandshake / update paths:\n");
    RUN_TEST(test_update_connected_sends_handshake);
    RUN_TEST(test_update_connected_handshake_fail);
    RUN_TEST(test_update_handshaking_timeout);
    RUN_TEST(test_update_handshaking_recv_init_response);
    RUN_TEST(test_update_handshaking_recv_connection_lost);
    RUN_TEST(test_update_handshaking_recv_network_error);
    RUN_TEST(test_update_handshaking_recv_network_error_version_mismatch);
    RUN_TEST(test_update_handshaking_recv_sync_network);
    RUN_TEST(test_update_handshaking_recv_disconnect);
    RUN_TEST(test_handshake_timeout_no_reconnect);
    RUN_TEST(test_handshake_passphrase_with_value);

    printf("\nReady / process_packets:\n");
    RUN_TEST(test_update_ready_process_packets_connection_lost);

    printf("\nBackoff:\n");
    RUN_TEST(test_update_backoff_expiry_and_retry);
    RUN_TEST(test_update_backoff_no_reconnect);

    printf("\nState machine edge cases:\n");
    RUN_TEST(test_update_disconnecting);
    RUN_TEST(test_update_error_state_noop);
    RUN_TEST(test_send_scan_connection_lost_callback);

    printf("\nString Conversion:\n");
    RUN_TEST(test_client_op_result_all_to_string);
    RUN_TEST(test_error_code_types);
    RUN_TEST(test_get_last_rtt_initial);
    RUN_TEST(test_get_last_error_code_initial);

    printf("\n========================================\n");
    printf("  Results: %d/%d passed\n",
           g_tests_passed, g_tests_passed + g_tests_failed);
    printf("========================================\n\n");

    return g_tests_failed > 0 ? 1 : 0;
}