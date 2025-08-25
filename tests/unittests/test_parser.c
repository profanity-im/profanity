#include "prof_cmocka.h"
#include <stdlib.h>

#include "tools/parser.h"

void
parse_null_returns_null(void** state)
{
    char* inp = NULL;
    gboolean result = TRUE;
    gchar** args = parse_args(inp, 1, 2, &result);

    assert_false(result);
    assert_null(args);
    g_strfreev(args);
}

void
parse_empty_returns_null(void** state)
{
    char* inp = "";
    gboolean result = TRUE;
    gchar** args = parse_args(inp, 1, 2, &result);

    assert_false(result);
    assert_null(args);
    g_strfreev(args);
}

void
parse_space_returns_null(void** state)
{
    char* inp = "   ";
    gboolean result = TRUE;
    gchar** args = parse_args(inp, 1, 2, &result);

    assert_false(result);
    assert_null(args);
    g_strfreev(args);
}

void
parse_cmd_no_args_returns_null(void** state)
{
    char* inp = "/cmd";
    gboolean result = TRUE;
    gchar** args = parse_args(inp, 1, 2, &result);

    assert_false(result);
    assert_null(args);
    g_strfreev(args);
}

void
parse_cmd_with_space_returns_null(void** state)
{
    char* inp = "/cmd   ";
    gboolean result = TRUE;
    gchar** args = parse_args(inp, 1, 2, &result);

    assert_false(result);
    assert_null(args);
    g_strfreev(args);
}

void
parse_cmd_with_too_few_returns_null(void** state)
{
    char* inp = "/cmd arg1";
    gboolean result = TRUE;
    gchar** args = parse_args(inp, 2, 3, &result);

    assert_false(result);
    assert_null(args);
    g_strfreev(args);
}

void
parse_cmd_with_too_many_returns_null(void** state)
{
    char* inp = "/cmd arg1 arg2 arg3 arg4";
    gboolean result = TRUE;
    gchar** args = parse_args(inp, 1, 3, &result);

    assert_false(result);
    assert_null(args);
    g_strfreev(args);
}

void
parse_cmd_one_arg(void** state)
{
    char* inp = "/cmd arg1";
    gboolean result = FALSE;
    gchar** args = parse_args(inp, 1, 2, &result);

    assert_true(result);
    assert_int_equal(1, g_strv_length(args));
    assert_string_equal("arg1", args[0]);
    g_strfreev(args);
}

void
parse_cmd_two_args(void** state)
{
    char* inp = "/cmd arg1 arg2";
    gboolean result = FALSE;
    gchar** args = parse_args(inp, 1, 2, &result);

    assert_true(result);
    assert_int_equal(2, g_strv_length(args));
    assert_string_equal("arg1", args[0]);
    assert_string_equal("arg2", args[1]);
    g_strfreev(args);
}

void
parse_cmd_three_args(void** state)
{
    char* inp = "/cmd arg1 arg2 arg3";
    gboolean result = FALSE;
    gchar** args = parse_args(inp, 3, 3, &result);

    assert_true(result);
    assert_int_equal(3, g_strv_length(args));
    assert_string_equal("arg1", args[0]);
    assert_string_equal("arg2", args[1]);
    assert_string_equal("arg3", args[2]);
    g_strfreev(args);
}

void
parse_cmd_three_args_with_spaces(void** state)
{
    char* inp = "  /cmd    arg1  arg2     arg3 ";
    gboolean result = FALSE;
    gchar** args = parse_args(inp, 3, 3, &result);

    assert_true(result);
    assert_int_equal(3, g_strv_length(args));
    assert_string_equal("arg1", args[0]);
    assert_string_equal("arg2", args[1]);
    assert_string_equal("arg3", args[2]);
    g_strfreev(args);
}

void
parse_cmd_with_freetext(void** state)
{
    char* inp = "/cmd this is some free text";
    gboolean result = FALSE;
    gchar** args = parse_args_with_freetext(inp, 1, 1, &result);

    assert_true(result);
    assert_int_equal(1, g_strv_length(args));
    assert_string_equal("this is some free text", args[0]);
    g_strfreev(args);
}

void
parse_cmd_one_arg_with_freetext(void** state)
{
    char* inp = "/cmd arg1 this is some free text";
    gboolean result = FALSE;
    gchar** args = parse_args_with_freetext(inp, 1, 2, &result);

    assert_true(result);
    assert_int_equal(2, g_strv_length(args));
    assert_string_equal("arg1", args[0]);
    assert_string_equal("this is some free text", args[1]);
    g_strfreev(args);
}

