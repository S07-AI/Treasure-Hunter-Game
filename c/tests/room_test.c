#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "room.h"

/* ============================================================
   Creation
============================================================ */

START_TEST(test_room_create_basic)
{
    Room *r = room_create(0, "A", 5, 5);
    ck_assert_ptr_nonnull(r);
    ck_assert_int_eq(room_get_width(r), 5);
    ck_assert_int_eq(room_get_height(r), 5);
    room_destroy(r);
}
END_TEST

START_TEST(test_room_create_clamps_dimensions)
{
    Room *r = room_create(0, "A", 0, 0);
    ck_assert_int_eq(room_get_width(r), 1);
    ck_assert_int_eq(room_get_height(r), 1);
    room_destroy(r);
}
END_TEST

/* room_create with NULL name must not crash */
START_TEST(test_room_create_null_name)
{
    Room *r = room_create(1, NULL, 4, 4);
    ck_assert_ptr_nonnull(r);
    room_destroy(r);
}
END_TEST

/* room_get_id returns the id set at creation */
START_TEST(test_room_get_id)
{
    Room *r = room_create(42, "X", 5, 5);
    ck_assert_int_eq(room_get_id(r), 42);
    room_destroy(r);
}
END_TEST

/* room_get_id on NULL returns -1 */
START_TEST(test_room_get_id_null)
{
    ck_assert_int_eq(room_get_id(NULL), -1);
}
END_TEST

/* room_get_width/height on NULL return 0 */
START_TEST(test_room_get_dimensions_null)
{
    ck_assert_int_eq(room_get_width(NULL), 0);
    ck_assert_int_eq(room_get_height(NULL), 0);
}
END_TEST

/* ============================================================
   Walkability
============================================================ */

START_TEST(test_room_walkable_out_of_bounds)
{
    Room *r = room_create(0, "A", 5, 5);
    ck_assert(!room_is_walkable(r, -1, 0));
    ck_assert(!room_is_walkable(r, 100, 100));
    room_destroy(r);
}
END_TEST

/* interior tiles are walkable when no floor_grid is set */
START_TEST(test_room_walkable_interior)
{
    Room *r = room_create(0, "A", 5, 5);
    ck_assert(room_is_walkable(r, 2, 2));
    room_destroy(r);
}
END_TEST

/* border tiles are walls when no floor_grid is set */
START_TEST(test_room_walkable_border_is_wall)
{
    Room *r = room_create(0, "A", 5, 5);
    ck_assert(!room_is_walkable(r, 0, 0));
    ck_assert(!room_is_walkable(r, 4, 4));
    room_destroy(r);
}
END_TEST

/* a pushable at a tile makes it non-walkable */
START_TEST(test_room_walkable_with_pushable)
{
    Room *r = room_create(0, "A", 5, 5);

    Pushable *pb = malloc(sizeof(Pushable));
    pb->id = 0; pb->name = NULL;
    pb->x = 2; pb->y = 2;
    pb->initial_x = 2; pb->initial_y = 2;
    r->pushables = pb;
    r->pushable_count = 1;

    ck_assert(!room_is_walkable(r, 2, 2));

    /* manually clear so room_destroy doesn't double-free */
    r->pushables = NULL;
    r->pushable_count = 0;
    free(pb->name);
    free(pb);

    room_destroy(r);
}
END_TEST

/* room_is_walkable on NULL returns false */
START_TEST(test_room_walkable_null)
{
    ck_assert(!room_is_walkable(NULL, 1, 1));
}
END_TEST

/* ============================================================
   Treasure
============================================================ */

START_TEST(test_room_place_treasure_basic)
{
    Room *r = room_create(0, "A", 5, 5);

    Treasure t;
    t.id = 1; t.x = 2; t.y = 2; t.name = NULL; t.collected = false;

    Status s = room_place_treasure(r, &t);
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(room_get_treasure_at(r, 2, 2), 1);

    room_destroy(r);
}
END_TEST

START_TEST(test_room_get_treasure_none)
{
    Room *r = room_create(0, "A", 5, 5);
    ck_assert_int_eq(room_get_treasure_at(r, 2, 2), -1);
    room_destroy(r);
}
END_TEST

