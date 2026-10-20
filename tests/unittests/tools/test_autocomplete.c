#include <glib.h>
#include "prof_cmocka.h"
#include <stdlib.h>

#include "xmpp/contact.h"
#include "tools/autocomplete.h"

void
autocomplete_complete__returns__null_when_empty(void** state)
{
    Autocomplete ac = autocomplete_new();
    gchar* result = autocomplete_complete(ac, "test", TRUE, FALSE);
    assert_null(result);
    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_reset__updates__after_create(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_reset(ac);
    autocomplete_free(ac);
}

void
autocomplete_complete__returns__null_after_create(void** state)
{
    Autocomplete ac = autocomplete_new();
    gchar* result = autocomplete_complete(ac, "hello", TRUE, FALSE);
    assert_null(result);
    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_create_list__returns__null_after_create(void** state)
{
    Autocomplete ac = autocomplete_new();
    GList* result = autocomplete_create_list(ac);

    assert_null(result);

    autocomplete_free(ac);
    g_list_free_full(result, g_free);
}

void
autocomplete_add__updates__one_and_complete(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    gchar* result = autocomplete_complete(ac, "Hel", TRUE, FALSE);

    assert_string_equal("Hello", result);

    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_complete__returns__first_of_two(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Help");
    gchar* result = autocomplete_complete(ac, "Hel", TRUE, FALSE);

    assert_string_equal("Hello", result);

    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_complete__returns__second_of_two(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Help");
    gchar* result1 = autocomplete_complete(ac, "Hel", TRUE, FALSE);
    gchar* result2 = autocomplete_complete(ac, result1, TRUE, FALSE);

    assert_string_equal("Help", result2);

    autocomplete_free(ac);
    g_free(result1);
    g_free(result2);
}

void
autocomplete_add__updates__two_elements(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Help");
    GList* result = autocomplete_create_list(ac);

    assert_int_equal(2, g_list_length(result));

    autocomplete_free(ac);
    g_list_free_full(result, g_free);
}

void
autocomplete_add__updates__only_once_for_same_value(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Hello");
    GList* result = autocomplete_create_list(ac);

    assert_int_equal(1, g_list_length(result));

    autocomplete_free(ac);
    g_list_free_full(result, g_free);
}

void
autocomplete_add__updates__existing_value(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Hello");
    autocomplete_add(ac, "Hello");
    GList* result = autocomplete_create_list(ac);

    GList* first = g_list_nth(result, 0);

    char* str = first->data;

    assert_string_equal("Hello", str);

    autocomplete_free(ac);
    g_list_free_full(result, g_free);
}

void
autocomplete_complete__returns__accented_with_accented(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "èâîô");

    gchar* result = autocomplete_complete(ac, "èâ", TRUE, FALSE);

    assert_string_equal("èâîô", result);

    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_complete__returns__accented_with_base(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "èâîô");

    gchar* result = autocomplete_complete(ac, "ea", TRUE, FALSE);

    assert_string_equal("èâîô", result);

    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_complete__returns__both_with_accented(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "eaooooo");
    autocomplete_add(ac, "èâîô");

    gchar* result1 = autocomplete_complete(ac, "èâ", TRUE, FALSE);
    gchar* result2 = autocomplete_complete(ac, result1, TRUE, FALSE);

    assert_string_equal("èâîô", result2);

    autocomplete_free(ac);
    g_free(result1);
    g_free(result2);
}

void
autocomplete_complete__returns__both_with_base(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "eaooooo");
    autocomplete_add(ac, "èâîô");

    gchar* result1 = autocomplete_complete(ac, "ea", TRUE, FALSE);
    gchar* result2 = autocomplete_complete(ac, result1, TRUE, FALSE);

    assert_string_equal("èâîô", result2);

    autocomplete_free(ac);

    g_free(result1);
    g_free(result2);
}

void
autocomplete_complete__is__case_insensitive(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "MyBuddy");

    gchar* result = autocomplete_complete(ac, "myb", TRUE, FALSE);

    assert_string_equal("MyBuddy", result);

    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_complete__returns__previous(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "MyBuddy1");
    autocomplete_add(ac, "MyBuddy2");
    autocomplete_add(ac, "MyBuddy3");

    gchar* result1 = autocomplete_complete(ac, "myb", TRUE, FALSE);
    gchar* result2 = autocomplete_complete(ac, result1, TRUE, FALSE);
    gchar* result3 = autocomplete_complete(ac, result2, TRUE, FALSE);
    gchar* result4 = autocomplete_complete(ac, result3, TRUE, TRUE);

    assert_string_equal("MyBuddy2", result4);

    autocomplete_free(ac);

    g_free(result1);
    g_free(result2);
    g_free(result3);
    g_free(result4);
}

void
autocomplete_complete__returns__greek_false_match(void** state)
{
    Autocomplete ac = autocomplete_new();
    // Σωκράτης (Socrates) and Πλάτων (Plato)
    autocomplete_add(ac, "Σωκράτης");
    autocomplete_add(ac, "Πλάτων");

    gchar* result = autocomplete_complete(ac, "Π", TRUE, FALSE);

    assert_string_equal("Πλάτων", result);

    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_complete__returns__greek(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Αριστοτέλης");

    gchar* result = autocomplete_complete(ac, "Αριστ", TRUE, FALSE);

    assert_non_null(result);
    assert_string_equal("Αριστοτέλης", result);

    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_complete__returns__greek_case_insensitive(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Σωκράτης");

    // Case insensitive search for Socrates
    // σω is the lowercase of Σω
    gchar* result = autocomplete_complete(ac, "σω", TRUE, FALSE);

    assert_non_null(result);
    assert_string_equal("Σωκράτης", result);

    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_complete__returns__russian(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Достоевский");
    autocomplete_add(ac, "Толстой");
    autocomplete_add(ac, "Пушкин");

    gchar* result = autocomplete_complete(ac, "дост", TRUE, FALSE);

    assert_non_null(result);
    assert_string_equal("Достоевский", result);

    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_complete__returns__chinese(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "孙子");
    autocomplete_add(ac, "诸葛亮");

    gchar* result = autocomplete_complete(ac, "孙", TRUE, FALSE);

    assert_non_null(result);
    assert_string_equal("孙子", result);

    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_complete__returns__transliterated(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "München");

    // Match 'ü' with 'u'
    gchar* result = autocomplete_complete(ac, "mun", TRUE, FALSE);

    assert_non_null(result);
    assert_string_equal("München", result);

    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_complete__returns__regular_ascii(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "London");

    gchar* result = autocomplete_complete(ac, "lon", TRUE, FALSE);

    assert_non_null(result);
    assert_string_equal("London", result);

    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_complete__handles__escaped_spaces(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Thor Odinson");

    gchar* result = autocomplete_complete(ac, "Thor\\ ", TRUE, FALSE);

    assert_string_equal("\"Thor Odinson\"", result);

    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_complete__handles__escaped_quotes(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "Thor \"The Thunderer\" Odinson");

    gchar* result = autocomplete_complete(ac, "Thor \\\"", TRUE, FALSE);

    assert_string_equal("\"Thor \\\"The Thunderer\\\" Odinson\"", result);

    autocomplete_free(ac);
    g_free(result);
}

void
autocomplete_param_with_ac__handles__quoted_input(void** state)
{
    Autocomplete ac = autocomplete_new();
    autocomplete_add(ac, "John Doe");
    autocomplete_add(ac, "Jane Roe");

    // Unquoted partial match
    gchar* result1 = autocomplete_param_with_ac("/msg Joh", "/msg", ac, TRUE, FALSE);
    assert_non_null(result1);
    assert_string_equal("/msg \"John Doe\"", result1);
    g_free(result1);

    // Quoted partial match
    gchar* result2 = autocomplete_param_with_ac("/msg \"Joh", "/msg", ac, TRUE, FALSE);
    assert_non_null(result2);
    assert_string_equal("/msg \"John Doe\"", result2);
    g_free(result2);

    // Cycle from completed quoted name to next match
    gchar* result3 = autocomplete_param_with_ac("/msg \"Jane Roe\"", "/msg", ac, TRUE, FALSE);
    assert_non_null(result3);
    assert_string_equal("/msg \"John Doe\"", result3);
    g_free(result3);

    autocomplete_free(ac);
}
