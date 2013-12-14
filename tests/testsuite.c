#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "test_common.h"
#include "test_command.h"

int main(int argc, char* argv[]) {
    const UnitTest tests[] = {
        unit_test(cmd_rooms_shows_message_when_not_connected),

        unit_test(replace_one_substr),
        unit_test(replace_one_substr_beginning),
        unit_test(replace_one_substr_end),
        unit_test(replace_two_substr),
        unit_test(replace_char),
        unit_test(replace_when_none),
        unit_test(replace_when_match),
        unit_test(replace_when_string_empty),
        unit_test(replace_when_string_null),
        unit_test(replace_when_sub_empty),
        unit_test(replace_when_sub_null),
        unit_test(replace_when_new_empty),
        unit_test(replace_when_new_null),
        unit_test(compare_win_nums_less),
        unit_test(compare_win_nums_equal),
        unit_test(compare_win_nums_greater),
        unit_test(compare_0s_equal),
        unit_test(compare_0_greater_than_1),
        unit_test(compare_1_less_than_0),
        unit_test(compare_0_less_than_11),
        unit_test(compare_11_greater_than_0),
        unit_test(compare_0_greater_than_9),
        unit_test(compare_9_less_than_0),
        unit_test(next_available_when_only_console),
        unit_test(next_available_3_at_end),
        unit_test(next_available_9_at_end),
        unit_test(next_available_0_at_end),
        unit_test(next_available_2_in_first_gap),
        unit_test(next_available_9_in_first_gap),
        unit_test(next_available_0_in_first_gap),
        unit_test(next_available_11_in_first_gap),
        unit_test(next_available_24_first_big_gap)
    };
    return run_tests(tests);
}
