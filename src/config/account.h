/*
 * account.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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
gchar* account_create_connect_jid(ProfAccount* account);
gboolean account_eval_password(ProfAccount* account);
void account_free(ProfAccount* account);
void account_set_server(ProfAccount* account, const char* server);
void account_set_port(ProfAccount* account, int port);
void account_set_tls_policy(ProfAccount* account, const char* tls_policy);
void account_set_auth_policy(ProfAccount* account, const char* auth_policy);

#endif
