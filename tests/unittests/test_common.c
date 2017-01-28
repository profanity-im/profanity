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

void test_p_sha1_hash1(void **state)
{
    char *inp = "<message>some message</message>\n<element>another element</element>\n";
    char *result = p_sha1_hash(inp);

    assert_string_equal(result, "ZJLLzkYc51Lug3fZ7MJJzK95Ikg=");
}

void test_p_sha1_hash2(void **state)
{
    char *inp = "";
    char *result = p_sha1_hash(inp);

    assert_string_equal(result, "2jmj7l5rSw0yVb/vlWAYkK/YBwk=");
}

void test_p_sha1_hash3(void **state)
{
    char *inp = "m";
    char *result = p_sha1_hash(inp);

    assert_string_equal(result, "aw0xwNVjIjAk2kVpFYRkOseMlug=");
}

void test_p_sha1_hash4(void **state)
{
    char *inp = "<element/>\n";
    char *result = p_sha1_hash(inp);

    assert_string_equal(result, "xcgld4ZfXvU0P7+cW3WFLUuE3C8=");
}

void test_p_sha1_hash5(void **state)
{
    char *inp = "  ";
    char *result = p_sha1_hash(inp);

    assert_string_equal(result, "CZYAoQqUQRSqxAbRNrYl+0Ft13k=");
}

void test_p_sha1_hash6(void **state)
{
    char *inp = " sdf  \n ";
    char *result = p_sha1_hash(inp);

    assert_string_equal(result, "zjtm8dKlTj1KhYDlM2z8FsmAhSQ=");
}

