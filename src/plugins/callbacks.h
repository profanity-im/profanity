/*
 * callbacks.h
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

#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <glib.h>

#include "command/cmd_defs.h"

typedef struct p_command {
    char *plugin_name;
    char *command_name;
    int min_args;
    int max_args;
    CommandHelp *help;
    void *callback;
    void (*callback_func)(struct p_command *command, gchar **args);
} PluginCommand;

typedef struct p_timed_function {
    void *callback;
    void (*callback_func)(struct p_timed_function *timed_function);
    int interval_seconds;
    GTimer *timer;
} PluginTimedFunction;

typedef struct p_window_input_callback {
    void *callback;
    void (*destroy)(void *callback);
    void (*callback_func)(struct p_window_input_callback *window_callback, const char *tag, const char * const line);
} PluginWindowCallback;

void callbacks_init(void);
void callbacks_close(void);

void callbacks_add_command(PluginCommand *command);
void callbacks_remove_commands(const char *const plugin_name);
void callbacks_add_timed(PluginTimedFunction *timed_function);
void callbacks_add_window_handler(const char *tag, PluginWindowCallback *window_callback);
void * callbacks_get_window_handler(const char *tag);

#endif
