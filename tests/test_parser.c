#include <stdlib.h>
#include <string.h>
#include <head-unit.h>
#include "tools/parser.h"

void
parse_null_returns_null(void)
{
    char *inp = NULL;
    gchar **result = parse_args(inp, 1, 2);

    assert_is_null(result);
    g_strfreev(result);
}

void
parse_empty_returns_null(void)
{
    char *inp = "";
    gchar **result = parse_args(inp, 1, 2);

    assert_is_null(result);
    g_strfreev(result);
}

void
parse_space_returns_null(void)
{
    char *inp = "   ";
    gchar **result = parse_args(inp, 1, 2);

    assert_is_null(result);
    g_strfreev(result);
}

void
parse_cmd_no_args_returns_null(void)
{
    char *inp = "/cmd";
    gchar **result = parse_args(inp, 1, 2);

    assert_is_null(result);
    g_strfreev(result);
}

void
parse_cmd_with_space_returns_null(void)
{
    char *inp = "/cmd   ";
    gchar **result = parse_args(inp, 1, 2);

    assert_is_null(result);
    g_strfreev(result);
}

void
parse_cmd_with_too_few_returns_null(void)
{
    char *inp = "/cmd arg1";
    gchar **result = parse_args(inp, 2, 3);

    assert_is_null(result);
    g_strfreev(result);
}

void
parse_cmd_with_too_many_returns_null(void)
{
    char *inp = "/cmd arg1 arg2 arg3 arg4";
    gchar **result = parse_args(inp, 1, 3);

    assert_is_null(result);
    g_strfreev(result);
}

void
parse_cmd_one_arg(void)
{
    char *inp = "/cmd arg1";
    gchar **result = parse_args(inp, 1, 2);

    assert_int_equals(1, g_strv_length(result));
    assert_string_equals("arg1", result[0]);
    g_strfreev(result);
}

void
parse_cmd_two_args(void)
{
    char *inp = "/cmd arg1 arg2";
    gchar **result = parse_args(inp, 1, 2);

    assert_int_equals(2, g_strv_length(result));
    assert_string_equals("arg1", result[0]);
    assert_string_equals("arg2", result[1]);
    g_strfreev(result);
}

void
parse_cmd_three_args(void)
{
    char *inp = "/cmd arg1 arg2 arg3";
    gchar **result = parse_args(inp, 3, 3);

    assert_int_equals(3, g_strv_length(result));
    assert_string_equals("arg1", result[0]);
    assert_string_equals("arg2", result[1]);
    assert_string_equals("arg3", result[2]);
    g_strfreev(result);
}

void
parse_cmd_three_args_with_spaces(void)
{
    char *inp = "  /cmd    arg1  arg2     arg3 ";
    gchar **result = parse_args(inp, 3, 3);

    assert_int_equals(3, g_strv_length(result));
    assert_string_equals("arg1", result[0]);
    assert_string_equals("arg2", result[1]);
    assert_string_equals("arg3", result[2]);
    g_strfreev(result);
}

void
parse_cmd_with_freetext(void)
{
    char *inp = "/cmd this is some free text";
    gchar **result = parse_args_with_freetext(inp, 1, 1);

    assert_int_equals(1, g_strv_length(result));
    assert_string_equals("this is some free text", result[0]);
    g_strfreev(result);
}

void
parse_cmd_one_arg_with_freetext(void)
{
    char *inp = "/cmd arg1 this is some free text";
    gchar **result = parse_args_with_freetext(inp, 1, 2);

    assert_int_equals(2, g_strv_length(result));
    assert_string_equals("arg1", result[0]);
    assert_string_equals("this is some free text", result[1]);
    g_strfreev(result);
}

void
parse_cmd_two_args_with_freetext(void)
{
    char *inp = "/cmd arg1 arg2 this is some free text";
    gchar **result = parse_args_with_freetext(inp, 1, 3);

    assert_int_equals(3, g_strv_length(result));
    assert_string_equals("arg1", result[0]);
    assert_string_equals("arg2", result[1]);
    assert_string_equals("this is some free text", result[2]);
    g_strfreev(result);
}

void
parse_cmd_min_zero(void)
{
    char *inp = "/cmd";
    gchar **result = parse_args(inp, 0, 2);

    assert_int_equals(0, g_strv_length(result));
    assert_is_null(result[0]);
    g_strfreev(result);
}

