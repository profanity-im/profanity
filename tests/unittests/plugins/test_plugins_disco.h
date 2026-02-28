#ifndef TESTS_TEST_PLUGINS_DISCO_H
#define TESTS_TEST_PLUGINS_DISCO_H

void disco_get_features__returns__empty_list_when_none(void** state);
void disco_add_feature__updates__added_feature(void** state);
void disco_close__updates__resets_features(void** state);
void disco_get_features__returns__all_added_features(void** state);
void disco_add_feature__updates__not_duplicate_feature(void** state);
void disco_remove_features__updates__removes_plugin_features(void** state);
void disco_remove_features__updates__not_remove_when_more_than_one_reference(void** state);

#endif
