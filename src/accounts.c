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

static gchar *accounts_loc;
static GKeyFile *accounts;

static Autocomplete all_ac;
static Autocomplete enabled_ac;

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

    char *jid = g_key_file_get_string(accounts, account_name, "jid", NULL);
    if (jid != NULL) {
        g_key_file_set_string(accounts, new_name, "jid", jid);
        free(jid);
    }

    char *server = g_key_file_get_string(accounts, account_name, "server", NULL);
    if (server != NULL) {
        g_key_file_set_string(accounts, new_name, "server", server);
        free(server);
    }

    char *resource = g_key_file_get_string(accounts, account_name, "resource", NULL);
    if (resource != NULL) {
        g_key_file_set_string(accounts, new_name, "resource", resource);
        free(resource);
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
