#ifndef A3_EXTENSIONS_H
#define A3_EXTENSIONS_H

#include "types.h"
#include "game_engine.h"

/* ============================================================
 * A3 Extended Feature: Collect All Treasure
 * ============================================================ */

/*
 * Get the total number of treasures across all rooms in the world.
 *
 * Parameters:
 *   eng:       The game engine.
 *   count_out: Receives the total treasure count.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if eng or count_out is NULL
 *   INTERNAL_ERROR if the graph is invalid
 */
Status game_engine_get_total_treasure_count(const GameEngine *eng,
                                             int *count_out);

/* ============================================================
 * A3 Extended Feature: Locked Doors (Switches)
 * ============================================================ */

/*
 * Check whether the portal at (x, y) in the player's current room
 * is currently locked (gated and its switch has no pushable on it).
 *
 * Parameters:
 *   eng:       The game engine.
 *   x, y:      Tile coordinates of the portal.
 *   locked_out: Receives true if locked, false if open or not gated.
 *
 * Returns:
 *   OK on success
 *   INVALID_ARGUMENT if eng or locked_out is NULL
 *   GE_NO_SUCH_ROOM if the current room cannot be found
 */
Status game_engine_is_portal_locked(const GameEngine *eng,
                                     int x, int y,
                                     bool *locked_out);

#endif /* A3_EXTENSIONS_H */