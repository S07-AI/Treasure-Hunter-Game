#include "room.h"

#include <stdlib.h>
#include <string.h>

/* Returns true if there is a switch at (x, y) in room r. */
static bool room_has_switch_at(const Room *r, int x, int y) {
    for (int i = 0; i < r->switch_count; i++) {
        if (r->switches[i].x == x && r->switches[i].y == y) {
            return true;
        }
    }
    return false;
}

Room *room_create(int id, const char *name, int width, int height) {
    Room *r = malloc(sizeof(Room));
    if (r == NULL) {
        return NULL;
    }

    r->id = id;
    r->width = (width < 1) ? 1 : width;
    r->height = (height < 1) ? 1 : height;

    if (name != NULL) {
        size_t len = strlen(name);
        r->name = malloc(len + 1);
        if (r->name == NULL) {
            free(r);
            return NULL;
        }
        strcpy(r->name, name);
    } else {
        r->name = NULL;
    }

    r->floor_grid = NULL;
    r->neighbors = NULL;
    r->neighbor_count = 0;
    r->portals = NULL;
    r->portal_count = 0;
    r->treasures = NULL;
    r->treasure_count = 0;
    r->pushables = NULL;
    r->pushable_count = 0;
    r->switches = NULL;
    r->switch_count = 0;

    return r;
}

int room_get_width(const Room *r) {
    if (r == NULL) {
        return 0;
    }
    return r->width;
}

int room_get_height(const Room *r) {
    if (r == NULL) {
        return 0;
    }
    return r->height;
}

Status room_set_floor_grid(Room *r, bool *floor_grid) {
    if (r == NULL) {
        return INVALID_ARGUMENT;
    }

    free(r->floor_grid);
    r->floor_grid = floor_grid;

    return OK;
}

Status room_set_portals(Room *r, Portal *portals, int portal_count) {
    if (r == NULL) {
        return INVALID_ARGUMENT;
    }

    if (portal_count < 0 || (portal_count > 0 && portals == NULL)) {
        return INVALID_ARGUMENT;
    }

    if (r->portals != NULL) {
        for (int i = 0; i < r->portal_count; i++) {
            free(r->portals[i].name);
        }
        free(r->portals);
    }

    r->portals = portals;
    r->portal_count = portal_count;

    return OK;
}

Status room_set_treasures(Room *r, Treasure *treasures, int treasure_count) {
    if (r == NULL) {
        return INVALID_ARGUMENT;
    }

    if (treasure_count < 0 || (treasure_count > 0 && treasures == NULL)) {
        return INVALID_ARGUMENT;
    }

    if (r->treasures != NULL) {
        for (int i = 0; i < r->treasure_count; i++) {
            free(r->treasures[i].name);
        }
        free(r->treasures);
    }

    r->treasures = treasures;
    r->treasure_count = treasure_count;

    return OK;
}

Status room_place_treasure(Room *r, const Treasure *treasure) {
    if (r == NULL || treasure == NULL) {
        return INVALID_ARGUMENT;
    }

    Treasure *new_arr = realloc(r->treasures, sizeof(Treasure) * (r->treasure_count + 1));
    if (new_arr == NULL) {
        return NO_MEMORY;
    }

    r->treasures = new_arr;

    Treasure *slot = &r->treasures[r->treasure_count];

    *slot = *treasure;
    slot->collected = false;

    if (treasure->name != NULL) {
        slot->name = strdup(treasure->name);
        if (slot->name == NULL) {
            return NO_MEMORY;
        }
    } else {
        slot->name = NULL;
    }

    r->treasure_count++;

    return OK;
}

int room_get_treasure_at(const Room *r, int x, int y) {
    if (r == NULL) {
        return -1;
    }

    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].x == x &&
            r->treasures[i].y == y &&
            !r->treasures[i].collected) {
            return r->treasures[i].id;
        }
    }

    return -1;
}

