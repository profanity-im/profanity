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

#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "config/preferences.h"
#include "log.h"
#include "plugins/callbacks.h"
#include "plugins/plugins.h"
#include "plugins/python_plugins.h"
#include "plugins/python_api.h"
#include "plugins/c_plugins.h"
#include "plugins/c_api.h"
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
    c_env_init();

    // load plugins
    gchar **plugins_load = prefs_get_plugins();
    if (plugins_load != NULL) {
        int i;
        for (i = 0; i < g_strv_length(plugins_load); i++)
        {
            gboolean loaded = FALSE;
            gchar *filename = plugins_load[i];
            if (g_str_has_suffix(filename, ".py")) {
                ProfPlugin *plugin = python_plugin_create(filename);
                if (plugin != NULL) {
                    plugins = g_slist_append(plugins, plugin);
                    loaded = TRUE;
                }
            } else if (g_str_has_suffix(filename, ".so")) {
                ProfPlugin *plugin = c_plugin_create(filename);
                if (plugin != NULL) {
                    plugins = g_slist_append(plugins, plugin);
                    loaded = TRUE;
                }
            } else if (g_str_has_suffix(filename, ".rb")) {
                ProfPlugin *plugin = ruby_plugin_create(filename);
                if (plugin != NULL) {
                    plugins = g_slist_append(plugins, plugin);
                    loaded = TRUE;
                }
            }
            if (loaded == TRUE) {
                log_info("Loaded plugin: %s", filename);
            }
        }

        // initialise plugins
        GSList *curr = plugins;
        while (curr != NULL) {
            ProfPlugin *plugin = curr->data;
            plugin->init_func(plugin, PROF_PACKAGE_VERSION, PROF_PACKAGE_STATUS);
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
plugins_on_connect(const char * const account_name, const char * const fulljid)
{
    GSList *curr = plugins;
    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        plugin->on_connect_func(plugin, account_name, fulljid);
        curr = g_slist_next(curr);
    }
}

char *
plugins_on_message_received(const char * const jid, const char *message)
{
    GSList *curr = plugins;
    char *new_message = NULL;
    char *curr_message = strdup(message);

    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->on_message_received_func(plugin, jid, curr_message);
        if (new_message != NULL) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    return curr_message;
}

char *
plugins_on_message_send(const char * const jid, const char *message)
{
    GSList *curr = plugins;
    char *new_message = NULL;
    char *curr_message = strdup(message);

    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->on_message_send_func(plugin, jid, curr_message);
        if (new_message != NULL) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    return curr_message;
}

void
plugins_shutdown(void)
{
    GSList *curr = plugins;

    python_shutdown();
    ruby_shutdown();
    c_shutdown();

    //FIXME do we need to clean the plugins list?
    //for the time being I'll just call dlclose for
    //every C plugin.

    while (curr != NULL) {
        ProfPlugin *plugin = curr->data;
        if (plugin->lang == LANG_C)
            c_close_library (plugin);

        curr = g_slist_next(curr);
    }
}

gchar *
plugins_get_dir(void)
{
    gchar *xdg_data = xdg_get_data_home();
    GString *plugins_dir = g_string_new(xdg_data);
    g_string_append(plugins_dir, "/profanity/plugins");
    gchar *result = strdup(plugins_dir->str);
    g_free(xdg_data);
    g_string_free(plugins_dir, TRUE);

    return result;
}
