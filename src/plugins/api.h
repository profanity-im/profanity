/*
 * api.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PLUGINS_API_H
#define PLUGINS_API_H

#include "plugins/callbacks.h"

void api_cons_alert(void);
int api_cons_show(const char* const message);
int api_cons_show_themed(const char* const group, const char* const item, const char* const def, const char* const message);
int api_cons_bad_cmd_usage(const char* const cmd);
void api_notify(const char* message, const char* category, int timeout_ms);
void api_send_line(char* line);

char* api_get_current_recipient(void);
char* api_get_current_muc(void);
gboolean api_current_win_is_console(void);
char* api_get_current_nick(void);
char* api_get_name_from_roster(const char* barejid);
char* api_get_barejid_from_roster(const char* name);
char** api_get_current_occupants(void);

char* api_get_room_nick(const char* barejid);

void api_register_command(const char* const plugin_name, const char* command_name, int min_args, int max_args,
                          char** synopsis, const char* description, char* arguments[][2], char** examples,
                          void* callback, void (*callback_func)(PluginCommand* command, gchar** args), void (*callback_destroy)(void* callback));
void api_register_timed(const char* const plugin_name, void* callback, int interval_seconds,
                        void (*callback_func)(PluginTimedFunction* timed_function), void (*callback_destroy)(void* callback));

void api_completer_add(const char* const plugin_name, const char* key, char** items);
void api_completer_remove(const char* const plugin_name, const char* key, char** items);
void api_completer_clear(const char* const plugin_name, const char* key);
void api_filepath_completer_add(const char* const plugin_name, const char* prefix);

void api_log_debug(const char* message);
void api_log_info(const char* message);
void api_log_warning(const char* message);
void api_log_error(const char* message);

int api_win_exists(const char* tag);
void api_win_create(
    const char* const plugin_name,
    const char* tag,
    void* callback,
    void (*callback_func)(PluginWindowCallback* window_callback, char* tag, char* line),
    void (*destroy)(void* callback));
int api_win_focus(const char* tag);
int api_win_show(const char* tag, const char* line);
int api_win_show_themed(const char* tag, const char* const group, const char* const key, const char* const def, const char* line);

int api_send_stanza(const char* const stanza);

gboolean api_settings_boolean_get(const char* const group, const char* const key, gboolean def);
void api_settings_boolean_set(const char* const group, const char* const key, gboolean value);
char* api_settings_string_get(const char* const group, const char* const key, const char* const def);
void api_settings_string_set(const char* const group, const char* const key, const char* const value);
int api_settings_int_get(const char* const group, const char* const key, int def);
void api_settings_int_set(const char* const group, const char* const key, int value);
char** api_settings_string_list_get(const char* const group, const char* const key);
void api_settings_string_list_add(const char* const group, const char* const key, const char* const value);
int api_settings_string_list_remove(const char* const group, const char* const key, const char* const value);
int api_settings_string_list_clear(const char* const group, const char* const key);

void api_incoming_message(const char* const barejid, const char* const resource, const char* const message);

void api_disco_add_feature(char* plugin_name, char* feature);

void api_encryption_reset(const char* const barejid);

int api_chat_set_titlebar_enctext(const char* const barejid, const char* const enctext);
int api_chat_unset_titlebar_enctext(const char* const barejid);
int api_chat_set_incoming_char(const char* const barejid, const char* const ch);
int api_chat_unset_incoming_char(const char* const barejid);
int api_chat_set_outgoing_char(const char* const barejid, const char* const ch);
int api_chat_unset_outgoing_char(const char* const barejid);
int api_room_set_titlebar_enctext(const char* const roomjid, const char* const enctext);
int api_room_unset_titlebar_enctext(const char* const roomjid);
int api_room_set_message_char(const char* const roomjid, const char* const ch);
int api_room_unset_message_char(const char* const roomjid);

int api_chat_show(const char* const barejid, const char* const message);
int api_chat_show_themed(const char* const barejid, const char* const group, const char* const key, const char* const def,
                         const char* const ch, const char* const message);

int api_room_show(const char* const roomjid, const char* message);
int api_room_show_themed(const char* const roomjid, const char* const group, const char* const key, const char* const def,
                         const char* const ch, const char* const message);

#endif