int room_get_portal_destination(const Room *r, int x, int y) {
    if (r == NULL) {
        return -1;
    }

    for (int i = 0; i < r->portal_count; i++) {
        if (r->portals[i].x == x &&
            r->portals[i].y == y) {
            return r->portals[i].target_room_id;
        }
    }

    return -1;
}

bool room_is_walkable(const Room *r, int x, int y) {
    if (r == NULL) {
        return false;
    }

    if (x < 0 || y < 0 ||
        x >= r->width || y >= r->height) {
        return false;
    }

    if (r->floor_grid != NULL) {
        if (!r->floor_grid[y * r->width + x]) {
            return false;
        }
    } else {
        if (x == 0 || y == 0 ||
            x == r->width - 1 ||
            y == r->height - 1) {
            return false;
        }
    }

    /* A tile occupied by a pushable is not walkable,
     * unless the pushable is consumed by a switch (sitting on one). */
    for (int i = 0; i < r->pushable_count; i++) {
        if (r->pushables[i].x == x && r->pushables[i].y == y) {
            /* If this pushable is on a switch it is consumed — treat as floor */
            if (room_has_switch_at(r, x, y)) {
                return true;
            }
            return false;
        }
    }

    return true;
}

/* Returns true and sets *out_id if an uncollected treasure is at (x,y). */
static bool find_treasure_at(const Room *r, int x, int y, int *out_id) {
    for (int i = 0; i < r->treasure_count; i++) {
        if (!r->treasures[i].collected &&
            r->treasures[i].x == x &&
            r->treasures[i].y == y) {
            if (out_id != NULL) {
                *out_id = r->treasures[i].id;
            }
            return true;
        }
    }
    return false;
}

/* Returns true and sets *out_id if a portal is at (x,y). */
static bool find_portal_at(const Room *r, int x, int y, int *out_id) {
    for (int i = 0; i < r->portal_count; i++) {
        if (r->portals[i].x == x && r->portals[i].y == y) {
            if (out_id != NULL) {
                *out_id = r->portals[i].target_room_id;
            }
            return true;
        }
    }
    return false;
}

/* Returns true and sets *out_id if a non-consumed pushable is at (x,y). */
static bool find_pushable_at(const Room *r, int x, int y, int *out_id) {
    for (int i = 0; i < r->pushable_count; i++) {
        if (r->pushables[i].x == x && r->pushables[i].y == y) {
            if (room_has_switch_at(r, x, y)) {
                return false; /* consumed by switch — treat as floor */
            }
            if (out_id != NULL) {
                *out_id = i;
            }
            return true;
        }
    }
    return false;
}

RoomTileType room_classify_tile(const Room *r, int x, int y, int *out_id)
{
    if (r == NULL) {
        return ROOM_TILE_INVALID;
    }
    if (x < 0 || y < 0 || x >= r->width || y >= r->height) {
        return ROOM_TILE_INVALID;
    }
    if (find_treasure_at(r, x, y, out_id)) {
        return ROOM_TILE_TREASURE;
    }
    if (find_portal_at(r, x, y, out_id)) {
        return ROOM_TILE_PORTAL;
    }
    if (find_pushable_at(r, x, y, out_id)) {
        return ROOM_TILE_PUSHABLE;
    }
    if (room_is_walkable(r, x, y)) {
        return ROOM_TILE_FLOOR;
    }
    return ROOM_TILE_WALL;
}

static void render_base_layer(const Room *r, const Charset *charset,
                               char *buffer, int width, int height)
{
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int idx = row * width + col;
            bool is_border = (row == 0 || row == height - 1 ||
                              col == 0 || col == width - 1);
            if (r->floor_grid != NULL) {
                if (r->floor_grid[idx]) {
                    buffer[idx] = charset->floor;
                } else {
                    buffer[idx] = charset->wall;
                }
            } else if (is_border) {
                buffer[idx] = charset->wall;
            } else {
                buffer[idx] = charset->floor;
            }
        }
    }
}

