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

void prof_strstr_contains(void **state)
{
    assert_true(prof_strstr(NULL,      "some string",           FALSE, FALSE) == FALSE);
    assert_true(prof_strstr("boothj5", NULL,                    FALSE, FALSE) == FALSE);
    assert_true(prof_strstr(NULL,      NULL,                    FALSE, FALSE) == FALSE);

    assert_true(prof_strstr("boothj5", "boothj5",               FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("boothj5", "boothj5 hello",         FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("boothj5", "hello boothj5",         FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("boothj5", "hello boothj5 there",   FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("boothj5", "helloboothj5test",      FALSE, FALSE) == TRUE);

    assert_true(prof_strstr("boothj5", "BoothJ5",               FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("boothj5", "BoothJ5 hello",         FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("boothj5", "hello BoothJ5",         FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("boothj5", "hello BoothJ5 there",   FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("boothj5", "helloBoothJ5test",      FALSE, FALSE) == TRUE);

    assert_true(prof_strstr("BoothJ5", "boothj5",               FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("BoothJ5", "boothj5 hello",         FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("BoothJ5", "hello boothj5",         FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("BoothJ5", "hello boothj5 there",   FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("BoothJ5", "helloboothj5test",      FALSE, FALSE) == TRUE);

    assert_true(prof_strstr("boothj5", "BoothJ5",               TRUE, FALSE) == FALSE);
    assert_true(prof_strstr("boothj5", "BoothJ5 hello",         TRUE, FALSE) == FALSE);
    assert_true(prof_strstr("boothj5", "hello BoothJ5",         TRUE, FALSE) == FALSE);
    assert_true(prof_strstr("boothj5", "hello BoothJ5 there",   TRUE, FALSE) == FALSE);
    assert_true(prof_strstr("boothj5", "helloBoothJ5test",      TRUE, FALSE) == FALSE);

    assert_true(prof_strstr("BoothJ5", "boothj5",               TRUE, FALSE) == FALSE);
    assert_true(prof_strstr("BoothJ5", "boothj5 hello",         TRUE, FALSE) == FALSE);
    assert_true(prof_strstr("BoothJ5", "hello boothj5",         TRUE, FALSE) == FALSE);
    assert_true(prof_strstr("BoothJ5", "hello boothj5 there",   TRUE, FALSE) == FALSE);
    assert_true(prof_strstr("BoothJ5", "helloboothj5test",      TRUE, FALSE) == FALSE);

    assert_true(prof_strstr("boothj5", "boothj5",               FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", "boothj5 hello",         FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", "hello boothj5",         FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", "hello boothj5 there",   FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", "boothj5test",           FALSE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", "helloboothj5",          FALSE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", "helloboothj5test",      FALSE, TRUE) == FALSE);

    assert_true(prof_strstr("boothj5", "BoothJ5",               TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", "BoothJ5 hello",         TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", "hello BoothJ5",         TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", "hello BoothJ5 there",   TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", "BoothJ5test",           TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", "helloBoothJ5",          TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", "helloBoothJ5test",      TRUE, TRUE) == FALSE);

    assert_true(prof_strstr("BoothJ5", "boothj5",               TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("BoothJ5", "boothj5 hello",         TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("BoothJ5", "hello boothj5",         TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("BoothJ5", "hello boothj5 there",   TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("BoothJ5", "boothj5test",           TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("BoothJ5", "helloboothj5",          TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("BoothJ5", "helloboothj5test",      TRUE, TRUE) == FALSE);

    assert_true(prof_strstr("boothj5", "boothj5:",              FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", "boothj5,",              FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", "boothj5-",              FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", ":boothj5",              FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", ",boothj5",              FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", "-boothj5",              FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", ":boothj5:",             FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", ",boothj5,",             FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", "-boothj5-",             FALSE, TRUE) == TRUE);

    assert_true(prof_strstr("boothj5", "BoothJ5:",              FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", "BoothJ5,",              FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", "BoothJ5-",              FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", ":BoothJ5",              FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", ",BoothJ5",              FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", "-BoothJ5",              FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", ":BoothJ5:",             FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", ",BoothJ5,",             FALSE, TRUE) == TRUE);
    assert_true(prof_strstr("boothj5", "-BoothJ5-",             FALSE, TRUE) == TRUE);

    assert_true(prof_strstr("boothj5", "BoothJ5:",              TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", "BoothJ5,",              TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", "BoothJ5-",              TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", ":BoothJ5",              TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", ",BoothJ5",              TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", "-BoothJ5",              TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", ":BoothJ5:",             TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", ",BoothJ5,",             TRUE, TRUE) == FALSE);
    assert_true(prof_strstr("boothj5", "-BoothJ5-",             TRUE, TRUE) == FALSE);

    assert_true(prof_strstr("K", "don't know", FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("K", "don't know", TRUE,  FALSE) == FALSE);
    assert_true(prof_strstr("K", "don't know", FALSE, TRUE) == FALSE);
    assert_true(prof_strstr("K", "don't know", TRUE, TRUE) == FALSE);

    assert_true(prof_strstr("K", "don't Know", FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("K", "don't Know", TRUE,  FALSE) == TRUE);
    assert_true(prof_strstr("K", "don't Know", FALSE, TRUE) == FALSE);
    assert_true(prof_strstr("K", "don't Know", TRUE, TRUE) == FALSE);

    assert_true(prof_strstr("K", "backwards", FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("K", "backwards", TRUE,  FALSE) == FALSE);
    assert_true(prof_strstr("K", "backwards", FALSE, TRUE) == FALSE);
    assert_true(prof_strstr("K", "backwards", TRUE, TRUE) == FALSE);

    assert_true(prof_strstr("K", "BACKWARDS", FALSE, FALSE) == TRUE);
    assert_true(prof_strstr("K", "BACKWARDS", TRUE,  FALSE) == TRUE);
    assert_true(prof_strstr("K", "BACKWARDS", FALSE, TRUE) == FALSE);
    assert_true(prof_strstr("K", "BACKWARDS", TRUE, TRUE) == FALSE);
}
