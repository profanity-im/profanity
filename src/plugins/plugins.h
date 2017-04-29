/*
 * plugins.h
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

#ifndef PLUGINS_PLUGINS_H
#define PLUGINS_PLUGINS_H

#include "command/cmd_defs.h"

typedef enum {
    LANG_PYTHON,
    LANG_C
} lang_t;

typedef struct prof_plugins_install_t {
    GSList *installed;
    GSList *failed;
} PluginsInstallResult;

typedef struct prof_plugin_t {
    char *name;
    lang_t lang;
    void *module;
    void (*init_func)(struct prof_plugin_t* plugin, const char * const version,
        const char * const status, const char *const account_name, const char *const fulljid);

    gboolean (*contains_hook)(struct prof_plugin_t* plugin, const char *const hook);

    void (*on_start_func)(struct prof_plugin_t* plugin);
    void (*on_shutdown_func)(struct prof_plugin_t* plugin);
    void (*on_unload_func)(struct prof_plugin_t* plugin);

    void (*on_connect_func)(struct prof_plugin_t* plugin, const char *const account_name, const char *const fulljid);
    void (*on_disconnect_func)(struct prof_plugin_t* plugin, const char *const account_name,
        const char *const fulljid);

    char* (*pre_chat_message_display)(struct prof_plugin_t* plugin, const char *const barejid, const char *const resource, const char *message);
    void (*post_chat_message_display)(struct prof_plugin_t* plugin, const char *const barejid, const char *const resource, const char *message);
    char* (*pre_chat_message_send)(struct prof_plugin_t* plugin, const char *const barejid, const char *message);
    void (*post_chat_message_send)(struct prof_plugin_t* plugin, const char *const barejid, const char *message);

    char* (*pre_room_message_display)(struct prof_plugin_t* plugin, const char *const barejid, const char *const nick,
        const char *message);
    void (*post_room_message_display)(struct prof_plugin_t* plugin, const char *const barejid, const char *const nick,
        const char *message);
    char* (*pre_room_message_send)(struct prof_plugin_t* plugin, const char *const barejid, const char *message);
    void (*post_room_message_send)(struct prof_plugin_t* plugin, const char *const barejid, const char *message);
    void (*on_room_history_message)(struct prof_plugin_t* plugin, const char *const barejid, const char *const nick, const char *const message,
        const char *const timestamp);

    char* (*pre_priv_message_display)(struct prof_plugin_t* plugin, const char *const barejid, const char *const nick,
        const char *message);
    void (*post_priv_message_display)(struct prof_plugin_t* plugin, const char *const barejid, const char *const nick,
        const char *message);
    char* (*pre_priv_message_send)(struct prof_plugin_t* plugin, const char *const barejid, const char *const nick,
        const char *const message);
    void (*post_priv_message_send)(struct prof_plugin_t* plugin, const char *const barejid, const char *const nick,
        const char *const message);

    char* (*on_message_stanza_send)(struct prof_plugin_t* plugin, const char *const text);
    gboolean (*on_message_stanza_receive)(struct prof_plugin_t* plugin, const char *const text);

    char* (*on_presence_stanza_send)(struct prof_plugin_t* plugin, const char *const text);
    gboolean (*on_presence_stanza_receive)(struct prof_plugin_t* plugin, const char *const text);

    char* (*on_iq_stanza_send)(struct prof_plugin_t* plugin, const char *const text);
    gboolean (*on_iq_stanza_receive)(struct prof_plugin_t* plugin, const char *const text);

    void (*on_contact_offline)(struct prof_plugin_t* plugin, const char *const barejid, const char *const resource,
        const char *const status);
    void (*on_contact_presence)(struct prof_plugin_t* plugin, const char *const barejid, const char *const resource,
        const char *const presence, const char *const status, const int priority);

    void (*on_chat_win_focus)(struct prof_plugin_t* plugin, const char *const barejid);
    void (*on_room_win_focus)(struct prof_plugin_t* plugin, const char *const barejid);
} ProfPlugin;

void plugins_init(void);
GSList *plugins_unloaded_list(void);
GList *plugins_loaded_list(void);
char* plugins_autocomplete(const char *const input, gboolean previous);
void plugins_reset_autocomplete(void);
void plugins_shutdown(void);

void plugins_free_install_result(PluginsInstallResult *result);

gboolean plugins_install(const char *const plugin_name, const char *const filename);
PluginsInstallResult* plugins_install_all(const char *const path);
gboolean plugins_load(const char *const name);
GSList* plugins_load_all(void);
gboolean plugins_unload(const char *const name);
gboolean plugins_unload_all(void);
gboolean plugins_reload(const char *const name);
void plugins_reload_all(void);

void plugins_on_start(void);
void plugins_on_shutdown(void);

void plugins_on_connect(const char *const account_name, const char *const fulljid);
void plugins_on_disconnect(const char *const account_name, const char *const fulljid);

char* plugins_pre_chat_message_display(const char *const barejid, const char *const resource, const char *message);
void plugins_post_chat_message_display(const char *const barejid, const char *const resource, const char *message);
char* plugins_pre_chat_message_send(const char *const barejid, const char *message);
void plugins_post_chat_message_send(const char *const barejid, const char *message);

char* plugins_pre_room_message_display(const char *const barejid, const char *const nick, const char *message);
void plugins_post_room_message_display(const char *const barejid, const char *const nick, const char *message);
char* plugins_pre_room_message_send(const char *const barejid, const char *message);
void plugins_post_room_message_send(const char *const barejid, const char *message);
void plugins_on_room_history_message(const char *const barejid, const char *const nick, const char *const message,
    GDateTime *timestamp);

char* plugins_pre_priv_message_display(const char *const fulljid, const char *message);
void plugins_post_priv_message_display(const char *const fulljid, const char *message);
char* plugins_pre_priv_message_send(const char *const fulljid, const char *const message);
void plugins_post_priv_message_send(const char *const fulljid, const char *const message);

void plugins_win_process_line(char *win, const char *const line);
void plugins_close_win(const char *const plugin_name, const char *const tag);

char* plugins_on_message_stanza_send(const char *const text);
gboolean plugins_on_message_stanza_receive(const char *const text);

char* plugins_on_presence_stanza_send(const char *const text);
gboolean plugins_on_presence_stanza_receive(const char *const text);

char* plugins_on_iq_stanza_send(const char *const text);
gboolean plugins_on_iq_stanza_receive(const char *const text);

void plugins_on_contact_offline(const char *const barejid, const char *const resource, const char *const status);
void plugins_on_contact_presence(const char *const barejid, const char *const resource, const char *const presence,
    const char *const status, const int priority);

void plugins_on_chat_win_focus(const char *const barejid);
void plugins_on_room_win_focus(const char *const barejid);

gboolean plugins_run_command(const char * const cmd);
void plugins_run_timed(void);
GList* plugins_get_command_names(void);
gchar * plugins_get_dir(void);
CommandHelp* plugins_get_help(const char *const cmd);

GList* plugins_get_disco_features(void);

#endif
