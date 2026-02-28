#ifndef TESTS_TEST_ROSTER_LIST_H
#define TESTS_TEST_ROSTER_LIST_H

void roster_get_contacts__returns__empty_list_when_none_added(void** state);
void roster_get_contacts__returns__one_element(void** state);
void roster_get_contacts__returns__correct_first_element(void** state);
void roster_get_contacts__returns__two_elements(void** state);
void roster_get_contacts__returns__correct_first_and_second_elements(void** state);
void roster_get_contacts__returns__three_elements(void** state);
void roster_get_contacts__returns__correct_first_three_elements(void** state);
void roster_add__updates__adds_once_when_called_twice_at_beginning(void** state);
void roster_add__updates__adds_once_when_called_twice_in_middle(void** state);
void roster_add__updates__adds_once_when_called_twice_at_end(void** state);
void roster_contact_autocomplete__returns__first_exists(void** state);
void roster_contact_autocomplete__returns__second_exists(void** state);
void roster_contact_autocomplete__returns__third_exists(void** state);
void roster_contact_autocomplete__returns__null_when_no_match(void** state);
void roster_contact_autocomplete__returns__null_on_empty_roster(void** state);
void roster_contact_autocomplete__returns__second_when_two_match(void** state);
void roster_contact_autocomplete__returns__fifth_when_multiple_match(void** state);
void roster_contact_autocomplete__returns__first_when_two_match_and_reset(void** state);
void roster_get_groups__returns__empty_for_no_group(void** state);
void roster_get_groups__returns__one_group(void** state);
void roster_get_groups__returns__two_groups(void** state);
void roster_get_groups__returns__three_groups(void** state);
void roster_update__updates__adding_two_groups(void** state);
void roster_update__updates__removing_one_group(void** state);
void roster_update__updates__removing_two_groups(void** state);
void roster_update__updates__removing_three_groups(void** state);
void roster_update__updates__two_new_groups(void** state);
void roster_remove__updates__contact_groups(void** state);
void roster_add__updates__different_groups(void** state);
void roster_add__updates__same_groups(void** state);
void roster_add__updates__overlapping_groups(void** state);
void roster_remove__updates__remaining_in_group(void** state);
void roster_get_display_name__returns__nickname_when_exists(void** state);
void roster_get_display_name__returns__barejid_when_nickname_empty(void** state);
void roster_get_display_name__returns__barejid_when_not_exists(void** state);

#endif
