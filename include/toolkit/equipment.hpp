/**
 * Player Overworld Equipment Management
 */
#pragma once

namespace botw::toolkit::equipment {

/** Init the system, install hooks */
void init();

/**
 * Re-create the overworld equipment actors to sync with inventory
 *
 * Return:
 * - 0 if successful
 * - -1 if failed to sync
 * - 1 if game is not paused with dpad
 */
int sync_with_pmdm();

} // namespace botw::toolkit::equipment
