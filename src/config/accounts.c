/*
 * accounts.c
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "accounts.h"

#include "common.h"
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

void
accounts_load(void)
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
        autocomplete_add(all_ac, strdup(account_names[i]));
        if (g_key_file_get_boolean(accounts, account_names[i], "enabled", NULL)) {
            autocomplete_add(enabled_ac, strdup(account_names[i]));
        }

        _fix_legacy_accounts(account_names[i]);
    }

    for (i = 0; i < naccounts; i++) {
        free(account_names[i]);
    }
    free(account_names);
}

void
accounts_close(void)
{
    autocomplete_free(all_ac);
    autocomplete_free(enabled_ac);
    g_key_file_free(accounts);
}

char *
accounts_find_enabled(char *prefix)
{
    return autocomplete_complete(enabled_ac, prefix);
}

char *
accounts_find_all(char *prefix)
{
    return autocomplete_complete(all_ac, prefix);
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
accounts_add(const char *account_name, const char *altdomain)
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
        autocomplete_add(all_ac, strdup(account_name));
        autocomplete_add(enabled_ac, strdup(account_name));
    }

    jid_destroy(jid);
}

gchar**
accounts_get_list(void)
{
    return g_key_file_get_groups(accounts, NULL);
}

ProfAccount*
accounts_get_account(const char * const name)
{
    if (!g_key_file_has_group(accounts, name)) {
        return NULL;
    } else {
        ProfAccount *account = malloc(sizeof(ProfAccount));
        account->name = strdup(name);

        gchar *jid = g_key_file_get_string(accounts, name, "jid", NULL);
        if (jid != NULL) {
            account->jid = strdup(jid);
        } else {
            account->jid = strdup(name);
            g_key_file_set_string(accounts, name, "jid", name);
            _save_accounts();
        }

        gchar *password = g_key_file_get_string(accounts, name, "password", NULL);
        if (password != NULL) {
            account->password = strdup(password);
            g_free(password);
        } else {
            account->password = NULL;
        }

        account->enabled = g_key_file_get_boolean(accounts, name, "enabled", NULL);

        gchar *server = g_key_file_get_string(accounts, name, "server", NULL);
        if (server != NULL) {
            account->server = strdup(server);
        } else {
            account->server = NULL;
        }

        gchar *resource = g_key_file_get_string(accounts, name, "resource", NULL);
        if (resource != NULL) {
            account->resource = strdup(resource);
        } else {
            account->resource = NULL;
        }

        gchar *presence = g_key_file_get_string(accounts, name, "presence.last", NULL);
        if (presence == NULL || (!valid_resource_presence_string(presence))) {
            account->last_presence = strdup("online");
        } else {
            account->last_presence = strdup(presence);
        }

        presence = g_key_file_get_string(accounts, name, "presence.login", NULL);
        if (presence == NULL) {
            account->login_presence = strdup("online");
        } else if (strcmp(presence, "last") == 0) {
            account->login_presence = strdup("last");
        } else if (!valid_resource_presence_string(presence)) {
            account->login_presence = strdup("online");
        } else {
            account->login_presence = strdup(presence);
        }

        account->priority_online = g_key_file_get_integer(accounts, name, "priority.online", NULL);
        account->priority_chat = g_key_file_get_integer(accounts, name, "priority.chat", NULL);
        account->priority_away = g_key_file_get_integer(accounts, name, "priority.away", NULL);
        account->priority_xa = g_key_file_get_integer(accounts, name, "priority.xa", NULL);
        account->priority_dnd = g_key_file_get_integer(accounts, name, "priority.dnd", NULL);

        gchar *muc_service = g_key_file_get_string(accounts, name, "muc.service", NULL);
        if (muc_service == NULL) {
            GString *g_muc_service = g_string_new("conference.");
            Jid *jidp = jid_create(account->jid);
            g_string_append(g_muc_service, jidp->domainpart);
            account->muc_service = strdup(g_muc_service->str);
            g_string_free(g_muc_service, TRUE);
            jid_destroy(jidp);
        } else {
            account->muc_service = strdup(muc_service);
        }

        gchar *muc_nick = g_key_file_get_string(accounts, name, "muc.nick", NULL);
        if (muc_nick == NULL) {
            Jid *jidp = jid_create(account->jid);
            account->muc_nick = strdup(jidp->localpart);
            jid_destroy(jidp);
        } else {
            account->muc_nick = strdup(muc_nick);
        }

        // get room history
        account->room_history = NULL;
        gsize history_size = 0;
        gchar **room_history_values = g_key_file_get_string_list(accounts, name,
            "rooms.history", &history_size, NULL);

        if (room_history_values != NULL) {
            int i = 0;
            for (i = 0; i < history_size; i++) {
                account->room_history = g_slist_append(account->room_history,
                    strdup(room_history_values[i]));
            }

            g_strfreev(room_history_values);
        }

        return account;
    }
}

void
accounts_free_account(ProfAccount *account)
{
    if (account != NULL) {
        FREE_SET_NULL(account->name);
        FREE_SET_NULL(account->jid);
        FREE_SET_NULL(account->resource);
        FREE_SET_NULL(account->password);
        FREE_SET_NULL(account->server);
        FREE_SET_NULL(account->last_presence);
        FREE_SET_NULL(account->login_presence);
        FREE_SET_NULL(account->muc_service);
        FREE_SET_NULL(account->muc_nick);
        FREE_SET_NULL(account);
    }
}

gboolean
accounts_enable(const char * const name)
{
    if (g_key_file_has_group(accounts, name)) {
        g_key_file_set_boolean(accounts, name, "enabled", TRUE);
        _save_accounts();
        autocomplete_add(enabled_ac, strdup(name));
        return TRUE;
    } else {
        return FALSE;
    }
}

gboolean
accounts_disable(const char * const name)
{
    if (g_key_file_has_group(accounts, name)) {
        g_key_file_set_boolean(accounts, name, "enabled", FALSE);
        _save_accounts();
        autocomplete_remove(enabled_ac, strdup(name));
        return TRUE;
    } else {
        return FALSE;
    }
}

gboolean
accounts_rename(const char * const account_name, const char * const new_name)
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
            free(value);
        }
    }

    g_key_file_remove_group(accounts, account_name, NULL);
    _save_accounts();

    autocomplete_remove(all_ac, strdup(account_name));
    autocomplete_add(all_ac, strdup(new_name));
    if (g_key_file_get_boolean(accounts, new_name, "enabled", NULL)) {
        autocomplete_remove(enabled_ac, strdup(account_name));
        autocomplete_add(enabled_ac, strdup(new_name));
    }

    return TRUE;
}

gboolean
accounts_account_exists(const char * const account_name)
{
    return g_key_file_has_group(accounts, account_name);

}

void
accounts_set_jid(const char * const account_name, const char * const value)
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

void
accounts_set_server(const char * const account_name, const char * const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "server", value);
        _save_accounts();
    }
}

void
accounts_set_resource(const char * const account_name, const char * const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "resource", value);
        _save_accounts();
    }
}

void
accounts_set_password(const char * const account_name, const char * const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "password", value);
        _save_accounts();
    }
}

void
accounts_clear_password(const char * const account_name)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_remove_key(accounts, account_name, "password", NULL);
        _save_accounts();
    }
}

void
accounts_set_muc_service(const char * const account_name, const char * const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "muc.service", value);
        _save_accounts();
    }
}

void
accounts_set_muc_nick(const char * const account_name, const char * const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "muc.nick", value);
        _save_accounts();
    }
}

void
accounts_set_priority_online(const char * const account_name, const gint value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_integer(accounts, account_name, "priority.online", value);
        _save_accounts();
    }
}

void
accounts_set_priority_chat(const char * const account_name, const gint value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_integer(accounts, account_name, "priority.chat", value);
        _save_accounts();
    }
}

void
accounts_set_priority_away(const char * const account_name, const gint value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_integer(accounts, account_name, "priority.away", value);
        _save_accounts();
    }
}

void
accounts_set_priority_xa(const char * const account_name, const gint value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_integer(accounts, account_name, "priority.xa", value);
        _save_accounts();
    }
}

void
accounts_set_priority_dnd(const char * const account_name, const gint value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_integer(accounts, account_name, "priority.dnd", value);
        _save_accounts();
    }
}

void
accounts_set_priority_all(const char * const account_name, const gint value)
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

gint
accounts_get_priority_for_presence_type(const char * const account_name,
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

void
accounts_set_last_presence(const char * const account_name, const char * const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "presence.last", value);
        _save_accounts();
    }
}

void
accounts_set_login_presence(const char * const account_name, const char * const value)
{
    if (accounts_account_exists(account_name)) {
        g_key_file_set_string(accounts, account_name, "presence.login", value);
        _save_accounts();
    }
}

resource_presence_t
accounts_get_last_presence(const char * const account_name)
{
    gchar *setting = g_key_file_get_string(accounts, account_name, "presence.last", NULL);
    if (setting == NULL || (strcmp(setting, "online") == 0)) {
        return RESOURCE_ONLINE;
    } else if (strcmp(setting, "chat") == 0) {
        return RESOURCE_CHAT;
    } else if (strcmp(setting, "away") == 0) {
        return RESOURCE_AWAY;
    } else if (strcmp(setting, "xa") == 0) {
        return RESOURCE_XA;
    } else if (strcmp(setting, "dnd") == 0) {
        return RESOURCE_DND;
    } else {
        log_warning("Error reading presence.last for account: '%s', value: '%s', defaulting to 'online'",
            account_name, setting);
        return RESOURCE_ONLINE;
    }
}

resource_presence_t
accounts_get_login_presence(const char * const account_name)
{
    gchar *setting = g_key_file_get_string(accounts, account_name, "presence.login", NULL);
    if (setting == NULL || (strcmp(setting, "online") == 0)) {
        return RESOURCE_ONLINE;
    } else if (strcmp(setting, "chat") == 0) {
        return RESOURCE_CHAT;
    } else if (strcmp(setting, "away") == 0) {
        return RESOURCE_AWAY;
    } else if (strcmp(setting, "xa") == 0) {
        return RESOURCE_XA;
    } else if (strcmp(setting, "dnd") == 0) {
        return RESOURCE_DND;
    } else if (strcmp(setting, "last") == 0) {
        return accounts_get_last_presence(account_name);
    } else {
        log_warning("Error reading presence.login for account: '%s', value: '%s', defaulting to 'online'",
            account_name, setting);
        return RESOURCE_ONLINE;
    }
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
    char *g_accounts_data = g_key_file_to_data(accounts, &g_data_size, NULL);
    g_file_set_contents(accounts_loc, g_accounts_data, g_data_size, NULL);
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

