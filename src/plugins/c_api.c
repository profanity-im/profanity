/*
 * c_api.c
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

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "log.h"
#include "plugins/api.h"
#include "plugins/c_api.h"
#include "plugins/callbacks.h"
#include "plugins/profapi.h"

typedef struct command_wrapper_t {
    void(*func)(char **args);
} CommandWrapper;

typedef struct timed_wrapper_t {
    void(*func)(void);
} TimedWrapper;

typedef struct window_wrapper_t {
    void(*func)(char *tag, char *line);
} WindowWrapper;

static char* _c_plugin_name(const char *filename);

static void
c_api_cons_alert(void)
{
    api_cons_alert();
}

static int
c_api_cons_show(const char * const message)
{
    return api_cons_show(message);
}

static int
c_api_cons_show_themed(const char *const group, const char *const item, const char *const def, const char *const message)
{
    return api_cons_show_themed(group, item, def, message);
}

static int
c_api_cons_bad_cmd_usage(const char *const cmd)
{
    return api_cons_bad_cmd_usage(cmd);
}

static void
c_api_register_command(const char *filename, const char *command_name, int min_args, int max_args,
    char **synopsis, const char *description, char *arguments[][2], char **examples,
    void(*callback)(char **args))
{
    char *plugin_name = _c_plugin_name(filename);
    log_debug("Register command %s for %s", command_name, plugin_name);

    CommandWrapper *wrapper = malloc(sizeof(CommandWrapper));
    wrapper->func = callback;
    api_register_command(plugin_name, command_name, min_args, max_args, synopsis,
        description, arguments, examples, wrapper, c_command_callback, free);

    free(plugin_name);
}

static void
c_api_register_timed(const char *filename, void(*callback)(void), int interval_seconds)
{
    char *plugin_name = _c_plugin_name(filename);
    log_debug("Register timed for %s", plugin_name);

    TimedWrapper *wrapper = malloc(sizeof(TimedWrapper));
    wrapper->func = callback;
    api_register_timed(plugin_name, wrapper, interval_seconds, c_timed_callback, free);

    free(plugin_name);
}

static void
c_api_completer_add(const char *filename, const char *key, char **items)
{
    char *plugin_name = _c_plugin_name(filename);
    log_debug("Autocomplete add %s for %s", key, plugin_name);

    api_completer_add(plugin_name, key, items);

    free(plugin_name);
}

static void
c_api_completer_remove(const char *filename, const char *key, char **items)
{
    char *plugin_name = _c_plugin_name(filename);
    log_debug("Autocomplete remove %s for %s", key, plugin_name);

    api_completer_remove(plugin_name, key, items);

    free(plugin_name);
}

static void
c_api_completer_clear(const char *filename, const char *key)
{
    char *plugin_name = _c_plugin_name(filename);
    log_debug("Autocomplete clear %s for %s", key, plugin_name);

    api_completer_clear(plugin_name, key);

    free(plugin_name);
}

static void
c_api_filepath_completer_add(const char *filename, const char *prefix)
{
    char *plugin_name = _c_plugin_name(filename);
    log_debug("Filepath autocomplete added '%s' for %s", prefix, plugin_name);

    api_filepath_completer_add(plugin_name, prefix);

    free(plugin_name);
}

static void
c_api_notify(const char *message, int timeout_ms, const char *category)
{
    api_notify(message, category, timeout_ms);
}

static void
c_api_send_line(char *line)
{
    api_send_line(line);
}

static char *
c_api_get_current_recipient(void)
{
    return api_get_current_recipient();
}

static char *
c_api_get_current_muc(void)
{
    return api_get_current_muc();
}

static int
c_api_current_win_is_console(void)
{
    return api_current_win_is_console();
}

static char*
c_api_get_current_nick(void)
{
    return api_get_current_nick();
}

static char**
c_api_get_current_occupants(void)
{
    return api_get_current_occupants();
}

static char*
c_api_get_room_nick(const char *barejid)
{
    return api_get_room_nick(barejid);
}

static void
c_api_log_debug(const char *message)
{
    api_log_debug(message);
}

static void
c_api_log_info(const char *message)
{
    api_log_info(message);
}

static void
c_api_log_warning(const char *message)
{
    api_log_warning(message);
}

static void
c_api_log_error(const char *message)
{
    api_log_error(message);
}

static int
c_api_win_exists(char *tag)
{
    return api_win_exists(tag);
}

static void
c_api_win_create(const char *filename, char *tag, void(*callback)(char *tag, char *line))
{
    char *plugin_name = _c_plugin_name(filename);

    WindowWrapper *wrapper = malloc(sizeof(WindowWrapper));
    wrapper->func = callback;
    api_win_create(plugin_name, tag, wrapper, c_window_callback, free);

    free(plugin_name);
}

static int
c_api_win_focus(char *tag)
{
    return api_win_focus(tag);
}

static int
c_api_win_show(char *tag, char *line)
{
    return api_win_show(tag, line);
}

static int
c_api_win_show_themed(char *tag, char *group, char *key, char *def, char *line)
{
    return api_win_show_themed(tag, group, key, def, line);
}

static int
c_api_send_stanza(char *stanza)
{
    return api_send_stanza(stanza);
}

static int
c_api_settings_boolean_get(char *group, char *key, int def)
{
    return api_settings_boolean_get(group, key, def);
}

static void
c_api_settings_boolean_set(char *group, char *key, int value)
{
    api_settings_boolean_set(group, key, value);
}

static char*
c_api_settings_string_get(char *group, char *key, char *def)
{
    return api_settings_string_get(group, key, def);
}

static void
c_api_settings_string_set(char *group, char *key, char *value)
{
    api_settings_string_set(group, key, value);
}

static char**
c_api_settings_string_list_get(char *group, char *key)
{
    return api_settings_string_list_get(group, key);
}

static void
c_api_settings_string_list_add(char *group, char *key, char* value)
{
    api_settings_string_list_add(group, key, value);
}

static int
c_api_settings_string_list_remove(char *group, char *key, char *value)
{
    return api_settings_string_list_remove(group, key, value);
}

static int
c_api_settings_string_list_clear(char *group, char *key)
{
    return api_settings_string_list_clear(group, key);
}

static int
c_api_settings_int_get(char *group, char *key, int def)
{
    return api_settings_int_get(group, key, def);
}

static void
c_api_settings_int_set(char *group, char *key, int value)
{
    api_settings_int_set(group, key, value);
}

static void
c_api_incoming_message(char *barejid, char *resource, char *message)
{
    api_incoming_message(barejid, resource, message);
}

static void
c_api_disco_add_feature(const char *filename, char *feature)
{
    char *plugin_name = _c_plugin_name(filename);
    api_disco_add_feature(plugin_name, feature);
    free(plugin_name);
}

static void
c_api_encryption_reset(const char *barejid)
{
    api_encryption_reset(barejid);
}

static int
c_api_chat_set_titlebar_enctext(const char *barejid, const char *enctext)
{
    return api_chat_set_titlebar_enctext(barejid, enctext);
}

static int
c_api_chat_unset_titlebar_enctext(const char *barejid)
{
    return api_chat_unset_titlebar_enctext(barejid);
}

static int
c_api_chat_set_incoming_char(const char *barejid, const char *ch)
{
    return api_chat_set_incoming_char(barejid, ch);
}

static int
c_api_chat_unset_incoming_char(const char *barejid)
{
    return api_chat_unset_incoming_char(barejid);
}

static int
c_api_chat_set_outgoing_char(const char *barejid, const char *ch)
{
    return api_chat_set_outgoing_char(barejid, ch);
}

static int
c_api_chat_unset_outgoing_char(const char *barejid)
{
    return api_chat_unset_outgoing_char(barejid);
}

static int
c_api_room_set_titlebar_enctext(const char *roomjid, const char *enctext)
{
    return api_room_set_titlebar_enctext(roomjid, enctext);
}

static int
c_api_room_unset_titlebar_enctext(const char *roomjid)
{
    return api_room_unset_titlebar_enctext(roomjid);
}

static int
c_api_room_set_message_char(const char *roomjid, const char *ch)
{
    return api_room_set_message_char(roomjid, ch);
}

static int
c_api_room_unset_message_char(const char *roomjid)
{
    return api_room_unset_message_char(roomjid);
}

static int
c_api_chat_show(const char *const barejid, const char *const message)
{
    return api_chat_show(barejid, message);
}

static int
c_api_chat_show_themed(const char *const barejid, const char *const group, const char *const item, const char *const def,
    const char *const ch, const char *const message)
{
    return api_chat_show_themed(barejid, group, item, def, ch, message);
}

static int
c_api_room_show(const char *const roomjid, const char *const message)
{
    return api_room_show(roomjid, message);
}

static int
c_api_room_show_themed(const char *const roomjid, const char *const group, const char *const item, const char *const def,
    const char *const ch, const char *const message)
{
    return api_room_show_themed(roomjid, group, item, def, ch, message);
}

void
c_command_callback(PluginCommand *command, gchar **args)
{
    CommandWrapper *wrapper = command->callback;
    void(*f)(gchar **args) = wrapper->func;
    f(args);
}

void
c_timed_callback(PluginTimedFunction *timed_function)
{
    TimedWrapper *wrapper = timed_function->callback;
    void(*f)(void) = wrapper->func;
    f();
}

void
c_window_callback(PluginWindowCallback *window_callback, char *tag, char *line)
{
    WindowWrapper *wrapper = window_callback->callback;
    void(*f)(char *tag, char *line) = wrapper->func;
    f(tag, line);
}

void
c_api_init(void)
{
    prof_cons_alert = c_api_cons_alert;
    prof_cons_show = c_api_cons_show;
    prof_cons_show_themed = c_api_cons_show_themed;
    prof_cons_bad_cmd_usage = c_api_cons_bad_cmd_usage;
    _prof_register_command = c_api_register_command;
    _prof_register_timed = c_api_register_timed;
    _prof_completer_add = c_api_completer_add;
    _prof_completer_remove = c_api_completer_remove;
    _prof_completer_clear = c_api_completer_clear;
    _prof_filepath_completer_add = c_api_filepath_completer_add;
    _prof_win_create = c_api_win_create;
    prof_notify = c_api_notify;
    prof_send_line = c_api_send_line;
    prof_get_current_recipient = c_api_get_current_recipient;
    prof_get_current_muc = c_api_get_current_muc;
    prof_current_win_is_console = c_api_current_win_is_console;
    prof_get_current_nick = c_api_get_current_nick;
    prof_get_current_occupants = c_api_get_current_occupants;
    prof_get_room_nick = c_api_get_room_nick;
    prof_log_debug = c_api_log_debug;
    prof_log_info = c_api_log_info;
    prof_log_warning = c_api_log_warning;
    prof_log_error = c_api_log_error;
    prof_win_exists = c_api_win_exists;
    prof_win_focus = c_api_win_focus;
    prof_win_show = c_api_win_show;
    prof_win_show_themed = c_api_win_show_themed;
    prof_send_stanza = c_api_send_stanza;
    prof_settings_boolean_get = c_api_settings_boolean_get;
    prof_settings_boolean_set = c_api_settings_boolean_set;
    prof_settings_string_get = c_api_settings_string_get;
    prof_settings_string_set = c_api_settings_string_set;
    prof_settings_int_get = c_api_settings_int_get;
    prof_settings_int_set = c_api_settings_int_set;
    prof_settings_string_list_get = c_api_settings_string_list_get;
    prof_settings_string_list_add = c_api_settings_string_list_add;
    prof_settings_string_list_remove = c_api_settings_string_list_remove;
    prof_settings_string_list_clear = c_api_settings_string_list_clear;
    prof_incoming_message = c_api_incoming_message;
    _prof_disco_add_feature = c_api_disco_add_feature;
    prof_encryption_reset = c_api_encryption_reset;
    prof_chat_set_titlebar_enctext = c_api_chat_set_titlebar_enctext;
    prof_chat_unset_titlebar_enctext = c_api_chat_unset_titlebar_enctext;
    prof_chat_set_incoming_char = c_api_chat_set_incoming_char;
    prof_chat_unset_incoming_char = c_api_chat_unset_incoming_char;
    prof_chat_set_outgoing_char = c_api_chat_set_outgoing_char;
    prof_chat_unset_outgoing_char = c_api_chat_unset_outgoing_char;
    prof_room_set_titlebar_enctext = c_api_room_set_titlebar_enctext;
    prof_room_unset_titlebar_enctext = c_api_room_unset_titlebar_enctext;
    prof_room_set_message_char = c_api_room_set_message_char;
    prof_room_unset_message_char = c_api_room_unset_message_char;
    prof_chat_show = c_api_chat_show;
    prof_chat_show_themed = c_api_chat_show_themed;
    prof_room_show = c_api_room_show;
    prof_room_show_themed = c_api_room_show_themed;
}

static char *
_c_plugin_name(const char *filename)
{
    GString *plugin_name_str = g_string_new("");
    gchar *name = g_strndup(filename, strlen(filename)-1);
    g_string_append(plugin_name_str, name);
    g_free(name);
    g_string_append(plugin_name_str, "so");
    char *result = plugin_name_str->str;
    g_string_free(plugin_name_str, FALSE);

    return result;
}
