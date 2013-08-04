/*
 * command.c
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

#include "command/command.h"
#include "plugins/command.h"
#include "plugins/plugins.h"
#include "tools/autocomplete.h"

#include "ui/ui.h"

// API functions

static GSList *p_commands = NULL;

void
add_command(PluginCommand *command)
{
    p_commands = g_slist_append(p_commands, command);
    cmd_autocomplete_add(command->command_name);
}

gboolean
plugins_command_run(const char * const cmd)
{
    GSList *p_command = p_commands;

    while (p_command != NULL) {
        PluginCommand *command = p_command->data;
        if (strcmp(command->command_name, cmd) == 0) {
            PyObject_CallObject(command->p_callback, NULL);
            return TRUE;
        }
        p_command = g_slist_next(p_command);
    }
    return FALSE;
}
