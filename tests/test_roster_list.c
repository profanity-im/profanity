#include <glib.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include "contact.h"
#include "roster_list.h"

void empty_list_when_none_added(void **state)
{
    roster_init();
    GSList *list = roster_get_contacts();
    assert_null(list);
    roster_free();
}

void contains_one_element(void **state)
{
    roster_init();
    roster_add("James", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts();
    assert_int_equal(1, g_slist_length(list));
    roster_free();
}

void first_element_correct(void **state)
{
    roster_init();
    roster_add("James", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts();
    PContact james = list->data;

    assert_string_equal("James", p_contact_barejid(james));
    roster_free();
}

void contains_two_elements(void **state)
{
    roster_init();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts();

    assert_int_equal(2, g_slist_length(list));
    roster_free();
}

void first_and_second_elements_correct(void **state)
{
    roster_init();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts();

    PContact first = list->data;
    PContact second = (g_slist_next(list))->data;

    assert_string_equal("Dave", p_contact_barejid(first));
    assert_string_equal("James", p_contact_barejid(second));
    roster_free();
}

void contains_three_elements(void **state)
{
    roster_init();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts();

    assert_int_equal(3, g_slist_length(list));
    roster_free();
}

void first_three_elements_correct(void **state)
{
    roster_init();
    roster_add("Bob", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("James", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts();
    PContact bob = list->data;
    PContact dave = (g_slist_next(list))->data;
    PContact james = (g_slist_next(g_slist_next(list)))->data;

    assert_string_equal("James", p_contact_barejid(james));
    assert_string_equal("Dave", p_contact_barejid(dave));
    assert_string_equal("Bob", p_contact_barejid(bob));
    roster_free();
}

void add_twice_at_beginning_adds_once(void **state)
{
    roster_init();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts();
    PContact first = list->data;
    PContact second = (g_slist_next(list))->data;
    PContact third = (g_slist_next(g_slist_next(list)))->data;

    assert_int_equal(3, g_slist_length(list));
    assert_string_equal("Bob", p_contact_barejid(first));
    assert_string_equal("Dave", p_contact_barejid(second));
    assert_string_equal("James", p_contact_barejid(third));
    roster_free();
}

void add_twice_in_middle_adds_once(void **state)
{
    roster_init();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts();
    PContact first = list->data;
    PContact second = (g_slist_next(list))->data;
    PContact third = (g_slist_next(g_slist_next(list)))->data;

    assert_int_equal(3, g_slist_length(list));
    assert_string_equal("Bob", p_contact_barejid(first));
    assert_string_equal("Dave", p_contact_barejid(second));
    assert_string_equal("James", p_contact_barejid(third));
    roster_free();
}

void add_twice_at_end_adds_once(void **state)
{
    roster_init();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);
    roster_add("James", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts();
    PContact first = list->data;
    PContact second = (g_slist_next(list))->data;
    PContact third = (g_slist_next(g_slist_next(list)))->data;

    assert_int_equal(3, g_slist_length(list));
    assert_string_equal("Bob", p_contact_barejid(first));
    assert_string_equal("Dave", p_contact_barejid(second));
    assert_string_equal("James", p_contact_barejid(third));
    roster_free();
}

void find_first_exists(void **state)
{
    roster_init();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);

    char *search = strdup("B");

    char *result = roster_find_contact(search);
    assert_string_equal("Bob", result);
    free(result);
    free(search);
    roster_free();
}

void find_second_exists(void **state)
{
    roster_init();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);

    char *result = roster_find_contact("Dav");
    assert_string_equal("Dave", result);
    free(result);
    roster_free();
}

void find_third_exists(void **state)
{
    roster_init();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);

    char *result = roster_find_contact("Ja");
    assert_string_equal("James", result);
    free(result);
    roster_free();
}

void find_returns_null(void **state)
{
    roster_init();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);

    char *result = roster_find_contact("Mike");
    assert_null(result);
    roster_free();
}

void find_on_empty_returns_null(void **state)
{
    roster_init();
    char *result = roster_find_contact("James");
    assert_null(result);
    roster_free();
}

void find_twice_returns_second_when_two_match(void **state)
{
    roster_init();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Jamie", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);

    char *result1 = roster_find_contact("Jam");
    char *result2 = roster_find_contact(result1);
    assert_string_equal("Jamie", result2);
    free(result1);
    free(result2);
    roster_free();
}

void find_five_times_finds_fifth(void **state)
{
    roster_init();
    roster_add("Jama", NULL, NULL, NULL, FALSE);
    roster_add("Jamb", NULL, NULL, NULL, FALSE);
    roster_add("Mike", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("Jamm", NULL, NULL, NULL, FALSE);
    roster_add("Jamn", NULL, NULL, NULL, FALSE);
    roster_add("Matt", NULL, NULL, NULL, FALSE);
    roster_add("Jamo", NULL, NULL, NULL, FALSE);
    roster_add("Jamy", NULL, NULL, NULL, FALSE);
    roster_add("Jamz", NULL, NULL, NULL, FALSE);

    char *result1 = roster_find_contact("Jam");
    char *result2 = roster_find_contact(result1);
    char *result3 = roster_find_contact(result2);
    char *result4 = roster_find_contact(result3);
    char *result5 = roster_find_contact(result4);
    assert_string_equal("Jamo", result5);
    free(result1);
    free(result2);
    free(result3);
    free(result4);
    free(result5);
    roster_free();
}

void find_twice_returns_first_when_two_match_and_reset(void **state)
{
    roster_init();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Jamie", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);

    char *result1 = roster_find_contact("Jam");
    roster_reset_search_attempts();
    char *result2 = roster_find_contact(result1);
    assert_string_equal("James", result2);
    free(result1);
    free(result2);
    roster_free();
}
