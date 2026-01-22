/**
 * @file process_monitor.hpp
 * @brief Process launch monitor for detecting LDN games
 *
 * This module monitors application launches using pm:shell service.
 * When a game starts, it checks the NACP for LocalCommunicationId
 * and stores whether the game supports LDN.
 *
 * This allows BSD MITM to intercept game sockets BEFORE ldn:u is opened,
 * solving the startup order problem (BSD opens before LDN).
 *
 * @copyright Copyright (c) 2026 ryu_ldn_nx contributors
 * @license GPL-2.0-or-later
 */

#pragma once

#include <stratosphere.hpp>
#include <atomic>

namespace ams::mitm::process_monitor {

/**
 * @brief Process launch monitor singleton
 *
 * Monitors application launches and checks their NACP for LDN support.
 * Results are stored in SharedState for BSD ShouldMitm to query.
 */
class ProcessMonitor {
public:
    /**
     * @brief Get singleton instance
     */
    static ProcessMonitor& GetInstance();

    /**
     * @brief Start the process monitor thread
     *
     * Called during sysmodule initialization.
     * Starts a background thread that monitors pm:shell events.
     */
    void Start();

    /**
     * @brief Stop the process monitor thread
     *
     * Called during sysmodule shutdown.
     */
    void Stop();

private:
    ProcessMonitor() = default;
    ~ProcessMonitor() = default;
    ProcessMonitor(const ProcessMonitor&) = delete;
    ProcessMonitor& operator=(const ProcessMonitor&) = delete;

    /**
     * @brief Thread entry point (static)
     */
    static void ThreadEntry(void* arg);

    /**
     * @brief Main monitoring loop
     */
    void MonitorLoop();

    /**
     * @brief Handle a process start event
     *
     * @param process_id The process ID that started
     */
    void OnProcessStarted(os::ProcessId process_id);

    /**
     * @brief Scan existing running processes at startup
     *
     * Handles the race condition where a game is already running
     * when the ProcessMonitor starts.
     */
    void ScanExistingProcesses();

    /**
     * @brief Check if a program supports LDN via NACP
     *
     * @param program_id The program ID to check
     * @return true if LocalCommunicationId[0] is non-zero
     */
    bool CheckLdnSupport(u64 program_id);

    os::ThreadType m_thread{};
    std::atomic<bool> m_running{false};
};

/**
 * @brief Initialize and start the process monitor
 *
 * Called from main.cpp during sysmodule startup.
 */
void Initialize();

/**
 * @brief Stop and cleanup the process monitor
 *
 * Called from main.cpp during sysmodule shutdown.
 */
void Finalize();

} // namespace ams::mitm::process_monitor
