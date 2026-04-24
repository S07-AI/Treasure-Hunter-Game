#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"

/* ============================================================
 * Helper Functions - Integer payloads for testing
 * ============================================================ */

static int compare_ints(const void *a, const void *b) {
    const int *ia = a;
    const int *ib = b;
    if (*ia < *ib) return -1;
    if (*ia > *ib) return 1;
    return 0;
}

static void destroy_int(void *p) {
    free(p);
}

static int *create_int(int value) {
    int *p = malloc(sizeof(int));
    if (p) *p = value;
    return p;
}

/* ============================================================
 * Setup and Teardown fixtures
 * ============================================================ */

static Graph *graph = NULL;

static void setup_graph(void)
{
    Graph *temp = NULL;
    GraphStatus status = graph_create(compare_ints, destroy_int, &temp);
    ck_assert_int_eq(status, GRAPH_STATUS_OK);
    ck_assert_ptr_nonnull(temp);
    graph = temp;
}

static void teardown_graph(void)
{
    graph_destroy(graph);
    graph = NULL;
}

/* ============================================================
 * Test Cases - Creation & Destruction
 * ============================================================ */

START_TEST(test_graph_create_success)
{
    Graph *g = NULL;
    GraphStatus status = graph_create(compare_ints, destroy_int, &g);
    
    ck_assert_int_eq(status, GRAPH_STATUS_OK);
    ck_assert_ptr_nonnull(g);
    ck_assert_int_eq(graph_size(g), 0);
    
    graph_destroy(g);
}
END_TEST

START_TEST(test_graph_create_null_compare)
{
    Graph *g = NULL;
    GraphStatus status = graph_create(NULL, destroy_int, &g);
    ck_assert_int_eq(status, GRAPH_STATUS_NULL_ARGUMENT);
}
END_TEST

START_TEST(test_graph_create_null_out)
{
    GraphStatus status = graph_create(compare_ints, destroy_int, NULL);
    ck_assert_int_eq(status, GRAPH_STATUS_NULL_ARGUMENT);
}
END_TEST

START_TEST(test_graph_create_null_destroy)
{
    Graph *g = NULL;
    GraphStatus status = graph_create(compare_ints, NULL, &g);
    
    ck_assert_int_eq(status, GRAPH_STATUS_OK);
    ck_assert_ptr_nonnull(g);
    
    graph_destroy(g);
}
END_TEST

START_TEST(test_graph_destroy_null_safe)
{
    graph_destroy(NULL);  // Should not crash
}
END_TEST

/* ============================================================
 * Test Cases - Payload Insertion
 * ============================================================ */

START_TEST(test_graph_insert_single)
{
    int *value = create_int(42);
    GraphStatus status = graph_insert(graph, value);
    
    ck_assert_int_eq(status, GRAPH_STATUS_OK);
    ck_assert_int_eq(graph_size(graph), 1);
    ck_assert(graph_contains(graph, value));
}
END_TEST

START_TEST(test_graph_insert_multiple)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    int *v3 = create_int(30);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    GraphStatus status = graph_insert(graph, v3);
    
    ck_assert_int_eq(status, GRAPH_STATUS_OK);
    ck_assert_int_eq(graph_size(graph), 3);
}
END_TEST

START_TEST(test_graph_insert_duplicate)
{
    int *v1 = create_int(42);
    int *v2 = create_int(42);
    
    graph_insert(graph, v1);
    GraphStatus status = graph_insert(graph, v2);
    
    ck_assert_int_eq(status, GRAPH_STATUS_DUPLICATE_PAYLOAD);
    ck_assert_int_eq(graph_size(graph), 1);
    
    free(v2);  // Since it wasn't inserted
}
END_TEST

START_TEST(test_graph_insert_null_graph)
{
    int *value = create_int(42);
    GraphStatus status = graph_insert(NULL, value);
    
    ck_assert_int_eq(status, GRAPH_STATUS_NULL_ARGUMENT);
    free(value);
}
END_TEST

START_TEST(test_graph_insert_null_payload)
{
    GraphStatus status = graph_insert(graph, NULL);
    ck_assert_int_eq(status, GRAPH_STATUS_NULL_ARGUMENT);
}
END_TEST

/* ============================================================
 * Test Cases - Edge Creation
 * ============================================================ */

START_TEST(test_graph_connect_success)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    
    GraphStatus status = graph_connect(graph, v1, v2);
    
    ck_assert_int_eq(status, GRAPH_STATUS_OK);
    ck_assert(graph_has_edge(graph, v1, v2));
    ck_assert_int_eq(graph_edge_count(graph), 1);
}
END_TEST

