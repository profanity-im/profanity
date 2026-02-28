#ifndef TESTS_TEST_PARSER_H
#define TESTS_TEST_PARSER_H

void parse_args__returns__null_from_null(void** state);
void parse_args__returns__null_from_empty(void** state);
void parse_args__returns__null_from_space(void** state);
void parse_args__returns__null_when_no_args(void** state);
void parse_args__returns__null_from_cmd_with_space(void** state);
void parse_args__returns__null_when_too_few(void** state);
void parse_args__returns__null_when_too_many(void** state);
void parse_args__returns__one_arg(void** state);
void parse_args__returns__two_args(void** state);
void parse_args__returns__three_args(void** state);
void parse_args__returns__three_args_with_spaces(void** state);
void parse_args_with_freetext__returns__freetext(void** state);
void parse_args_with_freetext__returns__one_arg_with_freetext(void** state);
void parse_args_with_freetext__returns__two_args_with_freetext(void** state);
void parse_args__returns__zero_args_when_min_zero(void** state);
void parse_args_with_freetext__returns__zero_args_when_min_zero(void** state);
void parse_args__returns__quoted_args(void** state);
void parse_args__returns__quoted_args_with_space(void** state);
void parse_args__returns__quoted_args_with_many_spaces(void** state);
void parse_args__returns__many_quoted_args_with_many_spaces(void** state);
void parse_args_with_freetext__returns__quoted_args(void** state);
void parse_args_with_freetext__returns__quoted_args_with_space(void** state);
void parse_args_with_freetext__returns__quoted_args_with_many_spaces(void** state);
void parse_args_with_freetext__returns__many_quoted_args_with_many_spaces(void** state);
void parse_args_with_freetext__returns__quoted_freetext(void** state);
void parse_args_with_freetext__returns__third_arg_quoted(void** state);
void parse_args_with_freetext__returns__second_arg_quoted(void** state);
void parse_args_with_freetext__returns__second_and_third_arg_quoted(void** state);
void count_tokens__returns__one_token(void** state);
void count_tokens__returns__one_token_quoted_no_whitespace(void** state);
void count_tokens__returns__one_token_quoted_with_whitespace(void** state);
void count_tokens__returns__two_tokens(void** state);
void count_tokens__returns__two_tokens_first_quoted(void** state);
void count_tokens__returns__two_tokens_second_quoted(void** state);
void count_tokens__returns__two_tokens_both_quoted(void** state);
void get_start__returns__first_of_one(void** state);
void get_start__returns__first_of_two(void** state);
void get_start__returns__first_two_of_three(void** state);
void get_start__returns__first_two_of_three_first_quoted(void** state);
void get_start__returns__first_two_of_three_second_quoted(void** state);
void get_start__returns__first_two_of_three_first_and_second_quoted(void** state);
void parse_options__returns__empty_hashmap_when_none(void** state);
void parse_options__returns__error_when_opt1_no_val(void** state);
void parse_options__returns__map_when_one(void** state);
void parse_options__returns__error_when_opt2_no_val(void** state);
void parse_options__returns__map_when_two(void** state);
void parse_options__returns__error_when_opt3_no_val(void** state);
void parse_options__returns__map_when_three(void** state);
void parse_options__returns__error_when_unknown_opt(void** state);
void parse_options__returns__error_when_duplicated_option(void** state);

#endif
