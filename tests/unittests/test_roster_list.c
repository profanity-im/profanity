#include <glib.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include "xmpp/contact.h"
#include "xmpp/roster_list.h"

void empty_list_when_none_added(void **state)
{
    roster_create();
    GSList *list = roster_get_contacts(ROSTER_ORD_NAME);
    assert_null(list);
    roster_destroy();
}

void contains_one_element(void **state)
{
    roster_create();
    roster_add("James", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts(ROSTER_ORD_NAME);
    assert_int_equal(1, g_slist_length(list));
    roster_destroy();
}

void first_element_correct(void **state)
{
    roster_create();
    roster_add("James", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts(ROSTER_ORD_NAME);
    PContact james = list->data;

    assert_string_equal("James", p_contact_barejid(james));
    roster_destroy();
}

void contains_two_elements(void **state)
{
    roster_create();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts(ROSTER_ORD_NAME);

    assert_int_equal(2, g_slist_length(list));
    roster_destroy();
}

void first_and_second_elements_correct(void **state)
{
    roster_create();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts(ROSTER_ORD_NAME);

    PContact first = list->data;
    PContact second = (g_slist_next(list))->data;

    assert_string_equal("Dave", p_contact_barejid(first));
    assert_string_equal("James", p_contact_barejid(second));
    roster_destroy();
}

void contains_three_elements(void **state)
{
    roster_create();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts(ROSTER_ORD_NAME);

    assert_int_equal(3, g_slist_length(list));
    roster_destroy();
}

void first_three_elements_correct(void **state)
{
    roster_create();
    roster_add("Bob", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("James", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts(ROSTER_ORD_NAME);
    PContact bob = list->data;
    PContact dave = (g_slist_next(list))->data;
    PContact james = (g_slist_next(g_slist_next(list)))->data;

    assert_string_equal("James", p_contact_barejid(james));
    assert_string_equal("Dave", p_contact_barejid(dave));
    assert_string_equal("Bob", p_contact_barejid(bob));
    roster_destroy();
}

void add_twice_at_beginning_adds_once(void **state)
{
    roster_create();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts(ROSTER_ORD_NAME);
    PContact first = list->data;
    PContact second = (g_slist_next(list))->data;
    PContact third = (g_slist_next(g_slist_next(list)))->data;

    assert_int_equal(3, g_slist_length(list));
    assert_string_equal("Bob", p_contact_barejid(first));
    assert_string_equal("Dave", p_contact_barejid(second));
    assert_string_equal("James", p_contact_barejid(third));
    roster_destroy();
}

void add_twice_in_middle_adds_once(void **state)
{
    roster_create();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts(ROSTER_ORD_NAME);
    PContact first = list->data;
    PContact second = (g_slist_next(list))->data;
    PContact third = (g_slist_next(g_slist_next(list)))->data;

    assert_int_equal(3, g_slist_length(list));
    assert_string_equal("Bob", p_contact_barejid(first));
    assert_string_equal("Dave", p_contact_barejid(second));
    assert_string_equal("James", p_contact_barejid(third));
    roster_destroy();
}

void add_twice_at_end_adds_once(void **state)
{
    roster_create();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);
    roster_add("James", NULL, NULL, NULL, FALSE);
    GSList *list = roster_get_contacts(ROSTER_ORD_NAME);
    PContact first = list->data;
    PContact second = (g_slist_next(list))->data;
    PContact third = (g_slist_next(g_slist_next(list)))->data;

    assert_int_equal(3, g_slist_length(list));
    assert_string_equal("Bob", p_contact_barejid(first));
    assert_string_equal("Dave", p_contact_barejid(second));
    assert_string_equal("James", p_contact_barejid(third));
    roster_destroy();
}

void find_first_exists(void **state)
{
    roster_create();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);

    char *search = strdup("B");

    char *result = roster_contact_autocomplete(search, FALSE);
    assert_string_equal("Bob", result);
    free(result);
    free(search);
    roster_destroy();
}

void find_second_exists(void **state)
{
    roster_create();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);

    char *result = roster_contact_autocomplete("Dav", FALSE);
    assert_string_equal("Dave", result);
    free(result);
    roster_destroy();
}

void find_third_exists(void **state)
{
    roster_create();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);

    char *result = roster_contact_autocomplete("Ja", FALSE);
    assert_string_equal("James", result);
    free(result);
    roster_destroy();
}

