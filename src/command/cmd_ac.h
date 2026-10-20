/*
 * cmd_ac.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef COMMAND_CMD_AC_H
#define COMMAND_CMD_AC_H

#include "config/preferences.h"
#include "command/cmd_funcs.h"

void cmd_ac_init(void);
void cmd_ac_uninit(void);
gchar* cmd_ac_complete(ProfWin* window, const gchar* const input, gboolean previous);
void cmd_ac_reset(ProfWin* window);
gboolean cmd_ac_exists(const gchar* const cmd);

void cmd_ac_add(const gchar* const value);
void cmd_ac_add_help(const gchar* const value);
void cmd_ac_add_cmd(const Command* command);
void cmd_ac_add_alias(ProfAlias* alias);
void cmd_ac_add_alias_value(const gchar* const value);

void cmd_ac_remove(const gchar* const value);
void cmd_ac_remove_help(const gchar* const value);
void cmd_ac_remove_alias_value(const gchar* const value);

void cmd_ac_add_form_fields(DataForm* form);
void cmd_ac_remove_form_fields(DataForm* form);

gchar* cmd_ac_complete_filepath(const gchar* const input, const gchar* const startstr, gboolean previous);

#endif
