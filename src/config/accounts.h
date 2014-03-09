/*
 * accounts.h
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

#ifndef ACCOUNTS_H
#define ACCOUNTS_H

#define MAX_PASSWORD_SIZE 64

#include "common.h"
#include "config/account.h"

void accounts_init_module(void);

void (*accounts_load)(void);
void (*accounts_close)(void);

char * (*accounts_find_all)(char *prefix);
char * (*accounts_find_enabled)(char *prefix);
void (*accounts_reset_all_search)(void);
void (*accounts_reset_enabled_search)(void);
void (*accounts_add)(const char *jid, const char *altdomain, const int port);
gchar** (*accounts_get_list)(void);
ProfAccount* (*accounts_get_account)(const char * const name);
gboolean (*accounts_enable)(const char * const name);
gboolean (*accounts_disable)(const char * const name);
gboolean (*accounts_rename)(const char * const account_name,
    const char * const new_name);
gboolean (*accounts_account_exists)(const char * const account_name);
void (*accounts_set_jid)(const char * const account_name, const char * const value);
void (*accounts_set_server)(const char * const account_name, const char * const value);
void (*accounts_set_port)(const char * const account_name, const int value);
void (*accounts_set_resource)(const char * const account_name, const char * const value);
void (*accounts_set_password)(const char * const account_name, const char * const value);
void (*accounts_set_muc_service)(const char * const account_name, const char * const value);
void (*accounts_set_muc_nick)(const char * const account_name, const char * const value);
void (*accounts_set_last_presence)(const char * const account_name, const char * const value);
void (*accounts_set_login_presence)(const char * const account_name, const char * const value);
resource_presence_t (*accounts_get_login_presence)(const char * const account_name);
resource_presence_t (*accounts_get_last_presence)(const char * const account_name);
void (*accounts_set_priority_online)(const char * const account_name, const gint value);
void (*accounts_set_priority_chat)(const char * const account_name, const gint value);
void (*accounts_set_priority_away)(const char * const account_name, const gint value);
void (*accounts_set_priority_xa)(const char * const account_name, const gint value);
void (*accounts_set_priority_dnd)(const char * const account_name, const gint value);
void (*accounts_set_priority_all)(const char * const account_name, const gint value);
gint (*accounts_get_priority_for_presence_type)(const char * const account_name,
    resource_presence_t presence_type);
void (*accounts_clear_password)(const char * const account_name);

#endif