/* collected treasure is invisible to get_treasure_at */
START_TEST(test_room_treasure_collected_invisible)
{
    Room *r = room_create(0, "A", 5, 5);
    Treasure t;
    t.id = 2; t.x = 1; t.y = 1; t.name = NULL; t.collected = false;
    room_place_treasure(r, &t);

    /* mark as collected via pick_up */
    Treasure *out = NULL;
    room_pick_up_treasure(r, 2, &out);

    ck_assert_int_eq(room_get_treasure_at(r, 1, 1), -1);
    room_destroy(r);
}
END_TEST

/* room_pick_up_treasure NULL args return INVALID_ARGUMENT */
START_TEST(test_room_pick_up_treasure_null)
{
    ck_assert_int_eq(room_pick_up_treasure(NULL, 0, NULL), INVALID_ARGUMENT);

    Room *r = room_create(0, "A", 5, 5);
    ck_assert_int_eq(room_pick_up_treasure(r, 0, NULL), INVALID_ARGUMENT);
    room_destroy(r);
}
END_TEST

/* picking up nonexistent treasure returns ROOM_NOT_FOUND */
START_TEST(test_room_pick_up_treasure_not_found)
{
    Room *r = room_create(0, "A", 5, 5);
    Treasure *out = NULL;
    Status s = room_pick_up_treasure(r, 999, &out);
    ck_assert_int_eq(s, ROOM_NOT_FOUND);
    room_destroy(r);
}
END_TEST

/* picking up already-collected treasure returns INVALID_ARGUMENT */
START_TEST(test_room_pick_up_treasure_already_collected)
{
    Room *r = room_create(0, "A", 5, 5);
    Treasure t;
    t.id = 3; t.x = 2; t.y = 2; t.name = NULL; t.collected = false;
    room_place_treasure(r, &t);

    Treasure *out = NULL;
    room_pick_up_treasure(r, 3, &out);

    /* second pick-up attempt on same id */
    Status s = room_pick_up_treasure(r, 3, &out);
    ck_assert_int_eq(s, INVALID_ARGUMENT);

    room_destroy(r);
}
END_TEST

/* ============================================================
   Portal
============================================================ */

START_TEST(test_room_get_portal_destination_none)
{
    Room *r = room_create(0, "A", 5, 5);
    ck_assert_int_eq(room_get_portal_destination(r, 2, 2), -1);
    room_destroy(r);
}
END_TEST

/* room_get_portal_destination returns destination after set_portals */
START_TEST(test_room_get_portal_destination_found)
{
    Room *r = room_create(0, "A", 10, 10);

    Portal *portals = malloc(sizeof(Portal));
    portals[0].id = 0; portals[0].name = NULL;
    portals[0].x = 3; portals[0].y = 3;
    portals[0].target_room_id = 7;
    portals[0].gated = false; portals[0].required_switch_id = -1;

    room_set_portals(r, portals, 1);
    ck_assert_int_eq(room_get_portal_destination(r, 3, 3), 7);

    room_destroy(r);
}
END_TEST

/* ============================================================
   Classify tile
============================================================ */

/* classify returns FLOOR for interior walkable tile */
START_TEST(test_room_classify_floor)
{
    Room *r = room_create(0, "A", 5, 5);
    RoomTileType type = room_classify_tile(r, 2, 2, NULL);
    ck_assert_int_eq(type, ROOM_TILE_FLOOR);
    room_destroy(r);
}
END_TEST

/* classify returns WALL for border tile */
START_TEST(test_room_classify_wall)
{
    Room *r = room_create(0, "A", 5, 5);
    RoomTileType type = room_classify_tile(r, 0, 0, NULL);
    ck_assert_int_eq(type, ROOM_TILE_WALL);
    room_destroy(r);
}
END_TEST

/* classify returns TREASURE for uncollected treasure tile */
START_TEST(test_room_classify_treasure)
{
    Room *r = room_create(0, "A", 5, 5);
    Treasure t;
    t.id = 1; t.x = 2; t.y = 2; t.name = NULL; t.collected = false;
    room_place_treasure(r, &t);

    int out_id = -1;
    RoomTileType type = room_classify_tile(r, 2, 2, &out_id);
    ck_assert_int_eq(type, ROOM_TILE_TREASURE);
    ck_assert_int_eq(out_id, 1);

    room_destroy(r);
}
END_TEST

