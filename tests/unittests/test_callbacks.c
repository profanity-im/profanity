#include "prof_cmocka.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "plugins/callbacks.h"
#include "plugins/plugins.h"

void
returns_no_commands(void** state)
{
    plugins_init();
    GList* commands = plugins_get_command_names();

    assert_true(commands == NULL);
}

void
returns_commands(void** state)
{
    plugins_init();
    PluginCommand* command1 = calloc(1, sizeof(PluginCommand));
    command1->command_name = strdup("command1");
    callbacks_add_command("plugin1", command1);

    PluginCommand* command2 = calloc(1, sizeof(PluginCommand));
    command2->command_name = strdup("command2");
    callbacks_add_command("plugin1", command2);

    PluginCommand* command3 = calloc(1, sizeof(PluginCommand));
    command3->command_name = strdup("command3");
    callbacks_add_command("plugin2", command3);

    GList* names = plugins_get_command_names();
    assert_true(g_list_length(names) == 3);

    gboolean foundCommand1 = FALSE;
    gboolean foundCommand2 = FALSE;
    gboolean foundCommand3 = FALSE;
    GList* curr = names;
    while (curr) {
        if (g_strcmp0(curr->data, "command1") == 0) {
            foundCommand1 = TRUE;
        }
        if (g_strcmp0(curr->data, "command2") == 0) {
            foundCommand2 = TRUE;
        }
        if (g_strcmp0(curr->data, "command3") == 0) {
            foundCommand3 = TRUE;
        }
        curr = g_list_next(curr);
    }

    assert_true(foundCommand1 && foundCommand2 && foundCommand3);

    g_list_free(names);
}
