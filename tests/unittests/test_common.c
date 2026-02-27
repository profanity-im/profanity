#include "xmpp/resource.h"
#include "common.h"
#include "prof_cmocka.h"
#include <stdlib.h>
#include "tests/unittests/ui/stub_ui.h" // Include for mocking cons_show

void
replace_one_substr(void** state)
{
    char* string = "it is a string";
    char* sub = "is";
    char* new = "was";

    char* result = str_replace(string, sub, new);

    assert_string_equal("it was a string", result);

    free(result);
}

void
replace_one_substr_beginning(void** state)
{
    char* string = "it is a string";
    char* sub = "it";
    char* new = "that";

    char* result = str_replace(string, sub, new);

    assert_string_equal("that is a string", result);

    free(result);
}

void
replace_one_substr_end(void** state)
{
    char* string = "it is a string";
    char* sub = "string";
    char* new = "thing";

    char* result = str_replace(string, sub, new);

    assert_string_equal("it is a thing", result);

    free(result);
}

void
replace_two_substr(void** state)
{
    char* string = "it is a is string";
    char* sub = "is";
    char* new = "was";

    char* result = str_replace(string, sub, new);

    assert_string_equal("it was a was string", result);

    free(result);
}

void
replace_char(void** state)
{
    char* string = "some & a thing & something else";
    char* sub = "&";
    char* new = "&amp;";

    char* result = str_replace(string, sub, new);

    assert_string_equal("some &amp; a thing &amp; something else", result);

    free(result);
}

void
replace_when_none(void** state)
{
    char* string = "its another string";
    char* sub = "haha";
    char* new = "replaced";

    char* result = str_replace(string, sub, new);

    assert_string_equal("its another string", result);

    free(result);
}

void
replace_when_match(void** state)
{
    char* string = "hello";
    char* sub = "hello";
    char* new = "goodbye";

    char* result = str_replace(string, sub, new);

    assert_string_equal("goodbye", result);

    free(result);
}

void
replace_when_string_empty(void** state)
{
    char* string = "";
    char* sub = "hello";
    char* new = "goodbye";

    char* result = str_replace(string, sub, new);

    assert_string_equal("", result);

    free(result);
}

void
replace_when_string_null(void** state)
{
    char* string = NULL;
    char* sub = "hello";
    char* new = "goodbye";

    char* result = str_replace(string, sub, new);

    assert_null(result);
}

void
replace_when_sub_empty(void** state)
{
    char* string = "hello";
    char* sub = "";
    char* new = "goodbye";

    char* result = str_replace(string, sub, new);

    assert_string_equal("hello", result);

    free(result);
}

void
replace_when_sub_null(void** state)
{
    char* string = "hello";
    char* sub = NULL;
    char* new = "goodbye";

    char* result = str_replace(string, sub, new);

    assert_string_equal("hello", result);

    free(result);
}

void
replace_when_new_empty(void** state)
{
    char* string = "hello";
    char* sub = "hello";
    char* new = "";

    char* result = str_replace(string, sub, new);

    assert_string_equal("", result);

    free(result);
}

void
replace_when_new_null(void** state)
{
    char* string = "hello";
    char* sub = "hello";
    char* new = NULL;

    char* result = str_replace(string, sub, new);

    assert_string_equal("hello", result);

    free(result);
}

void
test_online_is_valid_resource_presence_string(void** state)
{
    assert_true(valid_resource_presence_string("online"));
}

void
test_chat_is_valid_resource_presence_string(void** state)
{
    assert_true(valid_resource_presence_string("chat"));
}

void
test_away_is_valid_resource_presence_string(void** state)
{
    assert_true(valid_resource_presence_string("away"));
}

void
test_xa_is_valid_resource_presence_string(void** state)
{
    assert_true(valid_resource_presence_string("xa"));
}

void
test_dnd_is_valid_resource_presence_string(void** state)
{
    assert_true(valid_resource_presence_string("dnd"));
}

void
test_available_is_not_valid_resource_presence_string(void** state)
{
    assert_false(valid_resource_presence_string("available"));
}

void
test_unavailable_is_not_valid_resource_presence_string(void** state)
{
    assert_false(valid_resource_presence_string("unavailable"));
}

void
test_blah_is_not_valid_resource_presence_string(void** state)
{
    assert_false(valid_resource_presence_string("blah"));
}

void
utf8_display_len_null_str(void** state)
{
    int result = utf8_display_len(NULL);

    assert_int_equal(0, result);
}

