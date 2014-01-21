#include "common.h"
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

void replace_one_substr(void **state)
{
    char *string = "it is a string";
    char *sub = "is";
    char *new = "was";

    char *result = str_replace(string, sub, new);

    assert_string_equal("it was a string", result);

    free(result);
}

void replace_one_substr_beginning(void **state)
{
    char *string = "it is a string";
    char *sub = "it";
    char *new = "that";

    char *result = str_replace(string, sub, new);

    assert_string_equal("that is a string", result);

    free(result);
}

void replace_one_substr_end(void **state)
{
    char *string = "it is a string";
    char *sub = "string";
    char *new = "thing";

    char *result = str_replace(string, sub, new);

    assert_string_equal("it is a thing", result);

    free(result);
}

void replace_two_substr(void **state)
{
    char *string = "it is a is string";
    char *sub = "is";
    char *new = "was";

    char *result = str_replace(string, sub, new);

    assert_string_equal("it was a was string", result);

    free(result);
}

void replace_char(void **state)
{
    char *string = "some & a thing & something else";
    char *sub = "&";
    char *new = "&amp;";

    char *result = str_replace(string, sub, new);

    assert_string_equal("some &amp; a thing &amp; something else", result);

    free(result);
}

void replace_when_none(void **state)
{
    char *string = "its another string";
    char *sub = "haha";
    char *new = "replaced";

    char *result = str_replace(string, sub, new);

    assert_string_equal("its another string", result);

    free(result);
}

void replace_when_match(void **state)
{
    char *string = "hello";
    char *sub = "hello";
    char *new = "goodbye";

    char *result = str_replace(string, sub, new);

    assert_string_equal("goodbye", result);

    free(result);
}

void replace_when_string_empty(void **state)
{
    char *string = "";
    char *sub = "hello";
    char *new = "goodbye";

    char *result = str_replace(string, sub, new);

    assert_string_equal("", result);

    free(result);
}

void replace_when_string_null(void **state)
{
    char *string = NULL;
    char *sub = "hello";
    char *new = "goodbye";

    char *result = str_replace(string, sub, new);

    assert_null(result);
}

void replace_when_sub_empty(void **state)
{
    char *string = "hello";
    char *sub = "";
    char *new = "goodbye";

    char *result = str_replace(string, sub, new);

    assert_string_equal("hello", result);

    free(result);
}

void replace_when_sub_null(void **state)
{
    char *string = "hello";
    char *sub = NULL;
    char *new = "goodbye";

    char *result = str_replace(string, sub, new);

    assert_string_equal("hello", result);

    free(result);
}

void replace_when_new_empty(void **state)
{
    char *string = "hello";
    char *sub = "hello";
    char *new = "";

    char *result = str_replace(string, sub, new);

    assert_string_equal("", result);

    free(result);
}

void replace_when_new_null(void **state)
{
    char *string = "hello";
    char *sub = "hello";
    char *new = NULL;

    char *result = str_replace(string, sub, new);

    assert_string_equal("hello", result);

    free(result);
}

void compare_win_nums_less(void **state)
{
    gconstpointer a = GINT_TO_POINTER(2);
    gconstpointer b = GINT_TO_POINTER(3);

    int result = cmp_win_num(a, b);

    assert_true(result < 0);
}

void compare_win_nums_equal(void **state)
{
    gconstpointer a = GINT_TO_POINTER(5);
    gconstpointer b = GINT_TO_POINTER(5);

    int result = cmp_win_num(a, b);

    assert_true(result == 0);
}

void compare_win_nums_greater(void **state)
{
    gconstpointer a = GINT_TO_POINTER(7);
    gconstpointer b = GINT_TO_POINTER(6);

    int result = cmp_win_num(a, b);

    assert_true(result > 0);
}

void compare_0s_equal(void **state)
{
    gconstpointer a = GINT_TO_POINTER(0);
    gconstpointer b = GINT_TO_POINTER(0);

    int result = cmp_win_num(a, b);

    assert_true(result == 0);
}

void compare_0_greater_than_1(void **state)
{
    gconstpointer a = GINT_TO_POINTER(0);
    gconstpointer b = GINT_TO_POINTER(1);

    int result = cmp_win_num(a, b);

    assert_true(result > 0);
}

void compare_1_less_than_0(void **state)
{
    gconstpointer a = GINT_TO_POINTER(1);
    gconstpointer b = GINT_TO_POINTER(0);

    int result = cmp_win_num(a, b);

    assert_true(result < 0);
}

void compare_0_less_than_11(void **state)
{
    gconstpointer a = GINT_TO_POINTER(0);
    gconstpointer b = GINT_TO_POINTER(11);

    int result = cmp_win_num(a, b);

    assert_true(result < 0);
}

