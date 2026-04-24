// game_engine.c

#include <stdlib.h>
#include <string.h>
#include "game_engine.h"
#include "a3_extensions.h"
#include "world_loader.h"
#include "player.h"
#include "room.h"
#include "graph.h"
#include "types.h"


static Room *get_current_room(const GameEngine *eng) {
    if (eng == NULL || eng->player == NULL || eng->graph == NULL) {
        return NULL;
    }

    int room_id = player_get_room(eng->player);

    Room search_key;
    search_key.id = room_id;

    const void *payload = graph_get_payload(eng->graph, &search_key);
    return (Room *)payload;
}

static Room *get_room_by_id(const GameEngine *eng, int room_id) {
    if (eng == NULL || eng->graph == NULL) {
        return NULL;
    }

    Room search_key;
    search_key.id = room_id;

    const void *payload = graph_get_payload(eng->graph, &search_key);
    return (Room *)payload;
}



Status game_engine_create(const char *config_file_path, GameEngine **engine_out)
{
    if (config_file_path == NULL || engine_out == NULL) {
        return INVALID_ARGUMENT;
    }

    GameEngine *eng = malloc(sizeof(GameEngine));
    if (eng == NULL) {
        return NO_MEMORY;
    }

    eng->graph = NULL;
    eng->player = NULL;
    eng->room_count = 0;

    Graph *graph = NULL;
    Room *first_room = NULL;
    int num_rooms = 0;
    Charset charset;

    Status status = loader_load_world(config_file_path, &graph, &first_room, &num_rooms, &charset);

    if (status != OK) {
        free(eng);
        return status;
    }

    eng->graph = graph;
    eng->charset = charset;
    eng->room_count = num_rooms;

    int start_x = 0;
    int start_y = 0;

    status = room_get_start_position(first_room, &start_x, &start_y);
    if (status != OK) {
        graph_destroy(graph);
        free(eng);
        return status;
    }

    eng->initial_room_id = first_room->id;
    eng->initial_player_x = start_x;
    eng->initial_player_y = start_y;

    Player *player = NULL;
    status = player_create(first_room->id, start_x, start_y, &player);
    if (status != OK) {
        graph_destroy(graph);
        free(eng);
        return status;
    }

    eng->player = player;

    *engine_out = eng;
    return OK;
}

void game_engine_destroy(GameEngine *eng) {
    if (eng == NULL) {
        return;
    }

    player_destroy(eng->player);
    graph_destroy(eng->graph);
    free(eng);
}


const Player *game_engine_get_player(const GameEngine *eng) {
    if (eng == NULL) {
        return NULL;
    }
    return eng->player;
}


static Status handle_treasure(GameEngine *eng, Room *room, int entity_id)
{
    Treasure *found = NULL;
    for (int i = 0; i < room->treasure_count; i++) {
        if (room->treasures[i].id == entity_id &&
            !room->treasures[i].collected) {
            found = &room->treasures[i];
            break;
        }
    }
    if (found == NULL) {
        return INTERNAL_ERROR;
    }
    Status s = player_try_collect(eng->player, found);
    return (s == OK) ? OK : INTERNAL_ERROR;
}

static Status handle_pushable(Room *room, int entity_id, Direction dir,
                               Player *player, int target_x, int target_y)
{
    Status s = room_try_push(room, entity_id, dir);
    if (s == ROOM_IMPASSABLE) {
        return ROOM_IMPASSABLE;
    }
    if (s != OK) {
        return INTERNAL_ERROR;
    }
    return player_set_position(player, target_x, target_y);
}

static Status handle_portal(GameEngine *eng, int dest_room_id,
                             int fallback_x, int fallback_y)
{
    /* Check if the portal at fallback_x, fallback_y is locked */
    bool locked = false;
    game_engine_is_portal_locked(eng, fallback_x, fallback_y, &locked);
    if (locked) {
        return ROOM_IMPASSABLE;
    }

    Room *dest_room = get_room_by_id(eng, dest_room_id);
    if (dest_room == NULL) {
        /* Portal has no valid destination (e.g. neighbor_id == -1).
         * Treat it as a plain floor tile: just move the player onto it. */
        return player_set_position(eng->player, fallback_x, fallback_y);
    }
    int dest_x = 0;
    int dest_y = 0;
    Status s = room_get_start_position(dest_room, &dest_x, &dest_y);
    if (s != OK) {
        return s;
    }
    s = player_move_to_room(eng->player, dest_room_id);
    if (s != OK) {
        return s;
    }
    return player_set_position(eng->player, dest_x, dest_y);
}

static Status compute_target(const GameEngine *eng, Direction dir,
                              int *tx, int *ty)
{
    int cx = 0;
    int cy = 0;
    if (player_get_position(eng->player, &cx, &cy) != OK) {
        return INTERNAL_ERROR;
    }
    *tx = cx;
    *ty = cy;
    switch (dir) {
        case DIR_NORTH: (*ty)--; return OK;
        case DIR_SOUTH: (*ty)++; return OK;
        case DIR_EAST:  (*tx)++; return OK;
        case DIR_WEST:  (*tx)--; return OK;
        default:                 return INVALID_ARGUMENT;
    }
}

