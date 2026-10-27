/*
 * plugins.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PLUGINS_PLUGINS_H
#define PLUGINS_PLUGINS_H

#include <glib.h>
#include "command/cmd_defs.h"

#ifdef HAVE_PLUGINS
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#endif

typedef struct prof_plugins_install_t
{
    GSList* installed;
    GSList* failed;
} PluginsInstallResult;

typedef struct prof_plugin_t
{
    char* name;
#ifdef HAVE_PLUGINS
    lua_State* L;
#endif
} ProfPlugin;

const char* plugins_get_lua_version(void);
void plugins_init(void);
GSList* plugins_unloaded_list(void);
GList* plugins_loaded_list(void);
char* plugins_autocomplete(const char* const input, gboolean previous);
void plugins_reset_autocomplete(void);

void plugins_free_install_result(PluginsInstallResult* result);
gboolean plugins_install(const char* const plugin_name, const char* const filename, GString* error_message);
PluginsInstallResult* plugins_install_all(const char* const path);
gboolean plugins_uninstall(const char* const plugin_name);

gboolean plugins_load(const char* const name, GString* error_message);
GSList* plugins_load_all(void);
gboolean plugins_unload(const char* const name);
gboolean plugins_unload_all(void);
gboolean plugins_reload(const char* const name, GString* error_message);
void plugins_reload_all(void);

void plugins_on_start(void);
void plugins_on_shutdown(void);
void plugins_close(void);
void plugins_on_connect(const char* const account_name, const char* const fulljid);
void plugins_on_disconnect(const char* const account_name, const char* const fulljid);

char* plugins_pre_chat_message_display(const char* const barejid, const char* const resource, const char* message);
void plugins_post_chat_message_display(const char* const barejid, const char* const resource, const char* message);
char* plugins_pre_chat_message_send(const char* const barejid, const char* message);
void plugins_post_chat_message_send(const char* const barejid, const char* message);

char* plugins_pre_room_message_display(const char* const barejid, const char* const nick, const char* message);
void plugins_post_room_message_display(const char* const barejid, const char* const nick, const char* message);
char* plugins_pre_room_message_send(const char* const barejid, const char* message);
void plugins_post_room_message_send(const char* const barejid, const char* message);
void plugins_on_room_history_message(const char* const barejid, const char* const nick, const char* const message,
                                     GDateTime* timestamp);

char* plugins_pre_priv_message_display(const char* const fulljid, const char* message);
void plugins_post_priv_message_display(const char* const fulljid, const char* message);
char* plugins_pre_priv_message_send(const char* const fulljid, const char* const message);
void plugins_post_priv_message_send(const char* const fulljid, const char* const message);

void plugins_win_process_line(char* win, const char* const line);
void plugins_close_win(const char* const plugin_name, const char* const tag);

char* plugins_on_message_stanza_send(const char* const text);
gboolean plugins_on_message_stanza_receive(const char* const text);
char* plugins_on_presence_stanza_send(const char* const text);
gboolean plugins_on_presence_stanza_receive(const char* const text);
char* plugins_on_iq_stanza_send(const char* const text);
gboolean plugins_on_iq_stanza_receive(const char* const text);

void plugins_on_contact_offline(const char* const barejid, const char* const resource, const char* const status);
void plugins_on_contact_presence(const char* const barejid, const char* const resource, const char* const presence,
                                 const char* const status, const int priority);

void plugins_on_chat_win_focus(const char* const barejid);
void plugins_on_room_win_focus(const char* const barejid);

void plugins_on_room_occupant_join(const char* const roomjid, const char* const nick);
void plugins_on_room_occupant_leave(const char* const roomjid, const char* const nick, const char* const reason);

void plugins_on_chat_state(const char* const barejid, const char* const state);
void plugins_on_message_correction(const char* const barejid, const char* const id, const char* const new_message);

void plugins_on_win_open(const char* const title, const char* const type);
void plugins_on_win_close(const char* const title, const char* const type);

gboolean plugins_run_command(const char* const cmd);
void plugins_run_timed(void);
GList* plugins_get_command_names(void);
CommandHelp* plugins_get_help(const char* const cmd);

GList* plugins_get_disco_features(void);

#endif
