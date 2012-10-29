#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <head-unit.h>
#include <glib.h>

#include "contact.h"
#include "prof_autocomplete.h"

static void clear_empty(void)
{
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_clear(ac);
}

static void reset_after_create(void)
{
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_reset(ac);
    p_autocomplete_clear(ac);
}

static void find_after_create(void)
{
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_complete(ac, "hello");
    p_autocomplete_clear(ac);
}

static void get_after_create_returns_null(void)
{
    PAutocomplete ac = p_autocomplete_new();
    GSList *result = p_autocomplete_get_list(ac);

    assert_is_null(result);

    p_autocomplete_clear(ac);
}

static void add_one_and_complete(void)
{
    char *item = strdup("Hello");
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_add(ac, item);
    char *result = p_autocomplete_complete(ac, "Hel");

    assert_string_equals("Hello", result);

    p_autocomplete_clear(ac);
}

static void add_two_and_complete_returns_first(void)
{
    char *item1 = strdup("Hello");
    char *item2 = strdup("Help");
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_add(ac, item1);
    p_autocomplete_add(ac, item2);
    char *result = p_autocomplete_complete(ac, "Hel");

    assert_string_equals("Hello", result);

    p_autocomplete_clear(ac);
}

static void add_two_and_complete_returns_second(void)
{
    char *item1 = strdup("Hello");
    char *item2 = strdup("Help");
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_add(ac, item1);
    p_autocomplete_add(ac, item2);
    char *result1 = p_autocomplete_complete(ac, "Hel");
    char *result2 = p_autocomplete_complete(ac, result1);

    assert_string_equals("Help", result2);

    p_autocomplete_clear(ac);
}

static void add_two_adds_two(void)
{
    char *item1 = strdup("Hello");
    char *item2 = strdup("Help");
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_add(ac, item1);
    p_autocomplete_add(ac, item2);
    GSList *result = p_autocomplete_get_list(ac);

    assert_int_equals(2, g_slist_length(result));

    p_autocomplete_clear(ac);
}

static void add_two_same_adds_one(void)
{
    char *item1 = strdup("Hello");
    char *item2 = strdup("Hello");
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_add(ac, item1);
    p_autocomplete_add(ac, item2);
    GSList *result = p_autocomplete_get_list(ac);

    assert_int_equals(1, g_slist_length(result));

    p_autocomplete_clear(ac);
}

static void add_two_same_updates(void)
{
    char *item1 = strdup("Hello");
    char *item2 = strdup("Hello");
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_add(ac, item1);
    p_autocomplete_add(ac, item2);
    GSList *result = p_autocomplete_get_list(ac);

    GSList *first = g_slist_nth(result, 0);

    char *str = first->data;

    assert_string_equals("Hello", str);

    p_autocomplete_clear(ac);
}

static void add_one_returns_true(void)
{
    char *item = strdup("Hello");
    PAutocomplete ac = p_autocomplete_new();
    int result = p_autocomplete_add(ac, item);

    assert_true(result);

    p_autocomplete_clear(ac);
}

static void add_two_different_returns_true(void)
{
    char *item1 = strdup("Hello");
    char *item2 = strdup("Hello there");
    PAutocomplete ac = p_autocomplete_new();
    int result1 = p_autocomplete_add(ac, item1);
    int result2 = p_autocomplete_add(ac, item2);

    assert_true(result1);
    assert_true(result2);

    p_autocomplete_clear(ac);
}

static void add_two_same_returns_false(void)
{
    char *item1 = strdup("Hello");
    char *item2 = strdup("Hello");
    PAutocomplete ac = p_autocomplete_new();
    int result1 = p_autocomplete_add(ac, item1);
    int result2 = p_autocomplete_add(ac, item2);

    assert_true(result1);
    assert_false(result2);

    p_autocomplete_clear(ac);
}

void register_prof_autocomplete_tests(void)
{
    TEST_MODULE("prof_autocomplete tests");
    TEST(clear_empty);
    TEST(reset_after_create);
    TEST(find_after_create);
    TEST(get_after_create_returns_null);
    TEST(add_one_and_complete);
    TEST(add_two_and_complete_returns_first);
    TEST(add_two_and_complete_returns_second);
    TEST(add_two_adds_two);
    TEST(add_two_same_adds_one);
    TEST(add_two_same_updates);
    TEST(add_one_returns_true);
    TEST(add_two_different_returns_true);
    TEST(add_two_same_returns_false);
}
