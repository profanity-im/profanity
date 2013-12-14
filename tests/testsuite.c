#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "test_autocomplete.h"
#include "test_common.h"
#include "test_command.h"
#include "test_history.h"
#include "test_jid.h"

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
        unit_test(next_available_24_first_big_gap),

        unit_test(clear_empty),
        unit_test(reset_after_create),
        unit_test(find_after_create),
        unit_test(get_after_create_returns_null),
        unit_test(add_one_and_complete),
        unit_test(add_two_and_complete_returns_first),
        unit_test(add_two_and_complete_returns_second),
        unit_test(add_two_adds_two),
        unit_test(add_two_same_adds_one),
        unit_test(add_two_same_updates),

        unit_test(previous_on_empty_returns_null),
        unit_test(next_on_empty_returns_null),
        unit_test(previous_once_returns_last),
        unit_test(previous_twice_when_one_returns_first),
        unit_test(previous_always_stops_at_first),
        unit_test(previous_goes_to_correct_element),
        unit_test(prev_then_next_returns_empty),
        unit_test(prev_with_val_then_next_returns_val),
        unit_test(prev_with_val_then_next_twice_returns_null),
        unit_test(navigate_then_append_new),
        unit_test(edit_item_mid_history),
        unit_test(edit_previous_and_append),
        unit_test(start_session_add_new_submit_previous),

        unit_test(create_jid_from_null_returns_null),
        unit_test(create_jid_from_empty_string_returns_null),
        unit_test(create_jid_from_full_returns_full),
        unit_test(create_jid_from_full_returns_bare),
        unit_test(create_jid_from_full_returns_resourcepart),
        unit_test(create_jid_from_full_returns_localpart),
        unit_test(create_jid_from_full_returns_domainpart),
        unit_test(create_jid_from_full_nolocal_returns_full),
        unit_test(create_jid_from_full_nolocal_returns_bare),
        unit_test(create_jid_from_full_nolocal_returns_resourcepart),
        unit_test(create_jid_from_full_nolocal_returns_domainpart),
        unit_test(create_jid_from_full_nolocal_returns_null_localpart),
        unit_test(create_jid_from_bare_returns_null_full),
        unit_test(create_jid_from_bare_returns_null_resource),
        unit_test(create_jid_from_bare_returns_bare),
        unit_test(create_jid_from_bare_returns_localpart),
        unit_test(create_jid_from_bare_returns_domainpart),
        unit_test(create_room_jid_returns_room),
        unit_test(create_room_jid_returns_nick),
        unit_test(create_with_slash_in_resource),
        unit_test(create_with_at_in_resource),
        unit_test(create_with_at_and_slash_in_resource),
        unit_test(create_full_with_trailing_slash)
    };
    return run_tests(tests);
}
