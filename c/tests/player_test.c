#include <check.h>
#include <stdlib.h>
#include "player.h"

/* ============================================================
   Basic Functionality
============================================================ */

START_TEST(test_player_create_basic)
{
    Player *p = NULL;
    Status s = player_create(1, 2, 3, &p);

    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(p);
    ck_assert_int_eq(player_get_room(p), 1);

    int x = 0, y = 0;
    player_get_position(p, &x, &y);
    ck_assert_int_eq(x, 2);
    ck_assert_int_eq(y, 3);

    player_destroy(p);
}
END_TEST

/* ============================================================
   NULL / Error Cases
============================================================ */

START_TEST(test_player_create_null_out)
{
    Status s = player_create(0, 0, 0, NULL);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_player_get_room_null)
{
    ck_assert_int_eq(player_get_room(NULL), -1);
}
END_TEST

START_TEST(test_player_get_position_null_player)
{
    int x, y;
    Status s = player_get_position(NULL, &x, &y);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_player_set_position_null)
{
    Status s = player_set_position(NULL, 5, 5);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_player_move_to_room_null)
{
    Status s = player_move_to_room(NULL, 5);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

START_TEST(test_player_destroy_null_safe)
{
    /* must not crash */
    player_destroy(NULL);
}
END_TEST

/* ============================================================
   Reset State
============================================================ */

START_TEST(test_player_reset)
{
    Player *p = NULL;
    player_create(5, 6, 7, &p);

    player_reset_to_start(p, 1, 2, 3);

    ck_assert_int_eq(player_get_room(p), 1);

    int x = 0, y = 0;
    player_get_position(p, &x, &y);
    ck_assert_int_eq(x, 2);
    ck_assert_int_eq(y, 3);

    player_destroy(p);
}
END_TEST

/* ============================================================
   A2: Treasure Collection
============================================================ */

/* player starts with zero collected treasures */
START_TEST(test_player_collected_count_initial)
{
    Player *p = NULL;
    player_create(0, 0, 0, &p);
    ck_assert_int_eq(player_get_collected_count(p), 0);
    player_destroy(p);
}
END_TEST

/* collecting a treasure increments count */
START_TEST(test_player_try_collect_increments_count)
{
    Player *p = NULL;
    player_create(0, 0, 0, &p);

    Treasure t;
    t.id = 10;
    t.name = NULL;
    t.collected = false;

    Status s = player_try_collect(p, &t);
    ck_assert_int_eq(s, OK);
    ck_assert_int_eq(player_get_collected_count(p), 1);

    player_destroy(p);
}
END_TEST

/* collecting marks treasure as collected */
START_TEST(test_player_try_collect_marks_treasure)
{
    Player *p = NULL;
    player_create(0, 0, 0, &p);

    Treasure t;
    t.id = 5;
    t.name = NULL;
    t.collected = false;

    player_try_collect(p, &t);
    ck_assert(t.collected);

    player_destroy(p);
}
END_TEST

/* collecting already-collected treasure returns INVALID_ARGUMENT */
START_TEST(test_player_try_collect_already_collected)
{
    Player *p = NULL;
    player_create(0, 0, 0, &p);

    Treasure t;
    t.id = 3;
    t.name = NULL;
    t.collected = true;

    Status s = player_try_collect(p, &t);
    ck_assert_int_eq(s, INVALID_ARGUMENT);

    player_destroy(p);
}
END_TEST

/* NULL player or treasure returns NULL_POINTER */
START_TEST(test_player_try_collect_null)
{
    ck_assert_int_eq(player_try_collect(NULL, NULL), NULL_POINTER);

    Player *p = NULL;
    player_create(0, 0, 0, &p);
    ck_assert_int_eq(player_try_collect(p, NULL), NULL_POINTER);
    player_destroy(p);
}
END_TEST

/* has_collected_treasure returns true after collection */
START_TEST(test_player_has_collected_treasure_true)
{
    Player *p = NULL;
    player_create(0, 0, 0, &p);

    Treasure t;
    t.id = 7;
    t.name = NULL;
    t.collected = false;

    player_try_collect(p, &t);
    ck_assert(player_has_collected_treasure(p, 7));

    player_destroy(p);
}
END_TEST

/* has_collected_treasure returns false for uncollected ID */
START_TEST(test_player_has_collected_treasure_false)
{
    Player *p = NULL;
    player_create(0, 0, 0, &p);
    ck_assert(!player_has_collected_treasure(p, 999));
    player_destroy(p);
}
END_TEST

/* has_collected_treasure returns false on NULL player */
START_TEST(test_player_has_collected_treasure_null)
{
    ck_assert(!player_has_collected_treasure(NULL, 1));
}
END_TEST

/* get_collected_count returns 0 on NULL player */
START_TEST(test_player_get_collected_count_null)
{
    ck_assert_int_eq(player_get_collected_count(NULL), 0);
}
END_TEST

/* get_collected_treasures returns NULL on NULL player */
START_TEST(test_player_get_collected_treasures_null)
{
    int count = 99;
    const Treasure * const *arr = player_get_collected_treasures(NULL, &count);
    ck_assert_ptr_null(arr);
}
END_TEST

/* get_collected_treasures returns array with correct count */
START_TEST(test_player_get_collected_treasures_array)
{
    Player *p = NULL;
    player_create(0, 0, 0, &p);

    Treasure t1;
    t1.id = 1; t1.name = NULL; t1.collected = false;
    Treasure t2;
    t2.id = 2; t2.name = NULL; t2.collected = false;

    player_try_collect(p, &t1);
    player_try_collect(p, &t2);

    int count = 0;
    const Treasure * const *arr = player_get_collected_treasures(p, &count);
    ck_assert_ptr_nonnull(arr);
    ck_assert_int_eq(count, 2);

    player_destroy(p);
}
END_TEST

/* reset clears collected treasure array */
START_TEST(test_player_reset_clears_treasures)
{
    Player *p = NULL;
    player_create(0, 0, 0, &p);

    Treasure t;
    t.id = 9; t.name = NULL; t.collected = false;
    player_try_collect(p, &t);
    ck_assert_int_eq(player_get_collected_count(p), 1);

    player_reset_to_start(p, 0, 0, 0);
    ck_assert_int_eq(player_get_collected_count(p), 0);

    player_destroy(p);
}
END_TEST

/* reset on NULL returns INVALID_ARGUMENT */
START_TEST(test_player_reset_null)
{
    Status s = player_reset_to_start(NULL, 0, 0, 0);
    ck_assert_int_eq(s, INVALID_ARGUMENT);
}
END_TEST

/* set_position and get_position round-trip */
START_TEST(test_player_set_get_position)
{
    Player *p = NULL;
    player_create(0, 0, 0, &p);

    player_set_position(p, 10, 20);

    int x = 0, y = 0;
    player_get_position(p, &x, &y);
    ck_assert_int_eq(x, 10);
    ck_assert_int_eq(y, 20);

    player_destroy(p);
}
END_TEST

/* move_to_room updates room_id */
START_TEST(test_player_move_to_room)
{
    Player *p = NULL;
    player_create(1, 0, 0, &p);

    player_move_to_room(p, 42);
    ck_assert_int_eq(player_get_room(p), 42);

    player_destroy(p);
}
END_TEST


Suite *player_suite(void)
{
    Suite *s = suite_create("Player");
    TCase *tc = tcase_create("Core");

    tcase_add_test(tc, test_player_create_basic);
    tcase_add_test(tc, test_player_create_null_out);
    tcase_add_test(tc, test_player_get_room_null);
    tcase_add_test(tc, test_player_get_position_null_player);
    tcase_add_test(tc, test_player_set_position_null);
    tcase_add_test(tc, test_player_move_to_room_null);
    tcase_add_test(tc, test_player_destroy_null_safe);
    tcase_add_test(tc, test_player_reset);
    tcase_add_test(tc, test_player_collected_count_initial);
    tcase_add_test(tc, test_player_try_collect_increments_count);
    tcase_add_test(tc, test_player_try_collect_marks_treasure);
    tcase_add_test(tc, test_player_try_collect_already_collected);
    tcase_add_test(tc, test_player_try_collect_null);
    tcase_add_test(tc, test_player_has_collected_treasure_true);
    tcase_add_test(tc, test_player_has_collected_treasure_false);
    tcase_add_test(tc, test_player_has_collected_treasure_null);
    tcase_add_test(tc, test_player_get_collected_count_null);
    tcase_add_test(tc, test_player_get_collected_treasures_null);
    tcase_add_test(tc, test_player_get_collected_treasures_array);
    tcase_add_test(tc, test_player_reset_clears_treasures);
    tcase_add_test(tc, test_player_reset_null);
    tcase_add_test(tc, test_player_set_get_position);
    tcase_add_test(tc, test_player_move_to_room);

    suite_add_tcase(s, tc);
    return s;
}