START_TEST(test_graph_connect_duplicate_edge)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    graph_connect(graph, v1, v2);
    
    GraphStatus status = graph_connect(graph, v1, v2);
    ck_assert_int_eq(status, GRAPH_STATUS_DUPLICATE_EDGE);
}
END_TEST

START_TEST(test_graph_connect_not_found)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    int *v3 = create_int(30);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    
    GraphStatus status = graph_connect(graph, v1, v3);
    ck_assert_int_eq(status, GRAPH_STATUS_NOT_FOUND);
    
    free(v3);
}
END_TEST

START_TEST(test_graph_connect_null_graph)
{
    int a = 10, b = 20;
    GraphStatus status = graph_connect(NULL, &a, &b);
    ck_assert_int_eq(status, GRAPH_STATUS_NULL_ARGUMENT);
}
END_TEST

/* ============================================================
 * Test Cases - Graph Queries
 * ============================================================ */

START_TEST(test_graph_size_empty)
{
    int size = graph_size(graph);
    ck_assert_int_eq(size, 0);
}
END_TEST

START_TEST(test_graph_size_null)
{
    int size = graph_size(NULL);
    ck_assert_int_eq(size, 0);
}
END_TEST

START_TEST(test_graph_contains_found)
{
    int *value = create_int(42);
    graph_insert(graph, value);
    
    ck_assert(graph_contains(graph, value));
}
END_TEST

START_TEST(test_graph_contains_not_found)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    
    graph_insert(graph, v1);
    
    ck_assert(!graph_contains(graph, v2));
    free(v2);
}
END_TEST

START_TEST(test_graph_contains_null)
{
    ck_assert(!graph_contains(NULL, NULL));
}
END_TEST

START_TEST(test_graph_get_payload_found)
{
    int *value = create_int(42);
    graph_insert(graph, value);
    
    int key = 42;
    const void *result = graph_get_payload(graph, &key);
    
    ck_assert_ptr_eq(result, value);
}
END_TEST

START_TEST(test_graph_get_payload_not_found)
{
    int *value = create_int(42);
    graph_insert(graph, value);
    
    int key = 99;
    const void *result = graph_get_payload(graph, &key);
    
    ck_assert_ptr_null(result);
}
END_TEST

/* ============================================================
 * Test Cases - Degree Queries
 * ============================================================ */

START_TEST(test_graph_outdegree)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    int *v3 = create_int(30);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    graph_insert(graph, v3);
    
    graph_connect(graph, v1, v2);
    graph_connect(graph, v1, v3);
    
    ck_assert_int_eq(graph_outdegree(graph, v1), 2);
    ck_assert_int_eq(graph_outdegree(graph, v2), 0);
}
END_TEST

START_TEST(test_graph_indegree)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    int *v3 = create_int(30);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    graph_insert(graph, v3);
    
    graph_connect(graph, v1, v3);
    graph_connect(graph, v2, v3);
    
    ck_assert_int_eq(graph_indegree(graph, v3), 2);
    ck_assert_int_eq(graph_indegree(graph, v1), 0);
}
END_TEST

START_TEST(test_graph_edge_count)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    int *v3 = create_int(30);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    graph_insert(graph, v3);
    
    ck_assert_int_eq(graph_edge_count(graph), 0);
    
    graph_connect(graph, v1, v2);
    ck_assert_int_eq(graph_edge_count(graph), 1);
    
    graph_connect(graph, v2, v3);
    ck_assert_int_eq(graph_edge_count(graph), 2);
}
END_TEST

START_TEST(test_graph_has_edge_exists)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    graph_connect(graph, v1, v2);
    
    ck_assert(graph_has_edge(graph, v1, v2));
}
END_TEST

START_TEST(test_graph_has_edge_not_exists)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    
    ck_assert(!graph_has_edge(graph, v1, v2));
}
END_TEST

/* ============================================================
 * Test Cases - All Payloads
 * ============================================================ */

START_TEST(test_graph_get_all_payloads)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    int *v3 = create_int(30);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    graph_insert(graph, v3);
    
    const void * const *payloads;
    int count;
    GraphStatus status = graph_get_all_payloads(graph, &payloads, &count);
    
    ck_assert_int_eq(status, GRAPH_STATUS_OK);
    ck_assert_int_eq(count, 3);
    ck_assert_ptr_nonnull(payloads);
}
END_TEST

