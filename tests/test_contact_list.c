#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <head-unit.h>
#include <glib.h>

#include "contact.h"
#include "contact_list.h"

static void setup(void)
{
    contact_list_init();
}

static void beforetest(void)
{
    contact_list_clear();
}

static void aftertest(void)
{
    contact_list_clear();
}

static void empty_list_when_none_added(void)
{
    GSList *list = get_contact_list();
    assert_is_null(list);
}

static void contains_one_element(void)
{
    contact_list_add("James", NULL, NULL);
    GSList *list = get_contact_list();
    assert_int_equals(1, g_slist_length(list));
}

static void first_element_correct(void)
{
    contact_list_add("James", NULL, NULL);
    GSList *list = get_contact_list();
    PContact james = list->data;

    assert_string_equals("James", p_contact_name(james));
}

static void contains_two_elements(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    GSList *list = get_contact_list();

    assert_int_equals(2, g_slist_length(list));
}

static void first_and_second_elements_correct(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    GSList *list = get_contact_list();

    PContact dave = list->data;
    PContact james = (g_slist_next(list))->data;

    assert_string_equals("James", p_contact_name(james));
    assert_string_equals("Dave", p_contact_name(dave));
}

static void contains_three_elements(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Bob", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    GSList *list = get_contact_list();

    assert_int_equals(3, g_slist_length(list));
}

static void first_three_elements_correct(void)
{
    contact_list_add("Bob", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    contact_list_add("James", NULL, NULL);
    GSList *list = get_contact_list();
    PContact bob = list->data;
    PContact dave = (g_slist_next(list))->data;
    PContact james = (g_slist_next(g_slist_next(list)))->data;

    assert_string_equals("James", p_contact_name(james));
    assert_string_equals("Dave", p_contact_name(dave));
    assert_string_equals("Bob", p_contact_name(bob));
}

static void add_twice_at_beginning_adds_once(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("James", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    contact_list_add("Bob", NULL, NULL);
    GSList *list = get_contact_list();
    PContact bob = list->data;
    PContact dave = (g_slist_next(list))->data;
    PContact james = (g_slist_next(g_slist_next(list)))->data;

    assert_int_equals(3, g_slist_length(list));
    assert_string_equals("James", p_contact_name(james));
    assert_string_equals("Dave", p_contact_name(dave));
    assert_string_equals("Bob", p_contact_name(bob));
}

static void add_twice_in_middle_adds_once(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    contact_list_add("James", NULL, NULL);
    contact_list_add("Bob", NULL, NULL);
    GSList *list = get_contact_list();
    PContact bob = list->data;
    PContact dave = (g_slist_next(list))->data;
    PContact james = (g_slist_next(g_slist_next(list)))->data;

    assert_int_equals(3, g_slist_length(list));
    assert_string_equals("James", p_contact_name(james));
    assert_string_equals("Dave", p_contact_name(dave));
    assert_string_equals("Bob", p_contact_name(bob));
}

static void add_twice_at_end_adds_once(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    contact_list_add("Bob", NULL, NULL);
    contact_list_add("James", NULL, NULL);
    GSList *list = get_contact_list();
    PContact bob = list->data;
    PContact dave = (g_slist_next(list))->data;
    PContact james = (g_slist_next(g_slist_next(list)))->data;

    assert_int_equals(3, g_slist_length(list));
    assert_string_equals("James", p_contact_name(james));
    assert_string_equals("Dave", p_contact_name(dave));
    assert_string_equals("Bob", p_contact_name(bob));
}

static void remove_when_none_does_nothing(void)
{
    contact_list_remove("James");
    GSList *list = get_contact_list();

    assert_int_equals(0, g_slist_length(list));
}

static void remove_when_one_removes(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_remove("James");
    GSList *list = get_contact_list();

    assert_int_equals(0, g_slist_length(list));
}

static void remove_first_when_two(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);

    contact_list_remove("James");
    GSList *list = get_contact_list();

    assert_int_equals(1, g_slist_length(list));
    PContact dave = list->data;
    assert_string_equals("Dave", p_contact_name(dave));
}

static void remove_second_when_two(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);

    contact_list_remove("Dave");
    GSList *list = get_contact_list();

    assert_int_equals(1, g_slist_length(list));
    PContact james = list->data;
    assert_string_equals("James", p_contact_name(james));
}

