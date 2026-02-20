/*
 * accounts.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
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

static gboolean
_accounts_has_group(const char* account_name)
{
    if (!account_name || !accounts)
        return FALSE;
    return g_key_file_has_group(accounts, account_name);
}

static void
_accounts_save(const char* account_name)
{
    prof_keyfile_t current;

    if (!load_data_keyfile(&current, FILE_ACCOUNTS)) {
        log_error("Could not load accounts");
        return;
    }

    gsize written = 0;
    if (_accounts_has_group(account_name)) {
        gsize naccounts;
        auto_gcharv gchar** account_names = g_key_file_get_groups(current.keyfile, &naccounts);
        for (gsize i = 0; i < naccounts; i++) {
            /* EITHER current already contains the account, OR we're at the end of
             * the list of accounts and we should add this new one.
             */
            if (g_strcmp0(account_name, account_names[i]) && (i != naccounts - 1))
                continue;
            gsize nkeys;
            auto_gcharv gchar** keys = g_key_file_get_keys(accounts_prof_keyfile.keyfile, account_name, &nkeys, NULL);
            for (gsize j = 0; j < nkeys; ++j) {
                auto_gchar gchar* new_value = g_key_file_get_value(accounts_prof_keyfile.keyfile, account_name, keys[j], NULL);
                g_key_file_set_value(current.keyfile, account_name, keys[j], new_value);
            }
            written++;
        }
    } else {
        g_key_file_remove_group(current.keyfile, account_name, NULL);
    }
    save_keyfile(&current);
    free_keyfile(&current);
}

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
    if (!load_data_keyfile(&accounts_prof_keyfile, FILE_ACCOUNTS)) {
        log_error("Could not load accounts");
    }
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

    if (_accounts_has_group(account_name)) {
        log_error("Can't add account \"%s\", it already exists.", account_name);
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

    _accounts_save(account_name);
    autocomplete_add(all_ac, account_name);
    autocomplete_add(enabled_ac, account_name);
}

gboolean
accounts_remove(const char* account_name)
{
    gboolean r = g_key_file_remove_group(accounts, account_name, NULL);
    _accounts_save(account_name);
    autocomplete_remove(all_ac, account_name);
    autocomplete_remove(enabled_ac, account_name);
    return r;
}

gchar**
accounts_get_list(void)
{
    return g_key_file_get_groups(accounts, NULL);
}

static GList*
_g_strv_to_glist(gchar** in, gsize length)
{
    if (in == NULL)
        return NULL;
    GList* out = NULL;
    for (gsize i = 0; i < length; i++) {
        out = g_list_append(out, in[i]);
    }
    g_free(in);
    return out;
}

static GList*
_accounts_get_glist(const gchar* group_name,
                    const gchar* key)
{
    gsize length = 0;
    gchar** list = g_key_file_get_string_list(accounts, group_name, key, &length, NULL);
    return _g_strv_to_glist(list, length);
}

