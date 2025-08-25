/*
 * accounts.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2025 Michael Vetter <jubalh@iodoru.org>
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

#include <glib.h>
#include <glib/gstdio.h>

#include "accounts.h"

#include "common.h"
#include "log.h"
#include "config/files.h"
#include "config/account.h"
#include "config/conflists.h"
#include "tools/autocomplete.h"
#include "xmpp/xmpp.h"
#include "xmpp/jid.h"

static prof_keyfile_t accounts_prof_keyfile;
static GKeyFile* accounts;

static Autocomplete all_ac;
static Autocomplete enabled_ac;

static void _save_accounts(void);

static void
_accounts_close(void)
{
    autocomplete_free(all_ac);
    autocomplete_free(enabled_ac);
    free_keyfile(&accounts_prof_keyfile);
    accounts = NULL;
}

void
accounts_load(void)
{
    log_info("Loading accounts");

    prof_add_shutdown_routine(_accounts_close);

    all_ac = autocomplete_new();
    enabled_ac = autocomplete_new();
    load_data_keyfile(&accounts_prof_keyfile, FILE_ACCOUNTS);
    accounts = accounts_prof_keyfile.keyfile;

    // create the logins searchable list for autocompletion
    gsize naccounts;
    auto_gcharv gchar** account_names = g_key_file_get_groups(accounts, &naccounts);

    for (gsize i = 0; i < naccounts; i++) {
        autocomplete_add(all_ac, account_names[i]);
        if (g_key_file_get_boolean(accounts, account_names[i], "enabled", NULL)) {
            autocomplete_add(enabled_ac, account_names[i]);
        }
    }
}

char*
accounts_find_enabled(const char* const prefix, gboolean previous, void* context)
{
    return autocomplete_complete(enabled_ac, prefix, TRUE, previous);
}

char*
accounts_find_all(const char* const prefix, gboolean previous, void* context)
{
    return autocomplete_complete(all_ac, prefix, TRUE, previous);
}

void
accounts_reset_all_search(void)
{
    autocomplete_reset(all_ac);
}

void
accounts_reset_enabled_search(void)
{
    autocomplete_reset(enabled_ac);
}

void
accounts_add(const char* account_name, const char* altdomain, const int port, const char* const tls_policy, const char* const auth_policy)
{
    // set account name and resource
    const char* barejid = account_name;
    auto_gchar gchar* resource = jid_random_resource();
    auto_jid Jid* jid = jid_create(account_name);
    if (jid) {
        barejid = jid->barejid;
        if (jid->resourcepart) {
            resource = g_strdup(jid->resourcepart);
        }
    }

    if (g_key_file_has_group(accounts, account_name)) {
        return;
    }

    g_key_file_set_boolean(accounts, account_name, "enabled", TRUE);
    g_key_file_set_string(accounts, account_name, "jid", barejid);
    g_key_file_set_string(accounts, account_name, "resource", resource);
    if (altdomain) {
        g_key_file_set_string(accounts, account_name, "server", altdomain);
    }
    if (port != 0) {
        g_key_file_set_integer(accounts, account_name, "port", port);
    }
    if (tls_policy) {
        g_key_file_set_string(accounts, account_name, "tls.policy", tls_policy);
    }
    if (auth_policy) {
        g_key_file_set_string(accounts, account_name, "auth.policy", auth_policy);
    }

    auto_jid Jid* jidp = jid_create(barejid);

    if (jidp->localpart == NULL) {
        g_key_file_set_string(accounts, account_name, "muc.nick", jidp->domainpart);
    } else {
        g_key_file_set_string(accounts, account_name, "muc.nick", jidp->localpart);
    }

    g_key_file_set_string(accounts, account_name, "presence.last", "online");
    g_key_file_set_string(accounts, account_name, "presence.login", "online");
    g_key_file_set_integer(accounts, account_name, "priority.online", 0);
    g_key_file_set_integer(accounts, account_name, "priority.chat", 0);
    g_key_file_set_integer(accounts, account_name, "priority.away", 0);
    g_key_file_set_integer(accounts, account_name, "priority.xa", 0);
    g_key_file_set_integer(accounts, account_name, "priority.dnd", 0);

    _save_accounts();
    autocomplete_add(all_ac, account_name);
    autocomplete_add(enabled_ac, account_name);
}

int
accounts_remove(const char* account_name)
{
    int r = g_key_file_remove_group(accounts, account_name, NULL);
    _save_accounts();
    autocomplete_remove(all_ac, account_name);
    autocomplete_remove(enabled_ac, account_name);
    return r;
}

gchar**
accounts_get_list(void)
{
    return g_key_file_get_groups(accounts, NULL);
}

ProfAccount*
accounts_get_account(const char* const name)
{
    if (!g_key_file_has_group(accounts, name)) {
        return NULL;
    } else {
        gchar* jid = g_key_file_get_string(accounts, name, "jid", NULL);

        // fix accounts that have no jid property by setting to name
        if (jid == NULL) {
            g_key_file_set_string(accounts, name, "jid", name);
            _save_accounts();
        }

        gchar* password = g_key_file_get_string(accounts, name, "password", NULL);
        gchar* eval_password = g_key_file_get_string(accounts, name, "eval_password", NULL);
        gboolean enabled = g_key_file_get_boolean(accounts, name, "enabled", NULL);

        gchar* server = g_key_file_get_string(accounts, name, "server", NULL);
        gchar* resource = g_key_file_get_string(accounts, name, "resource", NULL);
        int port = g_key_file_get_integer(accounts, name, "port", NULL);

        gchar* last_presence = g_key_file_get_string(accounts, name, "presence.last", NULL);
        gchar* login_presence = g_key_file_get_string(accounts, name, "presence.login", NULL);

        int priority_online = g_key_file_get_integer(accounts, name, "priority.online", NULL);
        int priority_chat = g_key_file_get_integer(accounts, name, "priority.chat", NULL);
        int priority_away = g_key_file_get_integer(accounts, name, "priority.away", NULL);
        int priority_xa = g_key_file_get_integer(accounts, name, "priority.xa", NULL);
        int priority_dnd = g_key_file_get_integer(accounts, name, "priority.dnd", NULL);

        gchar* muc_service = NULL;
        if (g_key_file_has_key(accounts, name, "muc.service", NULL)) {
            muc_service = g_key_file_get_string(accounts, name, "muc.service", NULL);
        } else {
            jabber_conn_status_t conn_status = connection_get_status();
            if (conn_status == JABBER_CONNECTED) {
                const char* conf_jid = connection_jid_for_feature(XMPP_FEATURE_MUC);
                if (conf_jid) {
                    muc_service = strdup(conf_jid);
                }
            }
        }
        gchar* muc_nick = g_key_file_get_string(accounts, name, "muc.nick", NULL);

        gchar* otr_policy = NULL;
        if (g_key_file_has_key(accounts, name, "otr.policy", NULL)) {
            otr_policy = g_key_file_get_string(accounts, name, "otr.policy", NULL);
        }

        gsize length;
        GList* otr_manual = NULL;
        auto_gcharv gchar** manual = g_key_file_get_string_list(accounts, name, "otr.manual", &length, NULL);
        if (manual) {
            for (int i = 0; i < length; i++) {
                otr_manual = g_list_append(otr_manual, strdup(manual[i]));
            }
        }

        GList* otr_opportunistic = NULL;
        auto_gcharv gchar** opportunistic = g_key_file_get_string_list(accounts, name, "otr.opportunistic", &length, NULL);
        if (opportunistic) {
            for (int i = 0; i < length; i++) {
                otr_opportunistic = g_list_append(otr_opportunistic, strdup(opportunistic[i]));
            }
        }

        GList* otr_always = NULL;
        auto_gcharv gchar** always = g_key_file_get_string_list(accounts, name, "otr.always", &length, NULL);
        if (always) {
            for (int i = 0; i < length; i++) {
                otr_always = g_list_append(otr_always, strdup(always[i]));
            }
        }

        gchar* omemo_policy = NULL;
        if (g_key_file_has_key(accounts, name, "omemo.policy", NULL)) {
            omemo_policy = g_key_file_get_string(accounts, name, "omemo.policy", NULL);
        }

        GList* omemo_enabled = NULL;
        auto_gcharv gchar** omemo_enabled_list = g_key_file_get_string_list(accounts, name, "omemo.enabled", &length, NULL);
        if (omemo_enabled_list) {
            for (int i = 0; i < length; i++) {
                omemo_enabled = g_list_append(omemo_enabled, strdup(omemo_enabled_list[i]));
            }
        }

        GList* omemo_disabled = NULL;
        auto_gcharv gchar** omemo_disabled_list = g_key_file_get_string_list(accounts, name, "omemo.disabled", &length, NULL);
        if (omemo_disabled_list) {
            for (int i = 0; i < length; i++) {
                omemo_disabled = g_list_append(omemo_disabled, strdup(omemo_disabled_list[i]));
            }
        }

        GList* ox_enabled = NULL;
        auto_gcharv gchar** ox_enabled_list = g_key_file_get_string_list(accounts, name, "ox.enabled", &length, NULL);
        if (ox_enabled_list) {
            for (int i = 0; i < length; i++) {
                ox_enabled = g_list_append(ox_enabled, strdup(ox_enabled_list[i]));
            }
        }

        GList* pgp_enabled = NULL;
        auto_gcharv gchar** pgp_enabled_list = g_key_file_get_string_list(accounts, name, "pgp.enabled", &length, NULL);
        if (pgp_enabled_list) {
            for (int i = 0; i < length; i++) {
                pgp_enabled = g_list_append(pgp_enabled, strdup(pgp_enabled_list[i]));
            }
        }

        gchar* pgp_keyid = NULL;
        if (g_key_file_has_key(accounts, name, "pgp.keyid", NULL)) {
            pgp_keyid = g_key_file_get_string(accounts, name, "pgp.keyid", NULL);
        }

        gchar* startscript = NULL;
        if (g_key_file_has_key(accounts, name, "script.start", NULL)) {
            startscript = g_key_file_get_string(accounts, name, "script.start", NULL);
        }

        gchar* client = NULL;
        if (g_key_file_has_key(accounts, name, "client.name", NULL)) {
            client = g_key_file_get_string(accounts, name, "client.name", NULL);
        }

        gchar* theme = NULL;
        if (g_key_file_has_key(accounts, name, "theme", NULL)) {
            theme = g_key_file_get_string(accounts, name, "theme", NULL);
        }

        gchar* tls_policy = g_key_file_get_string(accounts, name, "tls.policy", NULL);
        if (tls_policy && ((g_strcmp0(tls_policy, "force") != 0) && (g_strcmp0(tls_policy, "allow") != 0) && (g_strcmp0(tls_policy, "trust") != 0) && (g_strcmp0(tls_policy, "disable") != 0) && (g_strcmp0(tls_policy, "legacy") != 0))) {
            g_free(tls_policy);
            tls_policy = NULL;
        }

        gchar* auth_policy = g_key_file_get_string(accounts, name, "auth.policy", NULL);

        int max_sessions = g_key_file_get_integer(accounts, name, "max.sessions", 0);

        ProfAccount* new_account = account_new(g_strdup(name), jid, password, eval_password, enabled,
                                               server, port, resource, last_presence, login_presence,
                                               priority_online, priority_chat, priority_away, priority_xa,
                                               priority_dnd, muc_service, muc_nick, otr_policy, otr_manual,
                                               otr_opportunistic, otr_always, omemo_policy, omemo_enabled,
                                               omemo_disabled, ox_enabled, pgp_enabled, pgp_keyid,
                                               startscript, theme, tls_policy, auth_policy, client, max_sessions);

        return new_account;
    }
}

gboolean
accounts_enable(const char* const name)
{
    if (g_key_file_has_group(accounts, name)) {
        g_key_file_set_boolean(accounts, name, "enabled", TRUE);
        _save_accounts();
        autocomplete_add(enabled_ac, name);
        return TRUE;
    } else {
        return FALSE;
    }
}

gboolean
accounts_disable(const char* const name)
{
    if (g_key_file_has_group(accounts, name)) {
        g_key_file_set_boolean(accounts, name, "enabled", FALSE);
        _save_accounts();
        autocomplete_remove(enabled_ac, name);
        return TRUE;
    } else {
        return FALSE;
    }
}

gboolean
accounts_rename(const char* const account_name, const char* const new_name)
{
    if (g_key_file_has_group(accounts, new_name)) {
        return FALSE;
    }

    if (!g_key_file_has_group(accounts, account_name)) {
        return FALSE;
    }

    // treat all properties as strings for copy
    gchar* string_keys[] = {
        "enabled",
        "jid",
        "server",
        "port",
        "resource",
        "password",
        "eval_password",
        "presence.last",
        "presence.laststatus",
        "presence.login",
        "priority.online",
        "priority.chat",
        "priority.away",
        "priority.xa",
        "priority.dnd",
        "muc.service",
        "muc.nick",
        "otr.policy",
        "otr.manual",
        "otr.opportunistic",
        "otr.always",
        "omemo.policy",
        "omemo.enabled",
        "omemo.disabled",
        "ox.enabled",
        "pgp.enabled",
        "pgp.keyid",
        "last.activity",
        "script.start",
        "tls.policy"
    };

    for (int i = 0; i < ARRAY_SIZE(string_keys); i++) {
        auto_gchar gchar* value = g_key_file_get_string(accounts, account_name, string_keys[i], NULL);
        if (value) {
            g_key_file_set_string(accounts, new_name, string_keys[i], value);
        }
    }

    g_key_file_remove_group(accounts, account_name, NULL);
    _save_accounts();

    autocomplete_remove(all_ac, account_name);
    autocomplete_add(all_ac, new_name);
    if (g_key_file_get_boolean(accounts, new_name, "enabled", NULL)) {
        autocomplete_remove(enabled_ac, account_name);
        autocomplete_add(enabled_ac, new_name);
    }

    return TRUE;
}

gboolean
accounts_account_exists(const char* const account_name)
{
    return g_key_file_has_group(accounts, account_name);
}

void
accounts_set_jid(const char* const account_name, const char* const value)
{
    auto_jid Jid* jid = jid_create(value);
    if (jid) {
        if (accounts_account_exists(account_name)) {
            g_key_file_set_string(accounts, account_name, "jid", jid->barejid);
            if (jid->resourcepart) {
                g_key_file_set_string(accounts, account_name, "resource", jid->resourcepart);
            }

            if (jid->localpart == NULL) {
                g_key_file_set_string(accounts, account_name, "muc.nick", jid->domainpart);
            } else {
                g_key_file_set_string(accounts, account_name, "muc.nick", jid->localpart);
            }

            _save_accounts();
        }
    }
}

void
accounts_set_server(const char* const account_name, const char* const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "server", value);
        _save_accounts();
    }
}

void
accounts_set_port(const char* const account_name, const int value)
{
    if (value != 0) {
        g_key_file_set_integer(accounts, account_name, "port", value);
        _save_accounts();
    }
}

static void
_accounts_set_string_option(const char* account_name, const char* const option, const char* const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, option, value ?: "");
        _save_accounts();
    }
}

static void
_accounts_set_int_option(const char* account_name, const char* const option, int value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_integer(accounts, account_name, option, value);
        _save_accounts();
    }
}

static void
_accounts_clear_string_option(const char* account_name, const char* const option)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_remove_key(accounts, account_name, option, NULL);
        _save_accounts();
    }
}

void
accounts_set_resource(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "resource", value);
}

void
accounts_set_password(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "password", value);
}

void
accounts_set_eval_password(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "eval_password", value);
}

void
accounts_set_pgp_keyid(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "pgp.keyid", value);
}

void
accounts_set_script_start(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "script.start", value);
}

void
accounts_set_client(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "client.name", value);
}

void
accounts_set_theme(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "theme", value);
}

void
accounts_set_max_sessions(const char* const account_name, const int value)
{
    _accounts_set_int_option(account_name, "max.sessions", value);
}

void
accounts_clear_password(const char* const account_name)
{
    _accounts_clear_string_option(account_name, "password");
}

void
accounts_clear_eval_password(const char* const account_name)
{
    _accounts_clear_string_option(account_name, "eval_password");
}

void
accounts_clear_server(const char* const account_name)
{
    _accounts_clear_string_option(account_name, "server");
}

void
accounts_clear_port(const char* const account_name)
{
    _accounts_clear_string_option(account_name, "port");
}

void
accounts_clear_pgp_keyid(const char* const account_name)
{
    _accounts_clear_string_option(account_name, "pgp.keyid");
}

void
accounts_clear_script_start(const char* const account_name)
{
    _accounts_clear_string_option(account_name, "script.start");
}

void
accounts_clear_client(const char* const account_name)
{
    _accounts_clear_string_option(account_name, "client.name");
}

void
accounts_clear_theme(const char* const account_name)
{
    _accounts_clear_string_option(account_name, "theme");
}

void
accounts_clear_muc(const char* const account_name)
{
    _accounts_clear_string_option(account_name, "muc.service");
}

void
accounts_clear_resource(const char* const account_name)
{
    _accounts_clear_string_option(account_name, "resource");
}

void
accounts_clear_otr(const char* const account_name)
{
    _accounts_clear_string_option(account_name, "otr.policy");
}

void
accounts_clear_max_sessions(const char* const account_name)
{
    _accounts_clear_string_option(account_name, "max.sessions");
}

void
accounts_add_otr_policy(const char* const account_name, const char* const contact_jid, const char* const policy)
{
    if (accounts_account_exists(account_name)) {
        GString* key = g_string_new("otr.");
        g_string_append(key, policy);
        conf_string_list_add(accounts, account_name, key->str, contact_jid);
        g_string_free(key, TRUE);

        // check for and remove from other lists
        if (strcmp(policy, "manual") == 0) {
            conf_string_list_remove(accounts, account_name, "otr.opportunistic", contact_jid);
            conf_string_list_remove(accounts, account_name, "otr.always", contact_jid);
        }
        if (strcmp(policy, "opportunistic") == 0) {
            conf_string_list_remove(accounts, account_name, "otr.manual", contact_jid);
            conf_string_list_remove(accounts, account_name, "otr.always", contact_jid);
        }
        if (strcmp(policy, "always") == 0) {
            conf_string_list_remove(accounts, account_name, "otr.opportunistic", contact_jid);
            conf_string_list_remove(accounts, account_name, "otr.manual", contact_jid);
        }

        _save_accounts();
    }
}

void
accounts_add_omemo_state(const char* const account_name, const char* const contact_jid, gboolean enabled)
{
    if (accounts_account_exists(account_name)) {
        if (enabled) {
            conf_string_list_add(accounts, account_name, "omemo.enabled", contact_jid);
            conf_string_list_remove(accounts, account_name, "omemo.disabled", contact_jid);
        } else {
            conf_string_list_add(accounts, account_name, "omemo.disabled", contact_jid);
            conf_string_list_remove(accounts, account_name, "omemo.enabled", contact_jid);
        }

        _save_accounts();
    }
}

void
accounts_add_ox_state(const char* const account_name, const char* const contact_jid, gboolean enabled)
{
    if (accounts_account_exists(account_name)) {
        if (enabled) {
            conf_string_list_add(accounts, account_name, "ox.enabled", contact_jid);
        } else {
            conf_string_list_remove(accounts, account_name, "ox.enabled", contact_jid);
        }

        _save_accounts();
    }
}

void
accounts_add_pgp_state(const char* const account_name, const char* const contact_jid, gboolean enabled)
{
    if (accounts_account_exists(account_name)) {
        if (enabled) {
            conf_string_list_add(accounts, account_name, "pgp.enabled", contact_jid);
            conf_string_list_remove(accounts, account_name, "pgp.disabled", contact_jid);
        } else {
            conf_string_list_add(accounts, account_name, "pgp.disabled", contact_jid);
            conf_string_list_remove(accounts, account_name, "pgp.enabled", contact_jid);
        }

        _save_accounts();
    }
}

void
accounts_clear_omemo_state(const char* const account_name, const char* const contact_jid)
{
    if (accounts_account_exists(account_name)) {
        conf_string_list_remove(accounts, account_name, "omemo.enabled", contact_jid);
        conf_string_list_remove(accounts, account_name, "omemo.disabled", contact_jid);
        _save_accounts();
    }
}

void
accounts_set_muc_service(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "muc.service", value);
}

void
accounts_set_muc_nick(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "muc.nick", value);
}

void
accounts_set_otr_policy(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "otr.policy", value);
}

void
accounts_set_omemo_policy(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "omemo.policy", value);
}

void
accounts_set_tls_policy(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "tls.policy", value);
}

void
accounts_set_auth_policy(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "auth.policy", value);
}

void
accounts_set_priority_online(const char* const account_name, const gint value)
{
    _accounts_set_int_option(account_name, "priority.online", value);
}

void
accounts_set_priority_chat(const char* const account_name, const gint value)
{
    _accounts_set_int_option(account_name, "priority.chat", value);
}

void
accounts_set_priority_away(const char* const account_name, const gint value)
{
    _accounts_set_int_option(account_name, "priority.away", value);
}

void
accounts_set_priority_xa(const char* const account_name, const gint value)
{
    _accounts_set_int_option(account_name, "priority.xa", value);
}

void
accounts_set_priority_dnd(const char* const account_name, const gint value)
{
    _accounts_set_int_option(account_name, "priority.dnd", value);
}

void
accounts_set_priority_all(const char* const account_name, const gint value)
{
    if (accounts_account_exists(account_name)) {
        accounts_set_priority_online(account_name, value);
        accounts_set_priority_chat(account_name, value);
        accounts_set_priority_away(account_name, value);
        accounts_set_priority_xa(account_name, value);
        accounts_set_priority_dnd(account_name, value);
    }
}

gint
accounts_get_priority_for_presence_type(const char* const account_name,
                                        resource_presence_t presence_type)
{
    gint result;

    switch (presence_type) {
    case (RESOURCE_ONLINE):
        result = g_key_file_get_integer(accounts, account_name, "priority.online", NULL);
        break;
    case (RESOURCE_CHAT):
        result = g_key_file_get_integer(accounts, account_name, "priority.chat", NULL);
        break;
    case (RESOURCE_AWAY):
        result = g_key_file_get_integer(accounts, account_name, "priority.away", NULL);
        break;
    case (RESOURCE_XA):
        result = g_key_file_get_integer(accounts, account_name, "priority.xa", NULL);
        break;
    default:
        result = g_key_file_get_integer(accounts, account_name, "priority.dnd", NULL);
        break;
    }

    if (result < JABBER_PRIORITY_MIN || result > JABBER_PRIORITY_MAX)
        result = 0;

    return result;
}

void
accounts_set_last_presence(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "presence.last", value);
}

void
accounts_set_last_status(const char* const account_name, const char* const value)
{
    _accounts_set_string_option(account_name, "presence.laststatus", value);
}

void
accounts_set_last_activity(const char* const account_name)
{
    if (accounts_account_exists(account_name)) {
        GDateTime* nowdt = g_date_time_new_now_utc();
        GTimeVal nowtv;
        gboolean res = g_date_time_to_timeval(nowdt, &nowtv);
        g_date_time_unref(nowdt);

        if (res) {
            auto_char char* timestr = g_time_val_to_iso8601(&nowtv);
            g_key_file_set_string(accounts, account_name, "last.activity", timestr);
            _save_accounts();
        }
    }
}

gchar*
accounts_get_last_activity(const char* const account_name)
{
    if (accounts_account_exists(account_name)) {
        return g_key_file_get_string(accounts, account_name, "last.activity", NULL);
    } else {
        return NULL;
    }
}

gchar*
accounts_get_resource(const char* const account_name)
{
    if (!accounts_account_exists(account_name)) {
        return NULL;
    }
    return g_key_file_get_string(accounts, account_name, "resource", NULL);
}

int
accounts_get_max_sessions(const char* const account_name)
{
    if (!accounts_account_exists(account_name)) {
        return 0;
    }
    return g_key_file_get_integer(accounts, account_name, "max.sessions", 0);
}

void
accounts_set_login_presence(const char* const account_name, const char* const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "presence.login", value);
        _save_accounts();
    }
}

resource_presence_t
accounts_get_last_presence(const char* const account_name)
{
    resource_presence_t result;
    auto_gchar gchar* setting = g_key_file_get_string(accounts, account_name, "presence.last", NULL);

    if (setting == NULL || (strcmp(setting, "online") == 0)) {
        result = RESOURCE_ONLINE;
    } else if (strcmp(setting, "chat") == 0) {
        result = RESOURCE_CHAT;
    } else if (strcmp(setting, "away") == 0) {
        result = RESOURCE_AWAY;
    } else if (strcmp(setting, "xa") == 0) {
        result = RESOURCE_XA;
    } else if (strcmp(setting, "dnd") == 0) {
        result = RESOURCE_DND;
    } else {
        log_warning("Error reading presence.last for account: '%s', value: '%s', defaulting to 'online'",
                    account_name, setting);
        result = RESOURCE_ONLINE;
    }

    return result;
}

char*
accounts_get_last_status(const char* const account_name)
{
    return g_key_file_get_string(accounts, account_name, "presence.laststatus", NULL);
}

resource_presence_t
accounts_get_login_presence(const char* const account_name)
{
    resource_presence_t result;
    auto_gchar gchar* setting = g_key_file_get_string(accounts, account_name, "presence.login", NULL);

    if (setting == NULL || (strcmp(setting, "online") == 0)) {
        result = RESOURCE_ONLINE;
    } else if (strcmp(setting, "chat") == 0) {
        result = RESOURCE_CHAT;
    } else if (strcmp(setting, "away") == 0) {
        result = RESOURCE_AWAY;
    } else if (strcmp(setting, "xa") == 0) {
        result = RESOURCE_XA;
    } else if (strcmp(setting, "dnd") == 0) {
        result = RESOURCE_DND;
    } else if (strcmp(setting, "last") == 0) {
        result = accounts_get_last_presence(account_name);
    } else {
        log_warning("Error reading presence.login for account: '%s', value: '%s', defaulting to 'online'", account_name, setting);
        result = RESOURCE_ONLINE;
    }

    return result;
}

char*
accounts_get_login_status(const char* const account_name)
{
    auto_gchar gchar* setting = g_key_file_get_string(accounts, account_name, "presence.login", NULL);
    gchar* status = NULL;

    if (g_strcmp0(setting, "last") == 0) {
        status = accounts_get_last_status(account_name);
    }

    return status;
}

static void
_save_accounts(void)
{
    save_keyfile(&accounts_prof_keyfile);
}
