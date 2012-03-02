#include <stdlib.h>
#include <head-unit.h>
#include "history.h"

static void beforetest(void)
{
    history_init();
}

static void previous_returns_null_after_init(void)
{
    char *prev = history_previous();
    assert_true(prev == NULL);
}

static void next_returns_null_after_init(void)
{
    char *next = history_next();
    assert_true(next == NULL);
}

static void append_after_init_doesnt_fail(void)
{
    history_append("try append");
    assert_true(1);
}

static void append_then_previous_returns_appended(void)
{
    history_append("try append");
    char *prev = history_previous();
    assert_string_equals(prev, "try append"); 
}

static void append_then_next_returns_null(void)
{
    history_append("try append");
    char *next = history_next();
    assert_true(next == NULL);
}

static void hits_null_at_top(void)
{
    history_append("cmd1");
    history_append("cmd2");
    history_previous(); // cmd2
    history_previous(); // cmd1
    char *prev = history_previous();
    assert_true(prev == NULL);
}

static void navigate_to_correct_item(void)
{
    history_append("cmd1");
    history_append("cmd2");
    history_append("cmd3");
    history_append("cmd4");
    history_append("cmd5");
    history_append("cmd6");

    history_previous(); // cmd6
    history_previous(); // cmd5
    history_previous(); // cmd4
    history_previous(); // cmd3
    history_next(); // cmd4
    history_previous(); // cmd3
    history_previous(); // cmd2
    char *str = history_next(); // cmd3

    assert_string_equals(str, "cmd3");
}

static void append_previous_item(void)
{
    history_append("cmd1");
    history_append("cmd2");
    history_append("cmd3");
    history_append("cmd4");
    history_append("cmd5");
    history_append("cmd6");

    history_previous(); // cmd6
    history_previous(); // cmd5
    history_previous(); // cmd4
    history_previous(); // cmd3
    history_next(); // cmd4
    history_previous(); // cmd3
    history_previous(); // cmd2
    char *str = history_next(); // cmd3

    history_append(str);

    char *cmd3_1 = history_previous();
    assert_string_equals(cmd3_1, "cmd3");

    char *cmd6 = history_previous();
    assert_string_equals(cmd6, "cmd6");

    char *cmd5 = history_previous();
    assert_string_equals(cmd5, "cmd5");

    char *cmd4 = history_previous();
    assert_string_equals(cmd4, "cmd4");

    char *cmd3 = history_previous();
    assert_string_equals(cmd3, "cmd3");

    char *cmd2 = history_previous();
    assert_string_equals(cmd2, "cmd2");

    char *cmd1 = history_previous();
    assert_string_equals(cmd1, "cmd1");

    char *end = history_previous();
    assert_true(end == NULL);
}

void register_history_tests(void)
{
    TEST_MODULE("history tests");
    BEFORETEST(beforetest);
    TEST(previous_returns_null_after_init);
    TEST(next_returns_null_after_init);
    TEST(append_after_init_doesnt_fail);
    TEST(append_then_previous_returns_appended);
    TEST(append_then_next_returns_null);
    TEST(hits_null_at_top);
    TEST(navigate_to_correct_item);
    TEST(append_previous_item);
}
