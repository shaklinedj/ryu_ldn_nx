/**
 * @file bsd_mitm_service.hpp
 * @brief BSD MITM Service - Main service class for bsd:u interception
 *
 * This service intercepts calls to the Nintendo bsd:u service to detect and
 * proxy LDN traffic through the RyuLdn server.
 *
 * ## How It Works
 *
 * 1. All BSD calls are intercepted
 * 2. For sockets that don't target LDN addresses, calls are forwarded to the
 *    real bsd:u service transparently
 * 3. For sockets that bind/connect to LDN addresses (10.114.x.x), we track
 *    them as "proxy sockets"
 * 4. Send/Recv on proxy sockets are routed through ProxyData packets instead
 *    of real network traffic
 *
 * @copyright Copyright (c) 2026 ryu_ldn_nx contributors
 * @license GPL-2.0-or-later
 */

#pragma once

#include <stratosphere.hpp>
#include "interfaces/ibsd_mitm_service.hpp"
#include "bsd_types.hpp"

namespace ams::mitm::bsd {

/**
 * @brief BSD MITM Service implementation
 *
 * This class implements the bsd:u MITM service. It forwards most calls to
 * the real service but intercepts and proxies LDN-related socket operations.
 */
class BsdMitmService : public sf::MitmServiceImplBase {
public:
    /**
     * @brief Constructor
     *
     * @param s Shared pointer to the original service
     * @param c MITM process info for the client
     */
    /// @gdb{tag="BSD:LIFECYCLE", msg="Constructor: program_id=0x%lx, pid=%lu", args="$x2, $x1"}
    BsdMitmService(std::shared_ptr<::Service>&& s, const sm::MitmProcessInfo& c);

    /**
     * @brief Destructor - cleanup tracked sockets
     */
    /// @gdb{tag="BSD:LIFECYCLE", msg="Destructor"}
    ~BsdMitmService();

    /**
     * @brief Determine if we should MITM this process
     *
     * We MITM all processes for now to ensure we catch any LDN traffic.
     * In the future, we could filter by title ID.
     *
     * @param client_info Process information for the client
     * @return true Always intercept
     */
    /// @gdb{tag="BSD:LIFECYCLE", msg="ShouldMitm: program_id=0x%lx", args="$x1"}
    static bool ShouldMitm(const sm::MitmProcessInfo& client_info);

    /**
     * @brief Clean up abandoned forward services
     *
     * Sessions that never received RegisterClient have their forward_service
     * moved to an abandoned list to prevent system freeze. This function
     * cleans up those services. Should be called when LDN disconnects or
     * when the game process exits.
     */
    static void CleanupAbandonedServices();

    /**
     * @brief Clean up abandoned forward services for a specific process ID
     *
     * Called when the last active intercepted session of a process is destroyed.
     *
     * @param pid Process ID of the game
     */
    static void CleanupAbandonedServicesForPid(u64 pid);

public:
    // =========================================================================
    // Command Implementations
    // All commands forward to the real service by default
    // =========================================================================

    /// @gdb{tag="BSD:LIFECYCLE", msg="RegisterClient: config_size=%u", args="$x2"}
    Result RegisterClient(
        sf::Out<u64> out_result,
        const ryu_ldn::bsd::LibraryConfigData& config,
        const sf::ClientProcessId& client_pid,
        u64 tmem_size,
        sf::CopyHandle&& transfer_memory);

    /// @gdb{tag="BSD:LIFECYCLE", msg="StartMonitoring: pid=%lu", args="$x2"}
    Result StartMonitoring(sf::Out<s32> out_errno, u64 pid);

    /// @gdb{tag="BSD:SOCKET", msg="Socket: domain=%d type=%d protocol=%d", args="$x3, $x4, $x5"}
    Result Socket(
        sf::Out<s32> out_errno, sf::Out<s32> out_fd,
        s32 domain, s32 type, s32 protocol);

    /// @gdb{tag="BSD:SOCKET", msg="SocketExempt: domain=%d type=%d protocol=%d", args="$x3, $x4, $x5"}
    Result SocketExempt(
        sf::Out<s32> out_errno, sf::Out<s32> out_fd,
        s32 domain, s32 type, s32 protocol);

    /// @gdb{tag="BSD:SOCKET", msg="Open"}
    Result Open(
        sf::Out<s32> out_errno, sf::Out<s32> out_fd,
        const sf::InBuffer& path);