void
utf8_display_len_1_non_wide(void** state)
{
    int result = utf8_display_len("1");

    assert_int_equal(1, result);
}

void
utf8_display_len_1_wide(void** state)
{
    int result = utf8_display_len("四");

    assert_int_equal(2, result);
}

void
utf8_display_len_non_wide(void** state)
{
    int result = utf8_display_len("123456789abcdef");

    assert_int_equal(15, result);
}

void
utf8_display_len_wide(void** state)
{
    int result = utf8_display_len("12三四56");

    assert_int_equal(8, result);
}

void
utf8_display_len_all_wide(void** state)
{
    int result = utf8_display_len("ひらがな");

    assert_int_equal(8, result);
}

void
strip_quotes_does_nothing_when_no_quoted(void** state)
{
    char* input = "/cmd test string";

    char* result = strip_arg_quotes(input);

    assert_string_equal("/cmd test string", result);

    free(result);
}

void
strip_quotes_strips_first(void** state)
{
    char* input = "/cmd \"test string";

    char* result = strip_arg_quotes(input);

    assert_string_equal("/cmd test string", result);

    free(result);
}

void
strip_quotes_strips_last(void** state)
{
    char* input = "/cmd test string\"";

    char* result = strip_arg_quotes(input);

    assert_string_equal("/cmd test string", result);

    free(result);
}

void
strip_quotes_strips_both(void** state)
{
    char* input = "/cmd \"test string\"";

    char* result = strip_arg_quotes(input);

    assert_string_equal("/cmd test string", result);

    free(result);
}

void test_valid_tls_policy_option(void** state) {
    // Valid inputs
    assert_true(valid_tls_policy_option("force"));
    assert_true(valid_tls_policy_option("allow"));
    assert_true(valid_tls_policy_option("trust"));
    assert_true(valid_tls_policy_option("disable"));
    assert_true(valid_tls_policy_option("legacy"));
    assert_true(valid_tls_policy_option("direct"));

    // Invalid inputs
    // Not an option
    expect_any_cons_show(); // For "Invalid TLS policy: 'profanity'"
    expect_any_cons_show(); // For "TLS policy must be one of: 'force', 'allow', 'trust', 'disable', 'legacy', or 'direct'."
    assert_false(valid_tls_policy_option("profanity"));

    // Empty
    expect_any_cons_show(); // For "Invalid TLS policy: ''"
    expect_any_cons_show(); // For "TLS policy must be one of: 'force', 'allow', 'trust', 'disable', 'legacy', or 'direct'."
    assert_false(valid_tls_policy_option(""));

	// NULL
    assert_true(valid_tls_policy_option(NULL));
}

void test_get_expanded_path(void** state) {
    gchar* expanded_path;

    // `file://` prefix
    expanded_path = get_expanded_path("file:///tmp/test.txt");
    assert_string_equal("/tmp/test.txt", expanded_path);
    g_free(expanded_path);

    // `~/"` prefix
    setenv("HOME", "/home/test", 1);
    expanded_path = get_expanded_path("~/folder/file.txt");
    assert_string_equal("/home/test/folder/file.txt", expanded_path);
    g_free(expanded_path);
    unsetenv("HOME");

    // regular path
    expanded_path = get_expanded_path("/home/test/file.pdf");
    assert_string_equal("/home/test/file.pdf", expanded_path);
    g_free(expanded_path);

    // empty path
    expanded_path = get_expanded_path("");
    assert_string_equal("", expanded_path);
    g_free(expanded_path);
}

void test_strtoi_range_valid_input(void** state) {
    int value;
    gchar* err_msg = NULL;
    gboolean result;

    result = strtoi_range("10", &value, 0, 20, &err_msg);
    assert_true(result);
    assert_int_equal(10, value);
    assert_null(err_msg);

    result = strtoi_range("0", &value, 0, 20, &err_msg);
    assert_true(result);
    assert_int_equal(0, value);
    assert_null(err_msg);

    result = strtoi_range("20", &value, 0, 20, &err_msg);
    assert_true(result);
    assert_int_equal(20, value);
    assert_null(err_msg);

    result = strtoi_range("-5", &value, -10, 0, &err_msg);
    assert_true(result);
    assert_int_equal(-5, value);
    assert_null(err_msg);
}

