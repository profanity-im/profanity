#ifndef TESTS_TEST_PREFERENCES_H
#define TESTS_TEST_PREFERENCES_H

void prefs_get_string__returns__all_for_console_default(void** state);
void prefs_get_string__returns__none_for_chat_default(void** state);
void prefs_get_string__returns__none_for_muc_default(void** state);

#endif