    /// @gdb{tag="BSD:CONFIG", msg="Select: nfds=%d", args="$x3"}
    Result Select(
        sf::Out<s32> out_errno, sf::Out<s32> out_count,
        s32 nfds, const sf::InAutoSelectBuffer& readfds_in,
        const sf::InAutoSelectBuffer& writefds_in,
        const sf::InAutoSelectBuffer& errorfds_in,
        const sf::InAutoSelectBuffer& timeout,
        sf::OutAutoSelectBuffer readfds_out,
        sf::OutAutoSelectBuffer writefds_out,
        sf::OutAutoSelectBuffer errorfds_out);

    /// @gdb{tag="BSD:CONFIG", msg="Poll: nfds=%d timeout=%d", args="$x4, $x5"}
    Result Poll(
        sf::Out<s32> out_errno, sf::Out<s32> out_count,
        const sf::InAutoSelectBuffer& fds_in,
        sf::OutAutoSelectBuffer fds_out,
        s32 nfds, s32 timeout);

    /// @gdb{tag="BSD:CONFIG", msg="Sysctl"}
    Result Sysctl(
        sf::Out<s32> out_errno,
        const sf::InBuffer& name,
        const sf::InBuffer& old_val_in,
        sf::OutBuffer old_val_out,
        const sf::InBuffer& new_val);

    /// @gdb{tag="BSD:DATA", msg="Recv: fd=%d flags=%d", args="$x3, $x4"}
    Result Recv(
        sf::Out<s32> out_errno, sf::Out<s32> out_size,
        s32 fd, s32 flags,
        sf::OutAutoSelectBuffer buffer);

    /// @gdb{tag="BSD:DATA", msg="RecvFrom: fd=%d flags=%d", args="$x3, $x4"}
    Result RecvFrom(
        sf::Out<s32> out_ret, sf::Out<s32> out_errno, sf::Out<u32> out_addrlen,
        s32 fd, s32 flags,
        sf::OutAutoSelectBuffer buffer,
        sf::OutAutoSelectBuffer addr_out);

    /// @gdb{tag="BSD:DATA", msg="Send: fd=%d flags=%d", args="$x3, $x4"}
    Result Send(
        sf::Out<s32> out_errno, sf::Out<s32> out_size,
        s32 fd, s32 flags,
        const sf::InAutoSelectBuffer& buffer);

    /// @gdb{tag="BSD:DATA", msg="SendTo: fd=%d flags=%d", args="$x3, $x4"}
    Result SendTo(
        sf::Out<s32> out_errno, sf::Out<s32> out_size,
        s32 fd, s32 flags,
        const sf::InAutoSelectBuffer& buffer,
        const sf::InAutoSelectBuffer& addr);

    /// @gdb{tag="BSD:CONNECT", msg="Accept: fd=%d", args="$x3"}
    Result Accept(
        sf::Out<s32> out_errno, sf::Out<s32> out_fd,
        s32 fd,
        sf::OutAutoSelectBuffer addr_out);

    /// @gdb{tag="BSD:CONNECT", msg="Bind: fd=%d", args="$x2"}
    Result Bind(
        sf::Out<s32> out_errno,
        s32 fd,
        const sf::InAutoSelectBuffer& addr);

    /// @gdb{tag="BSD:CONNECT", msg="Connect: fd=%d", args="$x2"}
    Result Connect(
        sf::Out<s32> out_errno,
        s32 fd,
        const sf::InAutoSelectBuffer& addr);

    /// @gdb{tag="BSD:CONNECT", msg="GetPeerName: fd=%d", args="$x2"}
    Result GetPeerName(
        sf::Out<s32> out_errno,
        s32 fd,
        sf::OutAutoSelectBuffer addr_out);

    /// @gdb{tag="BSD:CONNECT", msg="GetSockName: fd=%d", args="$x2"}
    Result GetSockName(
        sf::Out<s32> out_errno,
        s32 fd,
        sf::OutAutoSelectBuffer addr_out);

    /// @gdb{tag="BSD:CONFIG", msg="GetSockOpt: fd=%d level=%d optname=%d", args="$x2, $x3, $x4"}
    Result GetSockOpt(
        sf::Out<s32> out_errno,
        s32 fd, s32 level, s32 optname,
        sf::OutAutoSelectBuffer optval);

