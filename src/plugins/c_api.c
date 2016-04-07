/*
 * c_api.c
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

#include <stdlib.h>
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
c_api_register_command(const char *command_name, int min_args, int max_args,
    const char **synopsis, const char *description, const char *arguments[][2], const char **examples,
    void(*callback)(char **args))
{
    CommandWrapper *wrapper = malloc(sizeof(CommandWrapper));
    wrapper->func = callback;
    api_register_command(command_name, min_args, max_args, synopsis,
        description, arguments, examples, wrapper, c_command_callback);
}

static void
c_api_register_timed(void(*callback)(void), int interval_seconds)
{
    TimedWrapper *wrapper = malloc(sizeof(TimedWrapper));
    wrapper->func = callback;
    api_register_timed(wrapper, interval_seconds, c_timed_callback);
}

static void
c_api_completer_add(const char *key, char **items)
{
    api_completer_add(key, items);
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
c_api_current_win_is_console()
{
    return api_current_win_is_console();
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
c_api_win_create(char *tag, void(*callback)(char *tag, char *line))
{
    WindowWrapper *wrapper = malloc(sizeof(WindowWrapper));
    wrapper->func = callback;
    api_win_create(tag, wrapper, free, c_window_callback);
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
c_api_settings_get_boolean(char *group, char *key, int def)
{
    return api_settings_get_boolean(group, key, def);
}

static void
c_api_settings_set_boolean(char *group, char *key, int value)
{
    api_settings_set_boolean(group, key, value);
}

static char*
c_api_settings_get_string(char *group, char *key, char *def)
{
    return api_settings_get_string(group, key, def);
}

static void
c_api_settings_set_string(char *group, char *key, char *value)
{
    api_settings_set_string(group, key, value);
}

static int
c_api_settings_get_int(char *group, char *key, int def)
{
    return api_settings_get_int(group, key, def);
}

static void
c_api_settings_set_int(char *group, char *key, int value)
{
    api_settings_set_int(group, key, value);
}

static void
c_api_incoming_message(char *barejid, char *resource, char *message)
{
    api_incoming_message(barejid, resource, message);
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
    prof_register_command = c_api_register_command;
    prof_register_timed = c_api_register_timed;
    prof_completer_add = c_api_completer_add;
    prof_notify = c_api_notify;
    prof_send_line = c_api_send_line;
    prof_get_current_recipient = c_api_get_current_recipient;
    prof_get_current_muc = c_api_get_current_muc;
    prof_current_win_is_console = c_api_current_win_is_console;
    prof_log_debug = c_api_log_debug;
    prof_log_info = c_api_log_info;
    prof_log_warning = c_api_log_warning;
    prof_log_error = c_api_log_error;
    prof_win_exists = c_api_win_exists;
    prof_win_create = c_api_win_create;
    prof_win_focus = c_api_win_focus;
    prof_win_show = c_api_win_show;
    prof_win_show_themed = c_api_win_show_themed;
    prof_send_stanza = c_api_send_stanza;
    prof_settings_get_boolean = c_api_settings_get_boolean;
    prof_settings_set_boolean = c_api_settings_set_boolean;
    prof_settings_get_string = c_api_settings_get_string;
    prof_settings_set_string = c_api_settings_set_string;
    prof_settings_get_int = c_api_settings_get_int;
    prof_settings_set_int = c_api_settings_set_int;
    prof_incoming_message = c_api_incoming_message;
}