/* classify returns PUSHABLE for pushable tile */
START_TEST(test_room_classify_pushable)
{
    Room *r = room_create(0, "A", 5, 5);

    Pushable *pb = malloc(sizeof(Pushable));
    pb->id = 0; pb->name = NULL;
    pb->x = 2; pb->y = 2;
    pb->initial_x = 2; pb->initial_y = 2;
    r->pushables = pb;
    r->pushable_count = 1;

    RoomTileType type = room_classify_tile(r, 2, 2, NULL);
    ck_assert_int_eq(type, ROOM_TILE_PUSHABLE);

    r->pushables = NULL;
    r->pushable_count = 0;
    free(pb->name);
    free(pb);

    room_destroy(r);
}
END_TEST

/* classify returns INVALID for NULL room */
START_TEST(test_room_classify_null)
{
    RoomTileType type = room_classify_tile(NULL, 0, 0, NULL);
    ck_assert_int_eq(type, ROOM_TILE_INVALID);
}
END_TEST

/* classify returns INVALID for out-of-bounds */
START_TEST(test_room_classify_oob)
{
    Room *r = room_create(0, "A", 5, 5);
    RoomTileType type = room_classify_tile(r, -1, -1, NULL);
    ck_assert_int_eq(type, ROOM_TILE_INVALID);
    room_destroy(r);
}
END_TEST

/* ============================================================
   Pushables
============================================================ */

/* room_has_pushable_at returns true when pushable present */
START_TEST(test_room_has_pushable_at_true)
{
    Room *r = room_create(0, "A", 7, 7);

    Pushable *pb = malloc(sizeof(Pushable));
    pb->id = 0; pb->name = NULL;
    pb->x = 3; pb->y = 3;
    pb->initial_x = 3; pb->initial_y = 3;
    r->pushables = pb;
    r->pushable_count = 1;

    int idx = -1;
    ck_assert(room_has_pushable_at(r, 3, 3, &idx));
    ck_assert_int_eq(idx, 0);

    r->pushables = NULL;
    r->pushable_count = 0;
    free(pb->name);
    free(pb);
    room_destroy(r);
}
END_TEST

/* room_has_pushable_at returns false when no pushable */
START_TEST(test_room_has_pushable_at_false)
{
    Room *r = room_create(0, "A", 5, 5);
    ck_assert(!room_has_pushable_at(r, 2, 2, NULL));
    room_destroy(r);
}
END_TEST

/* room_has_pushable_at returns false on NULL room */
START_TEST(test_room_has_pushable_null)
{
    ck_assert(!room_has_pushable_at(NULL, 0, 0, NULL));
}
END_TEST

/* room_try_push invalid index returns INVALID_ARGUMENT */
START_TEST(test_room_try_push_invalid_index)
{
    Room *r = room_create(0, "A", 7, 7);
    Status s = room_try_push(r, -1, DIR_NORTH);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
    room_destroy(r);
}
END_TEST

/* room_try_push invalid direction returns INVALID_ARGUMENT */
START_TEST(test_room_try_push_invalid_direction)
{
    Room *r = room_create(0, "A", 7, 7);

    Pushable *pb = malloc(sizeof(Pushable));
    pb->id = 0; pb->name = NULL;
    pb->x = 3; pb->y = 3;
    pb->initial_x = 3; pb->initial_y = 3;
    r->pushables = pb;
    r->pushable_count = 1;

    /* 99 is not a valid Direction enum value */
    Status s = room_try_push(r, 0, (Direction)99);
    ck_assert_int_eq(s, INVALID_ARGUMENT);

    r->pushables = NULL;
    r->pushable_count = 0;
    free(pb->name);
    free(pb);
    room_destroy(r);
}
END_TEST

