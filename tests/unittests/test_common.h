#ifndef TESTS_TEST_COMMON_H
#define TESTS_TEST_COMMON_H

void str_replace__returns__one_substr(void** state);
void str_replace__returns__one_substr_beginning(void** state);
void str_replace__returns__one_substr_end(void** state);
void str_replace__returns__two_substr(void** state);
void str_replace__returns__char(void** state);
void str_replace__returns__original_when_none(void** state);
void str_replace__returns__replaced_when_match(void** state);
void str_replace__returns__empty_when_string_empty(void** state);
void str_replace__returns__null_when_string_null(void** state);
void str_replace__returns__original_when_sub_empty(void** state);
void str_replace__returns__original_when_sub_null(void** state);
void str_replace__returns__empty_when_new_empty(void** state);
void str_replace__returns__original_when_new_null(void** state);

void valid_resource_presence_string__is__true_for_online(void** state);
void valid_resource_presence_string__is__true_for_chat(void** state);
void valid_resource_presence_string__is__true_for_away(void** state);
void valid_resource_presence_string__is__true_for_xa(void** state);
void valid_resource_presence_string__is__true_for_dnd(void** state);
void valid_resource_presence_string__is__false_for_available(void** state);
void valid_resource_presence_string__is__false_for_unavailable(void** state);
void valid_resource_presence_string__is__false_for_blah(void** state);

void utf8_display_len__returns__0_for_null(void** state);
void utf8_display_len__returns__1_for_non_wide(void** state);
void utf8_display_len__returns__2_for_wide(void** state);
void utf8_display_len__returns__correct_for_non_wide(void** state);
void utf8_display_len__returns__correct_for_wide(void** state);
void utf8_display_len__returns__correct_for_all_wide(void** state);

void strip_arg_quotes__returns__original_when_no_quotes(void** state);
void strip_arg_quotes__returns__stripped_first(void** state);
void strip_arg_quotes__returns__stripped_last(void** state);
void strip_arg_quotes__returns__stripped_both(void** state);

void prof_occurrences__tests__partial(void** state);
void prof_occurrences__tests__whole(void** state);
void prof_occurrences__tests__large_message(void** state);
void unique_filename_from_url__tests__table_driven(void** state);
void format_call_external_argv__tests__table_driven(void** state);
void get_expanded_path__returns__expanded(void** state);
void string_to_verbosity__returns__correct_values(void** state);
void strtoi_range__returns__true_for_valid_input(void** state);
void strtoi_range__returns__false_for_out_of_range(void** state);
void strtoi_range__returns__false_for_invalid_input(void** state);
void strtoi_range__returns__false_for_null_empty_input(void** state);
void strtoi_range__returns__correct_values_when_err_msg_null(void** state);
void string_matches_one_of__tests__edge_cases(void** state);
void valid_tls_policy_option__is__correct_for_various_inputs(void** state);
void get_mentions__tests__various(void** state);
void release_is_new__tests__various(void** state);

#endif