START_TEST(test_graph_get_all_payloads_empty)
{
    const void * const *payloads;
    int count;
    GraphStatus status = graph_get_all_payloads(graph, &payloads, &count);
    
    ck_assert_int_eq(status, GRAPH_STATUS_OK);
    ck_assert_int_eq(count, 0);
}
END_TEST

START_TEST(test_graph_get_all_payloads_null)
{
    const void * const *payloads;
    int count;
    GraphStatus status = graph_get_all_payloads(NULL, &payloads, &count);
    
    ck_assert_int_eq(status, GRAPH_STATUS_NULL_ARGUMENT);
}
END_TEST

/* ============================================================
 * Test Cases - Reachability
 * ============================================================ */

START_TEST(test_graph_reachable_direct)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    graph_connect(graph, v1, v2);
    
    ck_assert(graph_reachable(graph, v1, v2));
}
END_TEST

START_TEST(test_graph_reachable_indirect)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    int *v3 = create_int(30);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    graph_insert(graph, v3);
    
    graph_connect(graph, v1, v2);
    graph_connect(graph, v2, v3);
    
    ck_assert(graph_reachable(graph, v1, v3));
}
END_TEST

START_TEST(test_graph_reachable_not)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    
    ck_assert(!graph_reachable(graph, v1, v2));
}
END_TEST

/* ============================================================
 * Test Cases - Cycle Detection
 * ============================================================ */

START_TEST(test_graph_has_cycle_true)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    int *v3 = create_int(30);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    graph_insert(graph, v3);
    
    graph_connect(graph, v1, v2);
    graph_connect(graph, v2, v3);
    graph_connect(graph, v3, v1);  // Creates cycle
    
    ck_assert(graph_has_cycle(graph));
}
END_TEST

START_TEST(test_graph_has_cycle_false)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    int *v3 = create_int(30);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    graph_insert(graph, v3);
    
    graph_connect(graph, v1, v2);
    graph_connect(graph, v2, v3);
    
    ck_assert(!graph_has_cycle(graph));
}
END_TEST

/* ============================================================
 * Test Cases - Connectivity
 * ============================================================ */

START_TEST(test_graph_is_connected_true)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    int *v3 = create_int(30);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    graph_insert(graph, v3);
    
    graph_connect(graph, v1, v2);
    graph_connect(graph, v1, v3);
    
    ck_assert(graph_is_connected(graph));
}
END_TEST

START_TEST(test_graph_is_connected_false)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    int *v3 = create_int(30);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    graph_insert(graph, v3);
    
    graph_connect(graph, v1, v2);
    // v3 is not reachable from v1
    
    ck_assert(!graph_is_connected(graph));
}
END_TEST

/* ============================================================
 * Test Cases - Removal
 * ============================================================ */

START_TEST(test_graph_disconnect_success)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    graph_connect(graph, v1, v2);
    
    GraphStatus status = graph_disconnect(graph, v1, v2);
    
    ck_assert_int_eq(status, GRAPH_STATUS_OK);
    ck_assert(!graph_has_edge(graph, v1, v2));
}
END_TEST

START_TEST(test_graph_disconnect_not_found)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    
    GraphStatus status = graph_disconnect(graph, v1, v2);
    ck_assert_int_eq(status, GRAPH_STATUS_NOT_FOUND);
}
END_TEST

START_TEST(test_graph_remove_success)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    
    graph_insert(graph, v1);
    graph_insert(graph, v2);
    graph_connect(graph, v1, v2);
    
    GraphStatus status = graph_remove(graph, v1);
    
    ck_assert_int_eq(status, GRAPH_STATUS_OK);
    ck_assert_int_eq(graph_size(graph), 1);
    ck_assert(!graph_contains(graph, v1));
}
END_TEST

START_TEST(test_graph_remove_not_found)
{
    int *v1 = create_int(10);
    int *v2 = create_int(20);
    
    graph_insert(graph, v1);
    
    GraphStatus status = graph_remove(graph, v2);
    ck_assert_int_eq(status, GRAPH_STATUS_NOT_FOUND);
    
    free(v2);
}
END_TEST

/* ============================================================
 * Suite Creation Function
 * ============================================================ */

