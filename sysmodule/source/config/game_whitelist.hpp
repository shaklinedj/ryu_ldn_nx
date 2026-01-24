/**
 * @file game_whitelist.hpp
 * @brief LDN Game Whitelist
 *
 * Loads the whitelist once at startup and checks program IDs in memory.
 *
 * The file should be at: sdmc:/config/ryu_ldn_nx/gamelist.txt
 * Format: One hex game ID per line (e.g., 0x0100152000022000)
 *
 * @copyright Copyright (c) 2026 ryu_ldn_nx contributors
 * @license GPL-2.0-or-later
 */

#pragma once

#include <cstdint>

namespace ryu_ldn::config {

/**
 * @brief Load the whitelist from file (call once at startup)
 *
 * Reads the entire whitelist file and stores program IDs in memory.
 * Must be called after fs::MountSdCard().
 */
void LoadWhitelist();

/**
 * @brief Check if a game is in the LDN whitelist
 *
 * Fast lookup in the pre-loaded whitelist.
 *
 * @param program_id The program ID to check
 * @return true if the game is in the whitelist
 */
bool IsGameInWhitelist(uint64_t program_id);

} // namespace ryu_ldn::config