void find_returns_null(void **state)
{
    roster_create();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Dave", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);

    char *result = roster_contact_autocomplete("Mike", FALSE);
    assert_null(result);
    roster_destroy();
}

void find_on_empty_returns_null(void **state)
{
    roster_create();
    char *result = roster_contact_autocomplete("James", FALSE);
    assert_null(result);
    roster_destroy();
}

void find_twice_returns_second_when_two_match(void **state)
{
    roster_create();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Jamie", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);

    char *result1 = roster_contact_autocomplete("Jam", FALSE);
    char *result2 = roster_contact_autocomplete(result1, FALSE);
    assert_string_equal("Jamie", result2);
    free(result1);
    free(result2);
    roster_destroy();
}

void find_five_times_finds_fifth(void **state)
{
    roster_create();
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

    char *result1 = roster_contact_autocomplete("Jam", FALSE);
    char *result2 = roster_contact_autocomplete(result1, FALSE);
    char *result3 = roster_contact_autocomplete(result2, FALSE);
    char *result4 = roster_contact_autocomplete(result3, FALSE);
    char *result5 = roster_contact_autocomplete(result4, FALSE);
    assert_string_equal("Jamo", result5);
    free(result1);
    free(result2);
    free(result3);
    free(result4);
    free(result5);
    roster_destroy();
}

void find_twice_returns_first_when_two_match_and_reset(void **state)
{
    roster_create();
    roster_add("James", NULL, NULL, NULL, FALSE);
    roster_add("Jamie", NULL, NULL, NULL, FALSE);
    roster_add("Bob", NULL, NULL, NULL, FALSE);

    char *result1 = roster_contact_autocomplete("Jam", FALSE);
    roster_reset_search_attempts();
    char *result2 = roster_contact_autocomplete(result1, FALSE);
    assert_string_equal("James", result2);
    free(result1);
    free(result2);
    roster_destroy();
}

void add_contact_with_no_group(void **state)
{
    roster_create();
    roster_add("person@server.org", NULL, NULL, NULL, FALSE);

    GSList *groups_res = roster_get_groups();
    assert_int_equal(g_slist_length(groups_res), 0);

    g_slist_free_full(groups_res, g_free);
    roster_destroy();
}

void add_contact_with_group(void **state)
{
    roster_create();

    GSList *groups = NULL;
    groups = g_slist_append(groups, strdup("friends"));
    roster_add("person@server.org", NULL, groups, NULL, FALSE);

    GSList *groups_res = roster_get_groups();
    assert_int_equal(g_slist_length(groups_res), 1);

    GSList *found = g_slist_find_custom(groups_res, "friends", g_strcmp0);
    assert_true(found != NULL);
    assert_string_equal(found->data, "friends");

    g_slist_free_full(groups_res, g_free);
    roster_destroy();
}

void add_contact_with_two_groups(void **state)
{
    roster_create();

    GSList *groups = NULL;
    groups = g_slist_append(groups, strdup("friends"));
    groups = g_slist_append(groups, strdup("work"));
    roster_add("person@server.org", NULL, groups, NULL, FALSE);

    GSList *groups_res = roster_get_groups();
    assert_int_equal(g_slist_length(groups_res), 2);

    GSList *found = g_slist_find_custom(groups_res, "friends", g_strcmp0);
    assert_true(found != NULL);
    assert_string_equal(found->data, "friends");
    found = g_slist_find_custom(groups_res, "work", g_strcmp0);
    assert_true(found != NULL);
    assert_string_equal(found->data, "work");

    g_slist_free_full(groups_res, g_free);
    roster_destroy();
}

