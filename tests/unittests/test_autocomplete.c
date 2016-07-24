#include <glib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include "xmpp/contact.h"
#include "tools/autocomplete.h"

void clear_empty(void **state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_clear(ac);
}

void reset_after_create(void **state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_reset(ac);
    autocomplete_clear(ac);
}

void find_after_create(void **state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_complete(ac, "hello", TRUE);
    autocomplete_clear(ac);
}

void get_after_create_returns_null(void **state)
{
    Autocomplete ac = autocomplete_new();
    GSList *result = autocomplete_create_list(ac);

    assert_null(result);

    autocomplete_clear(ac);
    g_slist_free_full(result, g_free);
}

void add_one_and_complete(void **state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    char *result = autocomplete_complete(ac, "Hel", TRUE);

    assert_string_equal("Hello", result);

    autocomplete_clear(ac);
}

void add_two_and_complete_returns_first(void **state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Help");
    char *result = autocomplete_complete(ac, "Hel", TRUE);

    assert_string_equal("Hello", result);

    autocomplete_clear(ac);
}

void add_two_and_complete_returns_second(void **state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Help");
    char *result1 = autocomplete_complete(ac, "Hel", TRUE);
    char *result2 = autocomplete_complete(ac, result1, TRUE);

    assert_string_equal("Help", result2);

    autocomplete_clear(ac);
}

void add_two_adds_two(void **state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Help");
    GSList *result = autocomplete_create_list(ac);

    assert_int_equal(2, g_slist_length(result));

    autocomplete_clear(ac);
    g_slist_free_full(result, g_free);
}

void add_two_same_adds_one(void **state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Hello");
    GSList *result = autocomplete_create_list(ac);

    assert_int_equal(1, g_slist_length(result));

    autocomplete_clear(ac);
    g_slist_free_full(result, g_free);
}

void add_two_same_updates(void **state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Hello");
    GSList *result = autocomplete_create_list(ac);

    GSList *first = g_slist_nth(result, 0);

    char *str = first->data;

    assert_string_equal("Hello", str);

    autocomplete_clear(ac);
    g_slist_free_full(result, g_free);
}
