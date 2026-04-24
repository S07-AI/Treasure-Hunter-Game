#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "player.h"
#include "game_engine.h"
#include "room.h"

static GameEngine *engine = NULL;

static void setup_engine(void)
{
    Status s = game_engine_create("../assets/starter.ini", &engine);
    ck_assert_int_eq(s, OK);
}

static void teardown_engine(void)
{
    game_engine_destroy(engine);
    engine = NULL;
}

/* ============================================================
   Creation / Destruction
============================================================ */

START_TEST(test_engine_create)
{
    ck_assert_ptr_nonnull(engine);
}
END_TEST

START_TEST(test_engine_create_null_path)
{
    GameEngine *e = NULL;
    Status s = game_engine_create(NULL, &e);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_engine_create_null_out)
{
    Status s = game_engine_create("../assets/starter.ini", NULL);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_engine_create_bad_path)
{
    GameEngine *e = NULL;
    Status s = game_engine_create("/nonexistent/path.ini", &e);
    ck_assert_int_ne(s, OK);
}
END_TEST

START_TEST(test_engine_destroy_null_safe)
{
    /* must not crash */
    game_engine_destroy(NULL);
}
END_TEST

/* ============================================================
   Player Access
============================================================ */

START_TEST(test_engine_get_player)
{
    const Player *p = game_engine_get_player(engine);
    ck_assert_ptr_nonnull(p);
}
END_TEST

START_TEST(test_engine_get_player_null)
{
    ck_assert_ptr_null(game_engine_get_player(NULL));
}
END_TEST

/* ============================================================
   Room Count
============================================================ */

START_TEST(test_engine_get_room_count)
{
    int count = 0;
    Status s = game_engine_get_room_count(engine, &count);
    ck_assert_int_eq(s, OK);
    ck_assert_int_gt(count, 0);
}
END_TEST

START_TEST(test_engine_get_room_count_null_engine)
{
    int count;
    Status s = game_engine_get_room_count(NULL, &count);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_engine_get_room_count_null_out)
{
    Status s = game_engine_get_room_count(engine, NULL);
    ck_assert_int_eq(s, NULL_POINTER);
}
END_TEST

/* ============================================================
   Room Dimensions
============================================================ */

START_TEST(test_engine_get_room_dimensions)
{
    int w = 0, h = 0;
    Status s = game_engine_get_room_dimensions(engine, &w, &h);
    ck_assert_int_eq(s, OK);
    ck_assert_int_gt(w, 0);
    ck_assert_int_gt(h, 0);
}
END_TEST

START_TEST(test_engine_get_room_dimensions_null_engine)
{
    int w, h;
    Status s = game_engine_get_room_dimensions(NULL, &w, &h);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_engine_get_room_dimensions_null_out)
{
    int w = 0;
    Status s = game_engine_get_room_dimensions(engine, NULL, &w);
    ck_assert_int_eq(s, NULL_POINTER);
    Status s2 = game_engine_get_room_dimensions(engine, &w, NULL);
    ck_assert_int_eq(s2, NULL_POINTER);
}
END_TEST

/* ============================================================
   Room IDs
============================================================ */

START_TEST(test_engine_get_room_ids)
{
    int *ids = NULL;
    int count = 0;
    Status s = game_engine_get_room_ids(engine, &ids, &count);
    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(ids);
    ck_assert_int_gt(count, 0);
    game_engine_free_string(ids);
}
END_TEST