void
parse_cmd_min_zero_with_freetext(void)
{
    char *inp = "/cmd";
    gchar **result = parse_args_with_freetext(inp, 0, 2);

    assert_int_equals(0, g_strv_length(result));
    assert_is_null(result[0]);
    g_strfreev(result);
}

void
parse_cmd_with_quoted(void)
{
    char *inp = "/cmd \"arg1\" arg2";
    gchar **result = parse_args(inp, 2, 2);

    assert_int_equals(2, g_strv_length(result));
    assert_string_equals("arg1", result[0]);
    assert_string_equals("arg2", result[1]);
    g_strfreev(result);
}

void
parse_cmd_with_quoted_and_space(void)
{
    char *inp = "/cmd \"the arg1\" arg2";
    gchar **result = parse_args(inp, 2, 2);

    assert_int_equals(2, g_strv_length(result));
    assert_string_equals("the arg1", result[0]);
    assert_string_equals("arg2", result[1]);
    g_strfreev(result);
}

void
parse_cmd_with_quoted_and_many_spaces(void)
{
    char *inp = "/cmd \"the arg1 is here\" arg2";
    gchar **result = parse_args(inp, 2, 2);

    assert_int_equals(2, g_strv_length(result));
    assert_string_equals("the arg1 is here", result[0]);
    assert_string_equals("arg2", result[1]);
    g_strfreev(result);
}

void
parse_cmd_with_many_quoted_and_many_spaces(void)
{
    char *inp = "/cmd \"the arg1 is here\" \"and arg2 is right here\"";
    gchar **result = parse_args(inp, 2, 2);

    assert_int_equals(2, g_strv_length(result));
    assert_string_equals("the arg1 is here", result[0]);
    assert_string_equals("and arg2 is right here", result[1]);
    g_strfreev(result);
}

void
parse_cmd_freetext_with_quoted(void)
{
    char *inp = "/cmd \"arg1\" arg2 hello there whats up";
    gchar **result = parse_args_with_freetext(inp, 3, 3);

    assert_int_equals(3, g_strv_length(result));
    assert_string_equals("arg1", result[0]);
    assert_string_equals("arg2", result[1]);
    assert_string_equals("hello there whats up", result[2]);
    g_strfreev(result);
}

void
parse_cmd_freetext_with_quoted_and_space(void)
{
    char *inp = "/cmd \"the arg1\" arg2 another bit of freetext";
    gchar **result = parse_args_with_freetext(inp, 3, 3);

    assert_int_equals(3, g_strv_length(result));
    assert_string_equals("the arg1", result[0]);
    assert_string_equals("arg2", result[1]);
    assert_string_equals("another bit of freetext", result[2]);
    g_strfreev(result);
}

void
parse_cmd_freetext_with_quoted_and_many_spaces(void)
{
    char *inp = "/cmd \"the arg1 is here\" arg2 some more freetext";
    gchar **result = parse_args_with_freetext(inp, 3, 3);

    assert_int_equals(3, g_strv_length(result));
    assert_string_equals("the arg1 is here", result[0]);
    assert_string_equals("arg2", result[1]);
    assert_string_equals("some more freetext", result[2]);
    g_strfreev(result);
}

void
parse_cmd_freetext_with_many_quoted_and_many_spaces(void)
{
    char *inp = "/cmd \"the arg1 is here\" \"and arg2 is right here\" and heres the free text";
    gchar **result = parse_args_with_freetext(inp, 3, 3);

    assert_int_equals(3, g_strv_length(result));
    assert_string_equals("the arg1 is here", result[0]);
    assert_string_equals("and arg2 is right here", result[1]);
    assert_string_equals("and heres the free text", result[2]);
    g_strfreev(result);
}

void
parse_cmd_with_quoted_freetext(void)
{
    char *inp = "/cmd arg1 here is \"some\" quoted freetext";
    gchar **result = parse_args_with_freetext(inp, 1, 2);

    assert_int_equals(2, g_strv_length(result));
    assert_string_equals("arg1", result[0]);
    assert_string_equals("here is \"some\" quoted freetext", result[1]);
    g_strfreev(result);
}

