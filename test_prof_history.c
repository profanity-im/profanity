#include <stdio.h>
#include <head-unit.h>
#include "prof_history.h"

void previous_on_empty_returns_null(void)
{
    PHistory history = p_history_new(10);
    char *item = p_history_previous(history);

    assert_is_null(item);
}

void next_on_empty_returns_null(void)
{
    PHistory history = p_history_new(10);
    char *item = p_history_next(history);

    assert_is_null(item);
}

void previous_once_returns_last(void)
{
    PHistory history = p_history_new(10);
    p_history_append(history, "Hello");

    char *item = p_history_previous(history);

    assert_string_equals("Hello", item);
}

void previous_twice_when_one_returns_first(void)
{
    PHistory history = p_history_new(10);
    p_history_append(history, "Hello");

    p_history_previous(history);
    char *item = p_history_previous(history);

    assert_string_equals("Hello", item);
}

void register_prof_history_tests(void)
{
    TEST_MODULE("prof_history tests");
    TEST(previous_on_empty_returns_null);
    TEST(next_on_empty_returns_null);
    TEST(previous_once_returns_last);
    TEST(previous_twice_when_one_returns_first);
}