static void remove_first_when_three(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    contact_list_add("Bob", NULL, NULL);

    contact_list_remove("James");
    GSList *list = get_contact_list();

    assert_int_equals(2, g_slist_length(list));
    PContact bob = list->data;
    PContact dave = (g_slist_next(list))->data;

    assert_string_equals("Dave", p_contact_name(dave));
    assert_string_equals("Bob", p_contact_name(bob));
}

static void remove_second_when_three(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    contact_list_add("Bob", NULL, NULL);

    contact_list_remove("Dave");
    GSList *list = get_contact_list();

    assert_int_equals(2, g_slist_length(list));
    PContact bob = list->data;
    PContact james = (g_slist_next(list))->data;

    assert_string_equals("James", p_contact_name(james));
    assert_string_equals("Bob", p_contact_name(bob));
}

static void remove_third_when_three(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    contact_list_add("Bob", NULL, NULL);

    contact_list_remove("Bob");
    GSList *list = get_contact_list();

    assert_int_equals(2, g_slist_length(list));
    PContact dave = list->data;
    PContact james = (g_slist_next(list))->data;

    assert_string_equals("James", p_contact_name(james));
    assert_string_equals("Dave", p_contact_name(dave));
}

static void test_show_when_value(void)
{
    contact_list_add("James", "away", NULL);
    GSList *list = get_contact_list();
    PContact james = list->data;

    assert_string_equals("away", p_contact_show(james));
}

static void test_show_online_when_no_value(void)
{
    contact_list_add("James", NULL, NULL);
    GSList *list = get_contact_list();
    PContact james = list->data;

    assert_string_equals("online", p_contact_show(james));
}

static void test_show_online_when_empty_string(void)
{
    contact_list_add("James", "", NULL);
    GSList *list = get_contact_list();
    PContact james = list->data;

    assert_string_equals("online", p_contact_show(james));
}

static void test_status_when_value(void)
{
    contact_list_add("James", NULL, "I'm not here right now");
    GSList *list = get_contact_list();
    PContact james = list->data;

    assert_string_equals("I'm not here right now", p_contact_status(james));
}

static void test_status_when_no_value(void)
{
    contact_list_add("James", NULL, NULL);
    GSList *list = get_contact_list();
    PContact james = list->data;

    assert_is_null(p_contact_status(james));
}

static void update_show(void)
{
    contact_list_add("James", "away", NULL);
    contact_list_add("James", "dnd", NULL);
    GSList *list = get_contact_list();

    assert_int_equals(1, g_slist_length(list));
    PContact james = list->data;
    assert_string_equals("James", p_contact_name(james));
    assert_string_equals("dnd", p_contact_show(james));
}

static void set_show_to_null(void)
{
    contact_list_add("James", "away", NULL);
    contact_list_add("James", NULL, NULL);
    GSList *list = get_contact_list();

    assert_int_equals(1, g_slist_length(list));
    PContact james = list->data;
    assert_string_equals("James", p_contact_name(james));
    assert_string_equals("online", p_contact_show(james));
}

static void update_status(void)
{
    contact_list_add("James", NULL, "I'm not here right now");
    contact_list_add("James", NULL, "Gone to lunch");
    GSList *list = get_contact_list();

    assert_int_equals(1, g_slist_length(list));
    PContact james = list->data;
    assert_string_equals("James", p_contact_name(james));
    assert_string_equals("Gone to lunch", p_contact_status(james));
}

static void set_status_to_null(void)
{
    contact_list_add("James", NULL, "Gone to lunch");
    contact_list_add("James", NULL, NULL);
    GSList *list = get_contact_list();

    assert_int_equals(1, g_slist_length(list));
    PContact james = list->data;
    assert_string_equals("James", p_contact_name(james));
    assert_is_null(p_contact_status(james));
}