void test_p_sha1_hash7(void **state)
{
    char *inp = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer nec odio. Praesent libero. Sed cursus ante dapibus diam. Sed nisi. Nulla quis sem at nibh elementum imperdiet. Duis sagittis ipsum. Praesent mauris. Fusce nec tellus sed augue semper porta. Mauris massa. Vestibulum lacinia arcu eget nulla. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Curabitur sodales ligula in libero. Sed dignissim lacinia nunc. Curabitur tortor. Pellentesque nibh. Aenean quam. In scelerisque sem at dolor. Maecenas mattis. Sed convallis tristique sem. Proin ut ligula vel nunc egestas porttitor. Morbi lectus risus, iaculis vel, suscipit quis, luctus non, massa. Fusce ac turpis quis ligula lacinia aliquet. Mauris ipsum. Nulla metus metus, ullamcorper vel, tincidunt sed, euismod in, nibh. Quisque volutpat condimentum velit. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Nam nec ante. Sed lacinia, urna non tincidunt mattis, tortor neque adipiscing diam, a cursus ipsum ante quis turpis. Nulla facilisi. Ut fringilla. Suspendisse potenti. Nunc feugiat mi a tellus consequat imperdiet. Vestibulum sapien. Proin quam. Etiam ultrices. Suspendisse in justo eu magna luctus suscipit. Sed lectus. Integer euismod lacus luctus magna. Quisque cursus, metus vitae pharetra auctor, sem massa mattis sem, at interdum magna augue eget diam. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Morbi lacinia molestie dui. Praesent blandit dolor. Sed non quam. In vel mi sit amet augue congue elementum. Morbi in ipsum sit amet pede facilisis laoreet. Donec lacus nunc, viverra nec, blandit vel, egestas et, augue. Vestibulum tincidunt malesuada tellus. Ut ultrices ultrices enim. Curabitur sit amet mauris. Morbi in dui quis est pulvinar ullamcorper. Nulla facilisi. Integer lacinia sollicitudin massa. Cras metus. Sed aliquet risus a tortor. Integer id quam. Morbi mi. Quisque nisl felis, venenatis tristique, dignissim in, ultrices sit amet, augue. Proin sodales libero eget ante. Nulla quam. Aenean laoreet. Vestibulum nisi lectus, commodo ac, facilisis ac, ultricies eu, pede. Ut orci risus, accumsan porttitor, cursus quis, aliquet eget, justo. Sed pretium blandit orci. Ut eu diam at pede suscipit sodales. Aenean lectus elit, fermentum non, convallis id, sagittis at, neque. Nullam mauris orci, aliquet et, iaculis et, viverra vitae, ligula. Nulla ut felis in purus aliquam imperdiet. Maecenas aliquet mollis lectus. Vivamus consectetuer risus et tortor. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer nec odio. Praesent libero. Sed cursus ante dapibus diam. Sed nisi. Nulla quis sem at nibh elementum imperdiet. Duis sagittis ipsum. Praesent mauris. Fusce nec tellus sed augue semper porta. Mauris massa. Vestibulum lacinia arcu eget nulla. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Curabitur sodales ligula in libero. Sed dignissim lacinia nunc. Curabitur tortor. Pellentesque nibh. Aenean quam. In scelerisque sem at dolor. Maecenas mattis. Sed convallis tristique sem. Proin ut ligula vel nunc egestas porttitor. Morbi lectus risus, iaculis vel, suscipit quis, luctus non, massa. Fusce ac turpis quis ligula lacinia aliquet. Mauris ipsum. Nulla metus metus, ullamcorper vel, tincidunt sed, euismod in, nibh. Quisque volutpat condimentum velit. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Nam nec ante. Sed lacinia, urna non tincidunt mattis, tortor neque adipiscing diam, a cursus ipsum ante quis turpis. Nulla facilisi. Ut fringilla. Suspendisse potenti. Nunc feugiat mi a tellus consequat imperdiet. Vestibulum sapien. Proin quam. Etiam ultrices. Suspendisse in justo eu magna luctus suscipit. Sed lectus. Integer euismod lacus luctus magna. Quisque cursus, metus vitae pharetra auctor, sem massa mattis sem, at interdum magna augue eget diam. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Morbi lacinia molestie dui. Praesent blandit dolor. Sed non quam. In vel mi sit amet augue congue elementum. Morbi in ipsum sit amet pede facilisis laoreet. Donec lacus nunc, viverra nec, blandit vel, egestas et, augue. Vestibulum tincidunt malesuada tellus. Ut ultrices ultrices enim. Curabitur sit amet mauris. Morbi in dui quis est pulvinar ullamcorper. Nulla facilisi. Integer lacinia sollicitudin massa. Cras metus. Sed aliquet risus a tortor. Integer id quam. Morbi mi. Quisque nisl felis, venenatis tristique, dignissim in, ultrices sit amet, augue. Proin sodales libero eget ante. Nulla quam. Aenean laoreet. Vestibulum nisi lectus, commodo ac, facilisis ac, ultricies eu, pede. Ut orci risus, accumsan porttitor, cursus quis, aliquet eget, justo. Sed pretium blandit orci. Ut eu diam at pede suscipit sodales. Aenean lectus elit, fermentum non, convallis id, sagittis at, neque. Nullam mauris orci, aliquet et, iaculis et, viverra vitae, ligula. Nulla ut felis in purus aliquam imperdiet. Maecenas aliquet mollis lectus. Vivamus consectetuer risus et tortor. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Integer nec odio. Praesent libero. Sed cursus ante dapibus diam. Sed nisi. Nulla quis sem at nibh elementum imperdiet. Duis sagittis ipsum. Praesent mauris. Fusce nec tellus sed augue semper porta. Mauris massa. Vestibulum lacinia arcu eget nulla. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Curabitur sodales ligula in libero. Sed dignissim lacinia nunc. Curabitur tortor. Pellentesque nibh. Aenean quam. In scelerisque sem at dolor. Maecenas mattis. Sed convallis tristique sem. Proin ut ligula vel nunc egestas porttitor. Morbi lectus risus, iaculis vel, suscipit quis, luctus non, massa. Fusce ac turpis quis ligula lacinia aliquet. Mauris ipsum. Nulla metus metus, ullamcorper vel, tincidunt sed, euismod in, nibh. Quisque volutpat condimentum velit. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Nam nec ante. Sed lacinia, urna non tincidunt mattis, tortor neque adipiscing diam, a cursus ipsum ante quis turpis. Nulla facilisi. Ut fringilla. Suspendisse potenti. Nunc feugiat mi a tellus consequat imperdiet. Vestibulum sapien. Proin quam. Etiam ultrices. Suspendisse in justo eu magna luctus suscipit. Sed lectus. Integer euismod lacus luctus magna. Quisque cursus, metus vitae pharetra auctor, sem massa mattis sem, at interdum magna augue eget diam. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Morbi lacinia molestie dui. Praesent blandit dolor. Sed non quam. In vel mi sit amet augue congue elementum. Morbi in ipsum si.";
    char *result = p_sha1_hash(inp);

    assert_string_equal(result, "bNfKVfqEOGmzlH8M+e8FYTB46SU=");
}

void utf8_display_len_null_str(void **state)
{
    int result = utf8_display_len(NULL);

    assert_int_equal(0, result);
}

void utf8_display_len_1_non_wide(void **state)
{
    int result = utf8_display_len("1");

    assert_int_equal(1, result);
}

void utf8_display_len_1_wide(void **state)
{
    int result = utf8_display_len("四");

    assert_int_equal(2, result);
}

void utf8_display_len_non_wide(void **state)
{
    int result = utf8_display_len("123456789abcdef");

    assert_int_equal(15, result);
}

void utf8_display_len_wide(void **state)
{
    int result = utf8_display_len("12三四56");

    assert_int_equal(8, result);
}

void utf8_display_len_all_wide(void **state)
{
    int result = utf8_display_len("ひらがな");

    assert_int_equal(8, result);
}

void strip_quotes_does_nothing_when_no_quoted(void **state)
{
    char *input = "/cmd test string";

    char *result = strip_arg_quotes(input);

    assert_string_equal("/cmd test string", result);

    free(result);
}

void strip_quotes_strips_first(void **state)
{
    char *input = "/cmd \"test string";

    char *result = strip_arg_quotes(input);

    assert_string_equal("/cmd test string", result);

    free(result);
}