void compare_11_greater_than_0(void **state)
{
    gconstpointer a = GINT_TO_POINTER(11);
    gconstpointer b = GINT_TO_POINTER(0);

    int result = cmp_win_num(a, b);

    assert_true(result > 0);
}

void compare_0_greater_than_9(void **state)
{
    gconstpointer a = GINT_TO_POINTER(0);
    gconstpointer b = GINT_TO_POINTER(9);

    int result = cmp_win_num(a, b);

    assert_true(result > 0);
}

void compare_9_less_than_0(void **state)
{
    gconstpointer a = GINT_TO_POINTER(9);
    gconstpointer b = GINT_TO_POINTER(0);

    int result = cmp_win_num(a, b);

    assert_true(result < 0);
}

void next_available_when_only_console(void **state)
{
    GList *used = NULL;
    used = g_list_append(used, GINT_TO_POINTER(1));

    int result = get_next_available_win_num(used);

    assert_int_equal(2, result);
}

void next_available_3_at_end(void **state)
{
    GList *used = NULL;
    used = g_list_append(used, GINT_TO_POINTER(1));
    used = g_list_append(used, GINT_TO_POINTER(2));

    int result = get_next_available_win_num(used);

    assert_int_equal(3, result);
}

void next_available_9_at_end(void **state)
{
    GList *used = NULL;
    used = g_list_append(used, GINT_TO_POINTER(1));
    used = g_list_append(used, GINT_TO_POINTER(2));
    used = g_list_append(used, GINT_TO_POINTER(3));
    used = g_list_append(used, GINT_TO_POINTER(4));
    used = g_list_append(used, GINT_TO_POINTER(5));
    used = g_list_append(used, GINT_TO_POINTER(6));
    used = g_list_append(used, GINT_TO_POINTER(7));
    used = g_list_append(used, GINT_TO_POINTER(8));

    int result = get_next_available_win_num(used);

    assert_int_equal(9, result);
}

void next_available_0_at_end(void **state)
{
    GList *used = NULL;
    used = g_list_append(used, GINT_TO_POINTER(1));
    used = g_list_append(used, GINT_TO_POINTER(2));
    used = g_list_append(used, GINT_TO_POINTER(3));
    used = g_list_append(used, GINT_TO_POINTER(4));
    used = g_list_append(used, GINT_TO_POINTER(5));
    used = g_list_append(used, GINT_TO_POINTER(6));
    used = g_list_append(used, GINT_TO_POINTER(7));
    used = g_list_append(used, GINT_TO_POINTER(8));
    used = g_list_append(used, GINT_TO_POINTER(9));

    int result = get_next_available_win_num(used);

    assert_int_equal(0, result);
}

void next_available_2_in_first_gap(void **state)
{
    GList *used = NULL;
    used = g_list_append(used, GINT_TO_POINTER(1));
    used = g_list_append(used, GINT_TO_POINTER(3));
    used = g_list_append(used, GINT_TO_POINTER(4));
    used = g_list_append(used, GINT_TO_POINTER(5));
    used = g_list_append(used, GINT_TO_POINTER(9));
    used = g_list_append(used, GINT_TO_POINTER(0));

    int result = get_next_available_win_num(used);

    assert_int_equal(2, result);
}

void next_available_9_in_first_gap(void **state)
{
    GList *used = NULL;
    used = g_list_append(used, GINT_TO_POINTER(1));
    used = g_list_append(used, GINT_TO_POINTER(2));
    used = g_list_append(used, GINT_TO_POINTER(3));
    used = g_list_append(used, GINT_TO_POINTER(4));
    used = g_list_append(used, GINT_TO_POINTER(5));
    used = g_list_append(used, GINT_TO_POINTER(6));
    used = g_list_append(used, GINT_TO_POINTER(7));
    used = g_list_append(used, GINT_TO_POINTER(8));
    used = g_list_append(used, GINT_TO_POINTER(0));
    used = g_list_append(used, GINT_TO_POINTER(11));
    used = g_list_append(used, GINT_TO_POINTER(12));
    used = g_list_append(used, GINT_TO_POINTER(13));
    used = g_list_append(used, GINT_TO_POINTER(20));

    int result = get_next_available_win_num(used);

    assert_int_equal(9, result);
}

