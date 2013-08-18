/*
 * plugins.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
 *
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config/preferences.h"
#include "plugins/callbacks.h"
#include "plugins/plugins.h"
#include "plugins/python_plugins.h"
#include "plugins/c_plugins.h"
#include "plugins/python_api.h"
#include "plugins/ruby_plugins.h"
#include "plugins/ruby_api.h"
#include "ui/ui.h"



static GSList* plugins;

void
plugins_init(void)
{
    plugins = NULL;

    python_env_init();
    ruby_env_init();

    // load plugins
    gchar **plugins_load = prefs_get_plugins();
    if (plugins_load != NULL) {
        int i;
        for (i = 0; i < g_strv_length(plugins_load); i++)
        {
            gchar *filename = plugins_load[i];
            ProfPlugin *plugin = NULL;
            if (g_str_has_suffix(filename, ".py")) {
                ProfPlugin *plugin = python_plugin_create(filename);
                if (plugin != NULL) {
                    plugins = g_slist_append(plugins, plugin);
                    cons_show("Loaded python plugin: %s", filename);
                }
            // TODO include configure option to vary on windows and
            // unix i.e. so, dll, or maybe we can come up with unified
            // shared library plugin name... dunno...
            } else if (g_str_has_suffix(filename, ".so")) {
                ProfPlugin *plugin = c_plugin_create(filename);
                if (plugin != NULL) {
                    plugins = g_slist_append(plugins, plugin);
                    cons_show("Loaded C plugin: %s", filename);
                }
            } else if (g_str_has_suffix(filename, ".rb")) {
                ProfPlugin *plugin = ruby_plugin_create(filename);
                if (plugin != NULL) {
                    plugins = g_slist_append(plugins, plugin);
                    cons_show("Loaded Ruby plugin: %s", filename);
                }
            }
        }

        // initialise plugins
        GSList *curr = plugins;
        while (curr != NULL) {
            ProfPlugin *plugin = curr->data;
            plugin->init_func(plugin, PROF_PACKAGE_VERSION, PROF_PACKAGE_STATUS);
            // TODO well, it should be more of a generic check error here
            python_check_error();
            curr = g_slist_next(curr);
        }
    }

    return;
}

void
plugins_on_start(void)
{
    GSList *curr = plugins;
    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        plugin->on_start_func(plugin);
        curr = g_slist_next(curr);
    }
}

void
plugins_on_connect(void)
{
    GSList *curr = plugins;
    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        plugin->on_connect_func(plugin);
        curr = g_slist_next(curr);
    }
}

void
plugins_on_message_received(const char * const jid, const char * const message)
{
    GSList *curr = plugins;
    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        plugin->on_message_received_func(plugin, jid, message);
        curr = g_slist_next(curr);
    }
}

void
plugins_shutdown(void)
{
    GSList *curr = plugins;

    python_shutdown();

    //FIXME do we need to clean the plugins list?
    //for the time being I'll just call dlclose for
    //every C plugin.

    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        if (plugin->lang == C)
            c_close_library (plugin);

        curr = g_slist_next(curr);
    }
}