void
parse_cmd_two_args_with_freetext(void** state)
{
    char* inp = "/cmd arg1 arg2 this is some free text";
    gboolean result = FALSE;
    gchar** args = parse_args_with_freetext(inp, 1, 3, &result);

    assert_true(result);
    assert_int_equal(3, g_strv_length(args));
    assert_string_equal("arg1", args[0]);
    assert_string_equal("arg2", args[1]);
    assert_string_equal("this is some free text", args[2]);
    g_strfreev(args);
}

void
parse_cmd_min_zero(void** state)
{
    char* inp = "/cmd";
    gboolean result = FALSE;
    gchar** args = parse_args(inp, 0, 2, &result);

    assert_true(result);
    assert_int_equal(0, g_strv_length(args));
    assert_null(args[0]);
    g_strfreev(args);
}

void
parse_cmd_min_zero_with_freetext(void** state)
{
    char* inp = "/cmd";
    gboolean result = FALSE;
    gchar** args = parse_args_with_freetext(inp, 0, 2, &result);

    assert_true(result);
    assert_int_equal(0, g_strv_length(args));
    assert_null(args[0]);
    g_strfreev(args);
}

void
parse_cmd_with_quoted(void** state)
{
    char* inp = "/cmd \"arg1\" arg2";
    gboolean result = FALSE;
    gchar** args = parse_args(inp, 2, 2, &result);

    assert_true(result);
    assert_int_equal(2, g_strv_length(args));
    assert_string_equal("arg1", args[0]);
    assert_string_equal("arg2", args[1]);
    g_strfreev(args);
}

void
parse_cmd_with_quoted_and_space(void** state)
{
    char* inp = "/cmd \"the arg1\" arg2";
    gboolean result = FALSE;
    gchar** args = parse_args(inp, 2, 2, &result);

    assert_true(result);
    assert_int_equal(2, g_strv_length(args));
    assert_string_equal("the arg1", args[0]);
    assert_string_equal("arg2", args[1]);
    g_strfreev(args);
}

void
parse_cmd_with_quoted_and_many_spaces(void** state)
{
    char* inp = "/cmd \"the arg1 is here\" arg2";
    gboolean result = FALSE;
    gchar** args = parse_args(inp, 2, 2, &result);

    assert_true(result);
    assert_int_equal(2, g_strv_length(args));
    assert_string_equal("the arg1 is here", args[0]);
    assert_string_equal("arg2", args[1]);
    g_strfreev(args);
}

void
parse_cmd_with_many_quoted_and_many_spaces(void** state)
{
    char* inp = "/cmd \"the arg1 is here\" \"and arg2 is right here\"";
    gboolean result = FALSE;
    gchar** args = parse_args(inp, 2, 2, &result);

    assert_true(result);
    assert_int_equal(2, g_strv_length(args));
    assert_string_equal("the arg1 is here", args[0]);
    assert_string_equal("and arg2 is right here", args[1]);
    g_strfreev(args);
}

void
parse_cmd_freetext_with_quoted(void** state)
{
    char* inp = "/cmd \"arg1\" arg2 hello there what's up";
    gboolean result = FALSE;
    gchar** args = parse_args_with_freetext(inp, 3, 3, &result);

    assert_true(result);
    assert_int_equal(3, g_strv_length(args));
    assert_string_equal("arg1", args[0]);
    assert_string_equal("arg2", args[1]);
    assert_string_equal("hello there what's up", args[2]);
    g_strfreev(args);
}

void
parse_cmd_freetext_with_quoted_and_space(void** state)
{
    char* inp = "/cmd \"the arg1\" arg2 another bit of freetext";
    gboolean result = FALSE;
    gchar** args = parse_args_with_freetext(inp, 3, 3, &result);

    assert_true(result);
    assert_int_equal(3, g_strv_length(args));
    assert_string_equal("the arg1", args[0]);
    assert_string_equal("arg2", args[1]);
    assert_string_equal("another bit of freetext", args[2]);
    g_strfreev(args);
}

void
parse_cmd_freetext_with_quoted_and_many_spaces(void** state)
{
    char* inp = "/cmd \"the arg1 is here\" arg2 some more freetext";
    gboolean result = FALSE;
    gchar** args = parse_args_with_freetext(inp, 3, 3, &result);

    assert_true(result);
    assert_int_equal(3, g_strv_length(args));
    assert_string_equal("the arg1 is here", args[0]);
    assert_string_equal("arg2", args[1]);
    assert_string_equal("some more freetext", args[2]);
    g_strfreev(args);
}

void
parse_cmd_freetext_with_many_quoted_and_many_spaces(void** state)
{
    char* inp = "/cmd \"the arg1 is here\" \"and arg2 is right here\" and heres the free text";
    gboolean result = FALSE;
    gchar** args = parse_args_with_freetext(inp, 3, 3, &result);

    assert_true(result);
    assert_int_equal(3, g_strv_length(args));
    assert_string_equal("the arg1 is here", args[0]);
    assert_string_equal("and arg2 is right here", args[1]);
    assert_string_equal("and heres the free text", args[2]);
    g_strfreev(args);
}

