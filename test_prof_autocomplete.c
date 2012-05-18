#include <stdio.h>
#include <head-unit.h>
#include <glib.h>

#include "contact.h"
#include "prof_autocomplete.h"

static void clear_empty(void)
{
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_clear(ac, NULL);
}

static void clear_empty_with_free_func(void)
{
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_clear(ac, (GDestroyNotify)p_contact_free);
}

static void reset_after_create(void)
{
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_reset(ac);
    p_autocomplete_clear(ac, NULL);
}

static void find_after_create(void)
{
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_complete(ac, "hello", NULL);
    p_autocomplete_clear(ac, NULL);
}

static void get_after_create_returns_null(void)
{
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_get_list(ac, NULL);
    p_autocomplete_clear(ac, NULL);
}

static void get_after_create_with_copy_func_returns_null(void)
{
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_get_list(ac, (PCopyFunc)p_contact_copy);
    p_autocomplete_clear(ac, NULL);
}

static void add_one_and_complete(void)
{
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_add(ac, "Hello", NULL, NULL);
    char *result = p_autocomplete_complete(ac, "Hel", NULL);

    assert_string_equals("Hello", result);
}

static void add_one_and_complete_with_funcs(void)
{
    PContact contact = p_contact_new("James", "Online", "I'm here");
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_add(ac, contact, (PStrFunc)p_contact_name, 
        (GDestroyNotify)p_contact_free);
    char *result = p_autocomplete_complete(ac, "Jam", (PStrFunc)p_contact_name);

    assert_string_equals("James", result);
}

static void add_two_and_complete_returns_first(void)
{
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_add(ac, "Hello", NULL, NULL);
    p_autocomplete_add(ac, "Help", NULL, NULL);
    char *result = p_autocomplete_complete(ac, "Hel", NULL);

    assert_string_equals("Hello", result);
}

static void add_two_and_complete_returns_first_with_funcs(void)
{
    PContact contact1 = p_contact_new("James", "Online", "I'm here");
    PContact contact2 = p_contact_new("Jamie", "Away", "Out to lunch");
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_add(ac, contact1, (PStrFunc)p_contact_name,
        (GDestroyNotify)p_contact_free);
    p_autocomplete_add(ac, contact2, (PStrFunc)p_contact_name,
        (GDestroyNotify)p_contact_free);
    char *result = p_autocomplete_complete(ac, "Jam", (PStrFunc)p_contact_name);

    assert_string_equals("James", result);
}

static void add_two_and_complete_returns_second(void)
{
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_add(ac, "Hello", NULL, NULL);
    p_autocomplete_add(ac, "Help", NULL, NULL);
    char *result1 = p_autocomplete_complete(ac, "Hel", NULL);
    char *result2 = p_autocomplete_complete(ac, result1, NULL);

    assert_string_equals("Help", result2);
}

static void add_two_and_complete_returns_second_with_funcs(void)
{
    PContact contact1 = p_contact_new("James", "Online", "I'm here");
    PContact contact2 = p_contact_new("Jamie", "Away", "Out to lunch");
    PAutocomplete ac = p_autocomplete_new();
    p_autocomplete_add(ac, contact1, (PStrFunc)p_contact_name,
        (GDestroyNotify)p_contact_free);
    p_autocomplete_add(ac, contact2, (PStrFunc)p_contact_name,
        (GDestroyNotify)p_contact_free);
    char *result1 = p_autocomplete_complete(ac, "Jam", (PStrFunc)p_contact_name);
    char *result2 = p_autocomplete_complete(ac, result1, (PStrFunc)p_contact_name);

    assert_string_equals("Jamie", result2);
}

void register_prof_autocomplete_tests(void)
{
    TEST_MODULE("prof_autocomplete tests");
    TEST(clear_empty);
    TEST(clear_empty_with_free_func);
    TEST(reset_after_create);
    TEST(find_after_create);
    TEST(get_after_create_returns_null);
    TEST(get_after_create_with_copy_func_returns_null);
    TEST(add_one_and_complete);
    TEST(add_one_and_complete_with_funcs);
    TEST(add_two_and_complete_returns_first);
    TEST(add_two_and_complete_returns_first_with_funcs);
    TEST(add_two_and_complete_returns_second);
    TEST(add_two_and_complete_returns_second_with_funcs);
}