ProfAccount*
accounts_get_account(const char* const account_name)
{
    if (!_accounts_has_group(account_name)) {
        return NULL;
    } else {
        gchar* jid = g_key_file_get_string(accounts, account_name, "jid", NULL);

        // fix accounts that have no jid property by setting to account_name
        if (jid == NULL) {
            g_key_file_set_string(accounts, account_name, "jid", account_name);
            _accounts_save(account_name);
        }

        gchar* password = g_key_file_get_string(accounts, account_name, "password", NULL);
        gchar* eval_password = g_key_file_get_string(accounts, account_name, "eval_password", NULL);
        gboolean enabled = g_key_file_get_boolean(accounts, account_name, "enabled", NULL);

        gchar* server = g_key_file_get_string(accounts, account_name, "server", NULL);
        gchar* resource = g_key_file_get_string(accounts, account_name, "resource", NULL);
        int port = g_key_file_get_integer(accounts, account_name, "port", NULL);

        gchar* last_presence = g_key_file_get_string(accounts, account_name, "presence.last", NULL);
        gchar* login_presence = g_key_file_get_string(accounts, account_name, "presence.login", NULL);

        int priority_online = g_key_file_get_integer(accounts, account_name, "priority.online", NULL);
        int priority_chat = g_key_file_get_integer(accounts, account_name, "priority.chat", NULL);
        int priority_away = g_key_file_get_integer(accounts, account_name, "priority.away", NULL);
        int priority_xa = g_key_file_get_integer(accounts, account_name, "priority.xa", NULL);
        int priority_dnd = g_key_file_get_integer(accounts, account_name, "priority.dnd", NULL);

        gchar* muc_service = NULL;
        if (g_key_file_has_key(accounts, account_name, "muc.service", NULL)) {
            muc_service = g_key_file_get_string(accounts, account_name, "muc.service", NULL);
        } else {
            jabber_conn_status_t conn_status = connection_get_status();
            if (conn_status == JABBER_CONNECTED) {
                const char* conf_jid = connection_jid_for_feature(XMPP_FEATURE_MUC);
                if (conf_jid) {
                    muc_service = g_strdup(conf_jid);
                }
            }
        }
        gchar* muc_nick = g_key_file_get_string(accounts, account_name, "muc.nick", NULL);

        gchar* otr_policy = g_key_file_get_string(accounts, account_name, "otr.policy", NULL);
        GList* otr_manual = _accounts_get_glist(account_name, "otr.manual");
        GList* otr_opportunistic = _accounts_get_glist(account_name, "otr.opportunistic");
        GList* otr_always = _accounts_get_glist(account_name, "otr.always");

        gchar* omemo_policy = g_key_file_get_string(accounts, account_name, "omemo.policy", NULL);
        GList* omemo_enabled = _accounts_get_glist(account_name, "omemo.enabled");
        GList* omemo_disabled = _accounts_get_glist(account_name, "omemo.disabled");

        GList* ox_enabled = _accounts_get_glist(account_name, "ox.enabled");

        GList* pgp_enabled = _accounts_get_glist(account_name, "pgp.enabled");

        gchar* pgp_keyid = g_key_file_get_string(accounts, account_name, "pgp.keyid", NULL);

        gchar* startscript = g_key_file_get_string(accounts, account_name, "script.start", NULL);

        gchar* client = g_key_file_get_string(accounts, account_name, "client.account_name", NULL);

        gchar* theme = g_key_file_get_string(accounts, account_name, "theme", NULL);

        gchar* tls_policy = g_key_file_get_string(accounts, account_name, "tls.policy", NULL);
        if (tls_policy && !valid_tls_policy_option(tls_policy)) {
            g_free(tls_policy);
            tls_policy = NULL;
        }

        gchar* auth_policy = g_key_file_get_string(accounts, account_name, "auth.policy", NULL);

        int max_sessions = g_key_file_get_integer(accounts, account_name, "max.sessions", 0);

        return account_new(g_strdup(account_name), jid, password, eval_password, enabled,
                           server, port, resource, last_presence, login_presence,
                           priority_online, priority_chat, priority_away, priority_xa,
                           priority_dnd, muc_service, muc_nick, otr_policy, otr_manual,
                           otr_opportunistic, otr_always, omemo_policy, omemo_enabled,
                           omemo_disabled, ox_enabled, pgp_enabled, pgp_keyid,
                           startscript, theme, tls_policy, auth_policy, client, max_sessions);
    }
}

gboolean
accounts_enable(const char* const account_name)
{
    if (_accounts_has_group(account_name)) {
        g_key_file_set_boolean(accounts, account_name, "enabled", TRUE);
        _accounts_save(account_name);
        autocomplete_add(enabled_ac, account_name);
        return TRUE;
    } else {
        return FALSE;
    }
}

gboolean
accounts_disable(const char* const account_name)
{
    if (_accounts_has_group(account_name)) {
        g_key_file_set_boolean(accounts, account_name, "enabled", FALSE);
        _accounts_save(account_name);
        autocomplete_remove(enabled_ac, account_name);
        return TRUE;
    } else {
        return FALSE;
    }
}

gboolean
accounts_rename(const char* const account_name, const char* const new_name)
{
    if (_accounts_has_group(new_name)) {
        return FALSE;
    }

    if (!_accounts_has_group(account_name)) {
        return FALSE;
    }

    gsize nkeys;
    auto_gcharv gchar** keys = g_key_file_get_keys(accounts, account_name, &nkeys, NULL);
    for (gsize i = 0; i < nkeys; i++) {
        auto_gchar gchar* new_value = g_key_file_get_value(accounts, account_name, keys[i], NULL);
        g_key_file_set_value(accounts, new_name, keys[i], new_value);
    }

    g_key_file_remove_group(accounts, account_name, NULL);
    _accounts_save(account_name);
    _accounts_save(new_name);

    autocomplete_remove(all_ac, account_name);
    autocomplete_remove(enabled_ac, account_name);
    autocomplete_add(all_ac, new_name);
    if (g_key_file_get_boolean(accounts, new_name, "enabled", NULL)) {
        autocomplete_add(enabled_ac, new_name);
    }

    return TRUE;
}

gboolean
accounts_account_exists(const char* const account_name)
{
    return _accounts_has_group(account_name);
}

void
accounts_set_jid(const char* const account_name, const char* const value)
{
    auto_jid Jid* jid = jid_create(value);
    if (jid) {
        if (_accounts_has_group(account_name)) {
            g_key_file_set_string(accounts, account_name, "jid", jid->barejid);
            if (jid->resourcepart) {
                g_key_file_set_string(accounts, account_name, "resource", jid->resourcepart);
            }

            if (jid->localpart == NULL) {
                g_key_file_set_string(accounts, account_name, "muc.nick", jid->domainpart);
            } else {
                g_key_file_set_string(accounts, account_name, "muc.nick", jid->localpart);
            }

            _accounts_save(account_name);
        }
    }
}

