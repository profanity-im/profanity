#ifndef TESTS_TEST_FORM_H
#define TESTS_TEST_FORM_H

void form_get_form_type_field__returns__null_no_fields(void** state);
void form_get_form_type_field__returns__null_when_not_present(void** state);
void form_get_form_type_field__returns__value_when_present(void** state);
void form_get_field_type__returns__unknown_when_no_fields(void** state);
void form_get_field_type__returns__correct_type(void** state);
void form_set_value__updates__adds_when_none(void** state);
void form_set_value__updates__updates_when_one(void** state);
void form_add_unique_value__updates__adds_when_none(void** state);
void form_add_unique_value__updates__does_nothing_when_exists(void** state);
void form_add_unique_value__updates__adds_when_doesnt_exist(void** state);
void form_add_value__updates__adds_when_none(void** state);
void form_add_value__updates__adds_when_some(void** state);
void form_add_value__updates__adds_when_exists(void** state);
void form_remove_value__updates__does_nothing_when_none(void** state);
void form_remove_value__updates__does_nothing_when_doesnt_exist(void** state);
void form_remove_value__updates__removes_when_one(void** state);
void form_remove_value__updates__removes_when_many(void** state);
void form_remove_text_multi_value__updates__does_nothing_when_none(void** state);
void form_remove_text_multi_value__updates__does_nothing_when_doesnt_exist(void** state);
void form_remove_text_multi_value__updates__removes_when_one(void** state);
void form_remove_text_multi_value__updates__removes_when_many(void** state);

#endif
