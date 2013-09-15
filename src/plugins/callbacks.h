/*
 * callbacks.h
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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
 */

#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <glib.h>

typedef struct p_command {
    const char *command_name;
    int min_args;
    int max_args;
    const char *usage;
    const char *short_help;
    const char *long_help;
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
    void (*callback_func)(struct p_window_input_callback *window_callback, const char *tag, const char * const line);
} PluginWindowCallback;

void callbacks_add_command(PluginCommand *command);
void callbacks_add_timed(PluginTimedFunction *timed_function);
void callbacks_add_window_handler(const char *tag, PluginWindowCallback *window_callback);
void * callbacks_get_window_handler(const char *tag);

#endif
