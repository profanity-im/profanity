/*
 * callbacks.h
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

#ifndef PLUGINS_CALLBACKS_H
#define PLUGINS_CALLBACKS_H

#include <glib.h>

#include "command/cmd_defs.h"

typedef struct p_command {
    char *command_name;
    int min_args;
    int max_args;
    CommandHelp *help;
    void *callback;
    void (*callback_exec)(struct p_command *command, gchar **args);
    void (*callback_destroy)(void *callback);
} PluginCommand;

typedef struct p_timed_function {
    void *callback;
    void (*callback_exec)(struct p_timed_function *timed_function);
    void (*callback_destroy)(void *callback);
    int interval_seconds;
    GTimer *timer;
} PluginTimedFunction;

typedef struct p_window_input_callback {
    void *callback;
    void (*callback_exec)(struct p_window_input_callback *window_callback, const char *tag, const char * const line);
    void (*callback_destroy)(void *callback);
} PluginWindowCallback;

void callbacks_init(void);
void callbacks_remove(const char *const plugin_name);
void callbacks_close(void);

void callbacks_add_command(const char *const plugin_name, PluginCommand *command);
void callbacks_add_timed(const char *const plugin_name, PluginTimedFunction *timed_function);
gboolean callbacks_win_exists(const char *const plugin_name, const char *tag);
void callbacks_add_window_handler(const char *const plugin_name, const char *tag, PluginWindowCallback *window_callback);
void * callbacks_get_window_handler(const char *tag);
void callbacks_remove_win(const char *const plugin_name, const char *const tag);

#endif
