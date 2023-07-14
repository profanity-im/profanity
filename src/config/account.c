/*
 * account.c
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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

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
    ProfAccount* new_account = calloc(1, sizeof(ProfAccount));

    new_account->name = name;

    if (jid) {
        new_account->jid = jid;
    } else {
        new_account->jid = strdup(name);
    }

    new_account->password = password;

    new_account->eval_password = eval_password;

    new_account->enabled = enabled;

    new_account->server = server;

    new_account->resource = resource;

    new_account->port = port;

    if (last_presence == NULL || !valid_resource_presence_string(last_presence)) {
        new_account->last_presence = strdup("online");
        g_free(last_presence);
    } else {
        new_account->last_presence = last_presence;
    }

    if (login_presence == NULL) {
        new_account->login_presence = strdup("online");
    } else if (strcmp(login_presence, "last") == 0) {
        new_account->login_presence = login_presence;
    } else if (!valid_resource_presence_string(login_presence)) {
        new_account->login_presence = strdup("online");
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
        new_account->muc_nick = strdup(jidp->domainpart);
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

char*
account_create_connect_jid(ProfAccount* account)
{
    if (account->resource) {
        return create_fulljid(account->jid, account->resource);
    } else {
        return strdup(account->jid);
    }
}

gboolean
account_eval_password(ProfAccount* account)
{
    assert(account != NULL);
    assert(account->eval_password != NULL);

    errno = 0;

    FILE* stream = popen(account->eval_password, "r");
    if (stream == NULL) {
        const char* errmsg = strerror(errno);
        if (errmsg) {
            log_error("Could not execute `eval_password` command (%s).",
                      errmsg);
        } else {
            log_error("Failed to allocate memory for `eval_password` command.");
        }
        return FALSE;
    }

    account->password = g_malloc(READ_BUF_SIZE);
    if (!account->password) {
        log_error("Failed to allocate enough memory to read `eval_password` "
                  "output.");
        return FALSE;
    }

    account->password = fgets(account->password, READ_BUF_SIZE, stream);
    if (!account->password) {
        log_error("Failed to read password from stream.");
        return FALSE;
    }

    int exit_status = pclose(stream);
    if (exit_status > 0) {
        log_error("Command for `eval_password` returned error status (%s).",
                  exit_status);
        return FALSE;
    } else if (exit_status < 0) {
        log_error("Failed to close stream for `eval_password` command output "
                  "(%s).",
                  strerror(errno));
        return FALSE;
    };

    // Remove leading and trailing whitespace from output.
    g_strstrip(account->password);
    if (!account->password) {
        log_error("Empty password returned by `eval_password` command.");
        return FALSE;
    }

    return TRUE;
}

void
account_free(ProfAccount* account)
{
    if (account == NULL) {
        return;
    }

    free(account->name);
    free(account->jid);
    free(account->password);
    free(account->eval_password);
    free(account->resource);
    free(account->server);
    free(account->last_presence);
    free(account->login_presence);
    free(account->muc_service);
    free(account->muc_nick);
    free(account->otr_policy);
    free(account->omemo_policy);
    free(account->pgp_keyid);
    free(account->startscript);
    free(account->client);
    free(account->theme);
    free(account->tls_policy);
    free(account->auth_policy);
    g_list_free_full(account->otr_manual, g_free);
    g_list_free_full(account->otr_opportunistic, g_free);
    g_list_free_full(account->otr_always, g_free);
    g_list_free_full(account->omemo_enabled, g_free);
    g_list_free_full(account->omemo_disabled, g_free);
    g_list_free_full(account->ox_enabled, g_free);
    g_list_free_full(account->pgp_enabled, g_free);
    free(account);
}

void
account_set_server(ProfAccount* account, const char* server)
{
    free(account->server);
    account->server = strdup(server);
}

void
account_set_port(ProfAccount* account, int port)
{
    account->port = port;
}

void
account_set_tls_policy(ProfAccount* account, const char* tls_policy)
{
    free(account->tls_policy);
    account->tls_policy = strdup(tls_policy);
}

void
account_set_auth_policy(ProfAccount* account, const char* auth_policy)
{
    free(account->auth_policy);
    account->auth_policy = strdup(auth_policy);
}
