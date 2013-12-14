#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <head-unit.h>
#include <glib.h>

#include "contact.h"
#include "roster_list.h"

static void beforetest(void)
{
    roster_init();
}

static void aftertest(void)
{
    roster_free();
}

static void empty_list_when_none_added(void)
{
    GSList *list = roster_get_contacts();
    assert_is_null(list);
}

static void contains_one_element(void)
{
    printf("0\n");
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    printf("1\n");
    GSList *list = roster_get_contacts();
    printf("2\n");
    assert_int_equals(1, g_slist_length(list));
    printf("3\n");
}

static void first_element_correct(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    GSList *list = roster_get_contacts();
    PContact james = list->data;

    assert_string_equals("James", p_contact_barejid(james));
}

static void contains_two_elements(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Dave", NULL, NULL, NULL, FALSE, TRUE);
    GSList *list = roster_get_contacts();

    assert_int_equals(2, g_slist_length(list));
}

static void first_and_second_elements_correct(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Dave", NULL, NULL, NULL, FALSE, TRUE);
    GSList *list = roster_get_contacts();

    PContact first = list->data;
    PContact second = (g_slist_next(list))->data;

    assert_string_equals("Dave", p_contact_barejid(first));
    assert_string_equals("James", p_contact_barejid(second));
}

static void contains_three_elements(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Bob", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Dave", NULL, NULL, NULL, FALSE, TRUE);
    GSList *list = roster_get_contacts();

    assert_int_equals(3, g_slist_length(list));
}

static void first_three_elements_correct(void)
{
    roster_add("Bob", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Dave", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    GSList *list = roster_get_contacts();
    PContact bob = list->data;
    PContact dave = (g_slist_next(list))->data;
    PContact james = (g_slist_next(g_slist_next(list)))->data;

    assert_string_equals("James", p_contact_barejid(james));
    assert_string_equals("Dave", p_contact_barejid(dave));
    assert_string_equals("Bob", p_contact_barejid(bob));
}

static void add_twice_at_beginning_adds_once(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Dave", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Bob", NULL, NULL, NULL, FALSE, TRUE);
    GSList *list = roster_get_contacts();
    PContact first = list->data;
    PContact second = (g_slist_next(list))->data;
    PContact third = (g_slist_next(g_slist_next(list)))->data;

    assert_int_equals(3, g_slist_length(list));
    assert_string_equals("Bob", p_contact_barejid(first));
    assert_string_equals("Dave", p_contact_barejid(second));
    assert_string_equals("James", p_contact_barejid(third));
}

static void add_twice_in_middle_adds_once(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Dave", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Bob", NULL, NULL, NULL, FALSE, TRUE);
    GSList *list = roster_get_contacts();
    PContact first = list->data;
    PContact second = (g_slist_next(list))->data;
    PContact third = (g_slist_next(g_slist_next(list)))->data;

    assert_int_equals(3, g_slist_length(list));
    assert_string_equals("Bob", p_contact_barejid(first));
    assert_string_equals("Dave", p_contact_barejid(second));
    assert_string_equals("James", p_contact_barejid(third));
}

static void add_twice_at_end_adds_once(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Dave", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Bob", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    GSList *list = roster_get_contacts();
    PContact first = list->data;
    PContact second = (g_slist_next(list))->data;
    PContact third = (g_slist_next(g_slist_next(list)))->data;

    assert_int_equals(3, g_slist_length(list));
    assert_string_equals("Bob", p_contact_barejid(first));
    assert_string_equals("Dave", p_contact_barejid(second));
    assert_string_equals("James", p_contact_barejid(third));
}

static void test_show_online_when_no_value(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    GSList *list = roster_get_contacts();
    PContact james = list->data;

    assert_string_equals("offline", p_contact_presence(james));
}

static void test_status_when_no_value(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    GSList *list = roster_get_contacts();
    PContact james = list->data;

    assert_is_null(p_contact_status(james));
}

static void find_first_exists(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Dave", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Bob", NULL, NULL, NULL, FALSE, TRUE);

    char *search = (char *) malloc(2 * sizeof(char));
    strcpy(search, "B");

    char *result = roster_find_contact(search);
    assert_string_equals("Bob", result);
    free(result);
    free(search);
}

static void find_second_exists(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Dave", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Bob", NULL, NULL, NULL, FALSE, TRUE);

    char *result = roster_find_contact("Dav");
    assert_string_equals("Dave", result);
    free(result);
}

static void find_third_exists(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Dave", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Bob", NULL, NULL, NULL, FALSE, TRUE);

    char *result = roster_find_contact("Ja");
    assert_string_equals("James", result);
    free(result);
}

static void find_returns_null(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Dave", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Bob", NULL, NULL, NULL, FALSE, TRUE);

    char *result = roster_find_contact("Mike");
    assert_is_null(result);
}

static void find_on_empty_returns_null(void)
{
    char *result = roster_find_contact("James");
    assert_is_null(result);
}

static void find_twice_returns_second_when_two_match(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Jamie", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Bob", NULL, NULL, NULL, FALSE, TRUE);

    char *result1 = roster_find_contact("Jam");
    char *result2 = roster_find_contact(result1);
    assert_string_equals("Jamie", result2);
    free(result1);
    free(result2);
}

static void find_five_times_finds_fifth(void)
{
    roster_add("Jama", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Jamb", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Mike", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Dave", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Jamm", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Jamn", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Matt", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Jamo", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Jamy", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Jamz", NULL, NULL, NULL, FALSE, TRUE);

    char *result1 = roster_find_contact("Jam");
    char *result2 = roster_find_contact(result1);
    char *result3 = roster_find_contact(result2);
    char *result4 = roster_find_contact(result3);
    char *result5 = roster_find_contact(result4);
    assert_string_equals("Jamo", result5);
    free(result1);
    free(result2);
    free(result3);
    free(result4);
    free(result5);
}

static void find_twice_returns_first_when_two_match_and_reset(void)
{
    roster_add("James", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Jamie", NULL, NULL, NULL, FALSE, TRUE);
    roster_add("Bob", NULL, NULL, NULL, FALSE, TRUE);

    char *result1 = roster_find_contact("Jam");
    roster_reset_search_attempts();
    char *result2 = roster_find_contact(result1);
    assert_string_equals("James", result2);
    free(result1);
    free(result2);
}

void register_roster_list_tests(void)
{
    TEST_MODULE("roster_list tests");
    BEFORETEST(beforetest);
    AFTERTEST(aftertest);
    TEST(empty_list_when_none_added);
    TEST(contains_one_element);
    TEST(first_element_correct);
    TEST(contains_two_elements);
    TEST(first_and_second_elements_correct);
    TEST(contains_three_elements);
    TEST(first_three_elements_correct);
    TEST(add_twice_at_beginning_adds_once);
    TEST(add_twice_in_middle_adds_once);
    TEST(add_twice_at_end_adds_once);
    TEST(test_show_online_when_no_value);
    TEST(test_status_when_no_value);
    TEST(find_first_exists);
    TEST(find_second_exists);
    TEST(find_third_exists);
    TEST(find_returns_null);
    TEST(find_on_empty_returns_null);
    TEST(find_twice_returns_second_when_two_match);
    TEST(find_twice_returns_first_when_two_match_and_reset);
    TEST(find_five_times_finds_fifth);
}
