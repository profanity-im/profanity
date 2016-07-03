#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "plugins/callbacks.h"
#include "plugins/plugins.h"

void returns_no_commands(void **state)
{
    callbacks_init();
    GList *commands = plugins_get_command_names();

    assert_true(commands == NULL);
}

void returns_commands(void **state)
{
    callbacks_init();
    PluginCommand *command = malloc(sizeof(PluginCommand));
    command->command_name = strdup("something");
    callbacks_add_command("Cool plugin", command);

    GList *commands = plugins_get_command_names();
    assert_true(g_list_length(commands) == 1);

    char *name = commands->data;
    assert_string_equal(name, "something");
}