void test_strtoi_range_out_of_range(void** state) {
    int value;
    gchar* err_msg = NULL;
    gboolean result;

    // too low, negative
    result = strtoi_range("-11", &value, -10, 0, &err_msg);
    assert_false(result);
    assert_string_equal("Value -11 out of range. Must be in -10..0.", err_msg);
    g_free(err_msg);
    err_msg = NULL;

    // too low
    result = strtoi_range("-1", &value, 0, 10, &err_msg);
    assert_false(result);
    assert_string_equal("Value -1 out of range. Must be in 0..10.", err_msg);
    g_free(err_msg);
    err_msg = NULL;

    // too high
    result = strtoi_range("11", &value, 0, 10, &err_msg);
    assert_false(result);
    assert_string_equal("Value 11 out of range. Must be in 0..10.", err_msg);
    g_free(err_msg);
    err_msg = NULL;
}

void test_strtoi_range_invalid_input(void** state) {
    int value;
    gchar* err_msg = NULL;
    gboolean result;

    // not a number
    result = strtoi_range("profanity", &value, 0, 10, &err_msg);
    assert_false(result);
    assert_string_equal("Could not convert \"profanity\" to a number.", err_msg);
    g_free(err_msg);
    err_msg = NULL;

    // not a number, start with digits
    result = strtoi_range("23kk", &value, 0, 10, &err_msg);
    assert_false(result);
    assert_string_equal("Could not convert \"23kk\" to a number.", err_msg);
    g_free(err_msg);
    err_msg = NULL;
}

void test_strtoi_range_null_empty_input(void** state) {
    int value;
    gchar* err_msg = NULL;
    gboolean result;

    // NULL input string
    result = strtoi_range(NULL, &value, 0, 10, &err_msg);
    assert_false(result);
    assert_string_equal("'str' input pointer can not be NULL", err_msg);
    g_free(err_msg);
    err_msg = NULL;

    // Empty input string
    result = strtoi_range("", &value, 0, 10, &err_msg);
    assert_false(result);
    assert_string_equal("Could not convert \"\" to a number.", err_msg);
    g_free(err_msg);
    err_msg = NULL;
}

void test_strtoi_range_null_err_msg(void** state) {
    int value;
    gboolean result;

    // valid conversion, err_msg is NULL
    result = strtoi_range("5", &value, 0, 10, NULL);
    assert_true(result);
    assert_int_equal(5, value);

    // invalid conversion, err_msg is NULL
    result = strtoi_range("profanity", &value, 0, 10, NULL);
    assert_false(result);
}

void test_string_to_verbosity(void** state) {
    int verbosity;
    gchar* err_msg = NULL;
    gboolean result;

    // Valid input string (0)
    result = string_to_verbosity("0", &verbosity, &err_msg);
    assert_true(result);
    assert_int_equal(0, verbosity);
    assert_null(err_msg);
    g_free(err_msg); err_msg = NULL; // Clear for next test

    // Valid input string (1)
    result = string_to_verbosity("1", &verbosity, &err_msg);
    assert_true(result);
    assert_int_equal(1, verbosity);
    assert_null(err_msg);
    g_free(err_msg); err_msg = NULL;

    // Valid input string (3)
    result = string_to_verbosity("3", &verbosity, &err_msg);
    assert_true(result);
    assert_int_equal(3, verbosity);
    assert_null(err_msg);
    g_free(err_msg); err_msg = NULL;

    // Invalid input string (not a number)
    result = string_to_verbosity("profanity", &verbosity, &err_msg);
    assert_false(result);
    assert_string_equal("Could not convert \"profanity\" to a number.", err_msg);
    g_free(err_msg);
	err_msg = NULL;

    // Valid input string, out of range (too low)
    result = string_to_verbosity("-1", &verbosity, &err_msg);
    assert_false(result);
    assert_string_equal("Value -1 out of range. Must be in 0..3.", err_msg);
    g_free(err_msg); err_msg = NULL;

    // Valid input string, out of range (too high)
    result = string_to_verbosity("4", &verbosity, &err_msg);
    assert_false(result);
    assert_string_equal("Value 4 out of range. Must be in 0..3.", err_msg);
    g_free(err_msg); err_msg = NULL;

    // NULL input string
    result = string_to_verbosity(NULL, &verbosity, &err_msg);
    assert_false(result);
    assert_string_equal("'str' input pointer can not be NULL", err_msg);
    g_free(err_msg); err_msg = NULL;

    // Empty input string
    result = string_to_verbosity("", &verbosity, &err_msg);
    assert_false(result);
    assert_string_equal("Could not convert \"\" to a number.", err_msg);
    g_free(err_msg); err_msg = NULL;

    // err_msg is NULL
    result = string_to_verbosity("abc", &verbosity, NULL);
    assert_false(result);
}

