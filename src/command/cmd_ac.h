/*
 * cmd_ac.h
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

#ifndef COMMAND_CMD_AC_H
#define COMMAND_CMD_AC_H

#include "config/preferences.h"
#include "command/cmd_funcs.h"

void cmd_ac_init(void);
void cmd_ac_uninit(void);
char* cmd_ac_complete(ProfWin *window, const char *const input, gboolean previous);
void cmd_ac_reset(ProfWin *window);
gboolean cmd_ac_exists(char *cmd);

void cmd_ac_add(const char *const value);
void cmd_ac_add_help(const char *const value);
void cmd_ac_add_cmd(Command *command);
void cmd_ac_add_alias(ProfAlias *alias);
void cmd_ac_add_alias_value(char *value);

void cmd_ac_remove(const char *const value);
void cmd_ac_remove_help(const char *const value);
void cmd_ac_remove_alias_value(char *value);

void cmd_ac_add_form_fields(DataForm *form);
void cmd_ac_remove_form_fields(DataForm *form);

char* cmd_ac_complete_filepath(const char *const input, char *const startstr, gboolean previous);

#endif