void strip_quotes_strips_last(void **state)
{
    char *input = "/cmd test string\"";

    char *result = strip_arg_quotes(input);

    assert_string_equal("/cmd test string", result);

    free(result);
}

void strip_quotes_strips_both(void **state)
{
    char *input = "/cmd \"test string\"";

    char *result = strip_arg_quotes(input);

    assert_string_equal("/cmd test string", result);

    free(result);
}

gboolean
_lists_equal(GSList *a, GSList *b)
{
    if (g_slist_length(a) != g_slist_length(b)) {
        return FALSE;
    }

    GSList *curra = a;
    GSList *currb = b;

    while (curra) {
        int aval = GPOINTER_TO_INT(curra->data);
        int bval = GPOINTER_TO_INT(currb->data);

        if (aval != bval) {
            return FALSE;
        }

        curra = g_list_next(curra);
        currb = g_list_next(currb);
    }

    return TRUE;
}

void prof_partial_occurrences_tests(void **state)
{
    GSList *actual = NULL;
    GSList *expected = NULL;
    assert_true(_lists_equal(prof_occurrences(NULL, NULL, 0, FALSE, &actual), expected));
    g_slist_free(actual); actual = NULL;

    assert_true(_lists_equal(prof_occurrences(NULL,         "some string",  0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5",    NULL,           0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences(NULL,         NULL,           0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5",    "Boothj5",      0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("Boothj5",    "boothj5",      0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5",         0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5hello",    0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5 hello",   0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(5));
    assert_true(_lists_equal(prof_occurrences("boothj5", "helloboothj5",        0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "helloboothj5hello",   0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(6));
    assert_true(_lists_equal(prof_occurrences("boothj5", "hello boothj5",       0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "hello boothj5 hello", 0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    expected = g_slist_append(expected, GINT_TO_POINTER(7));
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5boothj5", 0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    expected = g_slist_append(expected, GINT_TO_POINTER(12));
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5helloboothj5", 0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    expected = g_slist_append(expected, GINT_TO_POINTER(14));
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5 hello boothj5", 0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(2));
    expected = g_slist_append(expected, GINT_TO_POINTER(16));
    expected = g_slist_append(expected, GINT_TO_POINTER(29));
    assert_true(_lists_equal(prof_occurrences("boothj5", "hiboothj5 hello boothj5there boothj5s", 0, FALSE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;
}

void prof_whole_occurrences_tests(void **state)
{
    GSList *actual = NULL;
    GSList *expected = NULL;
    assert_true(_lists_equal(prof_occurrences(NULL, NULL, 0, FALSE, &actual), expected));
    g_slist_free(actual); actual = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5",      0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5 hi",   0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5: hi",  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5, hi",  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "我能吞下玻璃而",      0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "我能吞下玻璃而 hi",   0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "我能吞下玻璃而: hi",  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "我能吞下玻璃而, hi",  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(6));
    assert_true(_lists_equal(prof_occurrences("boothj5", "hello boothj5",        0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "hello boothj5 there",  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "heyy @boothj5, there", 0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(6));
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hello 我能吞下玻璃而",        0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hello 我能吞下玻璃而 there",  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "heyy @我能吞下玻璃而, there", 0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(6));
    expected = g_slist_append(expected, GINT_TO_POINTER(26));
    assert_true(_lists_equal(prof_occurrences("boothj5", "hello boothj5 some more a boothj5 stuff",  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "hello boothj5 there ands #boothj5",        0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "heyy @boothj5, there hows boothj5?",       0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(6));
    expected = g_slist_append(expected, GINT_TO_POINTER(26));
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hello 我能吞下玻璃而 some more a 我能吞下玻璃而 stuff",  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hello 我能吞下玻璃而 there ands #我能吞下玻璃而",        0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "heyy @我能吞下玻璃而, there hows 我能吞下玻璃而?",       0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(6));
    assert_true(_lists_equal(prof_occurrences("p", "ppppp p",   0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    assert_true(_lists_equal(prof_occurrences("p", "p ppppp",   0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(4));
    assert_true(_lists_equal(prof_occurrences("p", "ppp p ppp", 0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5hello",        0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "heyboothj5",          0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "heyboothj5hithere",   0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "hey boothj5hithere",  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "hey @boothj5hithere", 0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "heyboothj5 hithere",  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "heyboothj5, hithere", 0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5boothj5",      0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5fillboothj5",  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k",       "dont know",           0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k",       "kick",                0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k",       "kick kick",           0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k",       "kick kickk",           0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k",       "kic",                 0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k",       "ick",                 0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k",       "kk",                  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k",       "kkkkkkk",                  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    expected = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "我能吞下玻璃而hello",        0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hey我能吞下玻璃而",          0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hey我能吞下玻璃而hithere",   0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hey 我能吞下玻璃而hithere",  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hey @我能吞下玻璃而hithere", 0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hey我能吞下玻璃而 hithere",  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hey我能吞下玻璃而, hithere", 0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "我能吞下玻璃而我能吞下玻璃而",      0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "我能吞下玻璃而fill我能吞下玻璃而",  0, TRUE, &actual), expected)); g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;
}
