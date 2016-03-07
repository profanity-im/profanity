/*
 * api.h
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

#ifndef API_H
#define API_H

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

void api_register_command(const char *command_name, int min_args, int max_args,
    const char **synopsis, const char *description, const char *arguments[][2], const char **examples,
    void *callback, void(*callback_func)(PluginCommand *command, gchar **args));
void api_register_timed(void *callback, int interval_seconds,
    void (*callback_func)(PluginTimedFunction *timed_function));
void api_register_ac(const char *key, char **items);

void api_log_debug(const char *message);
void api_log_info(const char *message);
void api_log_warning(const char *message);
void api_log_error(const char *message);

int api_win_exists(const char *tag);
void api_win_create(
    const char *tag,
    void *callback,
    void(*destroy)(void *callback),
    void(*callback_func)(PluginWindowCallback *window_callback, char *tag, char *line));
int api_win_focus(const char *tag);
int api_win_show(const char *tag, const char *line);
int api_win_show_themed(const char *tag, const char *const group, const char *const key, const char *const def, const char *line);

#endif
