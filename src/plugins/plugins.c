/*
 * plugins.c
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#include <string.h>
#include <stdlib.h>

#include "prof_config.h"
#include "common.h"
#include "config/preferences.h"
#include "log.h"
#include "plugins/callbacks.h"
#include "plugins/autocompleters.h"
#include "plugins/api.h"
#include "plugins/plugins.h"
#include "plugins/themes.h"

#ifdef PROF_HAVE_PYTHON
#include "plugins/python_plugins.h"
#include "plugins/python_api.h"
#endif

#ifdef PROF_HAVE_C
#include "plugins/c_plugins.h"
#include "plugins/c_api.h"
#endif

#include "ui/ui.h"

static GSList* plugins;

void
plugins_init(void)
{
    plugins = NULL;
    autocompleters_init();

#ifdef PROF_HAVE_PYTHON
    python_env_init();
#endif
#ifdef PROF_HAVE_C
    c_env_init();
#endif

    plugin_themes_init();

    // load plugins
    gchar **plugins_load = prefs_get_plugins();
    if (plugins_load) {
        int i;
        for (i = 0; i < g_strv_length(plugins_load); i++)
        {
            gboolean loaded = FALSE;
            gchar *filename = plugins_load[i];
#ifdef PROF_HAVE_PYTHON
            if (g_str_has_suffix(filename, ".py")) {
                ProfPlugin *plugin = python_plugin_create(filename);
                if (plugin) {
                    plugins = g_slist_append(plugins, plugin);
                    loaded = TRUE;
                }
            }
#endif
#ifdef PROF_HAVE_C
            if (g_str_has_suffix(filename, ".so")) {
                ProfPlugin *plugin = c_plugin_create(filename);
                if (plugin) {
                    plugins = g_slist_append(plugins, plugin);
                    loaded = TRUE;
                }
            }
#endif
            if (loaded == TRUE) {
                log_info("Loaded plugin: %s", filename);
            }
        }

        // initialise plugins
        GSList *curr = plugins;
        while (curr) {
            ProfPlugin *plugin = curr->data;
            plugin->init_func(plugin, PROF_PACKAGE_VERSION, PROF_PACKAGE_STATUS);
            curr = g_slist_next(curr);
        }
    }

    prefs_free_plugins(plugins_load);

    return;
}

GSList *
plugins_get_list(void)
{
    return plugins;
}

char *
plugins_get_lang_string(ProfPlugin *plugin)
{
    switch (plugin->lang)
    {
        case LANG_PYTHON:
            return "Python";
        case LANG_C:
            return "C";
        default:
            return "Unknown";
    }
}

char *
plugins_autocomplete(const char * const input)
{
    return autocompleters_complete(input);
}

void
plugins_reset_autocomplete(void)
{
    autocompleters_reset();
}

void
plugins_win_process_line(char *win, const char * const line)
{
    PluginWindowCallback *window = callbacks_get_window_handler(win);
    window->callback_func(window, win, line);
}

void
plugins_on_start(void)
{
    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_start_func(plugin);
        curr = g_slist_next(curr);
    }
}

void
plugins_on_shutdown(void)
{
    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_shutdown_func(plugin);
        curr = g_slist_next(curr);
    }
}

void
plugins_on_connect(const char * const account_name, const char * const fulljid)
{
    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_connect_func(plugin, account_name, fulljid);
        curr = g_slist_next(curr);
    }
}

void
plugins_on_disconnect(const char * const account_name, const char * const fulljid)
{
    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_disconnect_func(plugin, account_name, fulljid);
        curr = g_slist_next(curr);
    }
}

char*
plugins_pre_chat_message_display(const char * const jid, const char *message)
{
    char *new_message = NULL;
    char *curr_message = strdup(message);

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->pre_chat_message_display(plugin, jid, curr_message);
        if (new_message) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    return curr_message;
}

void
plugins_post_chat_message_display(const char * const jid, const char *message)
{
    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->post_chat_message_display(plugin, jid, message);
        curr = g_slist_next(curr);
    }
}

char*
plugins_pre_chat_message_send(const char * const jid, const char *message)
{
    char *new_message = NULL;
    char *curr_message = strdup(message);

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->pre_chat_message_send(plugin, jid, curr_message);
        if (new_message) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    return curr_message;
}

void
plugins_post_chat_message_send(const char * const jid, const char *message)
{
    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->post_chat_message_send(plugin, jid, message);
        curr = g_slist_next(curr);
    }
}

char*
plugins_pre_room_message_display(const char * const room, const char * const nick, const char *message)
{
    char *new_message = NULL;
    char *curr_message = strdup(message);

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->pre_room_message_display(plugin, room, nick, curr_message);
        if (new_message) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    return curr_message;
}

void
plugins_post_room_message_display(const char * const room, const char * const nick, const char *message)
{
    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->post_room_message_display(plugin, room, nick, message);
        curr = g_slist_next(curr);
    }
}

char*
plugins_pre_room_message_send(const char * const room, const char *message)
{
    char *new_message = NULL;
    char *curr_message = strdup(message);

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->pre_room_message_send(plugin, room, curr_message);
        if (new_message) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    return curr_message;
}

void
plugins_post_room_message_send(const char * const room, const char *message)
{
    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->post_room_message_send(plugin, room, message);
        curr = g_slist_next(curr);
    }
}

char*
plugins_pre_priv_message_display(const char * const jid, const char *message)
{
    Jid *jidp = jid_create(jid);
    char *new_message = NULL;
    char *curr_message = strdup(message);

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->pre_priv_message_display(plugin, jidp->barejid, jidp->resourcepart, curr_message);
        if (new_message) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    jid_destroy(jidp);
    return curr_message;
}

void
plugins_post_priv_message_display(const char * const jid, const char *message)
{
    Jid *jidp = jid_create(jid);

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->post_priv_message_display(plugin, jidp->barejid, jidp->resourcepart, message);
        curr = g_slist_next(curr);
    }

    jid_destroy(jidp);
}

char*
plugins_pre_priv_message_send(const char * const jid, const char * const message)
{
    Jid *jidp = jid_create(jid);
    char *new_message = NULL;
    char *curr_message = strdup(message);

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->pre_priv_message_send(plugin, jidp->barejid, jidp->resourcepart, curr_message);
        if (new_message) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_slist_next(curr);
    }

    jid_destroy(jidp);
    return curr_message;
}

void
plugins_post_priv_message_send(const char * const jid, const char * const message)
{
    Jid *jidp = jid_create(jid);

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->post_priv_message_send(plugin, jidp->barejid, jidp->resourcepart, message);
        curr = g_slist_next(curr);
    }

    jid_destroy(jidp);
}

void
plugins_shutdown(void)
{
    GSList *curr = plugins;

    while (curr) {
#ifdef PROF_HAVE_PYTHON
        if (((ProfPlugin *)curr->data)->lang == LANG_PYTHON) {
            python_plugin_destroy(curr->data);
        }
#endif
#ifdef PROF_HAVE_C
        if (((ProfPlugin *)curr->data)->lang == LANG_C) {
            c_plugin_destroy(curr->data);
        }
#endif

        curr = g_slist_next(curr);
    }
#ifdef PROF_HAVE_PYTHON
    python_shutdown();
#endif
#ifdef PROF_HAVE_C
    c_shutdown();
#endif

    autocompleters_destroy();
    plugin_themes_close();
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