void
parse_cmd_with_quoted_freetext(void** state)
{
    char* inp = "/cmd arg1 here is \"some\" quoted freetext";
    gboolean result = FALSE;
    gchar** args = parse_args_with_freetext(inp, 1, 2, &result);

    assert_true(result);
    assert_int_equal(2, g_strv_length(args));
    assert_string_equal("arg1", args[0]);
    assert_string_equal("here is \"some\" quoted freetext", args[1]);
    g_strfreev(args);
}

void
parse_cmd_with_third_arg_quoted_0_min_3_max(void** state)
{
    char* inp = "/group add friends \"The User\"";
    gboolean result = FALSE;
    gchar** args = parse_args_with_freetext(inp, 0, 3, &result);

    assert_true(result);
    assert_int_equal(3, g_strv_length(args));
    assert_string_equal("add", args[0]);
    assert_string_equal("friends", args[1]);
    assert_string_equal("The User", args[2]);

    g_strfreev(args);
}

void
parse_cmd_with_second_arg_quoted_0_min_3_max(void** state)
{
    char* inp = "/group add \"The Group\" friend";
    gboolean result = FALSE;
    gchar** args = parse_args_with_freetext(inp, 0, 3, &result);

    assert_true(result);
    assert_int_equal(3, g_strv_length(args));
    assert_string_equal("add", args[0]);
    assert_string_equal("The Group", args[1]);
    assert_string_equal("friend", args[2]);

    g_strfreev(args);
}

void
parse_cmd_with_second_and_third_arg_quoted_0_min_3_max(void** state)
{
    char* inp = "/group add \"The Group\" \"The User\"";
    gboolean result = FALSE;
    gchar** args = parse_args_with_freetext(inp, 0, 3, &result);

    assert_true(result);
    assert_int_equal(3, g_strv_length(args));
    assert_string_equal("add", args[0]);
    assert_string_equal("The Group", args[1]);
    assert_string_equal("The User", args[2]);

    g_strfreev(args);
}

void
count_one_token(void** state)
{
    char* inp = "one";
    int result = count_tokens(inp);

    assert_int_equal(1, result);
}

void
count_one_token_quoted_no_whitespace(void** state)
{
    char* inp = "\"one\"";
    int result = count_tokens(inp);

    assert_int_equal(1, result);
}

void
count_one_token_quoted_with_whitespace(void** state)
{
    char* inp = "\"one two\"";
    int result = count_tokens(inp);

    assert_int_equal(1, result);
}

void
count_two_tokens(void** state)
{
    char* inp = "one two";
    int result = count_tokens(inp);

    assert_int_equal(2, result);
}

void
count_two_tokens_first_quoted(void** state)
{
    char* inp = "\"one and\" two";
    int result = count_tokens(inp);

    assert_int_equal(2, result);
}

void
count_two_tokens_second_quoted(void** state)
{
    char* inp = "one \"two and\"";
    int result = count_tokens(inp);

    assert_int_equal(2, result);
}

void
count_two_tokens_both_quoted(void** state)
{
    char* inp = "\"one and then\" \"two and\"";
    int result = count_tokens(inp);

    assert_int_equal(2, result);
}

void
get_first_of_one(void** state)
{
    char* inp = "one";
    char* result = get_start(inp, 2);

    assert_string_equal("one", result);
    free(result);
}

void
get_first_of_two(void** state)
{
    char* inp = "one two";
    char* result = get_start(inp, 2);

    assert_string_equal("one ", result);
    free(result);
}

void
get_first_two_of_three(void** state)
{
    char* inp = "one two three";
    char* result = get_start(inp, 3);

    assert_string_equal("one two ", result);
    free(result);
}

void
get_first_two_of_three_first_quoted(void** state)
{
    char* inp = "\"one\" two three";
    char* result = get_start(inp, 3);

    assert_string_equal("\"one\" two ", result);
    free(result);
}

void
get_first_two_of_three_second_quoted(void** state)
{
    char* inp = "one \"two\" three";
    char* result = get_start(inp, 3);

    assert_string_equal("one \"two\" ", result);
    free(result);
}

void
get_first_two_of_three_first_and_second_quoted(void** state)
{
    char* inp = "\"one\" \"two\" three";
    char* result = get_start(inp, 3);

    assert_string_equal("\"one\" \"two\" ", result);
    free(result);
}

