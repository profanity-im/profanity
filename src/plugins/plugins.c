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

#include "config.h"
#include "common.h"
#include "config/preferences.h"
#include "log.h"
#include "plugins/callbacks.h"
#include "plugins/autocompleters.h"
#include "plugins/api.h"
#include "plugins/plugins.h"
#include "plugins/themes.h"
#include "plugins/settings.h"

#ifdef HAVE_PYTHON
#include "plugins/python_plugins.h"
#include "plugins/python_api.h"
#endif

#ifdef HAVE_C
#include "plugins/c_plugins.h"
#include "plugins/c_api.h"
#endif

#include "ui/ui.h"

static GSList* plugins;

void
plugins_init(void)
{
    plugins = NULL;
    callbacks_init();
    autocompleters_init();

#ifdef HAVE_PYTHON
    python_env_init();
#endif
#ifdef HAVE_C
    c_env_init();
#endif

    plugin_themes_init();
    plugin_settings_init();

    // load plugins
    gchar **plugins_pref = prefs_get_plugins();
    if (plugins_pref) {
        int i;
        for (i = 0; i < g_strv_length(plugins_pref); i++)
        {
            gboolean loaded = FALSE;
            gchar *filename = plugins_pref[i];
#ifdef HAVE_PYTHON
            if (g_str_has_suffix(filename, ".py")) {
                ProfPlugin *plugin = python_plugin_create(filename);
                if (plugin) {
                    plugins = g_slist_append(plugins, plugin);
                    loaded = TRUE;
                }
            }
#endif
#ifdef HAVE_C
            if (g_str_has_suffix(filename, ".so")) {
                ProfPlugin *plugin = c_plugin_create(filename);
                if (plugin) {
                    plugins = g_slist_append(plugins, plugin);
                    loaded = TRUE;
                }
            }
#endif
            if (loaded) {
                log_info("Loaded plugin: %s", filename);
            } else {
                log_info("Failed to load plugin: %s", filename);
            }
        }

        // initialise plugins
        GSList *curr = plugins;
        while (curr) {
            ProfPlugin *plugin = curr->data;
            plugin->init_func(plugin, PACKAGE_VERSION, PACKAGE_STATUS);
            curr = g_slist_next(curr);
        }
    }

    prefs_free_plugins(plugins_pref);

    return;
}

gboolean
_find_by_name(gconstpointer pluginp, gconstpointer namep)
{
    char *name = (char*)namep;
    ProfPlugin *plugin = (ProfPlugin*)pluginp;

    return g_strcmp0(name, plugin->name);
}

gboolean
plugins_load(const char *const name)
{
    GSList *found = g_slist_find_custom(plugins, name, (GCompareFunc)_find_by_name);
    if (found) {
        log_info("Failed to load plugin: %s, plugin already loaded", name);
        return FALSE;
    }

    ProfPlugin *plugin = NULL;
#ifdef HAVE_PYTHON
    if (g_str_has_suffix(name, ".py")) {
        plugin = python_plugin_create(name);
    }
#endif
#ifdef HAVE_C
    if (g_str_has_suffix(name, ".so")) {
        plugin = c_plugin_create(name);
    }
#endif
    if (plugin) {
        plugins = g_slist_append(plugins, plugin);
        plugin->init_func(plugin, PACKAGE_VERSION, PACKAGE_STATUS);
        log_info("Loaded plugin: %s", name);
        return TRUE;
    } else {
        log_info("Failed to load plugin: %s", name);
        return FALSE;
    }
}

GSList *
plugins_get_list(void)
{
    return plugins;
}

static gchar*
_get_plugins_dir(void)
{
    gchar *xdg_data = xdg_get_data_home();
    GString *plugins_dir = g_string_new(xdg_data);
    g_free(xdg_data);
    g_string_append(plugins_dir, "/profanity/plugins");
    return g_string_free(plugins_dir, FALSE);
}

void
_plugins_list_dir(const gchar *const dir, GSList **result)
{
    GDir *plugins = g_dir_open(dir, 0, NULL);
    if (plugins == NULL) {
        return;
    }

    const gchar *plugin = g_dir_read_name(plugins);
    while (plugin) {
        if (g_str_has_suffix(plugin, ".so") || g_str_has_suffix(plugin, ".py")) {
            *result = g_slist_append(*result, strdup(plugin));
        }
        plugin = g_dir_read_name(plugins);
    }
    g_dir_close(plugins);
}

