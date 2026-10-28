#include "prof_cmocka.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "plugins/callbacks.h"
#include "plugins/plugins.h"
#include "test_callbacks.h"

void
plugins_get_command_names__returns__no_commands(void** state)
{
    plugins_init();
    GList* commands = plugins_get_command_names();

    assert_true(commands == NULL);
}

void
plugins_get_command_names__returns__commands_when_added(void** state)
{
    plugins_init();
    PluginCommand* command1 = g_new0(PluginCommand, 1);
    command1->command_name = g_strdup("command1");
    callbacks_add_command("plugin1", command1);

    PluginCommand* command2 = g_new0(PluginCommand, 1);
    command2->command_name = g_strdup("command2");
    callbacks_add_command("plugin1", command2);

    PluginCommand* command3 = g_new0(PluginCommand, 1);
    command3->command_name = g_strdup("command3");
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

void
plugins_commands__lookup_and_removal(void** state)
{
    plugins_init();

    PluginCommand* command1 = g_new0(PluginCommand, 1);
    command1->command_name = g_strdup("/cmd1");
    CommandHelp* help1 = g_new0(CommandHelp, 1);
    help1->synopsis[0] = g_strdup("/cmd1 synopsis");
    command1->help = help1;
    callbacks_add_command("plugin1", command1);

    PluginCommand* command2 = g_new0(PluginCommand, 1);
    command2->command_name = g_strdup("/cmd2");
    callbacks_add_command("plugin2", command2);

    CommandHelp* help = plugins_get_help("/cmd1");
    assert_non_null(help);
    assert_string_equal(help->synopsis[0], "/cmd1 synopsis");

    assert_null(plugins_get_help("/nonexistent"));

    callbacks_remove("plugin1");

    assert_null(plugins_get_help("/cmd1"));

    GList* remaining = plugins_get_command_names();
    assert_int_equal(g_list_length(remaining), 1);
    assert_string_equal(remaining->data, "/cmd2");
    g_list_free(remaining);

    callbacks_close();
}

void
plugins_get_lua_version__returns_version_string(void** state)
{
    const char* ver = plugins_get_lua_version();
    assert_non_null(ver);
#ifdef HAVE_PLUGINS
    assert_string_not_equal(ver, "none");
#endif
}
