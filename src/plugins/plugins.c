/*
 * plugins.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
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

#include "log.h"
#include "config.h"
#include "common.h"
#include "config/files.h"
#include "config/preferences.h"
#include "event/client_events.h"
#include "plugins/callbacks.h"
#include "plugins/autocompleters.h"
#include "plugins/api.h"
#include "plugins/plugins.h"
#include "plugins/themes.h"
#include "plugins/settings.h"
#include "plugins/disco.h"
#include "ui/ui.h"
#include "xmpp/xmpp.h"

#ifdef HAVE_PYTHON
#include "plugins/python_plugins.h"
#include "plugins/python_api.h"
#endif

#ifdef HAVE_C
#include "plugins/c_plugins.h"
#include "plugins/c_api.h"
#endif

static GHashTable *plugins;

void
plugins_init(void)
{
    plugins = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
    callbacks_init();
    autocompleters_init();
    plugin_themes_init();
    plugin_settings_init();

#ifdef HAVE_PYTHON
    python_env_init();
#endif
#ifdef HAVE_C
    c_env_init();
#endif

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
                    g_hash_table_insert(plugins, strdup(filename), plugin);
                    loaded = TRUE;
                }
            }
#endif
#ifdef HAVE_C
            if (g_str_has_suffix(filename, ".so")) {
                ProfPlugin *plugin = c_plugin_create(filename);
                if (plugin) {
                    g_hash_table_insert(plugins, strdup(filename), plugin);
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
        GList *values = g_hash_table_get_values(plugins);
        GList *curr = values;
        while (curr) {
            ProfPlugin *plugin = curr->data;
            plugin->init_func(plugin, PACKAGE_VERSION, PACKAGE_STATUS, NULL, NULL);
            curr = g_list_next(curr);
        }
        g_list_free(values);

    }

    prefs_free_plugins(plugins_pref);

    return;
}

void
plugins_free_install_result(PluginsInstallResult *result)
{
    if (!result) {
        return;
    }
    g_slist_free_full(result->installed, free);
    g_slist_free_full(result->failed, free);
}

PluginsInstallResult*
plugins_install_all(const char *const path)
{
    PluginsInstallResult *result = malloc(sizeof(PluginsInstallResult));
    result->installed = NULL;
    result->failed = NULL;
    GSList *contents = NULL;
    get_file_paths_recursive(path, &contents);

    GSList *curr = contents;
    while (curr) {
        if (g_str_has_suffix(curr->data, ".py") || g_str_has_suffix(curr->data, ".so")) {
            gchar *plugin_name = g_path_get_basename(curr->data);
            if (plugins_install(plugin_name, curr->data)) {
                result->installed = g_slist_append(result->installed, strdup(curr->data));
            } else {
                result->failed = g_slist_append(result->failed, strdup(curr->data));
            }
        }
        curr = g_slist_next(curr);
    }

    g_slist_free_full(contents, g_free);

    return result;
}

gboolean
plugins_install(const char *const plugin_name, const char *const filename)
{
    char *plugins_dir = files_get_data_path(DIR_PLUGINS);
    GString *target_path = g_string_new(plugins_dir);
    free(plugins_dir);
    g_string_append(target_path, "/");
    g_string_append(target_path, plugin_name);

    ProfPlugin *plugin = g_hash_table_lookup(plugins, plugin_name);
    if (plugin) {
        plugins_unload(plugin_name);
    }

    gboolean result = copy_file(filename, target_path->str);
    g_string_free(target_path, TRUE);

    if (result) {
        result = plugins_load(plugin_name);
    }

    return result;
}

GSList*
plugins_load_all(void)
{
    GSList *plugins = plugins_unloaded_list();
    GSList *loaded = NULL;
    GSList *curr = plugins;
    while (curr) {
        if (plugins_load(curr->data)) {
            loaded = g_slist_append(loaded, strdup(curr->data));
        }
        curr = g_slist_next(curr);
    }
    g_slist_free_full(plugins, g_free);

    return loaded;
}

gboolean
plugins_load(const char *const name)
{
    ProfPlugin *plugin = g_hash_table_lookup(plugins, name);
    if (plugin) {
        log_info("Failed to load plugin: %s, plugin already loaded", name);
        return FALSE;
    }

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
        g_hash_table_insert(plugins, strdup(name), plugin);
        if (connection_get_status() == JABBER_CONNECTED) {
            const char *account_name = session_get_account_name();
            const char *fulljid = connection_get_fulljid();
            plugin->init_func(plugin, PACKAGE_VERSION, PACKAGE_STATUS, account_name, fulljid);
        } else {
            plugin->init_func(plugin, PACKAGE_VERSION, PACKAGE_STATUS, NULL, NULL);
        }
        log_info("Loaded plugin: %s", name);
        prefs_add_plugin(name);
        return TRUE;
    } else {
        log_info("Failed to load plugin: %s", name);
        return FALSE;
    }
}

gboolean
plugins_unload_all(void)
{
    gboolean result = FALSE;
    GList *plugin_names = g_hash_table_get_keys(plugins);
    GList *plugin_names_dup = NULL;
    GList *curr = plugin_names;
    while (curr) {
        plugin_names_dup = g_list_append(plugin_names_dup, strdup(curr->data));
        curr = g_list_next(curr);
    }
    g_list_free(plugin_names);

    curr = plugin_names_dup;
    while (curr) {
        if (plugins_unload(curr->data)) {
            result = TRUE;
        }
        curr = g_list_next(curr);
    }

    g_list_free_full(plugin_names_dup, free);

    return result;
}

gboolean
plugins_unload(const char *const name)
{
    ProfPlugin *plugin = g_hash_table_lookup(plugins, name);
    if (plugin) {
        plugin->on_unload_func(plugin);
#ifdef HAVE_PYTHON
        if (plugin->lang == LANG_PYTHON) {
            python_plugin_destroy(plugin);
        }
#endif
#ifdef HAVE_C
        if (plugin->lang == LANG_C) {
            c_plugin_destroy(plugin);
        }
#endif
        prefs_remove_plugin(name);
        g_hash_table_remove(plugins, name);

        caps_reset_ver();
        // resend presence to update server's disco info data for this client
        if (connection_get_status() == JABBER_CONNECTED) {
            char* account_name = session_get_account_name();
            resource_presence_t last_presence = accounts_get_last_presence(account_name);
            cl_ev_presence_send(last_presence, 0);
        }
    }
    return TRUE;
}

void
plugins_reload_all(void)
{
    GList *plugin_names = g_hash_table_get_keys(plugins);
    GList *plugin_names_dup = NULL;
    GList *curr = plugin_names;
    while (curr) {
        plugin_names_dup = g_list_append(plugin_names_dup, strdup(curr->data));
        curr = g_list_next(curr);
    }
    g_list_free(plugin_names);

    curr = plugin_names_dup;
    while (curr) {
        plugins_reload(curr->data);
        curr = g_list_next(curr);
    }

    g_list_free_full(plugin_names_dup, free);
}

gboolean
plugins_reload(const char *const name)
{
    gboolean res = plugins_unload(name);
    if (res) {
        res = plugins_load(name);
    }

    return res;
}

void
_plugins_unloaded_list_dir(const gchar *const dir, GSList **result)
{
    GDir *plugins_dir = g_dir_open(dir, 0, NULL);
    if (plugins_dir == NULL) {
        return;
    }

    const gchar *plugin = g_dir_read_name(plugins_dir);
    while (plugin) {
        ProfPlugin *found = g_hash_table_lookup(plugins, plugin);
        if ((g_str_has_suffix(plugin, ".so") || g_str_has_suffix(plugin, ".py")) && !found) {
            *result = g_slist_append(*result, strdup(plugin));
        }
        plugin = g_dir_read_name(plugins_dir);
    }
    g_dir_close(plugins_dir);
}

GSList*
plugins_unloaded_list(void)
{
    GSList *result = NULL;
    char *plugins_dir = files_get_data_path(DIR_PLUGINS);
    _plugins_unloaded_list_dir(plugins_dir, &result);
    free(plugins_dir);

    return result;
}

GList*
plugins_loaded_list(void)
{
    return g_hash_table_get_keys(plugins);
}

char *
plugins_autocomplete(const char * const input, gboolean previous)
{
    return autocompleters_complete(input, previous);
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
    if (window) {
        window->callback_exec(window, win, line);
    }
}

void
plugins_close_win(const char *const plugin_name, const char *const tag)
{
    callbacks_remove_win(plugin_name, tag);
}

void
plugins_on_start(void)
{
    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_start_func(plugin);
        curr = g_list_next(curr);
    }
    g_list_free(values);
}

void
plugins_on_shutdown(void)
{
    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_shutdown_func(plugin);
        curr = g_list_next(curr);
    }
    g_list_free(values);
}

void
plugins_on_connect(const char * const account_name, const char * const fulljid)
{
    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_connect_func(plugin, account_name, fulljid);
        curr = g_list_next(curr);
    }
    g_list_free(values);
}

void
plugins_on_disconnect(const char * const account_name, const char * const fulljid)
{
    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_disconnect_func(plugin, account_name, fulljid);
        curr = g_list_next(curr);
    }
    g_list_free(values);
}

char*
plugins_pre_chat_message_display(const char * const barejid, const char *const resource, const char *message)
{
    char *new_message = NULL;
    char *curr_message = strdup(message);

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->pre_chat_message_display(plugin, barejid, resource, curr_message);
        if (new_message) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);

    return curr_message;
}

void
plugins_post_chat_message_display(const char * const barejid, const char *const resource, const char *message)
{
    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->post_chat_message_display(plugin, barejid, resource, message);
        curr = g_list_next(curr);
    }
    g_list_free(values);
}

char*
plugins_pre_chat_message_send(const char * const barejid, const char *message)
{
    char *new_message = NULL;
    char *curr_message = strdup(message);

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        if (plugin->contains_hook(plugin, "prof_pre_chat_message_send")) {
            new_message = plugin->pre_chat_message_send(plugin, barejid, curr_message);
            if (new_message) {
                free(curr_message);
                curr_message = strdup(new_message);
                free(new_message);
            } else {
                free(curr_message);
                g_list_free(values);

                return NULL;
            }
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);

    return curr_message;
}

void
plugins_post_chat_message_send(const char * const barejid, const char *message)
{
    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->post_chat_message_send(plugin, barejid, message);
        curr = g_list_next(curr);
    }
    g_list_free(values);
}

char*
plugins_pre_room_message_display(const char * const barejid, const char * const nick, const char *message)
{
    char *new_message = NULL;
    char *curr_message = strdup(message);

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->pre_room_message_display(plugin, barejid, nick, curr_message);
        if (new_message) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);

    return curr_message;
}

void
plugins_post_room_message_display(const char * const barejid, const char * const nick, const char *message)
{
    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->post_room_message_display(plugin, barejid, nick, message);
        curr = g_list_next(curr);
    }
    g_list_free(values);
}

char*
plugins_pre_room_message_send(const char * const barejid, const char *message)
{
    char *new_message = NULL;
    char *curr_message = strdup(message);

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        if (plugin->contains_hook(plugin, "prof_pre_room_message_send")) {
            new_message = plugin->pre_room_message_send(plugin, barejid, curr_message);
            if (new_message) {
                free(curr_message);
                curr_message = strdup(new_message);
                free(new_message);
            } else {
                free(curr_message);
                g_list_free(values);

                return NULL;
            }
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);

    return curr_message;
}

void
plugins_post_room_message_send(const char * const barejid, const char *message)
{
    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->post_room_message_send(plugin, barejid, message);
        curr = g_list_next(curr);
    }
    g_list_free(values);
}

void
plugins_on_room_history_message(const char *const barejid, const char *const nick, const char *const message,
    GDateTime *timestamp)
{
    char *timestamp_str = NULL;
    GTimeVal timestamp_tv;
    gboolean res = g_date_time_to_timeval(timestamp, &timestamp_tv);
    if (res) {
        timestamp_str = g_time_val_to_iso8601(&timestamp_tv);
    }

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_room_history_message(plugin, barejid, nick, message, timestamp_str);
        curr = g_list_next(curr);
    }
    g_list_free(values);

    free(timestamp_str);
}

char*
plugins_pre_priv_message_display(const char * const fulljid, const char *message)
{
    Jid *jidp = jid_create(fulljid);
    char *new_message = NULL;
    char *curr_message = strdup(message);

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_message = plugin->pre_priv_message_display(plugin, jidp->barejid, jidp->resourcepart, curr_message);
        if (new_message) {
            free(curr_message);
            curr_message = strdup(new_message);
            free(new_message);
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);

    jid_destroy(jidp);
    return curr_message;
}

void
plugins_post_priv_message_display(const char * const fulljid, const char *message)
{
    Jid *jidp = jid_create(fulljid);

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->post_priv_message_display(plugin, jidp->barejid, jidp->resourcepart, message);
        curr = g_list_next(curr);
    }
    g_list_free(values);

    jid_destroy(jidp);
}

char*
plugins_pre_priv_message_send(const char * const fulljid, const char * const message)
{
    Jid *jidp = jid_create(fulljid);
    char *new_message = NULL;
    char *curr_message = strdup(message);

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        if (plugin->contains_hook(plugin, "prof_pre_priv_message_send")) {
            new_message = plugin->pre_priv_message_send(plugin, jidp->barejid, jidp->resourcepart, curr_message);
            if (new_message) {
                free(curr_message);
                curr_message = strdup(new_message);
                free(new_message);
            } else {
                free(curr_message);
                g_list_free(values);
                jid_destroy(jidp);

                return NULL;
            }
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);

    jid_destroy(jidp);
    return curr_message;
}

void
plugins_post_priv_message_send(const char * const fulljid, const char * const message)
{
    Jid *jidp = jid_create(fulljid);

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->post_priv_message_send(plugin, jidp->barejid, jidp->resourcepart, message);
        curr = g_list_next(curr);
    }
    g_list_free(values);

    jid_destroy(jidp);
}

char*
plugins_on_message_stanza_send(const char *const text)
{
    char *new_stanza = NULL;
    char *curr_stanza = strdup(text);

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_stanza = plugin->on_message_stanza_send(plugin, curr_stanza);
        if (new_stanza) {
            free(curr_stanza);
            curr_stanza = strdup(new_stanza);
            free(new_stanza);
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);

    return curr_stanza;
}

gboolean
plugins_on_message_stanza_receive(const char *const text)
{
    gboolean cont = TRUE;

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        gboolean res = plugin->on_message_stanza_receive(plugin, text);
        if (res == FALSE) {
            cont = FALSE;
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);

    return cont;
}

char*
plugins_on_presence_stanza_send(const char *const text)
{
    char *new_stanza = NULL;
    char *curr_stanza = strdup(text);

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_stanza = plugin->on_presence_stanza_send(plugin, curr_stanza);
        if (new_stanza) {
            free(curr_stanza);
            curr_stanza = strdup(new_stanza);
            free(new_stanza);
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);

    return curr_stanza;
}

gboolean
plugins_on_presence_stanza_receive(const char *const text)
{
    gboolean cont = TRUE;

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        gboolean res = plugin->on_presence_stanza_receive(plugin, text);
        if (res == FALSE) {
            cont = FALSE;
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);

    return cont;
}

char*
plugins_on_iq_stanza_send(const char *const text)
{
    char *new_stanza = NULL;
    char *curr_stanza = strdup(text);

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        new_stanza = plugin->on_iq_stanza_send(plugin, curr_stanza);
        if (new_stanza) {
            free(curr_stanza);
            curr_stanza = strdup(new_stanza);
            free(new_stanza);
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);

    return curr_stanza;
}

gboolean
plugins_on_iq_stanza_receive(const char *const text)
{
    gboolean cont = TRUE;

    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        gboolean res = plugin->on_iq_stanza_receive(plugin, text);
        if (res == FALSE) {
            cont = FALSE;
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);

    return cont;
}

void
plugins_on_contact_offline(const char *const barejid, const char *const resource, const char *const status)
{
    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_contact_offline(plugin, barejid, resource, status);
        curr = g_list_next(curr);
    }
    g_list_free(values);
}

void
plugins_on_contact_presence(const char *const barejid, const char *const resource, const char *const presence, const char *const status, const int priority)
{
    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_contact_presence(plugin, barejid, resource, presence, status, priority);
        curr = g_list_next(curr);
    }
    g_list_free(values);
}

void
plugins_on_chat_win_focus(const char *const barejid)
{
    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_chat_win_focus(plugin, barejid);
        curr = g_list_next(curr);
    }
    g_list_free(values);
}

void
plugins_on_room_win_focus(const char *const barejid)
{
    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;
    while (curr) {
        ProfPlugin *plugin = curr->data;
        plugin->on_room_win_focus(plugin, barejid);
        curr = g_list_next(curr);
    }
    g_list_free(values);
}

GList*
plugins_get_disco_features(void)
{
    return disco_get_features();
}

void
plugins_shutdown(void)
{
    GList *values = g_hash_table_get_values(plugins);
    GList *curr = values;

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

        curr = g_list_next(curr);
    }
    g_list_free(values);
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
    disco_close();
    g_hash_table_destroy(plugins);
    plugins = NULL;
}
