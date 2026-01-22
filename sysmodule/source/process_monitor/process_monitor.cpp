/**
 * @file process_monitor.cpp
 * @brief Implementation of process launch monitor
 *
 * @copyright Copyright (c) 2026 ryu_ldn_nx contributors
 * @license GPL-2.0-or-later
 */

#include "process_monitor.hpp"
#include "../ldn/ldn_shared_state.hpp"
#include "../debug/log.hpp"

extern "C" {
#include <switch/services/ns.h>
#include <switch/services/pm.h>
#include <switch/nacp.h>
}

namespace ams::mitm::process_monitor {

namespace {
    // Thread stack for process monitor
    alignas(os::ThreadStackAlignment) u8 g_thread_stack[0x4000];
}

ProcessMonitor& ProcessMonitor::GetInstance() {
    static ProcessMonitor instance;
    return instance;
}

void ProcessMonitor::Start() {
    if (m_running.exchange(true)) {
        // Already running
        return;
    }

    LOG_INFO("ProcessMonitor: Starting process monitor thread");

    R_ABORT_UNLESS(os::CreateThread(
        &m_thread,
        ThreadEntry,
        this,
        g_thread_stack,
        sizeof(g_thread_stack),
        AMS_GET_SYSTEM_THREAD_PRIORITY(pgl, ProcessControlTask)
    ));

    os::SetThreadNamePointer(&m_thread, "ryu_proc_mon");
    os::StartThread(&m_thread);
}

void ProcessMonitor::Stop() {
    if (!m_running.exchange(false)) {
        // Not running
        return;
    }

    LOG_INFO("ProcessMonitor: Stopping process monitor thread");

    // Thread will exit on next timeout check (100ms max)
    // Wait for thread to finish
    os::WaitThread(&m_thread);
    os::DestroyThread(&m_thread);

    LOG_INFO("ProcessMonitor: Thread stopped");
}

void ProcessMonitor::ThreadEntry(void* arg) {
    auto* self = static_cast<ProcessMonitor*>(arg);
    self->MonitorLoop();
}

void ProcessMonitor::MonitorLoop() {
    LOG_INFO("ProcessMonitor: Monitor loop started");

    // Initialize pm:shell service
    Result rc = pmshellInitialize();
    if (R_FAILED(rc)) {
        LOG_ERROR("ProcessMonitor: pmshellInitialize failed: 0x%x", rc);
        m_running = false;
        return;
    }
    LOG_INFO("ProcessMonitor: pm:shell initialized");

    // Initialize pm:dmnt service (for GetProgramId)
    rc = pmdmntInitialize();
    if (R_FAILED(rc)) {
        LOG_ERROR("ProcessMonitor: pmdmntInitialize failed: 0x%x", rc);
        pmshellExit();
        m_running = false;
        return;
    }
    LOG_INFO("ProcessMonitor: pm:dmnt initialized");

    // Scan existing running application BEFORE listening for events
    // This handles the race condition where a game is already running
    ScanExistingProcesses();

    // Get process event handle from pm:shell
    os::SystemEvent process_event;
    rc = ams::pm::shell::GetProcessEventEvent(&process_event);

    if (R_FAILED(rc)) {
        LOG_ERROR("ProcessMonitor: Failed to get process event handle: 0x%x", rc);
        pmdmntExit();
        pmshellExit();
        m_running = false;
        return;
    }

    while (m_running) {
        // Wait for process event with timeout (100ms)
        // This allows us to check m_running periodically for shutdown
        bool signaled = process_event.TimedWait(TimeSpan::FromMilliSeconds(100));

        if (!m_running) {
            break;
        }

        if (signaled) {
            process_event.Clear();

            // Process all pending events
            bool continue_processing = true;
            while (continue_processing && m_running) {
                ams::pm::ProcessEventInfo event_info;
                rc = ams::pm::shell::GetProcessEventInfo(&event_info);

                if (R_FAILED(rc)) {
                    LOG_WARN("ProcessMonitor: GetProcessEventInfo failed: 0x%x", rc);
                    break;
                }

                switch (event_info.GetProcessEvent()) {
                    case ams::pm::ProcessEvent::None:
                        // No more events
                        continue_processing = false;
                        break;

                    case ams::pm::ProcessEvent::Started:
                        // Process started - check if it's an LDN game
                        OnProcessStarted(event_info.process_id);
                        break;

                    case ams::pm::ProcessEvent::Exited:
                        // Process exited - could clean up, but not strictly necessary
                        LOG_VERBOSE("ProcessMonitor: Process exited: pid=%lu",
                                    static_cast<u64>(event_info.process_id));
                        break;

                    default:
                        // Other events (Exception, DebugRunning, DebugBreak)
                        break;
                }
            }
        }
    }

    // Cleanup pm services
    pmdmntExit();
    pmshellExit();
    LOG_INFO("ProcessMonitor: Monitor loop ended");
}

void ProcessMonitor::OnProcessStarted(os::ProcessId process_id) {
    LOG_INFO("ProcessMonitor: Process started: pid=%lu", static_cast<u64>(process_id));

    // Get the program_id for this process
    ncm::ProgramId program_id;
    Result rc = ams::pm::dmnt::GetProgramId(&program_id, process_id);

    if (R_FAILED(rc)) {
        LOG_VERBOSE("ProcessMonitor: GetProgramId failed for pid=%lu: 0x%x",
                    static_cast<u64>(process_id), rc);
        return;
    }

    LOG_INFO("ProcessMonitor: Process pid=%lu has program_id=0x%016lx",
             static_cast<u64>(process_id), program_id.value);

    // Only check applications (program_id >= 0x0100000000000000)
    if (program_id.value < 0x0100000000000000ULL) {
        LOG_VERBOSE("ProcessMonitor: Not an application, skipping");
        return;
    }

    // Check if this application supports LDN
    if (CheckLdnSupport(program_id.value)) {
        // Store in SharedState for BSD ShouldMitm to query
        auto& shared_state = ldn::SharedState::GetInstance();
        shared_state.AddLdnGame(program_id.value);
        LOG_INFO("ProcessMonitor: Added LDN game: 0x%016lx", program_id.value);
    }
}

void ProcessMonitor::ScanExistingProcesses() {
    LOG_INFO("ProcessMonitor: Scanning existing processes...");

    // Get the currently running application process ID (if any)
    u64 app_pid = 0;
    Result rc = pmdmntGetApplicationProcessId(&app_pid);

    if (R_SUCCEEDED(rc) && app_pid != 0) {
        LOG_INFO("ProcessMonitor: Found running application: pid=%lu", app_pid);
        // Treat it as a newly started process
        OnProcessStarted(os::ProcessId{app_pid});
    } else {
        LOG_INFO("ProcessMonitor: No application currently running (rc=0x%x)", rc);
    }
}

bool ProcessMonitor::CheckLdnSupport(u64 program_id) {
    Result rc;
    bool has_ldn = false;

    // Initialize ns service
    rc = nsInitialize();
    if (R_FAILED(rc)) {
        LOG_WARN("ProcessMonitor: nsInitialize failed: 0x%x", rc);
        return false;
    }

    // Allocate buffer for control data (NACP + icon)
    NsApplicationControlData* control_data = static_cast<NsApplicationControlData*>(
        std::malloc(sizeof(NsApplicationControlData))
    );

    if (!control_data) {
        LOG_ERROR("ProcessMonitor: Failed to allocate NsApplicationControlData");
        nsExit();
        return false;
    }

    std::memset(control_data, 0, sizeof(NsApplicationControlData));

    u64 actual_size = 0;
    rc = nsGetApplicationControlData(
        NsApplicationControlSource_Storage,
        program_id,
        control_data,
        sizeof(NsApplicationControlData),
        &actual_size
    );

    if (R_SUCCEEDED(rc) && actual_size >= sizeof(NacpStruct)) {
        // Check if LocalCommunicationId[0] is non-zero
        u64 local_comm_id = control_data->nacp.local_communication_id[0];
        has_ldn = (local_comm_id != 0);

        if (has_ldn) {
            LOG_INFO("ProcessMonitor: program_id=0x%016lx supports LDN (LocalCommId=0x%016lx)",
                     program_id, local_comm_id);
        } else {
            LOG_VERBOSE("ProcessMonitor: program_id=0x%016lx does NOT support LDN",
                        program_id);
        }
    } else {
        LOG_VERBOSE("ProcessMonitor: nsGetApplicationControlData failed for 0x%016lx: 0x%x",
                    program_id, rc);
    }

    std::free(control_data);
    nsExit();

    return has_ldn;
}

// Module-level functions

void Initialize() {
    ProcessMonitor::GetInstance().Start();
}

void Finalize() {
    ProcessMonitor::GetInstance().Stop();
}

} // namespace ams::mitm::process_monitor
