/*
 * api.h
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

#ifndef PLUGINS_API_H
#define PLUGINS_API_H

#include "plugins/callbacks.h"

void api_cons_alert(void);
int api_cons_show(const char * const message);
int api_cons_show_themed(const char *const group, const char *const item, const char *const def, const char *const message);
int api_cons_bad_cmd_usage(const char *const cmd);
void api_notify(const char *message, const char *category, int timeout_ms);
void api_send_line(char *line);

char * api_get_current_recipient(void);
char * api_get_current_muc(void);
gboolean api_current_win_is_console(void);
char* api_get_current_nick(void);
char** api_get_current_occupants(void);

char* api_get_room_nick(const char *barejid);

void api_register_command(const char *const plugin_name, const char *command_name, int min_args, int max_args,
    char **synopsis, const char *description, char *arguments[][2], char **examples,
    void *callback, void(*callback_func)(PluginCommand *command, gchar **args), void(*callback_destroy)(void *callback));
void api_register_timed(const char *const plugin_name, void *callback, int interval_seconds,
    void (*callback_func)(PluginTimedFunction *timed_function), void(*callback_destroy)(void *callback));

void api_completer_add(const char *const plugin_name, const char *key, char **items);
void api_completer_remove(const char *const plugin_name, const char *key, char **items);
void api_completer_clear(const char *const plugin_name, const char *key);
void api_filepath_completer_add(const char *const plugin_name, const char *prefix);

void api_log_debug(const char *message);
void api_log_info(const char *message);
void api_log_warning(const char *message);
void api_log_error(const char *message);

int api_win_exists(const char *tag);
void api_win_create(
    const char *const plugin_name,
    const char *tag,
    void *callback,
    void(*callback_func)(PluginWindowCallback *window_callback, char *tag, char *line),
    void(*destroy)(void *callback));
int api_win_focus(const char *tag);
int api_win_show(const char *tag, const char *line);
int api_win_show_themed(const char *tag, const char *const group, const char *const key, const char *const def, const char *line);

int api_send_stanza(const char *const stanza);

gboolean api_settings_boolean_get(const char *const group, const char *const key, gboolean def);
void api_settings_boolean_set(const char *const group, const char *const key, gboolean value);
char* api_settings_string_get(const char *const group, const char *const key, const char *const def);
void api_settings_string_set(const char *const group, const char *const key, const char *const value);
int api_settings_int_get(const char *const group, const char *const key, int def);
void api_settings_int_set(const char *const group, const char *const key, int value);
char** api_settings_string_list_get(const char *const group, const char *const key);
void api_settings_string_list_add(const char *const group, const char *const key, const char *const value);
int api_settings_string_list_remove(const char *const group, const char *const key, const char *const value);
int api_settings_string_list_clear(const char *const group, const char *const key);

void api_incoming_message(const char *const barejid, const char *const resource, const char *const message);

void api_disco_add_feature(char *plugin_name, char *feature);

void api_encryption_reset(const char *const barejid);

int api_chat_set_titlebar_enctext(const char *const barejid, const char *const enctext);
int api_chat_unset_titlebar_enctext(const char *const barejid);
int api_chat_set_incoming_char(const char *const barejid, const char *const ch);
int api_chat_unset_incoming_char(const char *const barejid);
int api_chat_set_outgoing_char(const char *const barejid, const char *const ch);
int api_chat_unset_outgoing_char(const char *const barejid);
int api_room_set_titlebar_enctext(const char *const roomjid, const char *const enctext);
int api_room_unset_titlebar_enctext(const char *const roomjid);
int api_room_set_message_char(const char *const roomjid, const char *const ch);
int api_room_unset_message_char(const char *const roomjid);

int api_chat_show(const char *const barejid, const char *const message);
int api_chat_show_themed(const char *const barejid, const char *const group, const char *const key, const char *const def,
    const char *const ch, const char *const message);

int api_room_show(const char *const roomjid, const char *message);
int api_room_show_themed(const char *const roomjid, const char *const group, const char *const key, const char *const def,
    const char *const ch, const char *const message);

#endif
