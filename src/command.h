/* 
 * command.h
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
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

// command help strings
struct cmd_help_t {
    const gchar *usage;
    const gchar *short_help;
    const gchar *long_help[50];
};

void command_init(void);
gboolean process_input(char *inp);
char * cmd_complete(char *inp);
void reset_command_completer(void);
GSList * cmd_get_help_list_basic(void);
GSList * cmd_get_help_list_settings(void);
GSList * cmd_get_help_list_status(void);

#endif