static void find_first_exists(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    contact_list_add("Bob", NULL, NULL);

    char *search = (char *) malloc(2 * sizeof(char));
    strcpy(search, "B");

    char *result = find_contact(search);
    assert_string_equals("Bob", result);
    free(result);
    free(search);
}

static void find_second_exists(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    contact_list_add("Bob", NULL, NULL);

    char *result = find_contact("Dav");
    assert_string_equals("Dave", result);
    free(result);
}

static void find_third_exists(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    contact_list_add("Bob", NULL, NULL);

    char *result = find_contact("Ja");
    assert_string_equals("James", result);
    free(result);
}

static void find_returns_null(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    contact_list_add("Bob", NULL, NULL);

    char *result = find_contact("Mike");
    assert_is_null(result);
}

static void find_on_empty_returns_null(void)
{
    char *result = find_contact("James");
    assert_is_null(result);
}

static void find_twice_returns_second_when_two_match(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Jamie", NULL, NULL);
    contact_list_add("Bob", NULL, NULL);

    char *result1 = find_contact("Jam");
    char *result2 = find_contact(result1);
    assert_string_equals("Jamie", result2);
    free(result1);
    free(result2);
}

static void find_five_times_finds_fifth(void)
{
    contact_list_add("Jama", NULL, NULL);
    contact_list_add("Jamb", NULL, NULL);
    contact_list_add("Mike", NULL, NULL);
    contact_list_add("Dave", NULL, NULL);
    contact_list_add("Jamm", NULL, NULL);
    contact_list_add("Jamn", NULL, NULL);
    contact_list_add("Matt", NULL, NULL);
    contact_list_add("Jamo", NULL, NULL);
    contact_list_add("Jamy", NULL, NULL);
    contact_list_add("Jamz", NULL, NULL);

    char *result1 = find_contact("Jam");
    char *result2 = find_contact(result1);
    char *result3 = find_contact(result2);
    char *result4 = find_contact(result3);
    char *result5 = find_contact(result4);
    assert_string_equals("Jamo", result5);
    free(result1);
    free(result2);
    free(result3);
    free(result4);
    free(result5);
}

static void find_twice_returns_first_when_two_match_and_reset(void)
{
    contact_list_add("James", NULL, NULL);
    contact_list_add("Jamie", NULL, NULL);
    contact_list_add("Bob", NULL, NULL);

    char *result1 = find_contact("Jam");
    reset_search_attempts();
    char *result2 = find_contact(result1);
    assert_string_equals("James", result2);
    free(result1);
    free(result2);
}

static void removed_contact_not_in_search(void)
{
    contact_list_add("Jamatron", NULL, NULL);
    contact_list_add("Bob", NULL, NULL);
    contact_list_add("Jambo", NULL, NULL);
    contact_list_add("James", NULL, NULL);
    contact_list_add("Jamie", NULL, NULL);

    char *result1 = find_contact("Jam"); // Jamatron
    char *result2 = find_contact(result1); // Jambo
    contact_list_remove("James");
    char *result3 = find_contact(result2);
    assert_string_equals("Jamie", result3);
    free(result1);
    free(result2);
    free(result3);
}

void register_contact_list_tests(void)
{
    TEST_MODULE("contact_list tests");
    SETUP(setup);
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
    TEST(remove_when_none_does_nothing);
    TEST(remove_when_one_removes);
    TEST(remove_first_when_two);
    TEST(remove_second_when_two);
    TEST(remove_first_when_three);
    TEST(remove_second_when_three);
    TEST(remove_third_when_three);
    TEST(test_show_when_value);
    TEST(test_show_online_when_no_value);
    TEST(test_show_online_when_empty_string);
    TEST(test_status_when_value);
    TEST(test_status_when_no_value);
    TEST(update_show);
    TEST(set_show_to_null);
    TEST(update_status);
    TEST(set_status_to_null);
    TEST(find_first_exists);
    TEST(find_second_exists);
    TEST(find_third_exists);
    TEST(find_returns_null);
    TEST(find_on_empty_returns_null);
    TEST(find_twice_returns_second_when_two_match);
    TEST(find_twice_returns_first_when_two_match_and_reset);
    TEST(removed_contact_not_in_search);
    TEST(find_five_times_finds_fifth);
}
