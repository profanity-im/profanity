/*
 * callbacks.c
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

#include "Python.h"

#include "command/command.h"
#include "plugins/callbacks.h"
#include "plugins/plugins.h"
#include "tools/autocomplete.h"
#include "tools/parser.h"

#include "ui/ui.h"

static GSList *p_commands = NULL;
static GSList *p_timed_functions = NULL;

void
callbacks_add_command(PluginCommand *command)
{
    p_commands = g_slist_append(p_commands, command);
    cmd_autocomplete_add(command->command_name);
}

void
callbacks_add_timed(PluginTimedFunction *timed_function)
{
    p_timed_functions = g_slist_append(p_timed_functions, timed_function);
}

void
python_command_callback(PluginCommand *command, gchar **args)
{
    PyObject *p_args = NULL;
    int num_args = g_strv_length(args);
    if (num_args == 0) {
        if (command->max_args == 1) {
            p_args = Py_BuildValue("(O)", Py_BuildValue(""));
            PyObject_CallObject(command->callback, p_args);
            Py_XDECREF(p_args);
        } else {
            PyObject_CallObject(command->callback, p_args);
        }
    } else if (num_args == 1) {
        p_args = Py_BuildValue("(s)", args[0]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    } else if (num_args == 2) {
        p_args = Py_BuildValue("ss", args[0], args[1]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    } else if (num_args == 3) {
        p_args = Py_BuildValue("sss", args[0], args[1], args[2]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    } else if (num_args == 4) {
        p_args = Py_BuildValue("ssss", args[0], args[1], args[2], args[3]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    } else if (num_args == 5) {
        p_args = Py_BuildValue("sssss", args[0], args[1], args[2], args[3], args[4]);
        PyObject_CallObject(command->callback, p_args);
        Py_XDECREF(p_args);
    }

    if (PyErr_Occurred()) {
        PyErr_Print();
        PyErr_Clear();
    }
}

void
python_timed_callback(PluginTimedFunction *timed_function)
{
    PyObject_CallObject(timed_function->callback, NULL);
}

gboolean
plugins_run_command(const char * const input)
{
    gchar **split = g_strsplit(input, " ", -1);

    GSList *p_command = p_commands;
    while (p_command != NULL) {
        PluginCommand *command = p_command->data;
        if (g_strcmp0(split[0], command->command_name) == 0) {
            gchar **args = parse_args(input, command->min_args, command->max_args);
            if (args == NULL) {
                cons_show("");
                cons_show("Usage: %s", command->usage);
                if (ui_current_win_type() == WIN_CHAT) {
                    char usage[strlen(command->usage) + 8];
                    sprintf(usage, "Usage: %s", command->usage);
                    ui_current_print_line(usage);
                }
                return TRUE;
            } else {
                python_command_callback(command, args);
                g_strfreev(split);
                return TRUE;
            }
            g_strfreev(args);
        }
        p_command = g_slist_next(p_command);
    }
    g_strfreev(split);
    return FALSE;
}

void
plugins_run_timed(void)
{
    GSList *p_timed_function = p_timed_functions;

    while (p_timed_function != NULL) {
        PluginTimedFunction *timed_function = p_timed_function->data;
        gdouble elapsed = g_timer_elapsed(timed_function->timer, NULL);

        if (timed_function->interval_seconds > 0 && elapsed >= timed_function->interval_seconds) {
            python_timed_callback(timed_function);
            g_timer_start(timed_function->timer);
        }

        p_timed_function = g_slist_next(p_timed_function);
    }
    return;
}