void
count_one_token(void)
{
    char *inp = "one";
    int result = count_tokens(inp);

    assert_int_equals(1, result);
}

void
count_one_token_quoted_no_whitespace(void)
{
    char *inp = "\"one\"";
    int result = count_tokens(inp);

    assert_int_equals(1, result);
}

void
count_one_token_quoted_with_whitespace(void)
{
    char *inp = "\"one two\"";
    int result = count_tokens(inp);

    assert_int_equals(1, result);
}

void
count_two_tokens(void)
{
    char *inp = "one two";
    int result = count_tokens(inp);

    assert_int_equals(2, result);
}

void
count_two_tokens_first_quoted(void)
{
    char *inp = "\"one and\" two";
    int result = count_tokens(inp);

    assert_int_equals(2, result);
}

void
count_two_tokens_second_quoted(void)
{
    char *inp = "one \"two and\"";
    int result = count_tokens(inp);

    assert_int_equals(2, result);
}

void
count_two_tokens_both_quoted(void)
{
    char *inp = "\"one and then\" \"two and\"";
    int result = count_tokens(inp);

    assert_int_equals(2, result);
}

void
get_first_of_one(void)
{
    char *inp = "one";
    char *result = get_start(inp, 2);

    assert_string_equals("one", result);
}

void
get_first_of_two(void)
{
    char *inp = "one two";
    char *result = get_start(inp, 2);

    assert_string_equals("one ", result);
}

void
get_first_two_of_three(void)
{
    char *inp = "one two three";
    char *result = get_start(inp, 3);

    assert_string_equals("one two ", result);
}

void
get_first_two_of_three_first_quoted(void)
{
    char *inp = "\"one\" two three";
    char *result = get_start(inp, 3);

    assert_string_equals("\"one\" two ", result);
}

void
get_first_two_of_three_second_quoted(void)
{
    char *inp = "one \"two\" three";
    char *result = get_start(inp, 3);

    assert_string_equals("one \"two\" ", result);
}

void
get_first_two_of_three_first_and_second_quoted(void)
{
    char *inp = "\"one\" \"two\" three";
    char *result = get_start(inp, 3);

    assert_string_equals("\"one\" \"two\" ", result);
}

void
register_parser_tests(void)
{
    TEST_MODULE("parser tests");
    TEST(parse_null_returns_null);
    TEST(parse_empty_returns_null);
    TEST(parse_space_returns_null);
    TEST(parse_cmd_no_args_returns_null);
    TEST(parse_cmd_with_space_returns_null);
    TEST(parse_cmd_one_arg);
    TEST(parse_cmd_two_args);
    TEST(parse_cmd_three_args);
    TEST(parse_cmd_three_args_with_spaces);
    TEST(parse_cmd_with_freetext);
    TEST(parse_cmd_one_arg_with_freetext);
    TEST(parse_cmd_two_args_with_freetext);
    TEST(parse_cmd_with_too_few_returns_null);
    TEST(parse_cmd_with_too_many_returns_null);
    TEST(parse_cmd_min_zero);
    TEST(parse_cmd_min_zero_with_freetext);
    TEST(parse_cmd_with_quoted);
    TEST(parse_cmd_with_quoted_and_space);
    TEST(parse_cmd_with_quoted_and_many_spaces);
    TEST(parse_cmd_with_many_quoted_and_many_spaces);
    TEST(parse_cmd_freetext_with_quoted);
    TEST(parse_cmd_freetext_with_quoted_and_space);
    TEST(parse_cmd_freetext_with_quoted_and_many_spaces);
    TEST(parse_cmd_freetext_with_many_quoted_and_many_spaces);
    TEST(parse_cmd_with_quoted_freetext);
    TEST(count_one_token);
    TEST(count_one_token_quoted_no_whitespace);
    TEST(count_one_token_quoted_with_whitespace);
    TEST(count_two_tokens);
    TEST(count_two_tokens_first_quoted);
    TEST(count_two_tokens_second_quoted);
    TEST(count_two_tokens_both_quoted);
    TEST(get_first_of_one);
    TEST(get_first_of_two);
    TEST(get_first_two_of_three);
    TEST(get_first_two_of_three_first_quoted);
    TEST(get_first_two_of_three_second_quoted);
    TEST(get_first_two_of_three_first_and_second_quoted);
}
