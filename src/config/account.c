/*
 * account.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/wait.h>

#include <glib.h>

#include "common.h"
#include "log.h"
#include "config/account.h"
#include "xmpp/jid.h"
#include "xmpp/resource.h"

ProfAccount*
account_new(gchar* name, gchar* jid, gchar* password, gchar* eval_password, gboolean enabled,
            gchar* server, int port, gchar* resource, gchar* last_presence, gchar* login_presence,
            int priority_online, int priority_chat, int priority_away, int priority_xa, int priority_dnd,
            gchar* muc_service, gchar* muc_nick,
            gchar* otr_policy, GList* otr_manual, GList* otr_opportunistic, GList* otr_always,
            gchar* omemo_policy, GList* omemo_enabled, GList* omemo_disabled,
            GList* ox_enabled, GList* pgp_enabled, gchar* pgp_keyid,
            gchar* startscript, gchar* theme, gchar* tls_policy, gchar* auth_policy,
            gchar* client, int max_sessions)
{
    ProfAccount* new_account = g_new0(ProfAccount, 1);

    new_account->name = name;

    if (jid) {
        new_account->jid = jid;
    } else {
        new_account->jid = g_strdup(name);
    }

    new_account->password = password;

    new_account->eval_password = eval_password;

    new_account->enabled = enabled;

    new_account->server = server;

    new_account->resource = resource;

    new_account->port = port;

    if (last_presence == NULL || !valid_resource_presence_string(last_presence)) {
        new_account->last_presence = g_strdup("online");
        g_free(last_presence);
    } else {
        new_account->last_presence = last_presence;
    }

    if (login_presence == NULL) {
        new_account->login_presence = g_strdup("online");
    } else if (strcmp(login_presence, "last") == 0) {
        new_account->login_presence = login_presence;
    } else if (!valid_resource_presence_string(login_presence)) {
        new_account->login_presence = g_strdup("online");
        g_free(login_presence);
    } else {
        new_account->login_presence = login_presence;
    }

    new_account->priority_online = priority_online;
    new_account->priority_chat = priority_chat;
    new_account->priority_away = priority_away;
    new_account->priority_xa = priority_xa;
    new_account->priority_dnd = priority_dnd;

    new_account->muc_service = muc_service;

    if (muc_nick == NULL) {
        auto_jid Jid* jidp = jid_create(new_account->jid);
        new_account->muc_nick = g_strdup(jidp->domainpart);
    } else {
        new_account->muc_nick = muc_nick;
    }

    new_account->otr_policy = otr_policy;

    new_account->otr_manual = otr_manual;
    new_account->otr_opportunistic = otr_opportunistic;
    new_account->otr_always = otr_always;

    new_account->omemo_policy = omemo_policy;
    new_account->omemo_enabled = omemo_enabled;
    new_account->omemo_disabled = omemo_disabled;

    new_account->ox_enabled = ox_enabled;

    new_account->pgp_enabled = pgp_enabled;
    new_account->pgp_keyid = pgp_keyid;

    new_account->startscript = startscript;

    new_account->client = client;

    new_account->theme = theme;

    new_account->tls_policy = tls_policy;

    new_account->auth_policy = auth_policy;

    new_account->max_sessions = max_sessions;

    return new_account;
}

gchar*
account_create_connect_jid(ProfAccount* account)
{
    if (account->resource) {
        return create_fulljid(account->jid, account->resource);
    } else {
        return g_strdup(account->jid);
    }
}

gboolean
account_eval_password(ProfAccount* account)
{
    assert(account != NULL);
    assert(account->eval_password != NULL);

    gchar* stdout_buf = NULL;
    GError* error = NULL;
    gint exit_status = 0;
    gchar** argv = NULL;

    if (!g_shell_parse_argv(account->eval_password, NULL, &argv, &error)) {
        log_error("Failed to parse `eval_password` command: %s", error->message);
        g_error_free(error);
        return FALSE;
    }

    if (!g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_CHILD_INHERITS_STDIN, NULL, NULL, &stdout_buf, NULL, &exit_status, &error)) {
        log_error("Failed to execute `eval_password` command: %s", error->message);
        g_strfreev(argv);
        g_error_free(error);
        return FALSE;
    }

    g_strfreev(argv);

    if (WIFEXITED(exit_status) && WEXITSTATUS(exit_status) == 0) {
        account->password = stdout_buf;
        g_strstrip(account->password);
        if (account->password[0] == '\0') {
            log_error("Empty password returned by `eval_password` command.");
            GFREE_SET_NULL(account->password);
            return FALSE;
        }
        return TRUE;
    } else {
        log_error("`eval_password` command failed with status %d", exit_status);
        g_free(stdout_buf);
        return FALSE;
    }
}

void
account_free(ProfAccount* account)
{
    if (account == NULL) {
        return;
    }

    g_free(account->name);
    g_free(account->jid);
    prof_wipe_string(account->password);
    g_free(account->password);
    g_free(account->eval_password);
    g_free(account->resource);
    g_free(account->server);
    g_free(account->last_presence);
    g_free(account->login_presence);
    g_free(account->muc_service);
    g_free(account->muc_nick);
    g_free(account->otr_policy);
    g_free(account->omemo_policy);
    g_free(account->pgp_keyid);
    g_free(account->startscript);
    g_free(account->client);
    g_free(account->theme);
    g_free(account->tls_policy);
    g_free(account->auth_policy);
    g_list_free_full(account->otr_manual, g_free);
    g_list_free_full(account->otr_opportunistic, g_free);
    g_list_free_full(account->otr_always, g_free);
    g_list_free_full(account->omemo_enabled, g_free);
    g_list_free_full(account->omemo_disabled, g_free);
    g_list_free_full(account->ox_enabled, g_free);
    g_list_free_full(account->pgp_enabled, g_free);
    g_free(account);
}

void
account_set_server(ProfAccount* account, const gchar* server)
{
    g_free(account->server);
    account->server = g_strdup(server);
}

void
account_set_port(ProfAccount* account, int port)
{
    account->port = port;
}

void
account_set_tls_policy(ProfAccount* account, const gchar* tls_policy)
{
    g_free(account->tls_policy);
    account->tls_policy = g_strdup(tls_policy);
}

void
account_set_auth_policy(ProfAccount* account, const gchar* auth_policy)
{
    g_free(account->auth_policy);
    account->auth_policy = g_strdup(auth_policy);
}