void next_available_0_in_first_gap(void **state)
{
    GList *used = NULL;
    used = g_list_append(used, GINT_TO_POINTER(1));
    used = g_list_append(used, GINT_TO_POINTER(2));
    used = g_list_append(used, GINT_TO_POINTER(3));
    used = g_list_append(used, GINT_TO_POINTER(4));
    used = g_list_append(used, GINT_TO_POINTER(5));
    used = g_list_append(used, GINT_TO_POINTER(6));
    used = g_list_append(used, GINT_TO_POINTER(7));
    used = g_list_append(used, GINT_TO_POINTER(8));
    used = g_list_append(used, GINT_TO_POINTER(9));
    used = g_list_append(used, GINT_TO_POINTER(11));
    used = g_list_append(used, GINT_TO_POINTER(12));
    used = g_list_append(used, GINT_TO_POINTER(13));
    used = g_list_append(used, GINT_TO_POINTER(20));

    int result = get_next_available_win_num(used);

    assert_int_equal(0, result);
}

void next_available_11_in_first_gap(void **state)
{
    GList *used = NULL;
    used = g_list_append(used, GINT_TO_POINTER(1));
    used = g_list_append(used, GINT_TO_POINTER(2));
    used = g_list_append(used, GINT_TO_POINTER(3));
    used = g_list_append(used, GINT_TO_POINTER(4));
    used = g_list_append(used, GINT_TO_POINTER(5));
    used = g_list_append(used, GINT_TO_POINTER(6));
    used = g_list_append(used, GINT_TO_POINTER(7));
    used = g_list_append(used, GINT_TO_POINTER(8));
    used = g_list_append(used, GINT_TO_POINTER(9));
    used = g_list_append(used, GINT_TO_POINTER(0));
    used = g_list_append(used, GINT_TO_POINTER(12));
    used = g_list_append(used, GINT_TO_POINTER(13));
    used = g_list_append(used, GINT_TO_POINTER(20));

    int result = get_next_available_win_num(used);

    assert_int_equal(11, result);
}

void next_available_24_first_big_gap(void **state)
{
    GList *used = NULL;
    used = g_list_append(used, GINT_TO_POINTER(1));
    used = g_list_append(used, GINT_TO_POINTER(2));
    used = g_list_append(used, GINT_TO_POINTER(3));
    used = g_list_append(used, GINT_TO_POINTER(4));
    used = g_list_append(used, GINT_TO_POINTER(5));
    used = g_list_append(used, GINT_TO_POINTER(6));
    used = g_list_append(used, GINT_TO_POINTER(7));
    used = g_list_append(used, GINT_TO_POINTER(8));
    used = g_list_append(used, GINT_TO_POINTER(9));
    used = g_list_append(used, GINT_TO_POINTER(0));
    used = g_list_append(used, GINT_TO_POINTER(11));
    used = g_list_append(used, GINT_TO_POINTER(12));
    used = g_list_append(used, GINT_TO_POINTER(13));
    used = g_list_append(used, GINT_TO_POINTER(14));
    used = g_list_append(used, GINT_TO_POINTER(15));
    used = g_list_append(used, GINT_TO_POINTER(16));
    used = g_list_append(used, GINT_TO_POINTER(17));
    used = g_list_append(used, GINT_TO_POINTER(18));
    used = g_list_append(used, GINT_TO_POINTER(19));
    used = g_list_append(used, GINT_TO_POINTER(20));
    used = g_list_append(used, GINT_TO_POINTER(21));
    used = g_list_append(used, GINT_TO_POINTER(22));
    used = g_list_append(used, GINT_TO_POINTER(23));
    used = g_list_append(used, GINT_TO_POINTER(51));
    used = g_list_append(used, GINT_TO_POINTER(52));
    used = g_list_append(used, GINT_TO_POINTER(53));
    used = g_list_append(used, GINT_TO_POINTER(89));
    used = g_list_append(used, GINT_TO_POINTER(90));
    used = g_list_append(used, GINT_TO_POINTER(100));
    used = g_list_append(used, GINT_TO_POINTER(101));
    used = g_list_append(used, GINT_TO_POINTER(102));

    int result = get_next_available_win_num(used);

    assert_int_equal(24, result);
}

void test_online_is_valid_resource_presence_string(void **state)
{
    assert_true(valid_resource_presence_string("online"));
}

void test_chat_is_valid_resource_presence_string(void **state)
{
    assert_true(valid_resource_presence_string("chat"));
}

void test_away_is_valid_resource_presence_string(void **state)
{
    assert_true(valid_resource_presence_string("away"));
}

void test_xa_is_valid_resource_presence_string(void **state)
{
    assert_true(valid_resource_presence_string("xa"));
}

void test_dnd_is_valid_resource_presence_string(void **state)
{
    assert_true(valid_resource_presence_string("dnd"));
}

void test_available_is_not_valid_resource_presence_string(void **state)
{
    assert_false(valid_resource_presence_string("available"));
}

void test_unavailable_is_not_valid_resource_presence_string(void **state)
{
    assert_false(valid_resource_presence_string("unavailable"));
}

void test_blah_is_not_valid_resource_presence_string(void **state)
{
    assert_false(valid_resource_presence_string("blah"));
}
