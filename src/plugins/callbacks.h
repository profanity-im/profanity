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

#include <Python.h>

#include <glib.h>

typedef struct p_command {
    const char *command_name;
    int min_args;
    int max_args;
    const char *usage;
    const char *short_help;
    const char *long_help;
    PyObject *p_callback;
} PluginCommand;

typedef struct p_timed_function {
    PyObject *p_callback;
    int interval_seconds;
    GTimer *timer;
} PluginTimedFunction;

void callbacks_add_command(PluginCommand *command);
void callbacks_add_timed(PluginTimedFunction *timed_function);

#endif
