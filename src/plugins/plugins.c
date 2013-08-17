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

#include <Python.h>

#include "config/preferences.h"
#include "plugins/api.h"
#include "plugins/callbacks.h"
#include "plugins/plugins.h"
#include "plugins/python_api.h"
#include "plugins/python_plugins.h"
#include "ui/ui.h"

static GSList* plugins;

void
plugins_init(void)
{
    plugins = NULL;

    // initialse python and path
    Py_Initialize();
    python_check_error();
    python_api_init();
    python_check_error();
    // TODO change to use XDG spec
    GString *path = g_string_new(Py_GetPath());
    g_string_append(path, ":./plugins/");
    PySys_SetPath(path->str);
    python_check_error();
    g_string_free(path, TRUE);

    // load plugins
    gchar **plugins_load = prefs_get_plugins();
    if (plugins_load != NULL) {
        int i;
        for (i = 0; i < g_strv_length(plugins_load); i++)
        {
            gchar *filename = plugins_load[i];
            if (g_str_has_suffix(filename, ".py")) {
                ProfPlugin *plugin = python_plugin_create(filename);
                if (plugin != NULL) {
                    plugins = g_slist_append(plugins, plugin);
                    cons_show("Loaded plugin: %s", filename);
                }
            }
        }

        // initialise plugins
        GSList *curr = plugins;
        while (curr != NULL) {
            ProfPlugin *plugin = curr->data;
            plugin->init_func(plugin, PACKAGE_VERSION, PACKAGE_STATUS);
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
plugins_on_message(const char * const jid, const char * const message)
{
    GSList *curr = plugins;
    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        plugin->on_message_func(plugin, jid, message);
        curr = g_slist_next(curr);
    }
}

void
plugins_shutdown(void)
{
    Py_Finalize();
}
