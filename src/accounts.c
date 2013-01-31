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
#include "autocomplete.h"
#include "common.h"
#include "files.h"
#include "jid.h"
#include "log.h"
#include "xmpp.h"

static gchar *accounts_loc;
static GKeyFile *accounts;

static Autocomplete all_ac;
static Autocomplete enabled_ac;

static gchar *string_keys[] = {"jid", "server", "resource", "presence.last", "presence.login"};

static void _fix_legacy_accounts(const char * const account_name);
static void _save_accounts(void);

void
accounts_load(void)
{
    log_info("Loading accounts");
    all_ac = autocomplete_new();
    enabled_ac = autocomplete_new();
    accounts_loc = files_get_accounts_file();

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
        if (presence == NULL || (!presence_valid_string(presence))) {
            account->last_presence = strdup("online");
        } else {
            account->last_presence = strdup(presence);
        }

        presence = g_key_file_get_string(accounts, name, "presence.login", NULL);
        if (presence == NULL) {
            account->login_presence = strdup("online");
        } else if (strcmp(presence, "last") == 0) {
            account->login_presence = strdup("last");
        } else if (!presence_valid_string(presence)) {
            account->login_presence = strdup("online");
        } else {
            account->login_presence = strdup(presence);
        }

        account->priority_online = g_key_file_get_integer(accounts, name, "priority.online", NULL);
        account->priority_chat = g_key_file_get_integer(accounts, name, "priority.chat", NULL);
        account->priority_away = g_key_file_get_integer(accounts, name, "priority.away", NULL);
        account->priority_xa = g_key_file_get_integer(accounts, name, "priority.xa", NULL);
        account->priority_dnd = g_key_file_get_integer(accounts, name, "priority.dnd", NULL);

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
        FREE_SET_NULL(account->server);
        FREE_SET_NULL(account->last_presence);
        FREE_SET_NULL(account->login_presence);
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
        g_key_file_get_boolean(accounts, account_name, "priority.online", NULL));
    g_key_file_set_integer(accounts, new_name, "priority.chat",
        g_key_file_get_boolean(accounts, account_name, "priority.chat", NULL));
    g_key_file_set_integer(accounts, new_name, "priority.away",
        g_key_file_get_boolean(accounts, account_name, "priority.away", NULL));
    g_key_file_set_integer(accounts, new_name, "priority.xa",
        g_key_file_get_boolean(accounts, account_name, "priority.xa", NULL));
    g_key_file_set_integer(accounts, new_name, "priority.dnd",
        g_key_file_get_boolean(accounts, account_name, "priority.dnd", NULL));

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
    jabber_presence_t presence_type)
{
    gint result;

    switch (presence_type)
    {
        case (PRESENCE_ONLINE):
            result = g_key_file_get_integer(accounts, account_name, "priority.online", NULL);
            break;
        case (PRESENCE_CHAT):
            result = g_key_file_get_integer(accounts, account_name, "priority.chat", NULL);
            break;
        case (PRESENCE_AWAY):
            result = g_key_file_get_integer(accounts, account_name, "priority.away", NULL);
            break;
        case (PRESENCE_XA):
            result = g_key_file_get_integer(accounts, account_name, "priority.xa", NULL);
            break;
        case (PRESENCE_DND):
            result = g_key_file_get_integer(accounts, account_name, "priority.dnd", NULL);
            break;
        default:
            result = 0;
            break;
    }

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

jabber_presence_t
accounts_get_last_presence(const char * const account_name)
{
    gchar *setting = g_key_file_get_string(accounts, account_name, "presence.last", NULL);
    if (setting == NULL || (strcmp(setting, "online") == 0)) {
        return PRESENCE_ONLINE;
    } else if (strcmp(setting, "chat") == 0) {
        return PRESENCE_CHAT;
    } else if (strcmp(setting, "away") == 0) {
        return PRESENCE_AWAY;
    } else if (strcmp(setting, "xa") == 0) {
        return PRESENCE_XA;
    } else if (strcmp(setting, "dnd") == 0) {
        return PRESENCE_DND;
    } else {
        log_warning("Error reading presence.last for account: '%s', value: '%s', defaulting to 'online'",
            account_name, setting);
        return PRESENCE_ONLINE;
    }
}

jabber_presence_t
accounts_get_login_presence(const char * const account_name)
{
    gchar *setting = g_key_file_get_string(accounts, account_name, "presence.login", NULL);
    if (setting == NULL || (strcmp(setting, "online") == 0)) {
        return PRESENCE_ONLINE;
    } else if (strcmp(setting, "chat") == 0) {
        return PRESENCE_CHAT;
    } else if (strcmp(setting, "away") == 0) {
        return PRESENCE_AWAY;
    } else if (strcmp(setting, "xa") == 0) {
        return PRESENCE_XA;
    } else if (strcmp(setting, "dnd") == 0) {
        return PRESENCE_DND;
    } else if (strcmp(setting, "last") == 0) {
        return accounts_get_last_presence(account_name);
    } else {
        log_warning("Error reading presence.login for account: '%s', value: '%s', defaulting to 'online'",
            account_name, setting);
        return PRESENCE_ONLINE;
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

    jid_destroy(jid);
}

static void
_save_accounts(void)
{
    gsize g_data_size;
    char *g_accounts_data = g_key_file_to_data(accounts, &g_data_size, NULL);
    g_file_set_contents(accounts_loc, g_accounts_data, g_data_size, NULL);
}