typedef struct
{
    char* template;
    char* url;
    char* filename;
    char* argv;
} format_call_external_argv_t;

void
format_call_external_argv_td(void** state)
{

    enum table { num_tests = 4 };

    format_call_external_argv_t tests[num_tests] = {
        (format_call_external_argv_t){
            .template = "/bin/echo %u %p",
            .url = "https://example.org",
            .filename = "image.jpeg",
            .argv = "/bin/echo https://example.org image.jpeg",
        },
        (format_call_external_argv_t){
            .template = "/bin/echo %p %u",
            .url = "https://example.org",
            .filename = "image.jpeg",
            .argv = "/bin/echo image.jpeg https://example.org",
        },
        (format_call_external_argv_t){
            .template = "/bin/echo %p",
            .url = "https://example.org",
            .filename = "image.jpeg",
            .argv = "/bin/echo image.jpeg",
        },
        (format_call_external_argv_t){
            .template = "/bin/echo %u",
            .url = "https://example.org",
            .filename = "image.jpeg",
            .argv = "/bin/echo https://example.org",
        },
    };

    gchar** got_argv = NULL;
    gchar* got_argv_str = NULL;
    for (int i = 0; i < num_tests; i++) {
        got_argv = format_call_external_argv(
            tests[i].template,
            tests[i].url,
            tests[i].filename);
        got_argv_str = g_strjoinv(" ", got_argv);

        assert_string_equal(got_argv_str, tests[i].argv);

        g_strfreev(got_argv);
        g_free(got_argv_str);
    }
}

typedef struct
{
    char* url;
    char* path;
    char* target;
    char* basename;
} unique_filename_from_url_t;

void
unique_filename_from_url_td(void** state)
{

    enum table { num_tests = 15 };
    auto_gchar gchar* pwd = g_get_current_dir();

    unique_filename_from_url_t tests[num_tests] = {
        (unique_filename_from_url_t){
            .url = "https://host.test/image.jpeg",
            .path = "./.",
            .target = pwd,
            .basename = "image.jpeg",
        },
        (unique_filename_from_url_t){
            .url = "https://host.test/image.jpeg",
            .path = NULL,
            .target = pwd,
            .basename = "image.jpeg",
        },
        (unique_filename_from_url_t){
            .url = "https://host.test/image.jpeg#somefragment",
            .path = "./",
            .target = pwd,
            .basename = "image.jpeg",
        },
        (unique_filename_from_url_t){
            .url = "https://host.test/image.jpeg?query=param",
            .path = "./",
            .target = pwd,
            .basename = "image.jpeg",
        },
        (unique_filename_from_url_t){
            .url = "https://host.test/image.jpeg?query=param&another=one",
            .path = "./",
            .target = pwd,
            .basename = "image.jpeg",
        },
        (unique_filename_from_url_t){
            .url = "https://host.test/image.jpeg?query=param&another=one",
            .path = "/tmp/",
            .target = "/tmp/",
            .basename = "image.jpeg",
        },
        (unique_filename_from_url_t){
            .url = "https://host.test/image.jpeg?query=param&another=one",
            .path = "/tmp/hopefully/this/file/does/not/exist",
            .target = "/tmp/hopefully/this/file/does/not/",
            .basename = "exist",
        },
        (unique_filename_from_url_t){
            .url = "https://host.test/image.jpeg?query=param&another=one",
            .path = "/tmp/hopefully/this/file/does/not/exist/",
            .target = "/tmp/hopefully/this/file/does/not/exist/",
            .basename = "image.jpeg",
        },
        (unique_filename_from_url_t){
            .url = "https://host.test/images/",
            .path = "./",
            .target = pwd,
            .basename = "images",
        },
        (unique_filename_from_url_t){
            .url = "https://host.test/images/../../file",
            .path = "./",
            .target = pwd,
            .basename = "file",
        },
        (unique_filename_from_url_t){
            .url = "https://host.test/images/../../file/..",
            .path = "./",
            .target = pwd,
            .basename = "index",
        },
        (unique_filename_from_url_t){
            .url = "https://host.test/images/..//",
            .path = "./",
            .target = pwd,
            .basename = "index",
        },
        (unique_filename_from_url_t){
            .url = "https://host.test/",
            .path = "./",
            .target = pwd,
            .basename = "index",
        },
        (unique_filename_from_url_t){
            .url = "https://host.test",
            .path = "./",
            .target = pwd,
            .basename = "index",
        },
        (unique_filename_from_url_t){
            .url = "aesgcm://host.test",
            .path = "./",
            .target = pwd,
            .basename = "index",
        },
    };

    char* got_filename = NULL;
    char* exp_filename = NULL;
    for (int i = 0; i < num_tests; i++) {
        got_filename = unique_filename_from_url(tests[i].url, tests[i].path);
        exp_filename = g_build_filename(tests[i].target, tests[i].basename, NULL);

        assert_string_equal(got_filename, exp_filename);

        g_free(got_filename);
        g_free(exp_filename);
    }
}