START_TEST(test_engine_get_room_ids_null)
{
    int *ids = NULL;
    int count = 0;
    Status s = game_engine_get_room_ids(NULL, &ids, &count);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

/* ============================================================
   Room By ID
============================================================ */

START_TEST(test_engine_get_room_ids_nonempty)
{
    /* The world must have at least one room with a valid id */
    int *ids = NULL;
    int count = 0;
    Status s = game_engine_get_room_ids(engine, &ids, &count);
    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(ids);
    ck_assert_int_gt(count, 0);
    game_engine_free_string(ids);
}
END_TEST

START_TEST(test_engine_get_room_ids_null_out)
{
    Status s = game_engine_get_room_ids(engine, NULL, NULL);
    ck_assert_int_eq(s, NULL_POINTER);
}
END_TEST

/* ============================================================
   Move Player
============================================================ */

START_TEST(test_engine_move_player)
{
    Status s = game_engine_move_player(engine, DIR_EAST);
    ck_assert(s == OK || s == ROOM_IMPASSABLE);
}
END_TEST

START_TEST(test_engine_move_null)
{
    Status s = game_engine_move_player(NULL, DIR_NORTH);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_engine_move_all_directions)
{
    Direction dirs[] = { DIR_NORTH, DIR_SOUTH, DIR_EAST, DIR_WEST };
    for (int i = 0; i < 4; i++) {
        Status s = game_engine_move_player(engine, dirs[i]);
        ck_assert(s == OK || s == ROOM_IMPASSABLE || s == GE_NO_SUCH_ROOM);
        game_engine_reset(engine);
    }
}
END_TEST

/* ============================================================
   Rendering
============================================================ */

START_TEST(test_engine_render_current_room)
{
    char *out = NULL;
    Status s = game_engine_render_current_room(engine, &out);
    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(out);
    ck_assert_ptr_nonnull(strchr(out, '\n'));
    game_engine_free_string(out);
}
END_TEST

START_TEST(test_engine_render_invalid_room)
{
    char *out = NULL;
    Status s = game_engine_render_room(engine, 999, &out);
    ck_assert_int_eq(s, GE_NO_SUCH_ROOM);
}
END_TEST

START_TEST(test_engine_render_null_out)
{
    Status s = game_engine_render_current_room(engine, NULL);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_engine_render_null_engine)
{
    char *out = NULL;
    Status s = game_engine_render_current_room(NULL, &out);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_engine_render_contains_player)
{
    char *out = NULL;
    game_engine_render_current_room(engine, &out);
    ck_assert_ptr_nonnull(strchr(out, '@'));
    game_engine_free_string(out);
}
END_TEST

/* ============================================================
   Reset
============================================================ */

START_TEST(test_engine_reset)
{
    int x0 = 0, y0 = 0;
    player_get_position(engine->player, &x0, &y0);

    game_engine_move_player(engine, DIR_SOUTH);
    game_engine_reset(engine);

    int x1 = 0, y1 = 0;
    player_get_position(engine->player, &x1, &y1);
    ck_assert_int_eq(x1, x0);
    ck_assert_int_eq(y1, y0);
}
END_TEST

START_TEST(test_engine_reset_null)
{
    Status s = game_engine_reset(NULL);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_engine_reset_restores_room)
{
    int room_before = player_get_room(engine->player);
    game_engine_move_player(engine, DIR_NORTH);
    game_engine_move_player(engine, DIR_NORTH);
    game_engine_reset(engine);
    int room_after = player_get_room(engine->player);
    ck_assert_int_eq(room_before, room_after);
}
END_TEST

/* ============================================================
   Free String
============================================================ */

START_TEST(test_engine_free_string_null)
{
    /* must not crash */
    game_engine_free_string(NULL);
}
END_TEST


Suite *engine_suite(void)
{
    Suite *s = suite_create("GameEngine");
    TCase *tc = tcase_create("Core");

    tcase_add_checked_fixture(tc, setup_engine, teardown_engine);

    tcase_add_test(tc, test_engine_create);
    tcase_add_test(tc, test_engine_create_null_path);
    tcase_add_test(tc, test_engine_create_null_out);
    tcase_add_test(tc, test_engine_create_bad_path);
    tcase_add_test(tc, test_engine_destroy_null_safe);
    tcase_add_test(tc, test_engine_get_player);
    tcase_add_test(tc, test_engine_get_player_null);
    tcase_add_test(tc, test_engine_get_room_count);
    tcase_add_test(tc, test_engine_get_room_count_null_engine);
    tcase_add_test(tc, test_engine_get_room_count_null_out);
    tcase_add_test(tc, test_engine_get_room_dimensions);
    tcase_add_test(tc, test_engine_get_room_dimensions_null_engine);
    tcase_add_test(tc, test_engine_get_room_dimensions_null_out);
    tcase_add_test(tc, test_engine_get_room_ids);
    tcase_add_test(tc, test_engine_get_room_ids_null);
    tcase_add_test(tc, test_engine_get_room_ids_nonempty);
    tcase_add_test(tc, test_engine_get_room_ids_null_out);
    tcase_add_test(tc, test_engine_move_player);
    tcase_add_test(tc, test_engine_move_null);
    tcase_add_test(tc, test_engine_move_all_directions);
    tcase_add_test(tc, test_engine_render_current_room);
    tcase_add_test(tc, test_engine_render_invalid_room);
    tcase_add_test(tc, test_engine_render_null_out);
    tcase_add_test(tc, test_engine_render_null_engine);
    tcase_add_test(tc, test_engine_render_contains_player);
    tcase_add_test(tc, test_engine_reset);
    tcase_add_test(tc, test_engine_reset_null);
    tcase_add_test(tc, test_engine_reset_restores_room);
    tcase_add_test(tc, test_engine_free_string_null);

    suite_add_tcase(s, tc);
    return s;
}