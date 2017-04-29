/*
 * accounts.h
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

#ifndef CONFIG_ACCOUNTS_H
#define CONFIG_ACCOUNTS_H

#define MAX_PASSWORD_SIZE 64

#include "common.h"
#include "config/account.h"

void accounts_load(void);
void accounts_close(void);

char* accounts_find_all(const char *const prefix, gboolean previous);
char* accounts_find_enabled(const char *const prefix, gboolean previous);
void accounts_reset_all_search(void);
void accounts_reset_enabled_search(void);
void accounts_add(const char *jid, const char *altdomain, const int port, const char *const tls_policy);
int  accounts_remove(const char *jid);
gchar** accounts_get_list(void);
ProfAccount* accounts_get_account(const char *const name);
gboolean accounts_enable(const char *const name);
gboolean accounts_disable(const char *const name);
gboolean accounts_rename(const char *const account_name,
    const char *const new_name);
gboolean accounts_account_exists(const char *const account_name);
void accounts_set_jid(const char *const account_name, const char *const value);
void accounts_set_server(const char *const account_name, const char *const value);
void accounts_set_port(const char *const account_name, const int value);
void accounts_set_resource(const char *const account_name, const char *const value);
void accounts_set_password(const char *const account_name, const char *const value);
void accounts_set_eval_password(const char *const account_name, const char *const value);
void accounts_set_muc_service(const char *const account_name, const char *const value);
void accounts_set_muc_nick(const char *const account_name, const char *const value);
void accounts_set_otr_policy(const char *const account_name, const char *const value);
void accounts_set_tls_policy(const char *const account_name, const char *const value);
void accounts_set_last_presence(const char *const account_name, const char *const value);
void accounts_set_last_status(const char *const account_name, const char *const value);
void accounts_set_last_activity(const char *const account_name);
char* accounts_get_last_activity(const char *const account_name);
void accounts_set_login_presence(const char *const account_name, const char *const value);
resource_presence_t accounts_get_login_presence(const char *const account_name);
char* accounts_get_last_status(const char *const account_name);
resource_presence_t accounts_get_last_presence(const char *const account_name);
void accounts_set_priority_online(const char *const account_name, const gint value);
void accounts_set_priority_chat(const char *const account_name, const gint value);
void accounts_set_priority_away(const char *const account_name, const gint value);
void accounts_set_priority_xa(const char *const account_name, const gint value);
void accounts_set_priority_dnd(const char *const account_name, const gint value);
void accounts_set_priority_all(const char *const account_name, const gint value);
gint accounts_get_priority_for_presence_type(const char *const account_name,
    resource_presence_t presence_type);
void accounts_set_pgp_keyid(const char *const account_name, const char *const value);
void accounts_set_script_start(const char *const account_name, const char *const value);
void accounts_set_theme(const char *const account_name, const char *const value);
void accounts_clear_password(const char *const account_name);
void accounts_clear_eval_password(const char *const account_name);
void accounts_clear_server(const char *const account_name);
void accounts_clear_port(const char *const account_name);
void accounts_clear_otr(const char *const account_name);
void accounts_clear_pgp_keyid(const char *const account_name);
void accounts_clear_script_start(const char *const account_name);
void accounts_clear_theme(const char *const account_name);
void accounts_clear_muc(const char *const account_name);
void accounts_clear_resource(const char *const account_name);
void accounts_add_otr_policy(const char *const account_name, const char *const contact_jid, const char *const policy);

#endif
