#include <stdio.h>
#include <head-unit.h>
#include "prof_history.h"

void previous_on_empty_returns_null(void)
{
    PHistory history = p_history_new(10);
    char *item = p_history_previous(history, "inp");

    assert_is_null(item);
}

void next_on_empty_returns_null(void)
{
    PHistory history = p_history_new(10);
    char *item = p_history_next(history, "inp");

    assert_is_null(item);
}

void previous_once_returns_last(void)
{
    PHistory history = p_history_new(10);
    p_history_append(history, "Hello");

    char *item = p_history_previous(history, "inp");

    assert_string_equals("Hello", item);
}

void previous_twice_when_one_returns_first(void)
{
    PHistory history = p_history_new(10);
    p_history_append(history, "Hello");

    char *item1 = p_history_previous(history, NULL);
    char *item2 = p_history_previous(history, item1);

    assert_string_equals("Hello", item2);
}

void previous_always_stops_at_first(void)
{
    PHistory history = p_history_new(10);
    p_history_append(history, "Hello");

    char *item1 = p_history_previous(history, NULL);
    char *item2 = p_history_previous(history, item1);
    char *item3 = p_history_previous(history, item2);
    char *item4 = p_history_previous(history, item3);
    char *item5 = p_history_previous(history, item4);
    char *item6 = p_history_previous(history, item5);

    assert_string_equals("Hello", item6);
}

void previous_goes_to_correct_element(void)
{
    PHistory history = p_history_new(10);
    p_history_append(history, "Hello");
    p_history_append(history, "world");
    p_history_append(history, "whats");
    p_history_append(history, "going");
    p_history_append(history, "on");
    p_history_append(history, "here");

    char *item1 = p_history_previous(history, NULL);
    char *item2 = p_history_previous(history, item1);
    char *item3 = p_history_previous(history, item2);

    assert_string_equals("going", item3);
}

void prev_then_next_returns_empty(void)
{
    PHistory history = p_history_new(10);
    p_history_append(history, "Hello");

    char *item1 = p_history_previous(history, NULL);
    char *item2 = p_history_next(history, item1);

    assert_string_equals("", item2);
}

void prev_with_val_then_next_returns_val(void)
{
    PHistory history = p_history_new(10);
    p_history_append(history, "Hello");

    char *item1 = p_history_previous(history, "Oioi");
    char *item2 = p_history_next(history, item1);

    assert_string_equals("Oioi", item2);
}

void prev_with_val_then_next_twice_returns_val(void)
{
    PHistory history = p_history_new(10);
    p_history_append(history, "Hello");

    char *item1 = p_history_previous(history, "Oioi");
    char *item2 = p_history_next(history, item1);
    char *item3 = p_history_next(history, item2);

    assert_string_equals("Oioi", item3);
}

void navigate_then_append_new(void)
{
    PHistory history = p_history_new(10);
    p_history_append(history, "Hello");
    p_history_append(history, "again");
    p_history_append(history, "testing");
    p_history_append(history, "history");
    p_history_append(history, "append");

    char *item1 = p_history_previous(history, "new text");
    assert_string_equals("append", item1);

    char *item2 = p_history_previous(history, item1);
    assert_string_equals("history", item2);

    char *item3 = p_history_previous(history, item2);
    assert_string_equals("testing", item3);

    char *item4 = p_history_next(history, item3);
    assert_string_equals("history", item4);

    char *item5 = p_history_next(history, item4);
    assert_string_equals("append", item5);

    char *item6 = p_history_next(history, item5);
    assert_string_equals("new text", item6);
}

void edit_item_mid_history(void)
{
    PHistory history = p_history_new(10);
    p_history_append(history, "Hello");
    p_history_append(history, "again");
    p_history_append(history, "testing");
    p_history_append(history, "history");
    p_history_append(history, "append");

    char *item1 = p_history_previous(history, "new item");
    assert_string_equals("append", item1);

    char *item2 = p_history_previous(history, item1);
    assert_string_equals("history", item2);

    char *item3 = p_history_previous(history, item2);
    assert_string_equals("testing", item3);

    char *item4 = p_history_previous(history, "EDITED");
    assert_string_equals("again", item4);

    char *item5 = p_history_previous(history, item4);
    assert_string_equals("Hello", item5);

    char *item6 = p_history_next(history, item5);
    assert_string_equals("again", item6);

    char *item7 = p_history_next(history, item6);
    assert_string_equals("EDITED", item7);

    char *item8 = p_history_next(history, item7);
    assert_string_equals("history", item8);

    char *item9 = p_history_next(history, item8);
    assert_string_equals("append", item9);

    char *item10 = p_history_next(history, item9);
    assert_string_equals("new item", item10);
}

void edit_previous_and_append(void)
{
    PHistory history = p_history_new(10);
    p_history_append(history, "Hello");
    p_history_append(history, "again");
    p_history_append(history, "testing");
    p_history_append(history, "history");
    p_history_append(history, "append");

    char *item1 = p_history_previous(history, "new item");
    assert_string_equals("append", item1);

    char *item2 = p_history_previous(history, item1);
    assert_string_equals("history", item2);

    char *item3 = p_history_previous(history, item2);
    assert_string_equals("testing", item3);

    p_history_append(history, "EDITED");

    char *item4 = p_history_previous(history, NULL);
    assert_string_equals("EDITED", item4);
}

void start_session_add_new_submit_previous(void)
{
    PHistory history = p_history_new(10);
    p_history_append(history, "hello");

    char *item1 = p_history_previous(history, NULL);
    assert_string_equals("hello", item1);

    char *item2 = p_history_next(history, item1);
    assert_string_equals("", item2);

    char *item3 = p_history_previous(history, "new text");
    assert_string_equals("hello", item3);

    p_history_append(history, item3);
}

void register_prof_history_tests(void)
{
    TEST_MODULE("prof_history tests");
    TEST(previous_on_empty_returns_null);
    TEST(next_on_empty_returns_null);
    TEST(previous_once_returns_last);
    TEST(previous_twice_when_one_returns_first);
    TEST(previous_always_stops_at_first);
    TEST(previous_goes_to_correct_element);
    TEST(prev_then_next_returns_empty);
    TEST(prev_with_val_then_next_returns_val);
    TEST(prev_with_val_then_next_twice_returns_val);
    TEST(navigate_then_append_new);
    TEST(edit_item_mid_history);
    TEST(edit_previous_and_append);
    TEST(start_session_add_new_submit_previous);
}
