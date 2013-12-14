#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include "tools/history.h"

void previous_on_empty_returns_null(void **state)
{
    History history = history_new(10);
    char *item = history_previous(history, "inp");

    assert_null(item);
}

void next_on_empty_returns_null(void **state)
{
    History history = history_new(10);
    char *item = history_next(history, "inp");

    assert_null(item);
}

void previous_once_returns_last(void **state)
{
    History history = history_new(10);
    history_append(history, "Hello");

    char *item = history_previous(history, "inp");

    assert_string_equal("Hello", item);
}

void previous_twice_when_one_returns_first(void **state)
{
    History history = history_new(10);
    history_append(history, "Hello");

    char *item1 = history_previous(history, NULL);
    char *item2 = history_previous(history, item1);

    assert_string_equal("Hello", item2);
}

void previous_always_stops_at_first(void **state)
{
    History history = history_new(10);
    history_append(history, "Hello");

    char *item1 = history_previous(history, NULL);
    char *item2 = history_previous(history, item1);
    char *item3 = history_previous(history, item2);
    char *item4 = history_previous(history, item3);
    char *item5 = history_previous(history, item4);
    char *item6 = history_previous(history, item5);

    assert_string_equal("Hello", item6);
}

void previous_goes_to_correct_element(void **state)
{
    History history = history_new(10);
    history_append(history, "Hello");
    history_append(history, "world");
    history_append(history, "whats");
    history_append(history, "going");
    history_append(history, "on");
    history_append(history, "here");

    char *item1 = history_previous(history, NULL);
    char *item2 = history_previous(history, item1);
    char *item3 = history_previous(history, item2);

    assert_string_equal("going", item3);
}

void prev_then_next_returns_empty(void **state)
{
    History history = history_new(10);
    history_append(history, "Hello");

    char *item1 = history_previous(history, NULL);
    char *item2 = history_next(history, item1);

    assert_string_equal("", item2);
}

void prev_with_val_then_next_returns_val(void **state)
{
    History history = history_new(10);
    history_append(history, "Hello");

    char *item1 = history_previous(history, "Oioi");
    char *item2 = history_next(history, item1);

    assert_string_equal("Oioi", item2);
}

void prev_with_val_then_next_twice_returns_null(void **state)
{
    History history = history_new(10);
    history_append(history, "Hello");

    char *item1 = history_previous(history, "Oioi");
    char *item2 = history_next(history, item1);
    char *item3 = history_next(history, item2);

    assert_null(item3);
}

void navigate_then_append_new(void **state)
{
    History history = history_new(10);
    history_append(history, "Hello");
    history_append(history, "again");
    history_append(history, "testing");
    history_append(history, "history");
    history_append(history, "append");

    char *item1 = history_previous(history, "new text");
    assert_string_equal("append", item1);

    char *item2 = history_previous(history, item1);
    assert_string_equal("history", item2);

    char *item3 = history_previous(history, item2);
    assert_string_equal("testing", item3);

    char *item4 = history_next(history, item3);
    assert_string_equal("history", item4);

    char *item5 = history_next(history, item4);
    assert_string_equal("append", item5);

    char *item6 = history_next(history, item5);
    assert_string_equal("new text", item6);
}

void edit_item_mid_history(void **state)
{
    History history = history_new(10);
    history_append(history, "Hello");
    history_append(history, "again");
    history_append(history, "testing");
    history_append(history, "history");
    history_append(history, "append");

    char *item1 = history_previous(history, "new item");
    assert_string_equal("append", item1);

    char *item2 = history_previous(history, item1);
    assert_string_equal("history", item2);

    char *item3 = history_previous(history, item2);
    assert_string_equal("testing", item3);

    char *item4 = history_previous(history, "EDITED");
    assert_string_equal("again", item4);

    char *item5 = history_previous(history, item4);
    assert_string_equal("Hello", item5);

    char *item6 = history_next(history, item5);
    assert_string_equal("again", item6);

    char *item7 = history_next(history, item6);
    assert_string_equal("EDITED", item7);

    char *item8 = history_next(history, item7);
    assert_string_equal("history", item8);

    char *item9 = history_next(history, item8);
    assert_string_equal("append", item9);

    char *item10 = history_next(history, item9);
    assert_string_equal("new item", item10);
}

void edit_previous_and_append(void **state)
{
    History history = history_new(10);
    history_append(history, "Hello");
    history_append(history, "again");
    history_append(history, "testing");
    history_append(history, "history");
    history_append(history, "append");

    char *item1 = history_previous(history, "new item");
    assert_string_equal("append", item1);

    char *item2 = history_previous(history, item1);
    assert_string_equal("history", item2);

    char *item3 = history_previous(history, item2);
    assert_string_equal("testing", item3);

    history_append(history, "EDITED");

    char *item4 = history_previous(history, NULL);
    assert_string_equal("EDITED", item4);
}

void start_session_add_new_submit_previous(void **state)
{
    History history = history_new(10);
    history_append(history, "hello");

    char *item1 = history_previous(history, NULL);
    assert_string_equal("hello", item1);

    char *item2 = history_next(history, item1);
    assert_string_equal("", item2);

    char *item3 = history_previous(history, "new text");
    assert_string_equal("hello", item3);

    history_append(history, item3);
}