void add_contact_with_three_groups(void **state)
{
    roster_create();

    GSList *groups = NULL;
    groups = g_slist_append(groups, strdup("friends"));
    groups = g_slist_append(groups, strdup("work"));
    groups = g_slist_append(groups, strdup("stuff"));
    roster_add("person@server.org", NULL, groups, NULL, FALSE);

    GSList *groups_res = roster_get_groups();
    assert_int_equal(g_slist_length(groups_res), 3);

    GSList *found = g_slist_find_custom(groups_res, "friends", g_strcmp0);
    assert_true(found != NULL);
    assert_string_equal(found->data, "friends");
    found = g_slist_find_custom(groups_res, "work", g_strcmp0);
    assert_true(found != NULL);
    assert_string_equal(found->data, "work");
    found = g_slist_find_custom(groups_res, "stuff", g_strcmp0);
    assert_true(found != NULL);
    assert_string_equal(found->data, "stuff");

    g_slist_free_full(groups_res, g_free);
    roster_destroy();
}

void add_contact_with_three_groups_update_adding_two(void **state)
{
    roster_create();

    GSList *groups1 = NULL;
    groups1 = g_slist_append(groups1, strdup("friends"));
    groups1 = g_slist_append(groups1, strdup("work"));
    groups1 = g_slist_append(groups1, strdup("stuff"));
    roster_add("person@server.org", NULL, groups1, NULL, FALSE);

    GSList *groups2 = NULL;
    groups2 = g_slist_append(groups2, strdup("friends"));
    groups2 = g_slist_append(groups2, strdup("work"));
    groups2 = g_slist_append(groups2, strdup("stuff"));
    groups2 = g_slist_append(groups2, strdup("things"));
    groups2 = g_slist_append(groups2, strdup("people"));
    roster_update("person@server.org", NULL, groups2, NULL, FALSE);

    GSList *groups_res = roster_get_groups();
    assert_int_equal(g_slist_length(groups_res), 5);

    GSList *found = g_slist_find_custom(groups_res, "friends", g_strcmp0);
    assert_true(found != NULL);
    assert_string_equal(found->data, "friends");
    found = g_slist_find_custom(groups_res, "work", g_strcmp0);
    assert_true(found != NULL);
    assert_string_equal(found->data, "work");
    found = g_slist_find_custom(groups_res, "stuff", g_strcmp0);
    assert_true(found != NULL);
    assert_string_equal(found->data, "stuff");
    found = g_slist_find_custom(groups_res, "things", g_strcmp0);
    assert_true(found != NULL);
    assert_string_equal(found->data, "things");
    found = g_slist_find_custom(groups_res, "people", g_strcmp0);
    assert_true(found != NULL);
    assert_string_equal(found->data, "people");

    g_slist_free_full(groups_res, g_free);
    roster_destroy();
}

void add_contact_with_three_groups_update_removing_one(void **state)
{
    roster_create();

    GSList *groups1 = NULL;
    groups1 = g_slist_append(groups1, strdup("friends"));
    groups1 = g_slist_append(groups1, strdup("work"));
    groups1 = g_slist_append(groups1, strdup("stuff"));
    roster_add("person@server.org", NULL, groups1, NULL, FALSE);

    GSList *groups2 = NULL;
    groups2 = g_slist_append(groups2, strdup("friends"));
    groups2 = g_slist_append(groups2, strdup("stuff"));
    roster_update("person@server.org", NULL, groups2, NULL, FALSE);

    GSList *groups_res = roster_get_groups();
    assert_int_equal(g_slist_length(groups_res), 2);

    GSList *found = g_slist_find_custom(groups_res, "friends", g_strcmp0);
    assert_true(found != NULL);
    assert_string_equal(found->data, "friends");
    found = g_slist_find_custom(groups_res, "stuff", g_strcmp0);
    assert_true(found != NULL);
    assert_string_equal(found->data, "stuff");

    g_slist_free_full(groups_res, g_free);
    roster_destroy();
}

