/*
 * accounts.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef CONFIG_ACCOUNTS_H
#define CONFIG_ACCOUNTS_H

#define MAX_PASSWORD_SIZE 64

#include "common.h"
#include "config/account.h"

void accounts_load(void);

gchar* accounts_find_all(const gchar* const prefix, gboolean previous, void* context);
gchar* accounts_find_enabled(const gchar* const prefix, gboolean previous, void* context);
void accounts_reset_all_search(void);
void accounts_reset_enabled_search(void);
void accounts_add(const gchar* jid, const gchar* altdomain, const int port, const gchar* const tls_policy, const gchar* const auth_policy);
gboolean accounts_remove(const gchar* jid);
gchar** accounts_get_list(void);
ProfAccount* accounts_get_account(const gchar* const name);
gboolean accounts_enable(const gchar* const name);
gboolean accounts_disable(const gchar* const name);
gboolean accounts_rename(const gchar* const account_name,
                         const gchar* const new_name);
gboolean accounts_account_exists(const gchar* const account_name);
void accounts_set_jid(const gchar* const account_name, const gchar* const value);
void accounts_set_server(const gchar* const account_name, const gchar* const value);
void accounts_set_port(const gchar* const account_name, const int value);
void accounts_set_resource(const gchar* const account_name, const gchar* const value);
gchar* accounts_get_resource(const gchar* const account_name);
void accounts_set_password(const gchar* const account_name, const gchar* const value);
void accounts_set_eval_password(const gchar* const account_name, const gchar* const value);
void accounts_set_muc_service(const gchar* const account_name, const gchar* const value);
void accounts_set_muc_nick(const gchar* const account_name, const gchar* const value);
void accounts_set_otr_policy(const gchar* const account_name, const gchar* const value);
void accounts_set_tls_policy(const gchar* const account_name, const gchar* const value);
void accounts_set_auth_policy(const gchar* const account_name, const gchar* const value);
void accounts_set_last_presence(const gchar* const account_name, const gchar* const value);
void accounts_set_last_status(const gchar* const account_name, const gchar* const value);
void accounts_set_last_activity(const gchar* const account_name);
gchar* accounts_get_last_activity(const gchar* const account_name);
void accounts_set_login_presence(const gchar* const account_name, const gchar* const value);
resource_presence_t accounts_get_login_presence(const gchar* const account_name);
gchar* accounts_get_last_status(const gchar* const account_name);
resource_presence_t accounts_get_last_presence(const gchar* const account_name);
gchar* accounts_get_login_status(const gchar* const account_name);
void accounts_set_priority_online(const gchar* const account_name, const gint value);
void accounts_set_priority_chat(const gchar* const account_name, const gint value);
void accounts_set_priority_away(const gchar* const account_name, const gint value);
void accounts_set_priority_xa(const gchar* const account_name, const gint value);
void accounts_set_priority_dnd(const gchar* const account_name, const gint value);
void accounts_set_priority_all(const gchar* const account_name, const gint value);
gint accounts_get_priority_for_presence_type(const gchar* const account_name,
                                             resource_presence_t presence_type);
void accounts_set_pgp_keyid(const gchar* const account_name, const gchar* const value);
void accounts_set_script_start(const gchar* const account_name, const gchar* const value);
void accounts_set_client(const gchar* const account_name, const gchar* const value);
void accounts_set_theme(const gchar* const account_name, const gchar* const value);
void accounts_set_max_sessions(const gchar* const account_name, const int value);
int accounts_get_max_sessions(const gchar* const account_name);
void accounts_clear_password(const gchar* const account_name);
void accounts_clear_eval_password(const gchar* const account_name);
void accounts_clear_server(const gchar* const account_name);
void accounts_clear_port(const gchar* const account_name);
void accounts_clear_otr(const gchar* const account_name);
void accounts_clear_pgp_keyid(const gchar* const account_name);
void accounts_clear_script_start(const gchar* const account_name);
void accounts_clear_client(const gchar* const account_name);
void accounts_clear_theme(const gchar* const account_name);
void accounts_clear_muc(const gchar* const account_name);
void accounts_clear_resource(const gchar* const account_name);
void accounts_clear_max_sessions(const gchar* const account_name);
void accounts_add_otr_policy(const gchar* const account_name, const gchar* const contact_jid, const gchar* const policy);
void accounts_add_omemo_state(const gchar* const account_name, const gchar* const contact_jid, gboolean enabled);
void accounts_add_ox_state(const gchar* const account_name, const gchar* const contact_jid, gboolean enabled);
void accounts_add_pgp_state(const gchar* const account_name, const gchar* const contact_jid, gboolean enabled);
void accounts_clear_omemo_state(const gchar* const account_name, const gchar* const contact_jid);

#endif
