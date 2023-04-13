/*
 * account.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
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

#ifndef CONFIG_ACCOUNT_H
#define CONFIG_ACCOUNT_H

#include "common.h"

typedef struct prof_account_t
{
    gchar* name;
    gchar* jid;
    gchar* password;
    gchar* eval_password;
    gchar* resource;
    gchar* server;
    int port;
    gchar* last_presence;
    gchar* login_presence;
    gint priority_online;
    gint priority_chat;
    gint priority_away;
    gint priority_xa;
    gint priority_dnd;
    gchar* muc_service;
    gchar* muc_nick;
    gboolean enabled;
    gchar* otr_policy;
    GList* otr_manual;
    GList* otr_opportunistic;
    GList* otr_always;
    gchar* omemo_policy;
    GList* omemo_enabled;
    GList* omemo_disabled;
    GList* ox_enabled;
    GList* pgp_enabled;
    gchar* pgp_keyid;
    gchar* startscript;
    gchar* theme;
    gchar* tls_policy;
    gchar* auth_policy;
    gchar* client;
    int max_sessions;
} ProfAccount;

ProfAccount* account_new(gchar* name, gchar* jid, gchar* password, gchar* eval_password, gboolean enabled,
                         gchar* server, int port, gchar* resource, gchar* last_presence, gchar* login_presence,
                         int priority_online, int priority_chat, int priority_away, int priority_xa, int priority_dnd,
                         gchar* muc_service, gchar* muc_nick,
                         gchar* otr_policy, GList* otr_manual, GList* otr_opportunistic, GList* otr_always,
                         gchar* omemo_policy, GList* omemo_enabled, GList* omemo_disabled,
                         GList* ox_enabled, GList* pgp_enabled, gchar* pgp_keyid,
                         gchar* startscript, gchar* theme, gchar* tls_policy, gchar* auth_policy,
                         gchar* client, int max_sessions);
char* account_create_connect_jid(ProfAccount* account);
gboolean account_eval_password(ProfAccount* account);
void account_free(ProfAccount* account);
void account_set_server(ProfAccount* account, const char* server);
void account_set_port(ProfAccount* account, int port);
void account_set_tls_policy(ProfAccount* account, const char* tls_policy);
void account_set_auth_policy(ProfAccount* account, const char* auth_policy);

#endif
