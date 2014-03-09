/*
 * command.h
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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

GHashTable *commands;

void cmd_init(void);
void cmd_uninit(void);

void cmd_autocomplete(char *input, int *size);
void cmd_reset_autocomplete(void);
void cmd_autocomplete_add(char *value);
void cmd_autocomplete_remove(char *value);
void cmd_alias_add(char *value);
void cmd_alias_remove(char *value);

gboolean cmd_execute(const char * const command, const char * const inp);
gboolean cmd_execute_alias(const char * const inp, gboolean *ran);
gboolean cmd_execute_default(const char * const inp);

gboolean cmd_exists(char *cmd);

GSList * cmd_get_basic_help(void);
GSList * cmd_get_settings_help(void);
GSList * cmd_get_presence_help(void);

void cmd_history_append(char *inp);
char *cmd_history_previous(char *inp, int *size);
char *cmd_history_next(char *inp, int *size);

#endif