void add_contact_with_three_groups_update_removing_two(void **state)
{
    roster_create();

    GSList *groups1 = NULL;
    groups1 = g_slist_append(groups1, strdup("friends"));
    groups1 = g_slist_append(groups1, strdup("work"));
    groups1 = g_slist_append(groups1, strdup("stuff"));
    roster_add("person@server.org", NULL, groups1, NULL, FALSE);

    GSList *groups2 = NULL;
    groups2 = g_slist_append(groups2, strdup("stuff"));
    roster_update("person@server.org", NULL, groups2, NULL, FALSE);

    GSList *groups_res = roster_get_groups();
    assert_int_equal(g_slist_length(groups_res), 1);

    GSList *found = g_slist_find_custom(groups_res, "stuff", g_strcmp0);
    assert_true(found != NULL);
    assert_string_equal(found->data, "stuff");

    g_slist_free_full(groups_res, g_free);
    roster_destroy();
}

void add_contact_with_three_groups_update_removing_three(void **state)
{
    roster_create();

    GSList *groups1 = NULL;
    groups1 = g_slist_append(groups1, strdup("friends"));
    groups1 = g_slist_append(groups1, strdup("work"));
    groups1 = g_slist_append(groups1, strdup("stuff"));
    roster_add("person@server.org", NULL, groups1, NULL, FALSE);

    roster_update("person@server.org", NULL, NULL, NULL, FALSE);

    GSList *groups_res = roster_get_groups();
    assert_int_equal(g_slist_length(groups_res), 0);

    g_slist_free_full(groups_res, g_free);
    roster_destroy();
}

void add_contact_with_three_groups_update_two_new(void **state)
{
    roster_create();

    GSList *groups1 = NULL;
    groups1 = g_slist_append(groups1, strdup("friends"));
    groups1 = g_slist_append(groups1, strdup("work"));
    groups1 = g_slist_append(groups1, strdup("stuff"));
    roster_add("person@server.org", NULL, groups1, NULL, FALSE);

    GSList *groups2 = NULL;
    groups2 = g_slist_append(groups2, strdup("newfriends"));
    groups2 = g_slist_append(groups2, strdup("somepeople"));
    roster_update("person@server.org", NULL, groups2, NULL, FALSE);

    GSList *groups_res = roster_get_groups();
    assert_int_equal(g_slist_length(groups_res), 2);

    GSList *found = g_slist_find_custom(groups_res, "newfriends", g_strcmp0);
    assert_true(found != NULL);
    found = g_slist_find_custom(groups_res, "somepeople", g_strcmp0);
    assert_true(found != NULL);

    g_slist_free_full(groups_res, g_free);
    roster_destroy();
}

void add_remove_contact_groups(void **state)
{
    roster_create();

    GSList *groups1 = NULL;
    groups1 = g_slist_append(groups1, strdup("friends"));
    groups1 = g_slist_append(groups1, strdup("work"));
    groups1 = g_slist_append(groups1, strdup("stuff"));
    roster_add("person@server.org", NULL, groups1, NULL, FALSE);

    roster_remove("person@server.org", "person@server.org");

    GSList *groups_res = roster_get_groups();
    assert_int_equal(g_slist_length(groups_res), 0);

    g_slist_free_full(groups_res, g_free);
    roster_destroy();
}

void add_contacts_with_different_groups(void **state)
{
    roster_create();

    GSList *groups1 = NULL;
    groups1 = g_slist_append(groups1, strdup("friends"));
    groups1 = g_slist_append(groups1, strdup("work"));
    groups1 = g_slist_append(groups1, strdup("stuff"));
    roster_add("person@server.org", NULL, groups1, NULL, FALSE);

    GSList *groups2 = NULL;
    groups2 = g_slist_append(groups2, strdup("newfriends"));
    groups2 = g_slist_append(groups2, strdup("somepeople"));
    roster_add("bob@server.org", NULL, groups2, NULL, FALSE);

    GSList *groups_res = roster_get_groups();
    assert_int_equal(g_slist_length(groups_res), 5);

    GSList *found = g_slist_find_custom(groups_res, "friends", g_strcmp0);
    assert_true(found != NULL);
    found = g_slist_find_custom(groups_res, "work", g_strcmp0);
    assert_true(found != NULL);
    found = g_slist_find_custom(groups_res, "stuff", g_strcmp0);
    assert_true(found != NULL);
    found = g_slist_find_custom(groups_res, "newfriends", g_strcmp0);
    assert_true(found != NULL);
    found = g_slist_find_custom(groups_res, "somepeople", g_strcmp0);
    assert_true(found != NULL);

    g_slist_free_full(groups_res, g_free);
    roster_destroy();
}