void
parse_options_when_none_returns_empty_hasmap(void** state)
{
    gchar* args[] = { "cmd1", "cmd2", NULL };
    gchar* keys[] = { "opt1", NULL };

    gboolean res = FALSE;

    GHashTable* options = parse_options(&args[2], keys, &res);

    assert_true(options != NULL);
    assert_int_equal(0, g_hash_table_size(options));
    assert_true(res);

    options_destroy(options);
}

void
parse_options_when_opt1_no_val_sets_error(void** state)
{
    gchar* args[] = { "cmd1", "cmd2", "opt1", NULL };
    gchar* keys[] = { "opt1", NULL };

    gboolean res = TRUE;

    GHashTable* options = parse_options(&args[2], keys, &res);

    assert_null(options);
    assert_false(res);

    options_destroy(options);
}

void
parse_options_when_one_returns_map(void** state)
{
    gchar* args[] = { "cmd1", "cmd2", "opt1", "val1", NULL };
    gchar* keys[] = { "opt1", NULL };

    gboolean res = FALSE;

    GHashTable* options = parse_options(&args[2], keys, &res);

    assert_int_equal(1, g_hash_table_size(options));
    assert_true(g_hash_table_contains(options, "opt1"));
    assert_string_equal("val1", g_hash_table_lookup(options, "opt1"));
    assert_true(res);

    options_destroy(options);
}

void
parse_options_when_opt2_no_val_sets_error(void** state)
{
    gchar* args[] = { "cmd1", "cmd2", "opt1", "val1", "opt2", NULL };
    gchar* keys[] = { "opt1", "opt2", NULL };

    gboolean res = TRUE;

    GHashTable* options = parse_options(&args[2], keys, &res);

    assert_null(options);
    assert_false(res);

    options_destroy(options);
}

void
parse_options_when_two_returns_map(void** state)
{
    gchar* args[] = { "cmd1", "cmd2", "opt1", "val1", "opt2", "val2", NULL };
    gchar* keys[] = { "opt1", "opt2", NULL };

    gboolean res = FALSE;

    GHashTable* options = parse_options(&args[2], keys, &res);

    assert_int_equal(2, g_hash_table_size(options));
    assert_true(g_hash_table_contains(options, "opt1"));
    assert_true(g_hash_table_contains(options, "opt2"));
    assert_string_equal("val1", g_hash_table_lookup(options, "opt1"));
    assert_string_equal("val2", g_hash_table_lookup(options, "opt2"));
    assert_true(res);

    options_destroy(options);
}

void
parse_options_when_opt3_no_val_sets_error(void** state)
{
    gchar* args[] = { "cmd1", "cmd2", "opt1", "val1", "opt2", "val2", "opt3", NULL };
    gchar* keys[] = { "opt1", "opt2", "opt3", NULL };

    gboolean res = TRUE;

    GHashTable* options = parse_options(&args[2], keys, &res);

    assert_null(options);
    assert_false(res);

    options_destroy(options);
}

void
parse_options_when_three_returns_map(void** state)
{
    gchar* args[] = { "cmd1", "cmd2", "opt1", "val1", "opt2", "val2", "opt3", "val3", NULL };
    gchar* keys[] = { "opt1", "opt2", "opt3", NULL };

    gboolean res = FALSE;

    GHashTable* options = parse_options(&args[2], keys, &res);

    assert_int_equal(3, g_hash_table_size(options));
    assert_true(g_hash_table_contains(options, "opt1"));
    assert_true(g_hash_table_contains(options, "opt2"));
    assert_true(g_hash_table_contains(options, "opt3"));
    assert_string_equal("val1", g_hash_table_lookup(options, "opt1"));
    assert_string_equal("val2", g_hash_table_lookup(options, "opt2"));
    assert_string_equal("val3", g_hash_table_lookup(options, "opt3"));
    assert_true(res);

    options_destroy(options);
}

void
parse_options_when_unknown_opt_sets_error(void** state)
{
    gchar* args[] = { "cmd1", "cmd2", "opt1", "val1", "oops", "val2", "opt3", "val3", NULL };
    gchar* keys[] = { "opt1", "opt2", "opt3", NULL };

    gboolean res = TRUE;

    GHashTable* options = parse_options(&args[2], keys, &res);

    assert_null(options);
    assert_false(res);

    options_destroy(options);
}

void
parse_options_with_duplicated_option_sets_error(void** state)
{
    gchar* args[] = { "cmd1", "cmd2", "opt1", "val1", "opt2", "val2", "opt1", "val3", NULL };
    gchar* keys[] = { "opt1", "opt2", "opt3", NULL };

    gboolean res = TRUE;

    GHashTable* options = parse_options(&args[2], keys, &res);

    assert_null(options);
    assert_false(res);

    options_destroy(options);
}