gboolean
_lists_equal(GSList* a, GSList* b)
{
    if (g_slist_length(a) != g_slist_length(b)) {
        return FALSE;
    }

    GSList* curra = a;
    GSList* currb = b;

    while (curra && currb) {
        int aval = GPOINTER_TO_INT(curra->data);
        int bval = GPOINTER_TO_INT(currb->data);

        if (aval != bval) {
            return FALSE;
        }

        curra = g_slist_next(curra);
        currb = g_slist_next(currb);
    }

    return TRUE;
}

void
prof_occurrences_of_large_message_tests(void** state)
{
    GSList* actual = NULL;
    GSList* expected = NULL;
    /* use this with the old implementation to create a segfault
     *     const size_t haystack_sz = 1024 * 1024;
     */
    const size_t haystack_sz = 1024;
    size_t haystack_cur = 0;
    auto_char char* haystack = malloc(haystack_sz);
    const char needle[] = "needle ";
    while (haystack_sz - haystack_cur > sizeof(needle)) {
        memcpy(&haystack[haystack_cur], needle, sizeof(needle));
        expected = g_slist_append(expected, GINT_TO_POINTER(haystack_cur));
        haystack_cur += sizeof(needle) - 1;
    }
    assert_true(_lists_equal(prof_occurrences("needle", haystack, 0, FALSE, &actual), expected));
    g_slist_free(actual);
    g_slist_free(expected);
}

