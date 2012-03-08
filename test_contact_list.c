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

void register_contact_list_tests(void)
{
    TEST_MODULE("contact_list tests");
    BEFORETEST(beforetest);
    TEST(empty_list_when_none_added);
    TEST(contains_one_element);
    TEST(first_element_correct);
    TEST(contains_two_elements);
    TEST(first_and_second_elements_correct);
}
