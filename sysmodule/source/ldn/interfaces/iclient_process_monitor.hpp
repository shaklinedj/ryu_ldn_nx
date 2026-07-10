/**
 * @file iclient_process_monitor.hpp
 * @brief Client Process Monitor interface definition
 *
 * Required for firmware 18.0.0+ compatibility.
 *
 * @copyright Copyright (c) 2026 ryu_ldn_nx contributors
 * @license GPL-2.0-or-later
 */

#pragma once

#include <stratosphere.hpp>

#define AMS_ICLIENT_PROCESS_MONITOR_INTERFACE(C, H) \
    AMS_SF_METHOD_INFO(C, H, 0, Result, RegisterClient, (const ams::sf::ClientProcessId &client_process_id), (client_process_id))

// codeql[cpp/unused-local-variable,cpp/unused-static-variable] — macro
// expansion uses `args` via perfect forwarding
AMS_SF_DEFINE_INTERFACE(ams::mitm::ldn, IClientProcessMonitorInterface, AMS_ICLIENT_PROCESS_MONITOR_INTERFACE, 0x4EF8C3F3)
