#include <check.h>
#include "world_loader.h"
#include "room.h"
#include "graph.h"

static Graph *graph = NULL;
static Room *first_room = NULL;
static int count = 0;
static Charset charset;

START_TEST(test_loader_success)
{
    Status s = loader_load_world("../assets/starter.ini",
                                 &graph,
                                 &first_room,
                                 &count,
                                 &charset);

    ck_assert_int_eq(s, OK);
    ck_assert_ptr_nonnull(graph);
    ck_assert_ptr_nonnull(first_room);
    ck_assert_int_gt(count, 0);

    graph_destroy(graph);
    graph = NULL;
}
END_TEST

START_TEST(test_loader_null_config)
{
    Status s = loader_load_world(NULL,
                                 &graph,
                                 &first_room,
                                 &count,
                                 &charset);
    ck_assert_int_ne(s, OK);
}
END_TEST

START_TEST(test_loader_null_graph_out)
{
    Status s = loader_load_world("../assets/starter.ini",
                                 NULL,
                                 &first_room,
                                 &count,
                                 &charset);
    ck_assert_int_ne(s, OK);
}
END_TEST

START_TEST(test_loader_null_first_room_out)
{
    Status s = loader_load_world("../assets/starter.ini",
                                 &graph,
                                 NULL,
                                 &count,
                                 &charset);
    ck_assert_int_ne(s, OK);
    if (graph != NULL) {
        graph_destroy(graph);
        graph = NULL;
    }
}
END_TEST

START_TEST(test_loader_bad_path)
{
    Status s = loader_load_world("/no/such/file.ini",
                                 &graph,
                                 &first_room,
                                 &count,
                                 &charset);
    ck_assert_int_ne(s, OK);
}
END_TEST

START_TEST(test_loader_first_room_valid)
{
    Status s = loader_load_world("../assets/starter.ini",
                                 &graph,
                                 &first_room,
                                 &count,
                                 &charset);
    ck_assert_int_eq(s, OK);
    /* first_room must be a valid room pointer with a non-negative id */
    ck_assert_int_ge(room_get_id(first_room), 0);
    graph_destroy(graph);
    graph = NULL;
}
END_TEST

START_TEST(test_loader_charset_loaded)
{
    Status s = loader_load_world("../assets/starter.ini",
                                 &graph,
                                 &first_room,
                                 &count,
                                 &charset);
    ck_assert_int_eq(s, OK);
    /* charset should have non-zero characters */
    ck_assert_int_ne(charset.wall, 0);
    ck_assert_int_ne(charset.floor, 0);
    graph_destroy(graph);
    graph = NULL;
}
END_TEST


Suite *loader_suite(void)
{
    Suite *s = suite_create("WorldLoader");
    TCase *tc = tcase_create("Core");

    tcase_add_test(tc, test_loader_success);
    tcase_add_test(tc, test_loader_null_config);
    tcase_add_test(tc, test_loader_null_graph_out);
    tcase_add_test(tc, test_loader_null_first_room_out);
    tcase_add_test(tc, test_loader_bad_path);
    tcase_add_test(tc, test_loader_first_room_valid);
    tcase_add_test(tc, test_loader_charset_loaded);

    suite_add_tcase(s, tc);
    return s;
}