void
accounts_set_server(const char* const account_name, const char* const value)
{
    if (_accounts_has_group(account_name)) {
        g_key_file_set_string(accounts, account_name, "server", value);
        _accounts_save(account_name);
    }
}

void
accounts_set_port(const char* const account_name, const int value)
{
    if (value != 0) {
        g_key_file_set_integer(accounts, account_name, "port", value);
        _accounts_save(account_name);
    }
}

static void
_accounts_set_string_option(const char* account_name, const char* const option, const char* const value)
{
    if (_accounts_has_group(account_name)) {
        g_key_file_set_string(accounts, account_name, option, value ?: "");
        _accounts_save(account_name);
    }
}

static void
_accounts_set_int_option(const char* account_name, const char* const option, int value)
{
    if (_accounts_has_group(account_name)) {
        g_key_file_set_integer(accounts, account_name, option, value);
        _accounts_save(account_name);
    }
}

static void
_accounts_clear_string_option(const char* account_name, const char* const option)
{
    if (_accounts_has_group(account_name)) {
        g_key_file_remove_key(accounts, account_name, option, NULL);
        _accounts_save(account_name);
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
    if (_accounts_has_group(account_name)) {
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

        _accounts_save(account_name);
    }
}

void
accounts_add_omemo_state(const char* const account_name, const char* const contact_jid, gboolean enabled)
{
    if (_accounts_has_group(account_name)) {
        if (enabled) {
            conf_string_list_add(accounts, account_name, "omemo.enabled", contact_jid);
            conf_string_list_remove(accounts, account_name, "omemo.disabled", contact_jid);
        } else {
            conf_string_list_add(accounts, account_name, "omemo.disabled", contact_jid);
            conf_string_list_remove(accounts, account_name, "omemo.enabled", contact_jid);
        }

        _accounts_save(account_name);
    }
}

void
accounts_add_ox_state(const char* const account_name, const char* const contact_jid, gboolean enabled)
{
    if (_accounts_has_group(account_name)) {
        if (enabled) {
            conf_string_list_add(accounts, account_name, "ox.enabled", contact_jid);
        } else {
            conf_string_list_remove(accounts, account_name, "ox.enabled", contact_jid);
        }

        _accounts_save(account_name);
    }
}

void
accounts_add_pgp_state(const char* const account_name, const char* const contact_jid, gboolean enabled)
{
    if (_accounts_has_group(account_name)) {
        if (enabled) {
            conf_string_list_add(accounts, account_name, "pgp.enabled", contact_jid);
            conf_string_list_remove(accounts, account_name, "pgp.disabled", contact_jid);
        } else {
            conf_string_list_add(accounts, account_name, "pgp.disabled", contact_jid);
            conf_string_list_remove(accounts, account_name, "pgp.enabled", contact_jid);
        }

        _accounts_save(account_name);
    }
}

void
accounts_clear_omemo_state(const char* const account_name, const char* const contact_jid)
{
    if (_accounts_has_group(account_name)) {
        conf_string_list_remove(accounts, account_name, "omemo.enabled", contact_jid);
        conf_string_list_remove(accounts, account_name, "omemo.disabled", contact_jid);
        _accounts_save(account_name);
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
    if (_accounts_has_group(account_name)) {
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
    if (_accounts_has_group(account_name)) {
        GDateTime* nowdt = g_date_time_new_now_utc();
        GTimeVal nowtv;
        gboolean res = g_date_time_to_timeval(nowdt, &nowtv);
        g_date_time_unref(nowdt);

        if (res) {
            auto_char char* timestr = g_time_val_to_iso8601(&nowtv);
            g_key_file_set_string(accounts, account_name, "last.activity", timestr);
            _accounts_save(account_name);
        }
    }
}

gchar*
accounts_get_last_activity(const char* const account_name)
{
    if (_accounts_has_group(account_name)) {
        return g_key_file_get_string(accounts, account_name, "last.activity", NULL);
    } else {
        return NULL;
    }
}

gchar*
accounts_get_resource(const char* const account_name)
{
    if (!_accounts_has_group(account_name)) {
        return NULL;
    }
    return g_key_file_get_string(accounts, account_name, "resource", NULL);
}

int
accounts_get_max_sessions(const char* const account_name)
{
    if (!_accounts_has_group(account_name)) {
        return 0;
    }
    return g_key_file_get_integer(accounts, account_name, "max.sessions", 0);
}

void
accounts_set_login_presence(const char* const account_name, const char* const value)
{
    if (_accounts_has_group(account_name)) {
        g_key_file_set_string(accounts, account_name, "presence.login", value);
        _accounts_save(account_name);
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
