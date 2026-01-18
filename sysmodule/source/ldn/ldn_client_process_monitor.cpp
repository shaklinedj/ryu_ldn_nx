/**
 * @file ldn_client_process_monitor.cpp
 * @brief Client Process Monitor implementation
 *
 * @copyright Copyright (c) 2026 ryu_ldn_nx contributors
 * @license GPL-2.0-or-later
 */

#include "ldn_client_process_monitor.hpp"
#include "../debug/log.hpp"

namespace ams::mitm::ldn {

IClientProcessMonitor::IClientProcessMonitor() {
    LOG_VERBOSE("IClientProcessMonitor created");
}

IClientProcessMonitor::~IClientProcessMonitor() {
    LOG_VERBOSE("IClientProcessMonitor destroyed");
}

Result IClientProcessMonitor::RegisterClient(const sf::ClientProcessId &client_process_id) {
    LOG_INFO("IClientProcessMonitor::RegisterClient pid=%lu", client_process_id.GetValue());

    // Stub implementation for firmware 18.0.0+ compatibility
    // Games require this function but don't use its functionality
    R_SUCCEED();
}

} // namespace ams::mitm::ldn