/* Returns true if any pushable in r is sitting on switch sw. */
static bool switch_is_activated(const Room *r, const Switch *sw) {
    for (int i = 0; i < r->pushable_count; i++) {
        if (r->pushables[i].x == sw->x && r->pushables[i].y == sw->y) {
            return true;
        }
    }
    return false;
}

/* Renders uncollected treasures into buffer. */
static void render_treasures(const Room *r, const Charset *charset,
                              char *buffer, int width) {
    for (int i = 0; i < r->treasure_count; i++) {
        if (!r->treasures[i].collected) {
            buffer[r->treasures[i].y * width + r->treasures[i].x] =
                (char)charset->treasure;
        }
    }
}

/* Renders switches (on/off character) into buffer. */
static void render_switches(const Room *r, const Charset *charset,
                             char *buffer, int width) {
    char ch_on  = '*';
    char ch_off = '^';
    if (charset->switch_on >= 33 && charset->switch_on <= 126) {
        ch_on = (char)charset->switch_on;
    }
    if (charset->switch_off >= 33 && charset->switch_off <= 126) {
        ch_off = (char)charset->switch_off;
    }
    for (int i = 0; i < r->switch_count; i++) {
        int idx = r->switches[i].y * width + r->switches[i].x;
        if (switch_is_activated(r, &r->switches[i])) {
            buffer[idx] = ch_on;
        } else {
            buffer[idx] = ch_off;
        }
    }
}

/* Returns true if the gated portal at index i is currently unlocked. */
static bool portal_is_unlocked(const Room *r, int portal_idx) {
    int sw_id = r->portals[portal_idx].required_switch_id;
    for (int j = 0; j < r->switch_count; j++) {
        if (r->switches[j].id == sw_id) {
            return switch_is_activated(r, &r->switches[j]);
        }
    }
    return false;
}

/* Renders portals (locked as 'L' or open) into buffer. */
static void render_portals(const Room *r, const Charset *charset,
                            char *buffer, int width) {
    for (int i = 0; i < r->portal_count; i++) {
        int idx = r->portals[i].y * width + r->portals[i].x;
        if (r->portals[i].gated && !portal_is_unlocked(r, i)) {
            buffer[idx] = 'L';
        } else {
            buffer[idx] = (char)charset->portal;
        }
    }
}

/* Renders pushables, skipping any that are consumed by a switch. */
static void render_pushables(const Room *r, const Charset *charset,
                              char *buffer, int width) {
    for (int i = 0; i < r->pushable_count; i++) {
        int px = r->pushables[i].x;
        int py = r->pushables[i].y;
        if (room_has_switch_at(r, px, py)) {
            continue; /* consumed — switch_on char already drawn */
        }
        buffer[py * width + px] = (char)charset->pushable;
    }
}

Status room_render(const Room *r, const Charset *charset, char *buffer,
                   int buffer_width, int buffer_height)
{
    if (r == NULL || charset == NULL || buffer == NULL) {
        return INVALID_ARGUMENT;
    }
    if (buffer_width != r->width || buffer_height != r->height) {
        return INVALID_ARGUMENT;
    }
    render_base_layer(r, charset, buffer, r->width, r->height);
    render_treasures(r, charset, buffer, r->width);
    render_switches(r, charset, buffer, r->width);
    render_portals(r, charset, buffer, r->width);
    render_pushables(r, charset, buffer, r->width);
    return OK;
}

Status room_get_start_position(const Room *r, int *x_out, int *y_out)
{
    if (r == NULL || x_out == NULL || y_out == NULL) {
        return INVALID_ARGUMENT;
    }

    if (r->portal_count > 0) {
        *x_out = r->portals[0].x;
        *y_out = r->portals[0].y;
        return OK;
    }

    for (int row = 0; row < r->height; row++) {
        for (int col = 0; col < r->width; col++) {
            if (room_is_walkable(r, col, row)) {
                *x_out = col;
                *y_out = row;
                return OK;
            }
        }
    }

    return ROOM_NOT_FOUND;
}