static Status dispatch_tile(GameEngine *eng, Room *room, Direction dir,
                             int tx, int ty)
{
    int entity_id = -1;
    RoomTileType tile = room_classify_tile(room, tx, ty, &entity_id);
    switch (tile) {
        case ROOM_TILE_INVALID:
        case ROOM_TILE_WALL:
            return ROOM_IMPASSABLE;
        case ROOM_TILE_FLOOR:
            return player_set_position(eng->player, tx, ty);
        case ROOM_TILE_TREASURE:
            /* Moving into a treasure tile collects it without advancing the player. */
            handle_treasure(eng, room, entity_id);
            return OK;
        case ROOM_TILE_PUSHABLE:
            return handle_pushable(room, entity_id, dir, eng->player, tx, ty);
        case ROOM_TILE_PORTAL:
            return handle_portal(eng, entity_id, tx, ty);
        default:
            return INTERNAL_ERROR;
    }
}

Status game_engine_move_player(GameEngine *eng, Direction dir) {
    if (eng == NULL) {
        return INVALID_ARGUMENT;
    }
    if (eng->player == NULL || eng->graph == NULL) {
        return INTERNAL_ERROR;
    }
    Room *current_room = get_current_room(eng);
    if (current_room == NULL) {
        return GE_NO_SUCH_ROOM;
    }

    int tx = 0;
    int ty = 0;
    Status s = compute_target(eng, dir, &tx, &ty);
    if (s != OK) {
        return s;
    }
    return dispatch_tile(eng, current_room, dir, tx, ty);
}

Status game_engine_get_room_count(const GameEngine *eng, int *count_out)
{
    if (eng == NULL) return INVALID_ARGUMENT;
    if (count_out == NULL) return NULL_POINTER;

    *count_out = eng->room_count;
    return OK;
}

Status game_engine_get_room_dimensions(const GameEngine *eng, int *width_out, int *height_out)
{
    if (eng == NULL) return INVALID_ARGUMENT;
    if (width_out == NULL || height_out == NULL) return NULL_POINTER;

    Room *room = get_current_room(eng);
    if (room == NULL) return GE_NO_SUCH_ROOM;

    *width_out = room_get_width(room);
    *height_out = room_get_height(room);
    return OK;
}


Status game_engine_reset(GameEngine *eng) {
    if (eng == NULL) return INVALID_ARGUMENT;
    if (eng->player == NULL) return INTERNAL_ERROR;

    /* Reset player position and clear collected treasures */
    Status status = player_reset_to_start(eng->player,
                                          eng->initial_room_id,
                                          eng->initial_player_x,
                                          eng->initial_player_y);
    if (status != OK) return status;

    /* Reset all rooms: restore pushables and uncollect treasures */
    const void * const *payloads = NULL;
    int payload_count = 0;
    GraphStatus gs = graph_get_all_payloads(eng->graph, &payloads, &payload_count);
    if (gs != GRAPH_STATUS_OK) return INTERNAL_ERROR;

    for (int i = 0; i < payload_count; i++) {
        Room *room = (Room *)payloads[i];

        /* Restore collected treasures to uncollected */
        for (int j = 0; j < room->treasure_count; j++) {
            room->treasures[j].collected = false;
            room->treasures[j].x = room->treasures[j].initial_x;
            room->treasures[j].y = room->treasures[j].initial_y;
        }

        /* Restore pushables to initial positions */
        for (int j = 0; j < room->pushable_count; j++) {
            room->pushables[j].x = room->pushables[j].initial_x;
            room->pushables[j].y = room->pushables[j].initial_y;
        }
    }

    return OK;
}


Status game_engine_render_current_room(const GameEngine *eng,char **str_out) {
    if (eng == NULL || str_out == NULL) {
        return INVALID_ARGUMENT;
    }

    Room *room = get_current_room(eng);
    if (room == NULL) return GE_NO_SUCH_ROOM;

    int width = room_get_width(room);
    int height = room_get_height(room);

    char *buffer = malloc((size_t)width * (size_t)height);

    if (buffer == NULL) return NO_MEMORY;

    Status status = room_render(room, &eng->charset, buffer, width, height);

    if (status != OK) {
        free(buffer);
        return status;
    }

    int px = 0;
    int py = 0;

    status = player_get_position(eng->player, &px, &py);
    if (status == OK &&
        px >= 0 && px < width &&
        py >= 0 && py < height)
    {
        buffer[py * width + px] = eng->charset.player;
    }

    char *output = malloc((width + 1) * height + 1);
    if (output == NULL) {
        free(buffer);
        return NO_MEMORY;
    }

    int out_idx = 0;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            output[out_idx++] = buffer[row * width + col];
        }
        output[out_idx++] = '\n';
    }
    output[out_idx] = '\0';

    free(buffer);
    *str_out = output;
    return OK;
}

