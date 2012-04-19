#include <stdlib.h>
#include <head-unit.h>
#include "util.h"

void replace_one_substr(void)
{
    char *string = "it is a string";
    char *sub = "is";
    char *new = "was";

    char *result = str_replace(string, sub, new);

    assert_string_equals("it was a string", result);
}

void replace_one_substr_beginning(void)
{
    char *string = "it is a string";
    char *sub = "it";
    char *new = "that";

    char *result = str_replace(string, sub, new);

    assert_string_equals("that is a string", result);
}

void replace_one_substr_end(void)
{
    char *string = "it is a string";
    char *sub = "string";
    char *new = "thing";

    char *result = str_replace(string, sub, new);

    assert_string_equals("it is a thing", result);
}

void replace_two_substr(void)
{
    char *string = "it is a is string";
    char *sub = "is";
    char *new = "was";

    char *result = str_replace(string, sub, new);

    assert_string_equals("it was a was string", result);
}

void replace_char(void)
{
    char *string = "some & a thing & something else";
    char *sub = "&";
    char *new = "&amp;";

    char *result = str_replace(string, sub, new);

    assert_string_equals("some &amp; a thing &amp; something else", result);
}

void register_util_tests(void)
{
    TEST_MODULE("util tests");
    TEST(replace_one_substr);
    TEST(replace_one_substr_beginning);
    TEST(replace_one_substr_end);
    TEST(replace_two_substr);
    TEST(replace_char);
}
