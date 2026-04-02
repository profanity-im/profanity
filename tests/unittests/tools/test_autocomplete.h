#ifndef TESTS_TEST_AUTOCOMPLETE_H
#define TESTS_TEST_AUTOCOMPLETE_H

void autocomplete_complete__returns__null_when_empty(void** state);
void autocomplete_reset__updates__after_create(void** state);
void autocomplete_complete__returns__null_after_create(void** state);
void autocomplete_complete__handles__escaped_spaces(void** state);
void autocomplete_complete__handles__escaped_quotes(void** state);
void autocomplete_create_list__returns__null_after_create(void** state);
void autocomplete_add__updates__one_and_complete(void** state);
void autocomplete_complete__returns__first_of_two(void** state);
void autocomplete_complete__returns__second_of_two(void** state);
void autocomplete_add__updates__two_elements(void** state);
void autocomplete_add__updates__only_once_for_same_value(void** state);
void autocomplete_add__updates__existing_value(void** state);
void autocomplete_complete__returns__accented_with_accented(void** state);
void autocomplete_complete__returns__accented_with_base(void** state);
void autocomplete_complete__returns__both_with_accented(void** state);
void autocomplete_complete__returns__both_with_base(void** state);
void autocomplete_complete__is__case_insensitive(void** state);
void autocomplete_complete__returns__previous(void** state);
void autocomplete_complete__returns__greek(void** state);
void autocomplete_complete__returns__greek_false_match(void** state);
void autocomplete_complete__returns__greek_case_insensitive(void** state);
void autocomplete_complete__returns__russian(void** state);
void autocomplete_complete__returns__chinese(void** state);
void autocomplete_complete__returns__transliterated(void** state);
void autocomplete_complete__returns__regular_ascii(void** state);

#endif