void add_contacts_with_same_groups(void **state)
{
    roster_create();

    GSList *groups1 = NULL;
    groups1 = g_slist_append(groups1, strdup("friends"));
    groups1 = g_slist_append(groups1, strdup("work"));
    groups1 = g_slist_append(groups1, strdup("stuff"));
    roster_add("person@server.org", NULL, groups1, NULL, FALSE);

    GSList *groups2 = NULL;
    groups2 = g_slist_append(groups2, strdup("friends"));
    groups2 = g_slist_append(groups2, strdup("work"));
    groups2 = g_slist_append(groups2, strdup("stuff"));
    roster_add("bob@server.org", NULL, groups2, NULL, FALSE);

    GSList *groups_res = roster_get_groups();
    assert_int_equal(g_slist_length(groups_res), 3);

    GSList *found = g_slist_find_custom(groups_res, "friends", g_strcmp0);
    assert_true(found != NULL);
    found = g_slist_find_custom(groups_res, "work", g_strcmp0);
    assert_true(found != NULL);
    found = g_slist_find_custom(groups_res, "stuff", g_strcmp0);
    assert_true(found != NULL);

    g_slist_free_full(groups_res, g_free);
    roster_destroy();
}

void add_contacts_with_overlapping_groups(void **state)
{
    roster_create();

    GSList *groups1 = NULL;
    groups1 = g_slist_append(groups1, strdup("friends"));
    groups1 = g_slist_append(groups1, strdup("work"));
    groups1 = g_slist_append(groups1, strdup("stuff"));
    roster_add("person@server.org", NULL, groups1, NULL, FALSE);

    GSList *groups2 = NULL;
    groups2 = g_slist_append(groups2, strdup("friends"));
    groups2 = g_slist_append(groups2, strdup("work"));
    groups2 = g_slist_append(groups2, strdup("different"));
    roster_add("bob@server.org", NULL, groups2, NULL, FALSE);

    GSList *groups_res = roster_get_groups();
    assert_int_equal(g_slist_length(groups_res), 4);

    GSList *found = g_slist_find_custom(groups_res, "friends", g_strcmp0);
    assert_true(found != NULL);
    found = g_slist_find_custom(groups_res, "work", g_strcmp0);
    assert_true(found != NULL);
    found = g_slist_find_custom(groups_res, "stuff", g_strcmp0);
    assert_true(found != NULL);
    found = g_slist_find_custom(groups_res, "different", g_strcmp0);
    assert_true(found != NULL);

    g_slist_free_full(groups_res, g_free);
    roster_destroy();
}

void remove_contact_with_remaining_in_group(void **state)
{
    roster_create();

    GSList *groups1 = NULL;
    groups1 = g_slist_append(groups1, strdup("friends"));
    groups1 = g_slist_append(groups1, strdup("work"));
    groups1 = g_slist_append(groups1, strdup("stuff"));
    roster_add("person@server.org", NULL, groups1, NULL, FALSE);

    GSList *groups2 = NULL;
    groups2 = g_slist_append(groups2, strdup("friends"));
    groups2 = g_slist_append(groups2, strdup("work"));
    groups2 = g_slist_append(groups2, strdup("different"));
    roster_add("bob@server.org", NULL, groups2, NULL, FALSE);

    roster_remove("bob@server.org", "bob@server.org");

    GSList *groups_res = roster_get_groups();
    assert_int_equal(g_slist_length(groups_res), 3);

    GSList *found = g_slist_find_custom(groups_res, "friends", g_strcmp0);
    assert_true(found != NULL);
    found = g_slist_find_custom(groups_res, "work", g_strcmp0);
    assert_true(found != NULL);
    found = g_slist_find_custom(groups_res, "stuff", g_strcmp0);
    assert_true(found != NULL);

    g_slist_free_full(groups_res, g_free);
    roster_destroy();
}
