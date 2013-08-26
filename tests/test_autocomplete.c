#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <head-unit.h>
#include <glib.h>

#include "contact.h"
#include "tools/autocomplete.h"

static void clear_empty(void)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_clear(ac);
}

static void reset_after_create(void)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_reset(ac);
    autocomplete_clear(ac);
}

static void find_after_create(void)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_complete(ac, "hello");
    autocomplete_clear(ac);
}

static void get_after_create_returns_null(void)
{
    Autocomplete ac = autocomplete_new();
    GSList *result = autocomplete_get_list(ac);

    assert_is_null(result);

    autocomplete_clear(ac);
}

static void add_one_and_complete(void)
{
    char *item = "Hello";
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    char *result = autocomplete_complete(ac, "Hel");

    assert_string_equals("Hello", result);

    autocomplete_clear(ac);
}

static void add_two_and_complete_returns_first(void)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Help");
    char *result = autocomplete_complete(ac, "Hel");

    assert_string_equals("Hello", result);

    autocomplete_clear(ac);
}

static void add_two_and_complete_returns_second(void)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Help");
    char *result1 = autocomplete_complete(ac, "Hel");
    char *result2 = autocomplete_complete(ac, result1);

    assert_string_equals("Help", result2);

    autocomplete_clear(ac);
}

static void add_two_adds_two(void)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Help");
    GSList *result = autocomplete_get_list(ac);

    assert_int_equals(2, g_slist_length(result));

    autocomplete_clear(ac);
}

static void add_two_same_adds_one(void)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Hello");
    GSList *result = autocomplete_get_list(ac);

    assert_int_equals(1, g_slist_length(result));

    autocomplete_clear(ac);
}

static void add_two_same_updates(void)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Hello");
    GSList *result = autocomplete_get_list(ac);

    GSList *first = g_slist_nth(result, 0);

    char *str = first->data;

    assert_string_equals("Hello", str);

    autocomplete_clear(ac);
}

static void add_one_returns_true(void)
{
    Autocomplete ac = autocomplete_new();
    int result = autocomplete_add(ac, "Hello");

    assert_true(result);

    autocomplete_clear(ac);
}

static void add_two_different_returns_true(void)
{
    Autocomplete ac = autocomplete_new();
    int result1 = autocomplete_add(ac, "Hello");
    int result2 = autocomplete_add(ac, "Hello there");

    assert_true(result1);
    assert_true(result2);

    autocomplete_clear(ac);
}

static void add_two_same_returns_false(void)
{
    Autocomplete ac = autocomplete_new();
    int result1 = autocomplete_add(ac, "Hello");
    int result2 = autocomplete_add(ac, "Hello");

    assert_true(result1);
    assert_false(result2);

    autocomplete_clear(ac);
}

void register_autocomplete_tests(void)
{
    TEST_MODULE("autocomplete tests");
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
