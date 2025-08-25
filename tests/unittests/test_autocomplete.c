#include <glib.h>
#include "prof_cmocka.h"
#include <stdlib.h>

#include "xmpp/contact.h"
#include "tools/autocomplete.h"

void
clear_empty(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_free(ac);
}

void
reset_after_create(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_reset(ac);
    autocomplete_free(ac);
}

void
find_after_create(void** state)
{
    Autocomplete ac = autocomplete_new();
    char* result = autocomplete_complete(ac, "hello", TRUE, FALSE);
    autocomplete_free(ac);
    free(result);
}

void
get_after_create_returns_null(void** state)
{
    Autocomplete ac = autocomplete_new();
    GList* result = autocomplete_create_list(ac);

    assert_null(result);

    autocomplete_free(ac);
    g_list_free_full(result, free);
}

void
add_one_and_complete(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    char* result = autocomplete_complete(ac, "Hel", TRUE, FALSE);

    assert_string_equal("Hello", result);

    autocomplete_free(ac);
    free(result);
}

void
add_two_and_complete_returns_first(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Help");
    char* result = autocomplete_complete(ac, "Hel", TRUE, FALSE);

    assert_string_equal("Hello", result);

    autocomplete_free(ac);
    free(result);
}

void
add_two_and_complete_returns_second(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Help");
    char* result1 = autocomplete_complete(ac, "Hel", TRUE, FALSE);
    char* result2 = autocomplete_complete(ac, result1, TRUE, FALSE);

    assert_string_equal("Help", result2);

    autocomplete_free(ac);
    free(result1);
    free(result2);
}

void
add_two_adds_two(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Help");
    GList* result = autocomplete_create_list(ac);

    assert_int_equal(2, g_list_length(result));

    autocomplete_free(ac);
    g_list_free_full(result, free);
}

void
add_two_same_adds_one(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Hello");
    GList* result = autocomplete_create_list(ac);

    assert_int_equal(1, g_list_length(result));

    autocomplete_free(ac);
    g_list_free_full(result, free);
}

void
add_two_same_updates(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Hello");
    GList* result = autocomplete_create_list(ac);

    GList* first = g_list_nth(result, 0);

    char* str = first->data;

    assert_string_equal("Hello", str);

    autocomplete_free(ac);
    g_list_free_full(result, free);
}

void
complete_accented_with_accented(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "èâîô");

    char* result = autocomplete_complete(ac, "èâ", TRUE, FALSE);

    assert_string_equal("èâîô", result);

    autocomplete_free(ac);
    free(result);
}

void
complete_accented_with_base(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "èâîô");

    char* result = autocomplete_complete(ac, "ea", TRUE, FALSE);

    assert_string_equal("èâîô", result);

    autocomplete_free(ac);
    free(result);
}

void
complete_both_with_accented(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "eaooooo");
    autocomplete_add(ac, "èâîô");

    char* result1 = autocomplete_complete(ac, "èâ", TRUE, FALSE);
    char* result2 = autocomplete_complete(ac, result1, TRUE, FALSE);

    assert_string_equal("èâîô", result2);

    autocomplete_free(ac);
    free(result1);
    free(result2);
}

void
complete_both_with_base(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "eaooooo");
    autocomplete_add(ac, "èâîô");

    char* result1 = autocomplete_complete(ac, "ea", TRUE, FALSE);
    char* result2 = autocomplete_complete(ac, result1, TRUE, FALSE);

    assert_string_equal("èâîô", result2);

    autocomplete_free(ac);

    free(result1);
    free(result2);
}

void
complete_ignores_case(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "MyBuddy");

    char* result = autocomplete_complete(ac, "myb", TRUE, FALSE);

    assert_string_equal("MyBuddy", result);

    autocomplete_free(ac);
    free(result);
}

void
complete_previous(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "MyBuddy1");
    autocomplete_add(ac, "MyBuddy2");
    autocomplete_add(ac, "MyBuddy3");

    char* result1 = autocomplete_complete(ac, "myb", TRUE, FALSE);
    char* result2 = autocomplete_complete(ac, result1, TRUE, FALSE);
    char* result3 = autocomplete_complete(ac, result2, TRUE, FALSE);
    char* result4 = autocomplete_complete(ac, result3, TRUE, TRUE);

    assert_string_equal("MyBuddy2", result4);

    autocomplete_free(ac);

    free(result1);
    free(result2);
    free(result3);
    free(result4);
}
