#include <stdio.h>
#include <head-unit.h>
#include "contact_list.h"

static void beforetest(void)
{
    contact_list_clear();
}

static void empty_list_when_none_added(void)
{
    struct contact_list *list = get_contact_list();
    assert_int_equals(0, list->size);
}

static void contains_one_element(void)
{
    contact_list_add("James");
    struct contact_list *list = get_contact_list();
    assert_int_equals(1, list->size);
}

static void first_element_correct(void)
{
    contact_list_add("James");
    struct contact_list *list = get_contact_list();

    assert_string_equals("James", list->contacts[0]);
}

static void contains_two_elements(void)
{
    contact_list_add("James");
    contact_list_add("Dave");
    struct contact_list *list = get_contact_list();

    assert_int_equals(2, list->size);
}

static void first_and_second_elements_correct(void)
{
    contact_list_add("James");
    contact_list_add("Dave");
    struct contact_list *list = get_contact_list();
    
    assert_string_equals("James", list->contacts[0]);
    assert_string_equals("Dave", list->contacts[1]);
}

static void contains_three_elements(void)
{
    contact_list_add("James");
    contact_list_add("Dave");
    contact_list_add("Bob");
    struct contact_list *list = get_contact_list();
    
    assert_int_equals(3, list->size);
}

static void first_three_elements_correct(void)
{
    contact_list_add("James");
    contact_list_add("Dave");
    contact_list_add("Bob");
    struct contact_list *list = get_contact_list();
    
    assert_string_equals("James", list->contacts[0]);
    assert_string_equals("Dave", list->contacts[1]);
    assert_string_equals("Bob", list->contacts[2]);
}

static void add_twice_at_beginning_adds_once(void)
{
    contact_list_add("James");
    contact_list_add("James");
    contact_list_add("Dave");
    contact_list_add("Bob");
    struct contact_list *list = get_contact_list();
    
    assert_int_equals(3, list->size);    
    assert_string_equals("James", list->contacts[0]);
    assert_string_equals("Dave", list->contacts[1]);
    assert_string_equals("Bob", list->contacts[2]);

}

static void add_twice_in_middle_adds_once(void)
{
    contact_list_add("James");
    contact_list_add("Dave");
    contact_list_add("James");
    contact_list_add("Bob");
    struct contact_list *list = get_contact_list();
    
    assert_int_equals(3, list->size);    
    assert_string_equals("James", list->contacts[0]);
    assert_string_equals("Dave", list->contacts[1]);
    assert_string_equals("Bob", list->contacts[2]);

}

static void add_twice_at_end_adds_once(void)
{
    contact_list_add("James");
    contact_list_add("Dave");
    contact_list_add("Bob");
    contact_list_add("James");
    struct contact_list *list = get_contact_list();
    
    assert_int_equals(3, list->size);    
    assert_string_equals("James", list->contacts[0]);
    assert_string_equals("Dave", list->contacts[1]);
    assert_string_equals("Bob", list->contacts[2]);

}
/*
static void remove_when_none_does_nothing(void)
{
    contact_list_remove("James");
    struct contact_list *list = get_contact_list();

    assert_int_equals(0, list->size);
}

static void remove_when_one_removes(void)
{
    contact_list_add("James");
    contact_list_remove("James");
    struct contact_list *list = get_contact_list();
    
    assert_int_equals(0, list->size);
}

static void remove_first_when_two(void)
{
    contact_list_add("James");
    contact_list_add("Dave");

    contact_list_remove("James");
    struct contact_list *list = get_contact_list();
    
    assert_int_equals(1, list->size);
    assert_string_equals("Dave", list->contacts[0]);
}

static void remove_second_when_two(void)
{
    contact_list_add("James");
    contact_list_add("Dave");

    contact_list_remove("Dave");
    struct contact_list *list = get_contact_list();
    
    assert_int_equals(1, list->size);
    assert_string_equals("James", list->contacts[0]);
}

static void remove_first_when_three(void)
{
    contact_list_add("James");
    contact_list_add("Dave");
    contact_list_add("Bob");

    contact_list_remove("James");
    struct contact_list *list = get_contact_list();
    
    assert_int_equals(2, list->size);
    assert_string_equals("Dave", list->contacts[0]);
    assert_string_equals("Bob", list->contacts[1]);
}

static void remove_second_when_three(void)
{
    contact_list_add("James");
    contact_list_add("Dave");
    contact_list_add("Bob");

    contact_list_remove("Dave");
    struct contact_list *list = get_contact_list();
    
    assert_int_equals(2, list->size);
    assert_string_equals("James", list->contacts[0]);
    assert_string_equals("Bob", list->contacts[1]);
}

static void remove_third_when_three(void)
{
    contact_list_add("James");
    contact_list_add("Dave");
    contact_list_add("Bob");

    contact_list_remove("Bob");
    struct contact_list *list = get_contact_list();
    
    assert_int_equals(2, list->size);
    assert_string_equals("James", list->contacts[0]);
    assert_string_equals("Dave", list->contacts[1]);
}*/

void register_contact_list_tests(void)
{
    TEST_MODULE("contact_list tests");
    BEFORETEST(beforetest);
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
/*    TEST(remove_when_none_does_nothing);
    TEST(remove_when_one_removes);
    TEST(remove_first_when_two);
    TEST(remove_second_when_two);
    TEST(remove_first_when_three);
    TEST(remove_second_when_three);
    TEST(remove_third_when_three);
*/
}
