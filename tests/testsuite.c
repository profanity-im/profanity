#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <sys/stat.h>

#include "config/helpers.h"
#include "test_autocomplete.h"
#include "test_common.h"
#include "test_contact.h"
#include "test_cmd_connect.h"
#include "test_cmd_account.h"
#include "test_cmd_rooms.h"
#include "test_cmd_sub.h"
#include "test_cmd_statuses.h"
#include "test_history.h"
#include "test_jid.h"
#include "test_parser.h"
#include "test_roster_list.h"
#include "test_preferences.h"
#include "test_server_events.h"
#include "test_cmd_alias.h"
#include "test_muc.h"

#define PROF_RUN_TESTS(name) fprintf(stderr, "\n-> Running %s\n", #name); \
    fflush(stderr); \
    result += run_tests(name);

int main(int argc, char* argv[]) {
    const UnitTest common_tests[] = {
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
        unit_test(test_online_is_valid_resource_presence_string),
        unit_test(test_chat_is_valid_resource_presence_string),
        unit_test(test_away_is_valid_resource_presence_string),
        unit_test(test_xa_is_valid_resource_presence_string),
        unit_test(test_dnd_is_valid_resource_presence_string),
        unit_test(test_available_is_not_valid_resource_presence_string),
        unit_test(test_unavailable_is_not_valid_resource_presence_string),
        unit_test(test_blah_is_not_valid_resource_presence_string),
    };

    const UnitTest autocomplete_tests[] = {
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
    };

    const UnitTest history_tests[] = {
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
    };

    const UnitTest jid_tests[] = {
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
        unit_test(create_full_with_trailing_slash),
    };

    const UnitTest parser_tests[] = {
        unit_test(parse_null_returns_null),
        unit_test(parse_empty_returns_null),
        unit_test(parse_space_returns_null),
        unit_test(parse_cmd_no_args_returns_null),
        unit_test(parse_cmd_with_space_returns_null),
        unit_test(parse_cmd_with_too_few_returns_null),
        unit_test(parse_cmd_with_too_many_returns_null),
        unit_test(parse_cmd_one_arg),
        unit_test(parse_cmd_two_args),
        unit_test(parse_cmd_three_args),
        unit_test(parse_cmd_three_args_with_spaces),
        unit_test(parse_cmd_with_freetext),
        unit_test(parse_cmd_one_arg_with_freetext),
        unit_test(parse_cmd_two_args_with_freetext),
        unit_test(parse_cmd_min_zero),
        unit_test(parse_cmd_min_zero_with_freetext),
        unit_test(parse_cmd_with_quoted),
        unit_test(parse_cmd_with_quoted_and_space),
        unit_test(parse_cmd_with_quoted_and_many_spaces),
        unit_test(parse_cmd_with_many_quoted_and_many_spaces),
        unit_test(parse_cmd_freetext_with_quoted),
        unit_test(parse_cmd_freetext_with_quoted_and_space),
        unit_test(parse_cmd_freetext_with_quoted_and_many_spaces),
        unit_test(parse_cmd_freetext_with_many_quoted_and_many_spaces),
        unit_test(parse_cmd_with_quoted_freetext),
        unit_test(parse_cmd_with_third_arg_quoted_0_min_3_max),
        unit_test(parse_cmd_with_second_arg_quoted_0_min_3_max),
        unit_test(parse_cmd_with_second_and_third_arg_quoted_0_min_3_max),
        unit_test(count_one_token),
        unit_test(count_one_token_quoted_no_whitespace),
        unit_test(count_one_token_quoted_with_whitespace),
        unit_test(count_two_tokens),
        unit_test(count_two_tokens_first_quoted),
        unit_test(count_two_tokens_second_quoted),
        unit_test(count_two_tokens_both_quoted),
        unit_test(get_first_of_one),
        unit_test(get_first_of_two),
        unit_test(get_first_two_of_three),
        unit_test(get_first_two_of_three_first_quoted),
        unit_test(get_first_two_of_three_second_quoted),
        unit_test(get_first_two_of_three_first_and_second_quoted),
    };

    const UnitTest roster_list_tests[] = {
        unit_test(empty_list_when_none_added),
        unit_test(contains_one_element),
        unit_test(first_element_correct),
        unit_test(contains_two_elements),
        unit_test(first_and_second_elements_correct),
        unit_test(contains_three_elements),
        unit_test(first_three_elements_correct),
        unit_test(add_twice_at_beginning_adds_once),
        unit_test(add_twice_in_middle_adds_once),
        unit_test(add_twice_at_end_adds_once),
        unit_test(find_first_exists),
        unit_test(find_second_exists),
        unit_test(find_third_exists),
        unit_test(find_returns_null),
        unit_test(find_on_empty_returns_null),
        unit_test(find_twice_returns_second_when_two_match),
        unit_test(find_five_times_finds_fifth),
        unit_test(find_twice_returns_first_when_two_match_and_reset),
    };

    const UnitTest cmd_connect_tests[] = {
        unit_test(cmd_connect_shows_message_when_disconnecting),
        unit_test(cmd_connect_shows_message_when_connecting),
        unit_test(cmd_connect_shows_message_when_connected),
        unit_test(cmd_connect_shows_message_when_undefined),
        unit_test(cmd_connect_when_no_account),
        unit_test(cmd_connect_fail_message),
        unit_test(cmd_connect_lowercases_argument),
        unit_test(cmd_connect_asks_password_when_not_in_account),
        unit_test(cmd_connect_shows_message_when_connecting_with_account),
        unit_test(cmd_connect_connects_with_account),
        unit_test(cmd_connect_shows_usage_when_no_server_value),
        unit_test(cmd_connect_shows_usage_when_server_no_port_value),
        unit_test(cmd_connect_shows_usage_when_no_port_value),
        unit_test(cmd_connect_shows_usage_when_port_no_server_value),
        unit_test(cmd_connect_shows_message_when_port_0),
        unit_test(cmd_connect_shows_message_when_port_minus1),
        unit_test(cmd_connect_shows_message_when_port_65536),
        unit_test(cmd_connect_shows_message_when_port_contains_chars),
        unit_test(cmd_connect_with_server_when_provided),
        unit_test(cmd_connect_with_port_when_provided),
        unit_test(cmd_connect_with_server_and_port_when_provided),
        unit_test(cmd_connect_shows_usage_when_server_provided_twice),
        unit_test(cmd_connect_shows_usage_when_port_provided_twice),
        unit_test(cmd_connect_shows_usage_when_invalid_first_property),
        unit_test(cmd_connect_shows_usage_when_invalid_second_property),
    };

    const UnitTest cmd_rooms_tests[] = {
        unit_test(cmd_rooms_shows_message_when_disconnected),
        unit_test(cmd_rooms_shows_message_when_disconnecting),
        unit_test(cmd_rooms_shows_message_when_connecting),
        unit_test(cmd_rooms_shows_message_when_started),
        unit_test(cmd_rooms_shows_message_when_undefined),
        unit_test(cmd_rooms_uses_account_default_when_no_arg),
        unit_test(cmd_rooms_arg_used_when_passed),
    };

    const UnitTest cmd_account_tests[] = {
        unit_test(cmd_account_shows_usage_when_not_connected_and_no_args),
        unit_test(cmd_account_shows_account_when_connected_and_no_args),
        unit_test(cmd_account_list_shows_accounts),
        unit_test(cmd_account_show_shows_usage_when_no_arg),
        unit_test(cmd_account_show_shows_message_when_account_does_not_exist),
        unit_test(cmd_account_show_shows_account_when_exists),
        unit_test(cmd_account_add_shows_usage_when_no_arg),
        unit_test(cmd_account_add_adds_account),
        unit_test(cmd_account_add_shows_message),
        unit_test(cmd_account_enable_shows_usage_when_no_arg),
        unit_test(cmd_account_enable_enables_account),
        unit_test(cmd_account_enable_shows_message_when_enabled),
        unit_test(cmd_account_enable_shows_message_when_account_doesnt_exist),
        unit_test(cmd_account_disable_shows_usage_when_no_arg),
        unit_test(cmd_account_disable_disables_account),
        unit_test(cmd_account_disable_shows_message_when_disabled),
        unit_test(cmd_account_disable_shows_message_when_account_doesnt_exist),
        unit_test(cmd_account_rename_shows_usage_when_no_args),
        unit_test(cmd_account_rename_shows_usage_when_one_arg),
        unit_test(cmd_account_rename_renames_account),
        unit_test(cmd_account_rename_shows_message_when_renamed),
        unit_test(cmd_account_rename_shows_message_when_not_renamed),
        unit_test(cmd_account_set_shows_usage_when_no_args),
        unit_test(cmd_account_set_shows_usage_when_one_arg),
        unit_test(cmd_account_set_shows_usage_when_two_args),
        unit_test(cmd_account_set_checks_account_exists),
        unit_test(cmd_account_set_shows_message_when_account_doesnt_exist),
        unit_test(cmd_account_set_jid_shows_message_for_malformed_jid),
        unit_test(cmd_account_set_jid_sets_barejid),
        unit_test(cmd_account_set_jid_sets_resource),
        unit_test(cmd_account_set_server_sets_server),
        unit_test(cmd_account_set_server_shows_message),
        unit_test(cmd_account_set_resource_sets_resource),
        unit_test(cmd_account_set_resource_shows_message),
        unit_test(cmd_account_set_password_sets_password),
        unit_test(cmd_account_set_password_shows_message),
        unit_test(cmd_account_set_muc_sets_muc),
        unit_test(cmd_account_set_muc_shows_message),
        unit_test(cmd_account_set_nick_sets_nick),
        unit_test(cmd_account_set_nick_shows_message),
        unit_test(cmd_account_set_status_shows_message_when_invalid_status),
        unit_test(cmd_account_set_status_sets_status_when_valid),
        unit_test(cmd_account_set_status_sets_status_when_last),
        unit_test(cmd_account_set_status_shows_message_when_set_valid),
        unit_test(cmd_account_set_status_shows_message_when_set_last),
        unit_test(cmd_account_set_invalid_presence_string_priority_shows_message),
        unit_test(cmd_account_set_last_priority_shows_message),
        unit_test(cmd_account_set_online_priority_sets_preference),
        unit_test(cmd_account_set_chat_priority_sets_preference),
        unit_test(cmd_account_set_away_priority_sets_preference),
        unit_test(cmd_account_set_xa_priority_sets_preference),
        unit_test(cmd_account_set_dnd_priority_sets_preference),
        unit_test(cmd_account_set_online_priority_shows_message),
        unit_test(cmd_account_set_priority_too_low_shows_message),
        unit_test(cmd_account_set_priority_too_high_shows_message),
        unit_test(cmd_account_set_priority_when_not_number_shows_message),
        unit_test(cmd_account_set_priority_when_empty_shows_message),
        unit_test(cmd_account_set_priority_updates_presence_when_account_connected_with_presence),
        unit_test(cmd_account_clear_shows_usage_when_no_args),
        unit_test(cmd_account_clear_shows_usage_when_one_arg),
        unit_test(cmd_account_clear_checks_account_exists),
        unit_test(cmd_account_clear_shows_message_when_account_doesnt_exist),
        unit_test(cmd_account_clear_shows_message_when_invalid_property),
    };

    const UnitTest cmd_sub_tests[] = {
        unit_test(cmd_sub_shows_message_when_not_connected),
        unit_test(cmd_sub_shows_usage_when_no_arg),
    };

    const UnitTest contact_tests[] = {
        unit_test(contact_in_group),
        unit_test(contact_not_in_group),
        unit_test(contact_name_when_name_exists),
        unit_test(contact_jid_when_name_not_exists),
        unit_test(contact_string_when_name_exists),
        unit_test(contact_string_when_name_not_exists),
        unit_test(contact_string_when_default_resource),
        unit_test(contact_presence_offline),
        unit_test(contact_presence_uses_highest_priority),
        unit_test(contact_presence_chat_when_same_prioroty),
        unit_test(contact_presence_online_when_same_prioroty),
        unit_test(contact_presence_away_when_same_prioroty),
        unit_test(contact_presence_xa_when_same_prioroty),
        unit_test(contact_presence_dnd),
        unit_test(contact_subscribed_when_to),
        unit_test(contact_subscribed_when_both),
        unit_test(contact_not_subscribed_when_from),
        unit_test(contact_not_subscribed_when_no_subscription_value),
        unit_test(contact_not_available),
        unit_test(contact_not_available_when_highest_priority_away),
        unit_test(contact_not_available_when_highest_priority_xa),
        unit_test(contact_not_available_when_highest_priority_dnd),
        unit_test(contact_available_when_highest_priority_online),
        unit_test(contact_available_when_highest_priority_chat),
    };

    const UnitTest cmd_statuses_tests[] = {
        unit_test(cmd_statuses_shows_usage_when_bad_subcmd),
        unit_test(cmd_statuses_shows_usage_when_bad_console_setting),
        unit_test(cmd_statuses_shows_usage_when_bad_chat_setting),
        unit_test(cmd_statuses_shows_usage_when_bad_muc_setting),
        unit_test_setup_teardown(cmd_statuses_console_sets_all,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(cmd_statuses_console_sets_online,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(cmd_statuses_console_sets_none,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(cmd_statuses_chat_sets_all,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(cmd_statuses_chat_sets_online,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(cmd_statuses_chat_sets_none,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(cmd_statuses_muc_sets_on,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(cmd_statuses_muc_sets_off,
            init_preferences,
            close_preferences),
    };

    const UnitTest preferences_tests[] = {
        unit_test_setup_teardown(statuses_console_defaults_to_all,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(statuses_chat_defaults_to_all,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(statuses_muc_defaults_to_on,
            init_preferences,
            close_preferences),
    };

    const UnitTest server_events_tests[] = {
        unit_test_setup_teardown(console_doesnt_show_online_presence_when_set_none,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(console_shows_online_presence_when_set_online,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(console_shows_online_presence_when_set_all,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(console_doesnt_show_dnd_presence_when_set_none,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(console_doesnt_show_dnd_presence_when_set_online,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(console_shows_dnd_presence_when_set_all,
            init_preferences,
            close_preferences),
        unit_test(handle_message_error_when_no_recipient),
        unit_test_setup_teardown(handle_message_error_when_recipient_cancel,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(handle_message_error_when_recipient_cancel_disables_chat_session,
            init_preferences,
            close_preferences),
        unit_test(handle_message_error_when_recipient_and_no_type),
        unit_test(handle_presence_error_when_no_recipient),
        unit_test(handle_presence_error_when_no_recipient_and_conflict),
        unit_test(handle_presence_error_when_nick_conflict_shows_recipient_error),
        unit_test(handle_presence_error_when_nick_conflict_does_not_join_room),
        unit_test(handle_presence_error_when_from_recipient_not_conflict),
    };

    const UnitTest cmd_alias_tests[] = {
        unit_test(cmd_alias_add_shows_usage_when_no_args),
        unit_test(cmd_alias_add_shows_usage_when_no_value),
        unit_test(cmd_alias_remove_shows_usage_when_no_args),
        unit_test(cmd_alias_show_usage_when_invalid_subcmd),
        unit_test_setup_teardown(cmd_alias_add_adds_alias,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(cmd_alias_add_shows_message_when_exists,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(cmd_alias_remove_removes_alias,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(cmd_alias_remove_shows_message_when_no_alias,
            init_preferences,
            close_preferences),
        unit_test_setup_teardown(cmd_alias_list_shows_all_aliases,
            init_preferences,
            close_preferences),
    };

    const UnitTest muc_tests[] = {
        unit_test_setup_teardown(test_muc_add_invite, muc_before_test, muc_after_test),
        unit_test_setup_teardown(test_muc_remove_invite, muc_before_test, muc_after_test),
        unit_test_setup_teardown(test_muc_invite_count_0, muc_before_test, muc_after_test),
        unit_test_setup_teardown(test_muc_invite_count_5, muc_before_test, muc_after_test),
        unit_test_setup_teardown(test_muc_room_is_not_active, muc_before_test, muc_after_test),
        unit_test_setup_teardown(test_muc_room_is_active, muc_before_test, muc_after_test),
    };

    int bak, bak2, new;
    fflush(stdout);
    fflush(stderr);
    bak = dup(1);
    bak2 = dup(2);
    remove("./testsuite.out");
    new = open("./testsuite.out", O_WRONLY | O_CREAT);
    chmod("./testsuite.out", S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    dup2(new, 1);
    dup2(new, 2);
    close(new);

    int result = 0;

    PROF_RUN_TESTS(common_tests);
    PROF_RUN_TESTS(autocomplete_tests);
    PROF_RUN_TESTS(history_tests);
    PROF_RUN_TESTS(jid_tests);
    PROF_RUN_TESTS(parser_tests);
    PROF_RUN_TESTS(roster_list_tests);
    PROF_RUN_TESTS(cmd_connect_tests);
    PROF_RUN_TESTS(cmd_rooms_tests);
    PROF_RUN_TESTS(cmd_account_tests);
    PROF_RUN_TESTS(cmd_sub_tests);
    PROF_RUN_TESTS(contact_tests);
    PROF_RUN_TESTS(cmd_statuses_tests);
    PROF_RUN_TESTS(preferences_tests);
    PROF_RUN_TESTS(cmd_alias_tests);
    PROF_RUN_TESTS(server_events_tests);
    PROF_RUN_TESTS(muc_tests);

    fflush(stdout);
    dup2(bak, 1);
    dup2(bak2, 2);
    close(bak);
    close(bak2);

    if (result > 0) {
        printf("\n\nFAILED TESTS, see ./testsuite.out\n\n");
        return 1;
    } else {
        printf("\n\nAll tests passed\n\n");
        return 0;
    }
}
