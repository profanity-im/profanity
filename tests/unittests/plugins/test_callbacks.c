#include "prof_cmocka.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "plugins/callbacks.h"
#include "plugins/autocompleters.h"
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
plugins_autocompleters__removal(void** state)
{
    autocompleters_init();

    char* items1[] = { "apple", "apricot", NULL };
    autocompleters_add("plugin1", "/fruit", items1);

    char* items2[] = { "banana", "blueberry", NULL };
    autocompleters_add("plugin2", "/berry", items2);

    autocompleters_filepath_add("plugin1", "/sendfile1");
    autocompleters_filepath_add("plugin2", "/sendfile2");

    char* res1 = autocompleters_complete("/fruit ap", FALSE);
    assert_non_null(res1);
    assert_string_equal(res1, "/fruit apple");
    g_free(res1);

    char* res2 = autocompleters_complete("/berry ban", FALSE);
    assert_non_null(res2);
    assert_string_equal(res2, "/berry banana");
    g_free(res2);

    autocompleters_remove_plugin("plugin1");

    char* res_removed = autocompleters_complete("/fruit ap", FALSE);
    assert_null(res_removed);

    char* res_retained = autocompleters_complete("/berry ban", FALSE);
    assert_non_null(res_retained);
    assert_string_equal(res_retained, "/berry banana");
    g_free(res_retained);

    autocompleters_remove_plugin("plugin2");

    char* res_none = autocompleters_complete("/berry ban", FALSE);
    assert_null(res_none);

    autocompleters_destroy();
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

void
plugins_find_plugin_path__tests(void** state)
{
    assert_null(plugins_find_plugin_path(NULL));
    assert_null(plugins_find_plugin_path("definitely_nonexistent_plugin_9999.lua"));
}
