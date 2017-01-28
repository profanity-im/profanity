/*
 * prof_api.h
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

#ifndef PLUGINS_PROF_API_H
#define PLUGINS_PROF_API_H

#define prof_register_command(command_name, min_args, max_args, synopsis, description, arguments, examples, callback) _prof_register_command(__FILE__, command_name, min_args, max_args, synopsis, description, arguments, examples, callback)
#define prof_register_timed(callback, interval_seconds) _prof_register_timed(__FILE__, callback, interval_seconds)
#define prof_completer_add(key, items) _prof_completer_add(__FILE__, key, items)
#define prof_completer_remove(key, items) _prof_completer_remove(__FILE__, key, items)
#define prof_completer_clear(key) _prof_completer_clear(__FILE__, key)
#define prof_filepath_completer_add(prefix) _prof_filepath_completer_add(__FILE__, prefix)
#define prof_win_create(win, input_handler) _prof_win_create(__FILE__, win, input_handler)
#define prof_disco_add_feature(feature) _prof_disco_add_feature(__FILE__, feature)

typedef char* PROF_WIN_TAG;
typedef void(*CMD_CB)(char **args);
typedef void(*TIMED_CB)(void);
typedef void(*WINDOW_CB)(PROF_WIN_TAG win, char *line);

void (*prof_cons_alert)(void);
int (*prof_cons_show)(const char * const message);
int (*prof_cons_show_themed)(const char *const group, const char *const item, const char *const def, const char *const message);
int (*prof_cons_bad_cmd_usage)(const char *const cmd);

void (*_prof_register_command)(const char *filename, const char *command_name, int min_args, int max_args,
    char **synopsis, const char *description, char *arguments[][2], char **examples,
    CMD_CB callback);

void (*_prof_register_timed)(const char *filename, TIMED_CB callback, int interval_seconds);

void (*_prof_completer_add)(const char *filename, const char *key, char **items);
void (*_prof_completer_remove)(const char *filename, const char *key, char **items);
void (*_prof_completer_clear)(const char *filename, const char *key);
void (*_prof_filepath_completer_add)(const char *filename, const char *prefix);

void (*prof_notify)(const char *message, int timeout_ms, const char *category);

void (*prof_send_line)(char *line);

char* (*prof_get_current_recipient)(void);
char* (*prof_get_current_muc)(void);
int (*prof_current_win_is_console)(void);
char* (*prof_get_current_nick)(void);
char** (*prof_get_current_occupants)(void);

char* (*prof_get_room_nick)(const char *barejid);

void (*prof_log_debug)(const char *message);
void (*prof_log_info)(const char *message);
void (*prof_log_warning)(const char *message);
void (*prof_log_error)(const char *message);

void (*_prof_win_create)(const char *filename, PROF_WIN_TAG win, WINDOW_CB input_handler);
int (*prof_win_exists)(PROF_WIN_TAG win);
int (*prof_win_focus)(PROF_WIN_TAG win);
int (*prof_win_show)(PROF_WIN_TAG win, char *line);
int (*prof_win_show_themed)(PROF_WIN_TAG tag, char *group, char *key, char *def, char *line);

int (*prof_send_stanza)(char *stanza);

int (*prof_settings_boolean_get)(char *group, char *key, int def);
void (*prof_settings_boolean_set)(char *group, char *key, int value);
char* (*prof_settings_string_get)(char *group, char *key, char *def);
void (*prof_settings_string_set)(char *group, char *key, char *value);
int (*prof_settings_int_get)(char *group, char *key, int def);
void (*prof_settings_int_set)(char *group, char *key, int value);
char** (*prof_settings_string_list_get)(char *group, char *key);
void (*prof_settings_string_list_add)(char *group, char *key, char *value);
int (*prof_settings_string_list_remove)(char *group, char *key, char *value);
int (*prof_settings_string_list_clear)(char *group, char *key);

void (*prof_incoming_message)(char *barejid, char *resource, char *message);

void (*_prof_disco_add_feature)(const char *filename, char *feature);

void (*prof_encryption_reset)(const char *barejid);

int (*prof_chat_set_titlebar_enctext)(const char *barejid, const char *enctext);
int (*prof_chat_unset_titlebar_enctext)(const char *barejid);
int (*prof_chat_set_incoming_char)(const char *barejid, const char *ch);
int (*prof_chat_unset_incoming_char)(const char *barejid);
int (*prof_chat_set_outgoing_char)(const char *barejid, const char *ch);
int (*prof_chat_unset_outgoing_char)(const char *barejid);
int (*prof_room_set_titlebar_enctext)(const char *roomjid, const char *enctext);
int (*prof_room_unset_titlebar_enctext)(const char *roomjid);
int (*prof_room_set_message_char)(const char *roomjid, const char *ch);
int (*prof_room_unset_message_char)(const char *roomjid);

int (*prof_chat_show)(const char *const barejid, const char *const message);
int (*prof_chat_show_themed)(const char *const barejid, const char *const group, const char *const item, const char *const def,
    const char *const ch, const char *const message);

int (*prof_room_show)(const char *const roomjid, const char *const message);
int (*prof_room_show_themed)(const char *const roomjid, const char *const group, const char *const item, const char *const def,
    const char *const ch, const char *const message);

#endif
