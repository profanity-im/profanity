/*
 * callbacks.c
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

#include <string.h>

#include "command/command.h"
#include "plugins/callbacks.h"
#include "plugins/plugins.h"
#include "tools/autocomplete.h"
#include "tools/parser.h"

#include "ui/ui.h"

static GSList *p_commands = NULL;
static GSList *p_timed_functions = NULL;
static GHashTable *p_window_callbacks = NULL;

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
callbacks_add_window_handler(const char *tag, PluginWindowCallback *window_callback)
{
    if (p_window_callbacks == NULL) {
        p_window_callbacks = g_hash_table_new(g_str_hash, g_str_equal);
    }

    g_hash_table_insert(p_window_callbacks, strdup(tag), window_callback);
}

void *
callbacks_get_window_handler(const char *tag)
{
    if (p_window_callbacks) {
        return g_hash_table_lookup(p_window_callbacks, tag);
    } else {
        return NULL;
    }
}

gboolean
plugins_run_command(const char * const input)
{
    gchar **split = g_strsplit(input, " ", -1);

    GSList *p_command = p_commands;
    while (p_command) {
        PluginCommand *command = p_command->data;
        if (g_strcmp0(split[0], command->command_name) == 0) {
            gboolean result;
            gchar **args = parse_args(input, command->min_args, command->max_args, &result);
            if (result == FALSE) {
                ui_invalid_command_usage(command->command_name, NULL);
                g_strfreev(split);
                return TRUE;
            } else {
                command->callback_func(command, args);
                g_strfreev(split);
                g_strfreev(args);
                return TRUE;
            }
        }
        p_command = g_slist_next(p_command);
    }
    g_strfreev(split);
    return FALSE;
}

CommandHelp*
plugins_get_help(const char *const cmd)
{
    GSList *curr = p_commands;
    while (curr) {
        PluginCommand *command = curr->data;
        if (g_strcmp0(cmd, command->command_name) == 0) {
            return command->help;
        }

        curr = g_slist_next(curr);
    }

    return NULL;
}

void
plugins_run_timed(void)
{
    GSList *p_timed_function = p_timed_functions;

    while (p_timed_function) {
        PluginTimedFunction *timed_function = p_timed_function->data;
        gdouble elapsed = g_timer_elapsed(timed_function->timer, NULL);

        if (timed_function->interval_seconds > 0 && elapsed >= timed_function->interval_seconds) {
            timed_function->callback_func(timed_function);
            g_timer_start(timed_function->timer);
        }

        p_timed_function = g_slist_next(p_timed_function);
    }
    return;
}
