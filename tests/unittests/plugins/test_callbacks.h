#ifndef TESTS_TEST_CALLBACKS_H
#define TESTS_TEST_CALLBACKS_H

void plugins_get_command_names__returns__no_commands(void** state);
void plugins_get_command_names__returns__commands_when_added(void** state);
void plugins_get_lua_version__returns_version_string(void** state);

#endif
