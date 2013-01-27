/*
 * accounts.h
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

#ifndef ACCOUNTS_H
#define ACCOUNTS_H

typedef struct prof_account_t {
    gchar *name;
    gchar *jid;
    gchar *resource;
    gchar *server;
    gboolean enabled;
} ProfAccount;

void accounts_load(void);
void accounts_close(void);

char * accounts_find_all(char *prefix);
char * accounts_find_enabled(char *prefix);
void accounts_reset_all_search(void);
void accounts_reset_enabled_search(void);
void accounts_add(const char *jid, const char *altdomain);
gchar** accounts_get_list(void);
ProfAccount* accounts_get_account(const char * const name);
void accounts_free_account(ProfAccount *account);
gboolean accounts_enable(const char * const name);
gboolean accounts_disable(const char * const name);
gboolean accounts_rename(const char * const account_name,
    const char * const new_name);
gboolean accounts_account_exists(const char * const account_name);
void accounts_set_jid(const char * const account_name, const char * const value);
void accounts_set_server(const char * const account_name, const char * const value);
void accounts_set_resource(const char * const account_name, const char * const value);

#endif