Status room_pick_up_treasure(Room *r, int treasure_id, Treasure **treasure_out) {
    if (r == NULL || treasure_out == NULL) {
        return INVALID_ARGUMENT;
    }

    for (int i = 0; i < r->treasure_count; i++) {
        if (r->treasures[i].id == treasure_id) {

            if (r->treasures[i].collected) {
                return INVALID_ARGUMENT;
            }

            r->treasures[i].collected = true;
            *treasure_out = &r->treasures[i];

            return OK;
        }
    }

    return ROOM_NOT_FOUND;
}



int room_get_id(const Room *r) {
    if (r == NULL) {
        return -1;
    }

    return r->id;
}


void destroy_treasure(Treasure *t)
{
    if (t == NULL) {
        return;
    }

    free(t->name);
    t->name = NULL;
}


/*
 * Check whether a pushable exists at (x,y).
 *
 * Preconditions:
 *   r is not NULL 
 *
 * Returns:
 *   true if a pushable exists at the specified coordinates
 *   false if it does not, or on invalid arguments
 *
 * Postconditions:
 *   If pushable_idx_out is non-NULL, it receives the pushable index used 
 *   internally by the room.
 */
bool room_has_pushable_at(const Room *r, int x, int y, int *pushable_idx_out) {
    if (r == NULL) {
        return false;
    }

    for (int i = 0; i < r->pushable_count; i++) {
        if (r->pushables[i].x == x && r->pushables[i].y == y) {
            /* Consumed pushables (on a switch) don't occupy the tile */
            if (room_has_switch_at(r, x, y)) {
                return false;
            }
            if (pushable_idx_out != NULL) {
                *pushable_idx_out = i;
            }
            return true;
        }
    }

    return false;
}

/*
 * Attempt to push a pushable in the given direction.
 *
 * Preconditions:
 *   r is not NULL 
 *   pushable_idx is not negative and is less than r->pushable_count
 *   dir is a valid member of the enum type Direction
 *
 * Returns:
 *   OK on success
 *   ROOM_IMPASSABLE if blocked
 *   INVALID_ARGUMENT if arguments are invalid
 *
 * Postconditions:
 *   If push was possible, the pushable's x and y coordinates in r->pushables
 *   have been correctly updated - i.e. the obstacle was pushed
 */
Status room_try_push(Room *r, int pushable_idx, Direction dir) {
    if (r == NULL) {
        return INVALID_ARGUMENT;
    }

    if (pushable_idx < 0 || pushable_idx >= r->pushable_count) {
        return INVALID_ARGUMENT;
    }

    Pushable *p = &r->pushables[pushable_idx];

    int new_x = p->x;
    int new_y = p->y;

    switch (dir) {
        case DIR_NORTH: new_y--; break;
        case DIR_SOUTH: new_y++; break;
        case DIR_EAST:  new_x++; break;
        case DIR_WEST:  new_x--; break;
        default: return INVALID_ARGUMENT;
    }

    if (!room_is_walkable(r, new_x, new_y)) {
        return ROOM_IMPASSABLE;
    }

    int other_idx = 0;
    if (room_has_pushable_at(r, new_x, new_y, &other_idx)) {
        return ROOM_IMPASSABLE;
    }

    p->x = new_x;
    p->y = new_y;

    return OK;
}




void room_destroy(Room *r) {
    if (r == NULL) {
        return;
    }

    free(r->name);
    free(r->floor_grid);
    free(r->neighbors);

    for (int i = 0; i < r->portal_count; i++) {
        free(r->portals[i].name);
    }
    free(r->portals);

    for (int i = 0; i < r->pushable_count; i++) {
        free(r->pushables[i].name);
    }
    free(r->pushables);

    free(r->switches);

    for (int i = 0; i < r->treasure_count; i++) {
        free(r->treasures[i].name);
    }
    free(r->treasures);

    free(r);
}