GSList*
plugins_file_list(void)
{
    GSList *result = NULL;
    char *plugins_dir = _get_plugins_dir();
    _plugins_list_dir(plugins_dir, &result);
    free(plugins_dir);

    return result;
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

void
plugins_on_room_history_message(const char *const room, const char *const nick, const char *const message,
    GDateTime *timestamp)
{
    char *timestamp_str = NULL;
    GTimeVal timestamp_tv;
    gboolean res = g_date_time_to_timeval(timestamp, &timestamp_tv);
    if (res) {
        timestamp_str = g_time_val_to_iso8601(&timestamp_tv);
    }

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_room_history_message(plugin, room, nick, message, timestamp_str);
        curr = g_slist_next(curr);
    }

    free(timestamp_str);
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

char*
plugins_on_message_stanza_send(const char *const text)
{
    char *new_stanza = NULL;
    char *curr_stanza = strdup(text);

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_stanza = plugin->on_message_stanza_send(plugin, curr_stanza);
        if (new_stanza) {
            free(curr_stanza);
            curr_stanza = strdup(new_stanza);
            free(new_stanza);
        }
        curr = g_slist_next(curr);
    }

    return curr_stanza;
}

gboolean
plugins_on_message_stanza_receive(const char *const text)
{
    gboolean cont = TRUE;

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        gboolean res = plugin->on_message_stanza_receive(plugin, text);
        if (res == FALSE) {
            cont = FALSE;
        }
        curr = g_slist_next(curr);
    }

    return cont;
}

char*
plugins_on_presence_stanza_send(const char *const text)
{
    char *new_stanza = NULL;
    char *curr_stanza = strdup(text);

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_stanza = plugin->on_presence_stanza_send(plugin, curr_stanza);
        if (new_stanza) {
            free(curr_stanza);
            curr_stanza = strdup(new_stanza);
            free(new_stanza);
        }
        curr = g_slist_next(curr);
    }

    return curr_stanza;
}

gboolean
plugins_on_presence_stanza_receive(const char *const text)
{
    gboolean cont = TRUE;

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        gboolean res = plugin->on_presence_stanza_receive(plugin, text);
        if (res == FALSE) {
            cont = FALSE;
        }
        curr = g_slist_next(curr);
    }

    return cont;
}

char*
plugins_on_iq_stanza_send(const char *const text)
{
    char *new_stanza = NULL;
    char *curr_stanza = strdup(text);

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_stanza = plugin->on_iq_stanza_send(plugin, curr_stanza);
        if (new_stanza) {
            free(curr_stanza);
            curr_stanza = strdup(new_stanza);
            free(new_stanza);
        }
        curr = g_slist_next(curr);
    }

    return curr_stanza;
}

gboolean
plugins_on_iq_stanza_receive(const char *const text)
{
    gboolean cont = TRUE;

    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        gboolean res = plugin->on_iq_stanza_receive(plugin, text);
        if (res == FALSE) {
            cont = FALSE;
        }
        curr = g_slist_next(curr);
    }

    return cont;
}

void
plugins_on_contact_offline(const char *const barejid, const char *const resource, const char *const status)
{
    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_contact_offline(plugin, barejid, resource, status);
        curr = g_slist_next(curr);
    }
}

void
plugins_on_contact_presence(const char *const barejid, const char *const resource, const char *const presence, const char *const status, const int priority)
{
    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_contact_presence(plugin, barejid, resource, presence, status, priority);
        curr = g_slist_next(curr);
    }
}

void
plugins_on_chat_win_focus(const char *const barejid)
{
    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_chat_win_focus(plugin, barejid);
        curr = g_slist_next(curr);
    }
}

void
plugins_on_room_win_focus(const char *const roomjid)
{
    GSList *curr = plugins;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_room_win_focus(plugin, roomjid);
        curr = g_slist_next(curr);
    }
}

void
plugins_shutdown(void)
{
    GSList *curr = plugins;

    while (curr) {
#ifdef HAVE_PYTHON
        if (((ProfPlugin *)curr->data)->lang == LANG_PYTHON) {
            python_plugin_destroy(curr->data);
        }
#endif
#ifdef HAVE_C
        if (((ProfPlugin *)curr->data)->lang == LANG_C) {
            c_plugin_destroy(curr->data);
        }
#endif

        curr = g_slist_next(curr);
    }
#ifdef HAVE_PYTHON
    python_shutdown();
#endif
#ifdef HAVE_C
    c_shutdown();
#endif

    autocompleters_destroy();
    plugin_themes_close();
    plugin_settings_close();
    callbacks_close();
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
