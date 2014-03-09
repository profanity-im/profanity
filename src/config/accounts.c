/*
 * accounts.c
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "accounts.h"

#include "common.h"
#include "config/account.h"
#include "jid.h"
#include "log.h"
#include "tools/autocomplete.h"
#include "xmpp/xmpp.h"

static gchar *accounts_loc;
static GKeyFile *accounts;

static Autocomplete all_ac;
static Autocomplete enabled_ac;

// used to rename account (copies properties to new account)
static gchar *string_keys[] = {
    "jid",
    "server",
    "port",
    "resource",
    "password",
    "presence.last",
    "presence.login",
    "muc.service",
    "muc.nick"
};

static void _fix_legacy_accounts(const char * const account_name);
static void _save_accounts(void);
static gchar * _get_accounts_file(void);

static void
_accounts_load(void)
{
    log_info("Loading accounts");
    all_ac = autocomplete_new();
    enabled_ac = autocomplete_new();
    accounts_loc = _get_accounts_file();

    accounts = g_key_file_new();
    g_key_file_load_from_file(accounts, accounts_loc, G_KEY_FILE_KEEP_COMMENTS,
        NULL);

    // create the logins searchable list for autocompletion
    gsize naccounts;
    gchar **account_names =
        g_key_file_get_groups(accounts, &naccounts);

    gsize i;
    for (i = 0; i < naccounts; i++) {
        autocomplete_add(all_ac, account_names[i]);
        if (g_key_file_get_boolean(accounts, account_names[i], "enabled", NULL)) {
            autocomplete_add(enabled_ac, account_names[i]);
        }

        _fix_legacy_accounts(account_names[i]);
    }

    g_strfreev(account_names);
}

static void
_accounts_close(void)
{
    autocomplete_free(all_ac);
    autocomplete_free(enabled_ac);
    g_key_file_free(accounts);
}

static char *
_accounts_find_enabled(char *prefix)
{
    return autocomplete_complete(enabled_ac, prefix);
}

static char *
_accounts_find_all(char *prefix)
{
    return autocomplete_complete(all_ac, prefix);
}

static void
_accounts_reset_all_search(void)
{
    autocomplete_reset(all_ac);
}

static void
_accounts_reset_enabled_search(void)
{
    autocomplete_reset(enabled_ac);
}

static void
_accounts_add(const char *account_name, const char *altdomain, const int port)
{
    // set account name and resource
    const char *barejid = account_name;
    const char *resource = "profanity";
    Jid *jid = jid_create(account_name);
    if (jid != NULL) {
        barejid = jid->barejid;
        if (jid->resourcepart != NULL) {
            resource = jid->resourcepart;
        }
    }

    // doesn't yet exist
    if (!g_key_file_has_group(accounts, account_name)) {
        g_key_file_set_boolean(accounts, account_name, "enabled", TRUE);
        g_key_file_set_string(accounts, account_name, "jid", barejid);
        g_key_file_set_string(accounts, account_name, "resource", resource);
        if (altdomain != NULL) {
            g_key_file_set_string(accounts, account_name, "server", altdomain);
        }
        if (port != 0) {
            g_key_file_set_integer(accounts, account_name, "port", port);
        }

        Jid *jidp = jid_create(barejid);
        GString *muc_service = g_string_new("conference.");
        g_string_append(muc_service, jidp->domainpart);
        g_key_file_set_string(accounts, account_name, "muc.service", muc_service->str);
        g_string_free(muc_service, TRUE);
        if (jidp->localpart == NULL) {
            g_key_file_set_string(accounts, account_name, "muc.nick", jidp->domainpart);
        } else {
            g_key_file_set_string(accounts, account_name, "muc.nick", jidp->localpart);
        }
        jid_destroy(jidp);

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

    jid_destroy(jid);
}

static gchar**
_accounts_get_list(void)
{
    return g_key_file_get_groups(accounts, NULL);
}

static ProfAccount*
_accounts_get_account(const char * const name)
{
    if (!g_key_file_has_group(accounts, name)) {
        return NULL;
    } else {
        gchar *jid = g_key_file_get_string(accounts, name, "jid", NULL);

        // fix accounts that have no jid property by setting to name
        if (jid == NULL) {
            g_key_file_set_string(accounts, name, "jid", name);
            _save_accounts();
        }

        gchar *password = g_key_file_get_string(accounts, name, "password", NULL);
        gboolean enabled = g_key_file_get_boolean(accounts, name, "enabled", NULL);

        gchar *server = g_key_file_get_string(accounts, name, "server", NULL);
        gchar *resource = g_key_file_get_string(accounts, name, "resource", NULL);
        int port = g_key_file_get_integer(accounts, name, "port", NULL);

        gchar *last_presence = g_key_file_get_string(accounts, name, "presence.last", NULL);
        gchar *login_presence = g_key_file_get_string(accounts, name, "presence.login", NULL);

        int priority_online = g_key_file_get_integer(accounts, name, "priority.online", NULL);
        int priority_chat = g_key_file_get_integer(accounts, name, "priority.chat", NULL);
        int priority_away = g_key_file_get_integer(accounts, name, "priority.away", NULL);
        int priority_xa = g_key_file_get_integer(accounts, name, "priority.xa", NULL);
        int priority_dnd = g_key_file_get_integer(accounts, name, "priority.dnd", NULL);

        gchar *muc_service = g_key_file_get_string(accounts, name, "muc.service", NULL);
        gchar *muc_nick = g_key_file_get_string(accounts, name, "muc.nick", NULL);

        ProfAccount *new_account = account_new(name, jid, password, enabled,
            server, port, resource, last_presence, login_presence,
            priority_online, priority_chat, priority_away, priority_xa,
            priority_dnd, muc_service, muc_nick);

        g_free(jid);
        g_free(password);
        g_free(server);
        g_free(resource);
        g_free(last_presence);
        g_free(login_presence);
        g_free(muc_service);
        g_free(muc_nick);

        return new_account;
    }
}

static gboolean
_accounts_enable(const char * const name)
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

static gboolean
_accounts_disable(const char * const name)
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

static gboolean
_accounts_rename(const char * const account_name, const char * const new_name)
{
    if (g_key_file_has_group(accounts, new_name)) {
        return FALSE;
    }

    if (!g_key_file_has_group(accounts, account_name)) {
        return FALSE;
    }

    g_key_file_set_boolean(accounts, new_name, "enabled",
        g_key_file_get_boolean(accounts, account_name, "enabled", NULL));

    g_key_file_set_integer(accounts, new_name, "priority.online",
        g_key_file_get_integer(accounts, account_name, "priority.online", NULL));
    g_key_file_set_integer(accounts, new_name, "priority.chat",
        g_key_file_get_integer(accounts, account_name, "priority.chat", NULL));
    g_key_file_set_integer(accounts, new_name, "priority.away",
        g_key_file_get_integer(accounts, account_name, "priority.away", NULL));
    g_key_file_set_integer(accounts, new_name, "priority.xa",
        g_key_file_get_integer(accounts, account_name, "priority.xa", NULL));
    g_key_file_set_integer(accounts, new_name, "priority.dnd",
        g_key_file_get_integer(accounts, account_name, "priority.dnd", NULL));

    // copy other string properties
    int i;
    for (i = 0; i < ARRAY_SIZE(string_keys); i++) {
        char *value = g_key_file_get_string(accounts, account_name, string_keys[i], NULL);
        if (value != NULL) {
            g_key_file_set_string(accounts, new_name, string_keys[i], value);
            g_free(value);
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

static gboolean
_accounts_account_exists(const char * const account_name)
{
    return g_key_file_has_group(accounts, account_name);

}

static void
_accounts_set_jid(const char * const account_name, const char * const value)
{
    Jid *jid = jid_create(value);
    if (jid != NULL) {
        if (accounts_account_exists(account_name)) {
            g_key_file_set_string(accounts, account_name, "jid", jid->barejid);
            if (jid->resourcepart != NULL) {
                g_key_file_set_string(accounts, account_name, "resource", jid->resourcepart);
            }

            GString *muc_service = g_string_new("conference.");
            g_string_append(muc_service, jid->domainpart);
            g_key_file_set_string(accounts, account_name, "muc.service", muc_service->str);
            g_string_free(muc_service, TRUE);
            if (jid->localpart == NULL) {
                g_key_file_set_string(accounts, account_name, "muc.nick", jid->domainpart);
            } else {
                g_key_file_set_string(accounts, account_name, "muc.nick", jid->localpart);
            }

            _save_accounts();
        }
    }
}

static void
_accounts_set_server(const char * const account_name, const char * const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "server", value);
        _save_accounts();
    }
}

static void
_accounts_set_port(const char * const account_name, const int value)
{
    if (value != 0) {
        g_key_file_set_integer(accounts, account_name, "port", value);
        _save_accounts();
    }
}

static void
_accounts_set_resource(const char * const account_name, const char * const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "resource", value);
        _save_accounts();
    }
}

static void
_accounts_set_password(const char * const account_name, const char * const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "password", value);
        _save_accounts();
    }
}

static void
_accounts_clear_password(const char * const account_name)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_remove_key(accounts, account_name, "password", NULL);
        _save_accounts();
    }
}

static void
_accounts_set_muc_service(const char * const account_name, const char * const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "muc.service", value);
        _save_accounts();
    }
}

static void
_accounts_set_muc_nick(const char * const account_name, const char * const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "muc.nick", value);
        _save_accounts();
    }
}

static void
_accounts_set_priority_online(const char * const account_name, const gint value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_integer(accounts, account_name, "priority.online", value);
        _save_accounts();
    }
}

static void
_accounts_set_priority_chat(const char * const account_name, const gint value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_integer(accounts, account_name, "priority.chat", value);
        _save_accounts();
    }
}

static void
_accounts_set_priority_away(const char * const account_name, const gint value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_integer(accounts, account_name, "priority.away", value);
        _save_accounts();
    }
}

static void
_accounts_set_priority_xa(const char * const account_name, const gint value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_integer(accounts, account_name, "priority.xa", value);
        _save_accounts();
    }
}

static void
_accounts_set_priority_dnd(const char * const account_name, const gint value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_integer(accounts, account_name, "priority.dnd", value);
        _save_accounts();
    }
}

static void
_accounts_set_priority_all(const char * const account_name, const gint value)
{
    if (accounts_account_exists(account_name)) {
        accounts_set_priority_online(account_name, value);
        accounts_set_priority_chat(account_name, value);
        accounts_set_priority_away(account_name, value);
        accounts_set_priority_xa(account_name, value);
        accounts_set_priority_dnd(account_name, value);
        _save_accounts();
    }
}

static gint
_accounts_get_priority_for_presence_type(const char * const account_name,
    resource_presence_t presence_type)
{
    gint result;

    switch (presence_type)
    {
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

static void
_accounts_set_last_presence(const char * const account_name, const char * const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "presence.last", value);
        _save_accounts();
    }
}

static void
_accounts_set_login_presence(const char * const account_name, const char * const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "presence.login", value);
        _save_accounts();
    }
}

static resource_presence_t
_accounts_get_last_presence(const char * const account_name)
{
    resource_presence_t result;
    gchar *setting = g_key_file_get_string(accounts, account_name, "presence.last", NULL);

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

    if (setting != NULL) {
        g_free(setting);
    }
    return result;
}

static resource_presence_t
_accounts_get_login_presence(const char * const account_name)
{
    resource_presence_t result;
    gchar *setting = g_key_file_get_string(accounts, account_name, "presence.login", NULL);

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
        log_warning("Error reading presence.login for account: '%s', value: '%s', defaulting to 'online'",
            account_name, setting);
        result = RESOURCE_ONLINE;
    }

    if (setting != NULL) {
        g_free(setting);
    }
    return result;
}

static void
_fix_legacy_accounts(const char * const account_name)
{
    // set barejid and resource
    const char *barejid = account_name;
    const char *resource = "profanity";
    Jid *jid = jid_create(account_name);
    if (jid != NULL) {
        barejid = jid->barejid;
        if (jid->resourcepart != NULL) {
            resource = jid->resourcepart;
        }
    }

    // accounts with no jid property
    if (!g_key_file_has_key(accounts, account_name, "jid", NULL)) {
        g_key_file_set_string(accounts, account_name, "jid", barejid);
        _save_accounts();
    }

    // accounts with no resource, property
    if (!g_key_file_has_key(accounts, account_name, "resource", NULL)) {
        g_key_file_set_string(accounts, account_name, "resource", resource);
        _save_accounts();
    }

    // acounts with no muc service or nick
    if (!g_key_file_has_key(accounts, account_name, "muc.service", NULL)) {
        gchar *account_jid = g_key_file_get_string(accounts, account_name, "jid", NULL);
        Jid *jidp = jid_create(account_jid);
        GString *muc_service = g_string_new("conference.");
        g_string_append(muc_service, jidp->domainpart);
        g_key_file_set_string(accounts, account_name, "muc.service", muc_service->str);
        g_string_free(muc_service, TRUE);
        jid_destroy(jidp);
    }
    if (!g_key_file_has_key(accounts, account_name, "muc.nick", NULL)) {
        gchar *account_jid = g_key_file_get_string(accounts, account_name, "jid", NULL);
        Jid *jidp = jid_create(account_jid);
        if (jidp->localpart == NULL) {
            g_key_file_set_string(accounts, account_name, "muc.nick", jidp->domainpart);
        } else {
            g_key_file_set_string(accounts, account_name, "muc.nick", jidp->localpart);
        }
        jid_destroy(jidp);
    }

    jid_destroy(jid);
}

static void
_save_accounts(void)
{
    gsize g_data_size;
    gchar *g_accounts_data = g_key_file_to_data(accounts, &g_data_size, NULL);
    g_file_set_contents(accounts_loc, g_accounts_data, g_data_size, NULL);
    g_free(g_accounts_data);
}

static gchar *
_get_accounts_file(void)
{
    gchar *xdg_data = xdg_get_data_home();
    GString *logfile = g_string_new(xdg_data);
    g_string_append(logfile, "/profanity/accounts");
    gchar *result = strdup(logfile->str);
    g_free(xdg_data);
    g_string_free(logfile, TRUE);

    return result;
}

void
accounts_init_module(void)
{
    accounts_load = _accounts_load;
    accounts_close = _accounts_close;
    accounts_find_all = _accounts_find_all;
    accounts_find_enabled = _accounts_find_enabled;
    accounts_reset_all_search = _accounts_reset_all_search;
    accounts_reset_enabled_search = _accounts_reset_enabled_search;
    accounts_add = _accounts_add;
    accounts_get_list = _accounts_get_list;
    accounts_get_account = _accounts_get_account;
    accounts_enable = _accounts_enable;
    accounts_disable = _accounts_disable;
    accounts_rename = _accounts_rename;
    accounts_account_exists = _accounts_account_exists;
    accounts_set_jid = _accounts_set_jid;
    accounts_set_server = _accounts_set_server;
    accounts_set_port = _accounts_set_port;
    accounts_set_resource = _accounts_set_resource;
    accounts_set_password = _accounts_set_password;
    accounts_set_muc_service = _accounts_set_muc_service;
    accounts_set_muc_nick = _accounts_set_muc_nick;
    accounts_set_last_presence = _accounts_set_last_presence;
    accounts_set_login_presence = _accounts_set_login_presence;
    accounts_get_last_presence = _accounts_get_last_presence;
    accounts_get_login_presence = _accounts_get_login_presence;
    accounts_set_priority_online = _accounts_set_priority_online;
    accounts_set_priority_chat = _accounts_set_priority_chat;
    accounts_set_priority_away = _accounts_set_priority_away;
    accounts_set_priority_xa = _accounts_set_priority_xa;
    accounts_set_priority_dnd = _accounts_set_priority_dnd;
    accounts_set_priority_all = _accounts_set_priority_all;
    accounts_get_priority_for_presence_type = _accounts_get_priority_for_presence_type;
    accounts_clear_password = _accounts_clear_password;
}