Suite *graph_suite(void)
{
    Suite *s = suite_create("Graph");
    
    // Creation & Destruction
    TCase *tc_create = tcase_create("Creation");
    tcase_add_test(tc_create, test_graph_create_success);
    tcase_add_test(tc_create, test_graph_create_null_compare);
    tcase_add_test(tc_create, test_graph_create_null_out);
    tcase_add_test(tc_create, test_graph_create_null_destroy);
    tcase_add_test(tc_create, test_graph_destroy_null_safe);
    suite_add_tcase(s, tc_create);
    
    // Payload Insertion
    TCase *tc_insert = tcase_create("Insertion");
    tcase_add_checked_fixture(tc_insert, setup_graph, teardown_graph);
    tcase_add_test(tc_insert, test_graph_insert_single);
    tcase_add_test(tc_insert, test_graph_insert_multiple);
    tcase_add_test(tc_insert, test_graph_insert_duplicate);
    tcase_add_test(tc_insert, test_graph_insert_null_graph);
    tcase_add_test(tc_insert, test_graph_insert_null_payload);
    suite_add_tcase(s, tc_insert);
    
    // Edge Creation
    TCase *tc_connect = tcase_create("EdgeCreation");
    tcase_add_checked_fixture(tc_connect, setup_graph, teardown_graph);
    tcase_add_test(tc_connect, test_graph_connect_success);
    tcase_add_test(tc_connect, test_graph_connect_duplicate_edge);
    tcase_add_test(tc_connect, test_graph_connect_not_found);
    tcase_add_test(tc_connect, test_graph_connect_null_graph);
    suite_add_tcase(s, tc_connect);
    
    // Graph Queries
    TCase *tc_queries = tcase_create("Queries");
    tcase_add_checked_fixture(tc_queries, setup_graph, teardown_graph);
    tcase_add_test(tc_queries, test_graph_size_empty);
    tcase_add_test(tc_queries, test_graph_size_null);
    tcase_add_test(tc_queries, test_graph_contains_found);
    tcase_add_test(tc_queries, test_graph_contains_not_found);
    tcase_add_test(tc_queries, test_graph_contains_null);
    tcase_add_test(tc_queries, test_graph_get_payload_found);
    tcase_add_test(tc_queries, test_graph_get_payload_not_found);
    suite_add_tcase(s, tc_queries);
    
    // Degree Queries
    TCase *tc_degree = tcase_create("DegreeQueries");
    tcase_add_checked_fixture(tc_degree, setup_graph, teardown_graph);
    tcase_add_test(tc_degree, test_graph_outdegree);
    tcase_add_test(tc_degree, test_graph_indegree);
    tcase_add_test(tc_degree, test_graph_edge_count);
    tcase_add_test(tc_degree, test_graph_has_edge_exists);
    tcase_add_test(tc_degree, test_graph_has_edge_not_exists);
    suite_add_tcase(s, tc_degree);
    
    // All Payloads
    TCase *tc_payloads = tcase_create("AllPayloads");
    tcase_add_checked_fixture(tc_payloads, setup_graph, teardown_graph);
    tcase_add_test(tc_payloads, test_graph_get_all_payloads);
    tcase_add_test(tc_payloads, test_graph_get_all_payloads_empty);
    tcase_add_test(tc_payloads, test_graph_get_all_payloads_null);
    suite_add_tcase(s, tc_payloads);
    
    // Reachability
    TCase *tc_reach = tcase_create("Reachability");
    tcase_add_checked_fixture(tc_reach, setup_graph, teardown_graph);
    tcase_add_test(tc_reach, test_graph_reachable_direct);
    tcase_add_test(tc_reach, test_graph_reachable_indirect);
    tcase_add_test(tc_reach, test_graph_reachable_not);
    suite_add_tcase(s, tc_reach);
    
    // Cycle Detection
    TCase *tc_cycle = tcase_create("CycleDetection");
    tcase_add_checked_fixture(tc_cycle, setup_graph, teardown_graph);
    tcase_add_test(tc_cycle, test_graph_has_cycle_true);
    tcase_add_test(tc_cycle, test_graph_has_cycle_false);
    suite_add_tcase(s, tc_cycle);
    
    // Connectivity
    TCase *tc_connected = tcase_create("Connectivity");
    tcase_add_checked_fixture(tc_connected, setup_graph, teardown_graph);
    tcase_add_test(tc_connected, test_graph_is_connected_true);
    tcase_add_test(tc_connected, test_graph_is_connected_false);
    suite_add_tcase(s, tc_connected);
    
    // Removal
    TCase *tc_remove = tcase_create("Removal");
    tcase_add_checked_fixture(tc_remove, setup_graph, teardown_graph);
    tcase_add_test(tc_remove, test_graph_disconnect_success);
    tcase_add_test(tc_remove, test_graph_disconnect_not_found);
    tcase_add_test(tc_remove, test_graph_remove_success);
    tcase_add_test(tc_remove, test_graph_remove_not_found);
    suite_add_tcase(s, tc_remove);
    
    return s;
}