Status game_engine_render_room(const GameEngine *eng, int room_id, char **str_out)
{
    if (eng == NULL) return INVALID_ARGUMENT;
    if (str_out == NULL) return NULL_POINTER;

    Room *room = get_room_by_id(eng, room_id);
    if (room == NULL) return GE_NO_SUCH_ROOM;

    int width = room_get_width(room);
    int height = room_get_height(room);

    char *buffer = malloc((size_t)width * (size_t)height);
    
    if (buffer == NULL) return NO_MEMORY;

    Status status = room_render(room, &eng->charset, buffer, width, height);
    if (status != OK) {
        free(buffer);
        return status;
    }

    char *output = malloc((width + 1) * height + 1);
    if (output == NULL) {
        free(buffer);
        return NO_MEMORY;
    }

    int out_idx = 0;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            output[out_idx++] = buffer[row * width + col];
        }
        output[out_idx++] = '\n';
    }
    output[out_idx] = '\0';

    free(buffer);
    *str_out = output;
    return OK;
}



Status game_engine_get_room_ids(const GameEngine *eng, int **ids_out, int *count_out)
{
    if (eng == NULL) return INVALID_ARGUMENT;
    if (ids_out == NULL || count_out == NULL) return NULL_POINTER;
    if (eng->graph == NULL) return INTERNAL_ERROR;

    const void * const *payloads = NULL;
    int payload_count = 0;

    GraphStatus gs =
        graph_get_all_payloads(eng->graph, &payloads, &payload_count);
    if (gs != GRAPH_STATUS_OK) {
        return INTERNAL_ERROR;
    }

    int *ids = malloc(sizeof(int) * payload_count);
    if (ids == NULL) {
        return NO_MEMORY;
    }

    for (int i = 0; i < payload_count; i++) {
        const Room *room = (const Room *)payloads[i];
        ids[i] = room->id;
    }

    *ids_out = ids;
    *count_out = payload_count;
    return OK;
}

/*
 * Free a heap buffer allocated by the game engine (e.g., render string).
 *
 * This is required for C/Python interoperability in A2 and A3
 *
 */
void game_engine_free_string(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    free(ptr);
}

Status game_engine_get_room_by_id(const GameEngine *eng,
                                   int room_id,
                                   Room **room_out)
{
    if (eng == NULL || room_out == NULL) {
        return INVALID_ARGUMENT;
    }

    Room *r = get_room_by_id(eng, room_id);
    if (r == NULL) {
        return GE_NO_SUCH_ROOM;
    }

    *room_out = r;
    return OK;
}


/* ============================================================
 * A3 Extended Feature: Collect All Treasure
 * ============================================================ */

Status game_engine_get_total_treasure_count(const GameEngine *eng,
                                             int *count_out)
{
    if (eng == NULL || count_out == NULL) return INVALID_ARGUMENT;
    if (eng->graph == NULL) return INTERNAL_ERROR;

    const void * const *payloads = NULL;
    int payload_count = 0;
    GraphStatus gs = graph_get_all_payloads(eng->graph, &payloads, &payload_count);
    if (gs != GRAPH_STATUS_OK) return INTERNAL_ERROR;

    int total = 0;
    for (int i = 0; i < payload_count; i++) {
        const Room *room = (const Room *)payloads[i];
        total += room->treasure_count;
    }
    *count_out = total;
    return OK;
}


/* ============================================================
 * A3 Extended Feature: Locked Doors (Switches)
 * ============================================================ */

Status game_engine_is_portal_locked(const GameEngine *eng,
                                     int x, int y,
                                     bool *locked_out)
{
    if (eng == NULL || locked_out == NULL) return INVALID_ARGUMENT;

    Room *room = get_current_room(eng);
    if (room == NULL) return GE_NO_SUCH_ROOM;

    /* Find the portal at (x, y) */
    Portal *portal = NULL;
    for (int i = 0; i < room->portal_count; i++) {
        if (room->portals[i].x == x && room->portals[i].y == y) {
            portal = &room->portals[i];
            break;
        }
    }

    /* No portal here or portal is not gated */
    if (portal == NULL || !portal->gated) {
        *locked_out = false;
        return OK;
    }

    /* Find the switch that controls this portal */
    int sw_id = portal->required_switch_id;
    Switch *sw = NULL;
    for (int i = 0; i < room->switch_count; i++) {
        if (room->switches[i].id == sw_id) {
            sw = &room->switches[i];
            break;
        }
    }

    if (sw == NULL) {
        /* Switch referenced but not found — treat as permanently locked */
        *locked_out = true;
        return OK;
    }

    /* Check if any pushable is resting on the switch tile */
    bool activated = false;
    for (int i = 0; i < room->pushable_count; i++) {
        if (room->pushables[i].x == sw->x && room->pushables[i].y == sw->y) {
            activated = true;
            break;
        }
    }

    *locked_out = !activated;
    return OK;
}