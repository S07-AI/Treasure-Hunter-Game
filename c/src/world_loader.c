#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "graph.h"
#include "datagen.h"
#include "world_loader.h"
#include "room.h"

int room_compare(const void *a, const void *b) {
    const Room *ra = (const Room *)a;
    const Room *rb = (const Room *)b;
    return ra->id - rb->id;
}

void room_destroy_wrapper(void *payload) {
    room_destroy((Room *)payload);
}

static Room *find_room_by_id(Room **rooms, int count, int id) {
    for (int i = 0; i < count; i++) {
        if (rooms[i]->id == id) {
            return rooms[i];
        }
    }
    return NULL;
}

static int copy_floor_grid_to_room(Room *room, const DG_Room *dg) {
    if (dg->floor_grid == NULL) {
        return 0;
    }
    int size = dg->width * dg->height;
    bool *grid = malloc(sizeof(bool) * size);
    if (grid == NULL) {
        return -1;
    }
    for (int i = 0; i < size; i++) {
        grid[i] = dg->floor_grid[i];
    }
    if (room_set_floor_grid(room, grid) != OK) {
        free(grid);
        return -1;
    }
    return 0;
}

static int copy_portals_to_room(Room *room, const DG_Room *dg) {
    if (dg->portal_count <= 0) {
        return 0;
    }
    Portal *portals = malloc(sizeof(Portal) * dg->portal_count);
    if (portals == NULL) {
        return -1;
    }
    for (int i = 0; i < dg->portal_count; i++) {
        portals[i].id = dg->portals[i].id;
        portals[i].x = dg->portals[i].x;
        portals[i].y = dg->portals[i].y;
        portals[i].target_room_id = dg->portals[i].neighbor_id;
        portals[i].name = NULL;
        portals[i].required_switch_id = dg->portals[i].required_switch_id;
        portals[i].gated = (dg->portals[i].required_switch_id >= 0);
    }
    if (room_set_portals(room, portals, dg->portal_count) != OK) {
        free(portals);
        return -1;
    }
    return 0;
}

static int copy_pushables_to_room(Room *room, const DG_Room *dg) {
    if (dg->pushable_count <= 0) {
        return 0;
    }
    Pushable *pushables = malloc(sizeof(Pushable) * dg->pushable_count);
    if (pushables == NULL) {
        return -1;
    }
    for (int i = 0; i < dg->pushable_count; i++) {
        pushables[i].id = dg->pushables[i].id;
        pushables[i].x = dg->pushables[i].x;
        pushables[i].y = dg->pushables[i].y;
        pushables[i].initial_x = dg->pushables[i].x;
        pushables[i].initial_y = dg->pushables[i].y;
        if (dg->pushables[i].name != NULL) {
            pushables[i].name = malloc(strlen(dg->pushables[i].name) + 1);
            if (pushables[i].name == NULL) {
                for (int j = 0; j < i; j++) {
                    free(pushables[j].name);
                }
                free(pushables);
                return -1;
            }
            strcpy(pushables[i].name, dg->pushables[i].name);
        } else {
            pushables[i].name = NULL;
        }
    }
    room->pushables = pushables;
    room->pushable_count = dg->pushable_count;
    return 0;
}

static int copy_treasures_to_room(Room *room, const DG_Room *dg) {
    if (dg->treasure_count <= 0) {
        return 0;
    }
    Treasure *treasures = malloc(sizeof(Treasure) * dg->treasure_count);
    if (treasures == NULL) {
        return -1;
    }
    for (int i = 0; i < dg->treasure_count; i++) {
        treasures[i].id = dg->treasures[i].global_id;
        treasures[i].x = dg->treasures[i].x;
        treasures[i].y = dg->treasures[i].y;
        treasures[i].initial_x = dg->treasures[i].x;
        treasures[i].initial_y = dg->treasures[i].y;
        treasures[i].starting_room_id = dg->id;
        treasures[i].collected = false;
        if (dg->treasures[i].name != NULL) {
            treasures[i].name = malloc(strlen(dg->treasures[i].name) + 1);
            if (treasures[i].name == NULL) {
                for (int j = 0; j < i; j++) {
                    free(treasures[j].name);
                }
                free(treasures);
                return -1;
            }
            strcpy(treasures[i].name, dg->treasures[i].name);
        } else {
            treasures[i].name = NULL;
        }
    }
    if (room_set_treasures(room, treasures, dg->treasure_count) != OK) {
        for (int i = 0; i < dg->treasure_count; i++) {
            free(treasures[i].name);
        }
        free(treasures);
        return -1;
    }
    return 0;
}