    /// @gdb{tag="BSD:CONNECT", msg="Listen: fd=%d backlog=%d", args="$x2, $x3"}
    Result Listen(
        sf::Out<s32> out_errno,
        s32 fd, s32 backlog);

    /// @gdb{tag="BSD:CONFIG", msg="Ioctl: fd=%d request=0x%x", args="$x3, $x4"}
    Result Ioctl(
        sf::Out<s32> out_errno, sf::Out<s32> out_result,
        s32 fd, u32 request, u32 bufcount,
        const sf::InAutoSelectBuffer& buf_in,
        sf::OutAutoSelectBuffer buf_out);

    /// @gdb{tag="BSD:CONFIG", msg="Fcntl: fd=%d cmd=%d arg=%d", args="$x3, $x4, $x5"}
    Result Fcntl(
        sf::Out<s32> out_errno, sf::Out<s32> out_result,
        s32 fd, s32 cmd, s32 arg);

    /// @gdb{tag="BSD:CONFIG", msg="SetSockOpt: fd=%d level=%d optname=%d", args="$x2, $x3, $x4"}
    Result SetSockOpt(
        sf::Out<s32> out_errno,
        s32 fd, s32 level, s32 optname,
        const sf::InAutoSelectBuffer& optval);

    /// @gdb{tag="BSD:CONNECT", msg="Shutdown: fd=%d how=%d", args="$x2, $x3"}
    Result Shutdown(
        sf::Out<s32> out_errno,
        s32 fd, s32 how);

    /// @gdb{tag="BSD:CONNECT", msg="ShutdownAllSockets: pid=%lu how=%d", args="$x2, $x3"}
    Result ShutdownAllSockets(
        sf::Out<s32> out_errno,
        u64 pid, s32 how);

    /// @gdb{tag="BSD:DATA", msg="Write: fd=%d", args="$x3"}
    Result Write(
        sf::Out<s32> out_errno, sf::Out<s32> out_size,
        s32 fd,
        const sf::InAutoSelectBuffer& buffer);

    /// @gdb{tag="BSD:DATA", msg="Read: fd=%d", args="$x3"}
    Result Read(
        sf::Out<s32> out_errno, sf::Out<s32> out_size,
        s32 fd,
        sf::OutAutoSelectBuffer buffer);

    /// @gdb{tag="BSD:SOCKET", msg="Close: fd=%d", args="$x2"}
    Result Close(
        sf::Out<s32> out_errno,
        s32 fd);

    /// @gdb{tag="BSD:SOCKET", msg="DuplicateSocket: fd=%d target_pid=%lu", args="$x3, $x4"}
    Result DuplicateSocket(
        sf::Out<s32> out_errno, sf::Out<s32> out_fd,
        s32 fd, u64 target_pid);

    Result GetResourceStatistics(
        sf::Out<s32> out_errno,
        sf::OutBuffer out_stats,
        u64 pid);

    Result RecvMMsg(
        sf::Out<s32> out_errno, sf::Out<s32> out_count,
        s32 fd, s32 vlen, s32 flags, s32 timeout,
        sf::OutAutoSelectBuffer out_data);

    Result SendMMsg(
        sf::Out<s32> out_errno, sf::Out<s32> out_count,
        s32 fd, s32 vlen, s32 flags,
        const sf::InAutoSelectBuffer& in_data);

    Result EventFd(
        sf::Out<s32> out_errno, sf::Out<s32> out_fd,
        u64 initval, s32 flags);

    Result RegisterResourceStatisticsName(
        sf::Out<s32> out_errno,
        u64 pid,
        const sf::InBuffer& name);

    Result RegisterClientShared(
        sf::Out<u64> out_result,
        const ryu_ldn::bsd::LibraryConfigData& config,
        const sf::ClientProcessId& client_pid,
        u64 tmem_size);

private:
    /// Client process ID for this session
    u64 m_client_pid;
    /// Number of commands received on this session (for debugging)
    u32 m_command_count = 0;
    /// Unique session ID for debugging (assigned in constructor)
    u32 m_session_id = 0;
    /// Whether RegisterClient was called on this session
    /// Sessions without RegisterClient should not be used for socket operations
    bool m_registered = false;
    /// Static counter for session IDs (atomic for thread safety)
    static inline std::atomic<u32> s_next_session_id{0};
};

// Verify interface compliance
static_assert(ams::mitm::bsd::IsIBsdMitmService<BsdMitmService>);

} // namespace ams::mitm::bsd