/* room_try_push NULL room returns INVALID_ARGUMENT */
START_TEST(test_room_try_push_null)
{
    Status s = room_try_push(NULL, 0, DIR_NORTH);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

/* room_try_push moves a pushable into a free tile */
START_TEST(test_room_try_push_success)
{
    Room *r = room_create(0, "A", 7, 7);

    Pushable *pb = malloc(sizeof(Pushable));
    pb->id = 0; pb->name = NULL;
    pb->x = 3; pb->y = 3;
    pb->initial_x = 3; pb->initial_y = 3;
    r->pushables = pb;
    r->pushable_count = 1;

    Status s = room_try_push(r, 0, DIR_SOUTH);
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(r->pushables[0].y, 4);

    r->pushables = NULL;
    r->pushable_count = 0;
    free(pb->name);
    free(pb);
    room_destroy(r);
}
END_TEST

/* pushing into a wall returns ROOM_IMPASSABLE */
START_TEST(test_room_try_push_blocked_by_wall)
{
    Room *r = room_create(0, "A", 5, 5);

    Pushable *pb = malloc(sizeof(Pushable));
    pb->id = 0; pb->name = NULL;
    /* one step from north border — pushing north hits wall */
    pb->x = 2; pb->y = 1;
    pb->initial_x = 2; pb->initial_y = 1;
    r->pushables = pb;
    r->pushable_count = 1;

    Status s = room_try_push(r, 0, DIR_NORTH);
    ck_assert_int_eq(s, ROOM_IMPASSABLE);

    r->pushables = NULL;
    r->pushable_count = 0;
    free(pb->name);
    free(pb);
    room_destroy(r);
}
END_TEST

/* ============================================================
   Render Validation
============================================================ */

START_TEST(test_room_render_dimension_mismatch)
{
    Room *r = room_create(0, "A", 3, 3);
    Charset c = { '#', '.', '@', 'O', '$', 'X', '+', '!' };
    char buffer[9];

    ck_assert_int_eq(room_render(r, &c, buffer, 2, 3), INVALID_ARGUMENT);
    ck_assert_int_eq(room_render(r, &c, buffer, 3, 2), INVALID_ARGUMENT);

    room_destroy(r);
}
END_TEST

START_TEST(test_room_render_with_floor_grid)
{
    Room *r = room_create(0, "GridRoom", 3, 3);
    ck_assert_ptr_nonnull(r);

    bool *grid = malloc(sizeof(bool) * 9);
    for (int i = 0; i < 9; i++) grid[i] = true;
    grid[1 * 3 + 1] = false;

    ck_assert_int_eq(room_set_floor_grid(r, grid), OK);

    Charset c = {
        .wall = '#', .floor = '.', .player = '@',
        .treasure = '$', .portal = 'O', .pushable = 'P'
    };

    char buffer[9];
    Status s = room_render(r, &c, buffer, 3, 3);
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(buffer[1 * 3 + 1], '#');

    room_destroy(r);
}
END_TEST

START_TEST(test_room_render_collected_treasure_hidden)
{
    Room *r = room_create(0, "TestRoom", 5, 5);

    Treasure *treasures = malloc(sizeof(Treasure));
    treasures[0].id = 1;
    treasures[0].name = strdup("Gold");
    treasures[0].starting_room_id = 0;
    treasures[0].initial_x = 2; treasures[0].initial_y = 2;
    treasures[0].x = 2; treasures[0].y = 2;
    treasures[0].collected = true;

    ck_assert_int_eq(room_set_treasures(r, treasures, 1), OK);

    Charset c = {
        .wall = '#', .floor = '.', .player = '@',
        .treasure = '$', .portal = 'O', .pushable = 'P'
    };

    char buffer[25];
    Status s = room_render(r, &c, buffer, 5, 5);
    ck_assert_int_eq(s, OK);
    ck_assert_int_ne(buffer[2 * 5 + 2], '$');

    room_destroy(r);
}
END_TEST

/* render NULL args return INVALID_ARGUMENT */
START_TEST(test_room_render_null_args)
{
    Room *r = room_create(0, "A", 3, 3);
    Charset c = { '#', '.', '@', 'O', '$', 'X', '+', '!' };
    char buffer[9];

    ck_assert_int_eq(room_render(NULL, &c, buffer, 3, 3), INVALID_ARGUMENT);
    ck_assert_int_eq(room_render(r, NULL, buffer, 3, 3), INVALID_ARGUMENT);
    ck_assert_int_eq(room_render(r, &c, NULL, 3, 3), INVALID_ARGUMENT);

    room_destroy(r);
}
END_TEST

/* ============================================================
   Start Position
============================================================ */

/* room_get_start_position uses first portal when available */
START_TEST(test_room_get_start_position_portal)
{
    Room *r = room_create(0, "A", 10, 10);

    Portal *portals = malloc(sizeof(Portal));
    portals[0].id = 0; portals[0].name = NULL;
    portals[0].x = 5; portals[0].y = 5;
    portals[0].target_room_id = 1;
    portals[0].gated = false; portals[0].required_switch_id = -1;

    room_set_portals(r, portals, 1);

    int x = 0, y = 0;
    Status s = room_get_start_position(r, &x, &y);
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(x, 5);
    ck_assert_int_eq(y, 5);

    room_destroy(r);
}
END_TEST

/* room_get_start_position NULL args return INVALID_ARGUMENT */
START_TEST(test_room_get_start_position_null)
{
    int x, y;
    ck_assert_int_eq(room_get_start_position(NULL, &x, &y), INVALID_ARGUMENT);

    Room *r = room_create(0, "A", 5, 5);
    ck_assert_int_eq(room_get_start_position(r, NULL, &y), INVALID_ARGUMENT);
    room_destroy(r);
}
END_TEST


Suite *room_suite(void)
{
    Suite *s = suite_create("Room");
    TCase *tc = tcase_create("Core");

    tcase_add_test(tc, test_room_create_basic);
    tcase_add_test(tc, test_room_create_clamps_dimensions);
    tcase_add_test(tc, test_room_create_null_name);
    tcase_add_test(tc, test_room_get_id);
    tcase_add_test(tc, test_room_get_id_null);
    tcase_add_test(tc, test_room_get_dimensions_null);
    tcase_add_test(tc, test_room_walkable_out_of_bounds);
    tcase_add_test(tc, test_room_walkable_interior);
    tcase_add_test(tc, test_room_walkable_border_is_wall);
    tcase_add_test(tc, test_room_walkable_with_pushable);
    tcase_add_test(tc, test_room_walkable_null);
    tcase_add_test(tc, test_room_place_treasure_basic);
    tcase_add_test(tc, test_room_get_treasure_none);
    tcase_add_test(tc, test_room_treasure_collected_invisible);
    tcase_add_test(tc, test_room_pick_up_treasure_null);
    tcase_add_test(tc, test_room_pick_up_treasure_not_found);
    tcase_add_test(tc, test_room_pick_up_treasure_already_collected);
    tcase_add_test(tc, test_room_get_portal_destination_none);
    tcase_add_test(tc, test_room_get_portal_destination_found);
    tcase_add_test(tc, test_room_classify_floor);
    tcase_add_test(tc, test_room_classify_wall);
    tcase_add_test(tc, test_room_classify_treasure);
    tcase_add_test(tc, test_room_classify_pushable);
    tcase_add_test(tc, test_room_classify_null);
    tcase_add_test(tc, test_room_classify_oob);
    tcase_add_test(tc, test_room_has_pushable_at_true);
    tcase_add_test(tc, test_room_has_pushable_at_false);
    tcase_add_test(tc, test_room_has_pushable_null);
    tcase_add_test(tc, test_room_try_push_invalid_index);
    tcase_add_test(tc, test_room_try_push_invalid_direction);
    tcase_add_test(tc, test_room_try_push_null);
    tcase_add_test(tc, test_room_try_push_success);
    tcase_add_test(tc, test_room_try_push_blocked_by_wall);
    tcase_add_test(tc, test_room_render_dimension_mismatch);
    tcase_add_test(tc, test_room_render_with_floor_grid);
    tcase_add_test(tc, test_room_render_collected_treasure_hidden);
    tcase_add_test(tc, test_room_render_null_args);
    tcase_add_test(tc, test_room_get_start_position_portal);
    tcase_add_test(tc, test_room_get_start_position_null);

    suite_add_tcase(s, tc);
    return s;
}