/* Added for A3: load switch data from datagen into the room */
static int copy_switches_to_room(Room *room, const DG_Room *dg) {
    if (dg->switch_count <= 0) {
        return 0;
    }
    Switch *switches = malloc(sizeof(Switch) * dg->switch_count);
    if (switches == NULL) {
        return -1;
    }
    for (int i = 0; i < dg->switch_count; i++) {
        switches[i].id        = dg->switches[i].id;
        switches[i].x         = dg->switches[i].x;
        switches[i].y         = dg->switches[i].y;
        switches[i].portal_id = dg->switches[i].portal_id;
    }
    room->switches      = switches;
    room->switch_count  = dg->switch_count;
    return 0;
}

static Room *copy_room(const DG_Room *dg) {
    Room *room = room_create(dg->id, NULL, dg->width, dg->height);
    if (room == NULL) {
        return NULL;
    }
    if (copy_floor_grid_to_room(room, dg) != 0 ||
        copy_portals_to_room(room, dg)   != 0 ||
        copy_pushables_to_room(room, dg) != 0 ||
        copy_treasures_to_room(room, dg) != 0 ||
        copy_switches_to_room(room, dg)  != 0) {
        room_destroy(room);
        return NULL;
    }
    return room;
}

static Status build_graph_edges(Graph *graph, Room **rooms, int room_count) {
    for (int i = 0; i < room_count; i++) {
        Room *room = rooms[i];
        for (int j = 0; j < room->portal_count; j++) {
            int neighbor_id = room->portals[j].target_room_id;
            if (neighbor_id < 0) {
                continue;
            }
            Room *neighbor = find_room_by_id(rooms, room_count, neighbor_id);
            if (neighbor == NULL) {
                continue;
            }
            GraphStatus gs = graph_connect(graph, room, neighbor);
            if (gs != GRAPH_STATUS_OK && gs != GRAPH_STATUS_DUPLICATE_EDGE) {
                return INTERNAL_ERROR;
            }
        }
    }
    return OK;
}

Status loader_load_world(const char *config_file, Graph **graph_out, Room **first_room_out, int *num_rooms_out, Charset *charset_out) {

    if (config_file == NULL ||
        graph_out == NULL ||
        first_room_out == NULL ||
        num_rooms_out == NULL ||
        charset_out == NULL) {
        return INVALID_ARGUMENT;
    }

    int result = start_datagen(config_file);

    if (result == DG_ERR_CONFIG) {
        return WL_ERR_CONFIG;
    }

    if (result != DG_OK) {
        return WL_ERR_DATAGEN;
    }

    const DG_Charset *dg_charset = dg_get_charset();
    if (dg_charset == NULL) {
        stop_datagen();
        return WL_ERR_DATAGEN;
    }

    charset_out->wall       = dg_charset->wall;
    charset_out->floor      = dg_charset->floor;
    charset_out->player     = dg_charset->player;
    charset_out->treasure   = dg_charset->treasure;
    charset_out->portal     = dg_charset->portal;
    charset_out->pushable   = dg_charset->pushable;
    charset_out->switch_off = dg_charset->switch_off;
    charset_out->switch_on  = dg_charset->switch_on;

    Graph *graph = NULL;
    GraphStatus gs = graph_create(room_compare,
                                  room_destroy_wrapper,
                                  &graph);

    if (gs != GRAPH_STATUS_OK || graph == NULL) {
        stop_datagen();
        return NO_MEMORY;
    }

    int room_count = 0;
    Room **rooms = NULL;

    while (has_more_rooms()) {

        DG_Room dg_room = get_next_room();

        Room *room = copy_room(&dg_room);
        if (room == NULL) {

            free(rooms);
            graph_destroy(graph);
            stop_datagen();
            return NO_MEMORY;
        }

        GraphStatus ins = graph_insert(graph, room);
        if (ins != GRAPH_STATUS_OK) {

            room_destroy(room);
            free(rooms);
            graph_destroy(graph);
            stop_datagen();
            return INTERNAL_ERROR;
        }

        Room **new_rooms = realloc(rooms,
                                   sizeof(Room *) * (room_count + 1));

        if (new_rooms == NULL) {

            free(rooms);
            graph_destroy(graph);
            stop_datagen();
            return NO_MEMORY;
        }

        rooms = new_rooms;
        rooms[room_count] = room;
        room_count++;
    }

    Status edge_status = build_graph_edges(graph, rooms, room_count);
    if (edge_status != OK) {
        free(rooms);
        graph_destroy(graph);
        stop_datagen();
        return edge_status;
    }

    Room *first_room = NULL;

    if (room_count > 0) {
        first_room = rooms[0];
    }

    free(rooms);
    stop_datagen();

    *graph_out = graph;
    *first_room_out = first_room;
    *num_rooms_out = room_count;

    return OK;
}