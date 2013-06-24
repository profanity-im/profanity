/*
 * command.h
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

#ifndef COMMAND_H
#define COMMAND_H

#include <glib.h>

// Command help strings
typedef struct cmd_help_t {
    const gchar *usage;
    const gchar *short_help;
    const gchar *long_help[50];
} CommandHelp;

void cmd_init(void);
void cmd_close(void);

void cmd_autocomplete(char *input, int *size);
void cmd_reset_autocomplete(void);

gboolean cmd_execute(const char * const command, const char * const inp);
gboolean cmd_execute_default(const char * const inp);

GSList * cmd_get_basic_help(void);
GSList * cmd_get_settings_help(void);
GSList * cmd_get_presence_help(void);

void cmd_history_append(char *inp);
char *cmd_history_previous(char *inp, int *size);
char *cmd_history_next(char *inp, int *size);

#endif