void
prof_partial_occurrences_tests(void** state)
{
    GSList* actual = NULL;
    GSList* expected = NULL;
    assert_true(_lists_equal(prof_occurrences(NULL, NULL, 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;

    assert_true(_lists_equal(prof_occurrences(NULL, "some string", 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", NULL, 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences(NULL, NULL, 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "Boothj5", 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("Boothj5", "boothj5", 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5", 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5hello", 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5 hello", 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(5));
    assert_true(_lists_equal(prof_occurrences("boothj5", "helloboothj5", 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "helloboothj5hello", 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(6));
    assert_true(_lists_equal(prof_occurrences("boothj5", "hello boothj5", 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "hello boothj5 hello", 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    expected = g_slist_append(expected, GINT_TO_POINTER(7));
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5boothj5", 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    expected = g_slist_append(expected, GINT_TO_POINTER(12));
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5helloboothj5", 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    expected = g_slist_append(expected, GINT_TO_POINTER(14));
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5 hello boothj5", 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(2));
    expected = g_slist_append(expected, GINT_TO_POINTER(16));
    expected = g_slist_append(expected, GINT_TO_POINTER(29));
    assert_true(_lists_equal(prof_occurrences("boothj5", "hiboothj5 hello boothj5there boothj5s", 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;
}

void
get_mentions_tests(void** state)
{
    GSList* actual = NULL;
    GSList* expected = NULL;

    // Basic match, case sensitive
    expected = g_slist_append(expected, GINT_TO_POINTER(6));
    actual = get_mentions(FALSE, TRUE, "hello boothj5", "boothj5");
    assert_true(_lists_equal(actual, expected));
    g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    // Case insensitive match
    expected = g_slist_append(expected, GINT_TO_POINTER(6));
    actual = get_mentions(FALSE, FALSE, "hello BOOTHJ5", "boothj5");
    assert_true(_lists_equal(actual, expected));
    g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    // Whole word match
    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    actual = get_mentions(TRUE, TRUE, "boothj5 hello", "boothj5");
    assert_true(_lists_equal(actual, expected));
    g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    // Whole word no match (partial)
    actual = get_mentions(TRUE, TRUE, "boothj5hello", "boothj5");
    assert_true(_lists_equal(actual, expected)); // expected is NULL
    g_slist_free(actual); actual = NULL;

    // Multiple matches
    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    expected = g_slist_append(expected, GINT_TO_POINTER(14));
    actual = get_mentions(FALSE, TRUE, "boothj5 hello boothj5", "boothj5");
    assert_true(_lists_equal(actual, expected));
    g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    // Nick with punctuation (whole word)
    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    actual = get_mentions(TRUE, TRUE, "boothj5: hi", "boothj5");
    assert_true(_lists_equal(actual, expected));
    g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    // Nick surrounded by punctuation
    expected = g_slist_append(expected, GINT_TO_POINTER(1));
    actual = get_mentions(TRUE, TRUE, "(boothj5)", "boothj5");
    assert_true(_lists_equal(actual, expected));
    g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;

    // Empty message
    actual = get_mentions(FALSE, TRUE, "", "boothj5");
    assert_true(_lists_equal(actual, expected)); // expected is NULL
    g_slist_free(actual); actual = NULL;

    // Empty nick
    actual = get_mentions(FALSE, TRUE, "hello", "");
    assert_true(_lists_equal(actual, expected)); // expected is NULL
    g_slist_free(actual); actual = NULL;

    // UTF-8 characters
    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    actual = get_mentions(TRUE, TRUE, "我能 hello", "我能");
    assert_true(_lists_equal(actual, expected));
    g_slist_free(actual); actual = NULL;
    g_slist_free(expected); expected = NULL;
}

void
release_is_new_tests(void** state)
{
    // Higher major version
    assert_true(release_is_new("0.16.0", "1.0.0"));
    // Higher minor version
    assert_true(release_is_new("0.16.0", "0.17.0"));
    // Higher patch version
    assert_true(release_is_new("0.16.0", "0.16.1"));

    // Same version
    assert_false(release_is_new("0.16.0", "0.16.0"));

    // Lower major version
    assert_false(release_is_new("0.16.0", "0.15.9"));
    // Lower minor version
    assert_false(release_is_new("0.16.0", "0.15.0"));
    // Lower patch version
    assert_false(release_is_new("0.16.1", "0.16.0"));

    // Higher version but with different current versions
    assert_true(release_is_new("1.2.3", "1.2.4"));
    assert_true(release_is_new("1.2.3", "1.3.0"));
    assert_true(release_is_new("1.2.3", "2.0.0"));

    // Malformed version strings
    assert_false(release_is_new("0.16.0", "0.16"));      // Missing patch in found
    assert_false(release_is_new("0.16", "0.16.0"));      // Missing patch in curr
    assert_false(release_is_new("0.16.0", "0.16.0.1"));  // Extra part

    assert_false(release_is_new("0.16.0", "abc.def.ghi"));
    assert_false(release_is_new("0.16.0", ""));
    assert_false(release_is_new(NULL, "1.0.0"));
    assert_false(release_is_new("1.0.0", NULL));
}

void
prof_whole_occurrences_tests(void** state)
{
    GSList* actual = NULL;
    GSList* expected = NULL;
    assert_true(_lists_equal(prof_occurrences(NULL, NULL, 0, FALSE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5 hi", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5: hi", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5, hi", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "我能吞下玻璃而", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "我能吞下玻璃而 hi", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "我能吞下玻璃而: hi", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "我能吞下玻璃而, hi", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(6));
    assert_true(_lists_equal(prof_occurrences("boothj5", "hello boothj5", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "hello boothj5 there", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "heyy @boothj5, there", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(6));
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hello 我能吞下玻璃而", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hello 我能吞下玻璃而 there", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "heyy @我能吞下玻璃而, there", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(6));
    expected = g_slist_append(expected, GINT_TO_POINTER(26));
    assert_true(_lists_equal(prof_occurrences("boothj5", "hello boothj5 some more a boothj5 stuff", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "hello boothj5 there ands #boothj5", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "heyy @boothj5, there hows boothj5?", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(6));
    expected = g_slist_append(expected, GINT_TO_POINTER(26));
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hello 我能吞下玻璃而 some more a 我能吞下玻璃而 stuff", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hello 我能吞下玻璃而 there ands #我能吞下玻璃而", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "heyy @我能吞下玻璃而, there hows 我能吞下玻璃而?", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(6));
    assert_true(_lists_equal(prof_occurrences("p", "ppppp p", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(0));
    assert_true(_lists_equal(prof_occurrences("p", "p ppppp", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = g_slist_append(expected, GINT_TO_POINTER(4));
    assert_true(_lists_equal(prof_occurrences("p", "ppp p ppp", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5hello", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "heyboothj5", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "heyboothj5hithere", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "hey boothj5hithere", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "hey @boothj5hithere", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "heyboothj5 hithere", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "heyboothj5, hithere", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5boothj5", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("boothj5", "boothj5fillboothj5", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k", "dont know", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k", "kick", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k", "kick kick", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k", "kick kickk", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k", "kic", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k", "ick", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k", "kk", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("k", "kkkkkkk", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;

    expected = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "我能吞下玻璃而hello", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hey我能吞下玻璃而", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hey我能吞下玻璃而hithere", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hey 我能吞下玻璃而hithere", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hey @我能吞下玻璃而hithere", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hey我能吞下玻璃而 hithere", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "hey我能吞下玻璃而, hithere", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "我能吞下玻璃而我能吞下玻璃而", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    assert_true(_lists_equal(prof_occurrences("我能吞下玻璃而", "我能吞下玻璃而fill我能吞下玻璃而", 0, TRUE, &actual), expected));
    g_slist_free(actual);
    actual = NULL;
    g_slist_free(expected);
    expected = NULL;
}

void test_string_matches_one_of_edge_cases(void** state) {
    // is is NULL, is_can_be_null is TRUE -> should return TRUE
    assert_true(string_matches_one_of(NULL, NULL, TRUE, "option1", "option2", NULL));

    // is is NULL, is_can_be_null is FALSE -> should return FALSE
    assert_false(string_matches_one_of(NULL, NULL, FALSE, "option1", "option2", NULL));

    // is matches one of the options
    assert_true(string_matches_one_of("Test", "option1", FALSE, "option1", "option2", NULL));
    assert_true(string_matches_one_of("Test", "option2", FALSE, "option1", "option2", NULL));

    // is does not match any of the options
    expect_any_cons_show(); // For "Invalid Test: 'option3'"
    expect_any_cons_show(); // For "Test must be one of: 'option1', or 'option2'."
    assert_false(string_matches_one_of("Test", "option3", FALSE, "option1", "option2", NULL));

    // what is NULL (no error message printed)
    assert_false(string_matches_one_of(NULL, "option3", FALSE, "option1", "option2", NULL));

    // Empty options list (first is NULL)
    expect_any_cons_show(); // For "Invalid Test: 'option1'"
    expect_any_cons_show(); // For "Test must be one of: ." (empty options list error message)
    assert_false(string_matches_one_of("Test", "option1", FALSE, NULL, NULL));
    assert_false(string_matches_one_of(NULL, NULL, FALSE, NULL, NULL));
    assert_true(string_matches_one_of(NULL, NULL, TRUE, NULL, NULL));

    // Single option, matches
    assert_true(string_matches_one_of("Test", "single", FALSE, "single", NULL));

    // Single option, no match
    expect_any_cons_show(); // For "Invalid Test: 'nomatch'"
    expect_any_cons_show(); // For "Test must be one of: 'single'."
    assert_false(string_matches_one_of("Test", "nomatch", FALSE, "single", NULL));

    // Multiple options, first matches
    assert_true(string_matches_one_of("Test", "first", FALSE, "first", "second", "third", NULL));

    // Multiple options, middle matches
    assert_true(string_matches_one_of("Test", "second", FALSE, "first", "second", "third", NULL));

    // Multiple options, last matches
    assert_true(string_matches_one_of("Test", "third", FALSE, "first", "second", "third", NULL));

    // Multiple options, no match
    expect_any_cons_show(); // For "Invalid Test: 'none'"
    expect_any_cons_show(); // For "Test must be one of: 'first', 'second', or 'third'."
    assert_false(string_matches_one_of("Test", "none", FALSE, "first", "second", "third", NULL));

    // is is an empty string, options are not
    expect_any_cons_show(); // For "Invalid Test: ''"
    expect_any_cons_show(); // For "Test must be one of: 'option1', or 'option2'."
    assert_false(string_matches_one_of("Test", "", FALSE, "option1", "option2", NULL));

    // is is an empty string, one of the options is an empty string
    assert_true(string_matches_one_of("Test", "", FALSE, "option1", "", "option2", NULL));
}
