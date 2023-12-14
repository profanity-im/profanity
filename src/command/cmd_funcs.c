/*
 * cmd_funcs.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2023 Michael Vetter <jubalh@iodoru.org>
 * Copyright (C) 2020 William Wennerström <william@wstrm.dev>
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <langinfo.h>
#include <ctype.h>

// fork / execl
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>

#include "profanity.h"
#include "log.h"
#include "common.h"
#include "command/cmd_funcs.h"
#include "command/cmd_defs.h"
#include "command/cmd_ac.h"
#include "config/files.h"
#include "config/accounts.h"
#include "config/account.h"
#include "config/cafile.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "config/tlscerts.h"
#include "config/scripts.h"
#include "event/client_events.h"
#include "tools/http_upload.h"
#include "tools/http_download.h"
#include "tools/autocomplete.h"
#include "tools/parser.h"
#include "tools/plugin_download.h"
#include "tools/bookmark_ignore.h"
#include "tools/editor.h"
#include "plugins/plugins.h"
#include "ui/inputwin.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/avatar.h"
#include "xmpp/chat_session.h"
#include "xmpp/connection.h"
#include "xmpp/contact.h"
#include "xmpp/jid.h"
#include "xmpp/muc.h"
#include "xmpp/roster_list.h"
#include "xmpp/session.h"
#include "xmpp/stanza.h"
#include "xmpp/vcard_funcs.h"
#include "xmpp/xmpp.h"

#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif

#ifdef HAVE_LIBGPGME
#include "pgp/gpg.h"
#include "pgp/ox.h"
#include "xmpp/ox.h"
#endif

#ifdef HAVE_OMEMO
#include "omemo/omemo.h"
#include "xmpp/omemo.h"
#include "tools/aesgcm_download.h"
#endif

#ifdef HAVE_GTK
#include "ui/tray.h"
#include "tools/clipboard.h"
#endif

#ifdef HAVE_PYTHON
#include "plugins/python_plugins.h"
#endif

static void _update_presence(const resource_presence_t presence,
                             const char* const show, gchar** args);
static gboolean _cmd_set_boolean_preference(gchar* arg, const char* const display,
                                            preference_t pref);
static void _who_room(ProfWin* window, const char* const command, gchar** args);
static void _who_roster(ProfWin* window, const char* const command, gchar** args);
static gboolean _cmd_execute(ProfWin* window, const char* const command, const char* const inp);
static gboolean _cmd_execute_default(ProfWin* window, const char* inp);
static gboolean _cmd_execute_alias(ProfWin* window, const char* const inp, gboolean* ran);
static gboolean
_string_matches_one_of(const char* what, const char* is, bool is_can_be_null, const char* first, ...) __attribute__((sentinel));
static gboolean
_download_install_plugin(ProfWin* window, gchar* url, gchar* path);

static gboolean
_string_matches_one_of(const char* what, const char* is, bool is_can_be_null, const char* first, ...)
{
    gboolean ret = FALSE;
    va_list ap;
    const char* cur = first;
    if (!is)
        return is_can_be_null;

    va_start(ap, first);
    while (cur != NULL) {
        if (g_strcmp0(is, cur) == 0) {
            ret = TRUE;
            break;
        }
        cur = va_arg(ap, const char*);
    }
    va_end(ap);
    if (!ret && what) {
        cons_show("Invalid %s: '%s'", what, is);
        char errmsg[256] = { 0 };
        size_t sz = 0;
        int s = snprintf(errmsg, sizeof(errmsg) - sz, "%s must be one of:", what);
        if (s < 0 || s + sz >= sizeof(errmsg))
            return ret;
        sz += s;

        cur = first;
        va_start(ap, first);
        while (cur != NULL) {
            const char* next = va_arg(ap, const char*);
            if (next) {
                s = snprintf(errmsg + sz, sizeof(errmsg) - sz, " '%s',", cur);
            } else {
                /* remove last ',' */
                sz--;
                errmsg[sz] = '\0';
                s = snprintf(errmsg + sz, sizeof(errmsg) - sz, " or '%s'.", cur);
            }
            if (s < 0 || s + sz >= sizeof(errmsg)) {
                log_debug("Error message too long or some other error occurred (%d).", s);
                s = -1;
                break;
            }
            sz += s;
            cur = next;
        }
        va_end(ap);
        if (s > 0)
            cons_show(errmsg);
    }
    return ret;
}

/**
 * @brief Processes a line of input and determines if profanity should continue.
 *
 * @param window
 * @param inp The input string to process.
 * @return Returns TRUE if profanity is to continue, FALSE otherwise.
 */
gboolean
cmd_process_input(ProfWin* window, char* inp)
{
    log_debug("Input received: %s", inp);
    g_strchomp(inp);

    if (strlen(inp) == 0) {
        return TRUE;
    }

    // handle command if input starts with a '/'
    if (inp[0] == '/') {
        auto_char char* inp_cpy = strdup(inp);
        char* command = strtok(inp_cpy, " ");
        char* question_mark = strchr(command, '?');
        if (question_mark) {
            *question_mark = '\0';
            auto_gchar gchar* fakeinp = g_strdup_printf("/help %s", command + 1);
            if (fakeinp) {
                return _cmd_execute(window, "/help", fakeinp);
            }
        } else {
            return _cmd_execute(window, command, inp);
        }
    }

    // call a default handler if input didn't start with '/'
    return _cmd_execute_default(window, inp);
}

// Command execution

void
cmd_execute_connect(ProfWin* window, const char* const account)
{
    auto_gchar gchar* command = g_strdup_printf("/connect %s", account);
    cmd_process_input(window, command);
}

gboolean
cmd_tls_certpath(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strcmp0(args[1], "set") == 0) {
        if (args[2] == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        if (g_file_test(args[2], G_FILE_TEST_IS_DIR)) {
            prefs_set_string(PREF_TLS_CERTPATH, args[2]);
            cons_show("Certificate path set to: %s", args[2]);
        } else {
            cons_show("Directory %s does not exist.", args[2]);
        }
        return TRUE;
    } else if (g_strcmp0(args[1], "clear") == 0) {
        prefs_set_string(PREF_TLS_CERTPATH, "none");
        cons_show("Certificate path cleared");
        return TRUE;
    } else if (g_strcmp0(args[1], "default") == 0) {
        prefs_set_string(PREF_TLS_CERTPATH, NULL);
        cons_show("Certificate path defaulted to finding system certpath.");
        return TRUE;
    } else if (args[1] == NULL) {
        auto_char char* path = prefs_get_tls_certpath();
        if (path) {
            cons_show("Trusted certificate path: %s", path);
        } else {
            cons_show("No trusted certificate path set.");
        }
        return TRUE;
    } else {
        cons_bad_cmd_usage(command);
        return TRUE;
    }
}

gboolean
cmd_tls_trust(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are currently not connected.");
        return TRUE;
    }
    if (!connection_is_secured()) {
        cons_show("No TLS connection established");
        return TRUE;
    }
    TLSCertificate* cert = connection_get_tls_peer_cert();
    if (!cert) {
        cons_show("Error getting TLS certificate.");
        return TRUE;
    }
    cafile_add(cert);
    if (tlscerts_exists(cert->fingerprint)) {
        cons_show("Certificate %s already trusted.", cert->fingerprint);
        tlscerts_free(cert);
        return TRUE;
    }
    cons_show("Adding %s to trusted certificates.", cert->fingerprint);
    tlscerts_add(cert);
    tlscerts_free(cert);
    return TRUE;
}

gboolean
cmd_tls_trusted(ProfWin* window, const char* const command, gchar** args)
{
    GList* certs = tlscerts_list();
    GList* curr = certs;

    if (curr) {
        cons_show("Trusted certificates:");
        cons_show("");
    } else {
        cons_show("No trusted certificates found.");
    }
    while (curr) {
        TLSCertificate* cert = curr->data;
        cons_show_tlscert_summary(cert);
        cons_show("");
        curr = g_list_next(curr);
    }
    g_list_free_full(certs, (GDestroyNotify)tlscerts_free);
    return TRUE;
}

gboolean
cmd_tls_revoke(ProfWin* window, const char* const command, gchar** args)
{
    if (args[1] == NULL) {
        cons_bad_cmd_usage(command);
    } else {
        gboolean res = tlscerts_revoke(args[1]);
        if (res) {
            cons_show("Trusted certificate revoked: %s", args[1]);
        } else {
            cons_show("Could not find certificate: %s", args[1]);
        }
    }
    return TRUE;
}

gboolean
cmd_tls_cert(ProfWin* window, const char* const command, gchar** args)
{
    if (args[1]) {
        TLSCertificate* cert = tlscerts_get_trusted(args[1]);
        if (!cert) {
            cons_show("No such certificate.");
        } else {
            cons_show_tlscert(cert);
            tlscerts_free(cert);
        }
        return TRUE;
    } else {
        jabber_conn_status_t conn_status = connection_get_status();
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }
        if (!connection_is_secured()) {
            cons_show("No TLS connection established");
            return TRUE;
        }
        TLSCertificate* cert = connection_get_tls_peer_cert();
        if (!cert) {
            cons_show("Error getting TLS certificate.");
            return TRUE;
        }
        cons_show_tlscert(cert);
        cons_show("");
        tlscerts_free(cert);
        return TRUE;
    }
}

gboolean
cmd_connect(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status != JABBER_DISCONNECTED) {
        cons_show("You are either connected already, or a login is in process.");
        return TRUE;
    }

    gchar* opt_keys[] = { "server", "port", "tls", "auth", NULL };
    gboolean parsed;

    GHashTable* options = parse_options(&args[args[0] ? 1 : 0], opt_keys, &parsed);
    if (!parsed) {
        cons_bad_cmd_usage(command);
        cons_show("");
        options_destroy(options);
        return TRUE;
    }

    char* altdomain = g_hash_table_lookup(options, "server");

    char* tls_policy = g_hash_table_lookup(options, "tls");
    if (!_string_matches_one_of("TLS policy", tls_policy, TRUE, "force", "allow", "trust", "disable", "legacy", NULL)) {
        cons_bad_cmd_usage(command);
        cons_show("");
        options_destroy(options);
        return TRUE;
    }

    char* auth_policy = g_hash_table_lookup(options, "auth");
    if (!_string_matches_one_of("Auth policy", auth_policy, TRUE, "default", "legacy", NULL)) {
        cons_bad_cmd_usage(command);
        cons_show("");
        options_destroy(options);
        return TRUE;
    }

    int port = 0;
    if (g_hash_table_contains(options, "port")) {
        char* port_str = g_hash_table_lookup(options, "port");
        auto_char char* err_msg = NULL;
        gboolean res = strtoi_range(port_str, &port, 1, 65535, &err_msg);
        if (!res) {
            cons_show(err_msg);
            cons_show("");
            port = 0;
            options_destroy(options);
            return TRUE;
        }
    }

    gchar* user_orig = args[0];
    auto_gchar gchar* def = prefs_get_string(PREF_DEFAULT_ACCOUNT);
    if (!user_orig) {
        if (def) {
            user_orig = def;
            cons_show("Using default account %s.", user_orig);
        } else {
            cons_show("No default account.");
            options_destroy(options);
            return TRUE;
        }
    }

    auto_char char* jid = NULL;
    auto_char char* user = strdup(user_orig);

    // connect with account
    ProfAccount* account = accounts_get_account(user);
    if (account) {
        // override account options with connect options
        if (altdomain != NULL)
            account_set_server(account, altdomain);
        if (port != 0)
            account_set_port(account, port);
        if (tls_policy != NULL)
            account_set_tls_policy(account, tls_policy);
        if (auth_policy != NULL)
            account_set_auth_policy(account, auth_policy);

        // use password if set
        if (account->password) {
            conn_status = cl_ev_connect_account(account);

            // use eval_password if set
        } else if (account->eval_password) {
            gboolean res = account_eval_password(account);
            if (res) {
                conn_status = cl_ev_connect_account(account);
                free(account->password);
                account->password = NULL;
            } else {
                cons_show("Error evaluating password, see logs for details.");
                account_free(account);
                options_destroy(options);
                return TRUE;
            }

            // no account password setting, prompt
        } else {
            account->password = ui_ask_password(false);
            conn_status = cl_ev_connect_account(account);
            free(account->password);
            account->password = NULL;
        }

        jid = account_create_connect_jid(account);
        account_free(account);

        // connect with JID
    } else {
        jid = g_utf8_strdown(user, -1);
        auto_char char* passwd = ui_ask_password(false);
        conn_status = cl_ev_connect_jid(jid, passwd, altdomain, port, tls_policy, auth_policy);
    }

    if (conn_status == JABBER_DISCONNECTED) {
        cons_show_error("Connection attempt for %s failed.", jid);
        log_info("Connection attempt for %s failed", jid);
    }

    options_destroy(options);

    return TRUE;
}

gboolean
cmd_account_list(ProfWin* window, const char* const command, gchar** args)
{
    auto_gcharv gchar** accounts = accounts_get_list();
    cons_show_account_list(accounts);

    return TRUE;
}

gboolean
cmd_account_show(ProfWin* window, const char* const command, gchar** args)
{
    gchar* account_name = args[1];
    if (account_name == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    ProfAccount* account = accounts_get_account(account_name);
    if (account == NULL) {
        cons_show("No such account.");
        cons_show("");
    } else {
        cons_show_account(account);
        account_free(account);
    }

    return TRUE;
}

gboolean
cmd_account_add(ProfWin* window, const char* const command, gchar** args)
{
    gchar* account_name = args[1];
    if (account_name == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    accounts_add(account_name, NULL, 0, NULL, NULL);
    cons_show("Account created.");
    cons_show("");

    return TRUE;
}

gboolean
cmd_account_remove(ProfWin* window, const char* const command, gchar** args)
{
    gchar* account_name = args[1];
    if (!account_name) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    auto_gchar gchar* def = prefs_get_string(PREF_DEFAULT_ACCOUNT);
    if (accounts_remove(account_name)) {
        cons_show("Account %s removed.", account_name);
        if (def && strcmp(def, account_name) == 0) {
            prefs_set_string(PREF_DEFAULT_ACCOUNT, NULL);
            cons_show("Default account removed because the corresponding account was removed.");
        }
    } else {
        cons_show("Failed to remove account %s.", account_name);
        cons_show("Either the account does not exist, or an unknown error occurred.");
    }
    cons_show("");

    return TRUE;
}

gboolean
cmd_account_enable(ProfWin* window, const char* const command, gchar** args)
{
    gchar* account_name = args[1];
    if (account_name == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (accounts_enable(account_name)) {
        cons_show("Account enabled.");
        cons_show("");
    } else {
        cons_show("No such account: %s", account_name);
        cons_show("");
    }

    return TRUE;
}
gboolean
cmd_account_disable(ProfWin* window, const char* const command, gchar** args)
{
    gchar* account_name = args[1];
    if (account_name == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (accounts_disable(account_name)) {
        cons_show("Account disabled.");
        cons_show("");
    } else {
        cons_show("No such account: %s", account_name);
        cons_show("");
    }

    return TRUE;
}

gboolean
cmd_account_rename(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strv_length(args) != 3) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    gchar* account_name = args[1];
    gchar* new_name = args[2];

    if (accounts_rename(account_name, new_name)) {
        cons_show("Account renamed.");
        cons_show("");
    } else {
        cons_show("Either account %s doesn't exist, or account %s already exists.", account_name, new_name);
        cons_show("");
    }

    return TRUE;
}

gboolean
cmd_account_default(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strv_length(args) == 1) {
        auto_gchar gchar* def = prefs_get_string(PREF_DEFAULT_ACCOUNT);
        if (def) {
            cons_show("The default account is %s.", def);
        } else {
            cons_show("No default account.");
        }
    } else if (g_strv_length(args) == 2) {
        if (strcmp(args[1], "off") == 0) {
            prefs_set_string(PREF_DEFAULT_ACCOUNT, NULL);
            cons_show("Removed default account.");
        } else {
            cons_bad_cmd_usage(command);
        }
    } else if (g_strv_length(args) == 3) {
        if (strcmp(args[1], "set") == 0) {
            ProfAccount* account_p = accounts_get_account(args[2]);
            if (account_p) {
                prefs_set_string(PREF_DEFAULT_ACCOUNT, args[2]);
                cons_show("Default account set to %s.", args[2]);
                account_free(account_p);
            } else {
                cons_show("Account %s does not exist.", args[2]);
            }
        } else {
            cons_bad_cmd_usage(command);
        }
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
_account_set_jid(char* account_name, char* jid)
{
    auto_jid Jid* jidp = jid_create(jid);
    if (jidp == NULL) {
        cons_show("Malformed jid: %s", jid);
    } else {
        accounts_set_jid(account_name, jidp->barejid);
        cons_show("Updated jid for account %s: %s", account_name, jidp->barejid);
        if (jidp->resourcepart) {
            accounts_set_resource(account_name, jidp->resourcepart);
            cons_show("Updated resource for account %s: %s", account_name, jidp->resourcepart);
        }
        cons_show("");
    }

    return TRUE;
}

gboolean
_account_set_server(char* account_name, char* server)
{
    accounts_set_server(account_name, server);
    cons_show("Updated server for account %s: %s", account_name, server);
    cons_show("");
    return TRUE;
}

gboolean
_account_set_port(char* account_name, char* port)
{
    int porti;
    auto_char char* err_msg = NULL;
    gboolean res = strtoi_range(port, &porti, 1, 65535, &err_msg);
    if (!res) {
        cons_show(err_msg);
        cons_show("");
    } else {
        accounts_set_port(account_name, porti);
        cons_show("Updated port for account %s: %s", account_name, port);
        cons_show("");
    }
    return TRUE;
}

gboolean
_account_set_resource(char* account_name, char* resource)
{
    accounts_set_resource(account_name, resource);
    if (connection_get_status() == JABBER_CONNECTED) {
        cons_show("Updated resource for account %s: %s, reconnect to pick up the change.", account_name, resource);
    } else {
        cons_show("Updated resource for account %s: %s", account_name, resource);
    }
    cons_show("");
    return TRUE;
}

gboolean
_account_set_password(char* account_name, char* password)
{
    ProfAccount* account = accounts_get_account(account_name);
    if (account->eval_password) {
        cons_show("Cannot set password when eval_password is set.");
    } else {
        accounts_set_password(account_name, password);
        cons_show("Updated password for account %s", account_name);
        cons_show("");
    }
    account_free(account);
    return TRUE;
}

gboolean
_account_set_eval_password(char* account_name, char* eval_password)
{
    ProfAccount* account = accounts_get_account(account_name);
    if (account->password) {
        cons_show("Cannot set eval_password when password is set.");
    } else {
        accounts_set_eval_password(account_name, eval_password);
        cons_show("Updated eval_password for account %s", account_name);
        cons_show("");
    }
    account_free(account);
    return TRUE;
}

gboolean
_account_set_muc(char* account_name, char* muc)
{
    accounts_set_muc_service(account_name, muc);
    cons_show("Updated muc service for account %s: %s", account_name, muc);
    cons_show("");
    return TRUE;
}

gboolean
_account_set_nick(char* account_name, char* nick)
{
    accounts_set_muc_nick(account_name, nick);
    cons_show("Updated muc nick for account %s: %s", account_name, nick);
    cons_show("");
    return TRUE;
}

gboolean
_account_set_otr(char* account_name, char* policy)
{
    if (_string_matches_one_of("OTR policy", policy, FALSE, "manual", "opportunistic", "always", NULL)) {
        accounts_set_otr_policy(account_name, policy);
        cons_show("Updated OTR policy for account %s: %s", account_name, policy);
        cons_show("");
    }
    return TRUE;
}

gboolean
_account_set_status(char* account_name, char* status)
{
    if (!valid_resource_presence_string(status) && (strcmp(status, "last") != 0)) {
        cons_show("Invalid status: %s", status);
    } else {
        accounts_set_login_presence(account_name, status);
        cons_show("Updated login status for account %s: %s", account_name, status);
    }
    cons_show("");
    return TRUE;
}

gboolean
_account_set_pgpkeyid(char* account_name, char* pgpkeyid)
{
#ifdef HAVE_LIBGPGME
    auto_char char* err_str = NULL;
    if (!p_gpg_valid_key(pgpkeyid, &err_str)) {
        cons_show("Invalid PGP key ID specified: %s, see /pgp keys", err_str);
    } else {
        accounts_set_pgp_keyid(account_name, pgpkeyid);
        cons_show("Updated PGP key ID for account %s: %s", account_name, pgpkeyid);
    }
#else
    cons_show("PGP support is not included in this build.");
#endif
    cons_show("");
    return TRUE;
}

gboolean
_account_set_startscript(char* account_name, char* script)
{
    accounts_set_script_start(account_name, script);
    cons_show("Updated start script for account %s: %s", account_name, script);
    return TRUE;
}

gboolean
_account_set_client(char* account_name, char* new_client)
{
    accounts_set_client(account_name, new_client);
    cons_show("Client name for account %s has been set to %s", account_name, new_client);
    return TRUE;
}

gboolean
_account_set_theme(char* account_name, char* theme)
{
    if (!theme_exists(theme)) {
        cons_show("Theme does not exist: %s", theme);
        return TRUE;
    }

    accounts_set_theme(account_name, theme);
    if (connection_get_status() == JABBER_CONNECTED) {
        ProfAccount* account = accounts_get_account(session_get_account_name());
        if (account) {
            if (g_strcmp0(account->name, account_name) == 0) {
                theme_load(theme, false);
                ui_load_colours();
                if (prefs_get_boolean(PREF_ROSTER)) {
                    ui_show_roster();
                } else {
                    ui_hide_roster();
                }
                if (prefs_get_boolean(PREF_OCCUPANTS)) {
                    ui_show_all_room_rosters();
                } else {
                    ui_hide_all_room_rosters();
                }
                ui_redraw();
            }
            account_free(account);
        }
    }
    cons_show("Updated theme for account %s: %s", account_name, theme);
    return TRUE;
}

gboolean
_account_set_tls(char* account_name, char* policy)
{
    if (_string_matches_one_of("TLS policy", policy, FALSE, "force", "allow", "trust", "disable", "legacy", NULL)) {
        accounts_set_tls_policy(account_name, policy);
        cons_show("Updated TLS policy for account %s: %s", account_name, policy);
        cons_show("");
    }
    return TRUE;
}

gboolean
_account_set_auth(char* account_name, char* policy)
{
    if (_string_matches_one_of("Auth policy", policy, FALSE, "default", "legacy", NULL)) {
        accounts_set_auth_policy(account_name, policy);
        cons_show("Updated auth policy for account %s: %s", account_name, policy);
        cons_show("");
    }
    return TRUE;
}

gboolean
_account_set_max_sessions(char* account_name, char* max_sessions_raw)
{
    int max_sessions;
    auto_char char* err_msg = NULL;
    gboolean res = strtoi_range(max_sessions_raw, &max_sessions, 0, INT_MAX, &err_msg);
    if (!res) {
        cons_show(err_msg);
        cons_show("");
        return TRUE;
    }
    accounts_set_max_sessions(account_name, max_sessions);
    if (max_sessions < 1) {
        cons_show("Max sessions alarm for account %s has been disabled.", account_name);
    } else {
        cons_show("Max sessions alarm for account %s has been set to %d", account_name, max_sessions);
    }
    cons_show("");
    return TRUE;
}

gboolean
_account_set_presence_priority(char* account_name, char* presence, char* priority)
{
    int intval;
    auto_char char* err_msg = NULL;
    gboolean res = strtoi_range(priority, &intval, -128, 127, &err_msg);
    if (!res) {
        cons_show(err_msg);
        return TRUE;
    }

    resource_presence_t presence_type = resource_presence_from_string(presence);
    switch (presence_type) {
    case (RESOURCE_ONLINE):
        accounts_set_priority_online(account_name, intval);
        break;
    case (RESOURCE_CHAT):
        accounts_set_priority_chat(account_name, intval);
        break;
    case (RESOURCE_AWAY):
        accounts_set_priority_away(account_name, intval);
        break;
    case (RESOURCE_XA):
        accounts_set_priority_xa(account_name, intval);
        break;
    case (RESOURCE_DND):
        accounts_set_priority_dnd(account_name, intval);
        break;
    }

    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status == JABBER_CONNECTED) {
        char* connected_account = session_get_account_name();
        resource_presence_t last_presence = accounts_get_last_presence(connected_account);
        if (presence_type == last_presence) {
            cl_ev_presence_send(last_presence, 0);
        }
    }
    cons_show("Updated %s priority for account %s: %s", presence, account_name, priority);
    cons_show("");
    return TRUE;
}

gboolean
cmd_account_set(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strv_length(args) != 4) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    gchar* account_name = args[1];
    if (!accounts_account_exists(account_name)) {
        cons_show("Account %s doesn't exist", account_name);
        cons_show("");
        return TRUE;
    }

    gchar* property = args[2];
    gchar* value = args[3];
    if (strcmp(property, "jid") == 0)
        return _account_set_jid(account_name, value);
    if (strcmp(property, "server") == 0)
        return _account_set_server(account_name, value);
    if (strcmp(property, "port") == 0)
        return _account_set_port(account_name, value);
    if (strcmp(property, "resource") == 0)
        return _account_set_resource(account_name, value);
    if (strcmp(property, "password") == 0)
        return _account_set_password(account_name, value);
    if (strcmp(property, "eval_password") == 0)
        return _account_set_eval_password(account_name, value);
    if (strcmp(property, "muc") == 0)
        return _account_set_muc(account_name, value);
    if (strcmp(property, "nick") == 0)
        return _account_set_nick(account_name, value);
    if (strcmp(property, "otr") == 0)
        return _account_set_otr(account_name, value);
    if (strcmp(property, "status") == 0)
        return _account_set_status(account_name, value);
    if (strcmp(property, "pgpkeyid") == 0)
        return _account_set_pgpkeyid(account_name, value);
    if (strcmp(property, "startscript") == 0)
        return _account_set_startscript(account_name, value);
    if (strcmp(property, "clientid") == 0)
        return _account_set_client(account_name, value);
    if (strcmp(property, "theme") == 0)
        return _account_set_theme(account_name, value);
    if (strcmp(property, "tls") == 0)
        return _account_set_tls(account_name, value);
    if (strcmp(property, "auth") == 0)
        return _account_set_auth(account_name, value);
    if (strcmp(property, "session_alarm") == 0)
        return _account_set_max_sessions(account_name, value);

    if (valid_resource_presence_string(property)) {
        return _account_set_presence_priority(account_name, property, value);
    }

    cons_show("Invalid property: %s", property);
    cons_show("");

    return TRUE;
}

gboolean
cmd_account_clear(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strv_length(args) != 3) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    gchar* account_name = args[1];
    if (!accounts_account_exists(account_name)) {
        cons_show("Account %s doesn't exist", account_name);
        cons_show("");
        return TRUE;
    }

    gchar* property = args[2];
    if (strcmp(property, "password") == 0) {
        accounts_clear_password(account_name);
        cons_show("Removed password for account %s", account_name);
    } else if (strcmp(property, "eval_password") == 0) {
        accounts_clear_eval_password(account_name);
        cons_show("Removed eval password for account %s", account_name);
    } else if (strcmp(property, "server") == 0) {
        accounts_clear_server(account_name);
        cons_show("Removed server for account %s", account_name);
    } else if (strcmp(property, "port") == 0) {
        accounts_clear_port(account_name);
        cons_show("Removed port for account %s", account_name);
    } else if (strcmp(property, "otr") == 0) {
        accounts_clear_otr(account_name);
        cons_show("OTR policy removed for account %s", account_name);
    } else if (strcmp(property, "pgpkeyid") == 0) {
        accounts_clear_pgp_keyid(account_name);
        cons_show("Removed PGP key ID for account %s", account_name);
    } else if (strcmp(property, "startscript") == 0) {
        accounts_clear_script_start(account_name);
        cons_show("Removed start script for account %s", account_name);
    } else if (strcmp(property, "clientid") == 0) {
        accounts_clear_client(account_name);
        cons_show("Reset client name for account %s", account_name);
    } else if (strcmp(property, "theme") == 0) {
        accounts_clear_theme(account_name);
        cons_show("Removed theme for account %s", account_name);
    } else if (strcmp(property, "muc") == 0) {
        accounts_clear_muc(account_name);
        cons_show("Removed MUC service for account %s", account_name);
    } else if (strcmp(property, "resource") == 0) {
        accounts_clear_resource(account_name);
        cons_show("Removed resource for account %s", account_name);
    } else if (strcmp(property, "session_alarm") == 0) {
        accounts_clear_max_sessions(account_name);
        cons_show("Disabled session alarm for account %s", account_name);
    } else {
        cons_show("Invalid property: %s", property);
    }
    cons_show("");
    return TRUE;
}

gboolean
cmd_account(ProfWin* window, const char* const command, gchar** args)
{
    if (args[0] != NULL) {
        cons_bad_cmd_usage(command);
        cons_show("");
        return TRUE;
    }

    if (connection_get_status() != JABBER_CONNECTED) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    ProfAccount* account = accounts_get_account(session_get_account_name());
    if (account) {
        cons_show_account(account);
        account_free(account);
    } else {
        log_error("Could not get accounts");
    }

    return TRUE;
}

gboolean
cmd_script(ProfWin* window, const char* const command, gchar** args)
{
    if ((g_strcmp0(args[0], "run") == 0) && args[1]) {
        gboolean res = scripts_exec(args[1]);
        if (!res) {
            cons_show("Could not find script %s", args[1]);
        }
    } else if (g_strcmp0(args[0], "list") == 0) {
        GSList* scripts = scripts_list();
        cons_show_scripts(scripts);
        g_slist_free_full(scripts, g_free);
    } else if ((g_strcmp0(args[0], "show") == 0) && args[1]) {
        GSList* commands = scripts_read(args[1]);
        cons_show_script(args[1], commands);
        g_slist_free_full(commands, g_free);
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

/* escape a string into csv and write it to the file descriptor */
static int
_writecsv(int fd, const char* const str)
{
    if (!str)
        return 0;
    size_t len = strlen(str);

    auto_char char* s = malloc(2 * len * sizeof(char));
    char* c = s;
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] != '"')
            *c++ = str[i];
        else {
            *c++ = '"';
            *c++ = '"';
            len++;
        }
    }
    if (-1 == write(fd, s, len)) {
        cons_show("error: failed to write '%s' to the requested file: %s", s, strerror(errno));
        return -1;
    }
    return 0;
}

gboolean
cmd_export(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        cons_show("");
        return TRUE;
    } else {
        int fd;
        GSList* list = NULL;
        auto_char char* path = get_expanded_path(args[0]);

        fd = open(path, O_WRONLY | O_CREAT, 00600);

        if (-1 == fd) {
            cons_show("error: cannot open %s: %s", args[0], strerror(errno));
            cons_show("");
            return TRUE;
        }

        if (-1 == write(fd, "jid,name\n", strlen("jid,name\n")))
            goto write_error;

        list = roster_get_contacts(ROSTER_ORD_NAME);
        if (list) {
            GSList* curr = list;
            while (curr) {
                PContact contact = curr->data;
                const char* jid = p_contact_barejid(contact);
                const char* name = p_contact_name(contact);

                /* write the data to the file */
                if (-1 == write(fd, "\"", 1))
                    goto write_error;
                if (-1 == _writecsv(fd, jid))
                    goto write_error;
                if (-1 == write(fd, "\",\"", 3))
                    goto write_error;
                if (-1 == _writecsv(fd, name))
                    goto write_error;
                if (-1 == write(fd, "\"\n", 2))
                    goto write_error;

                /* loop */
                curr = g_slist_next(curr);
            }
            cons_show("Contacts exported successfully");
            cons_show("");
        } else {
            cons_show("No contacts in roster.");
            cons_show("");
        }

        g_slist_free(list);
        close(fd);
        return TRUE;
write_error:
        cons_show("error: write failed: %s", strerror(errno));
        cons_show("");
        g_slist_free(list);
        close(fd);
        return TRUE;
    }
}

gboolean
cmd_sub(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are currently not connected.");
        return TRUE;
    }

    char *subcmd, *jid;
    subcmd = args[0];
    jid = args[1];

    if (subcmd == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (strcmp(subcmd, "sent") == 0) {
        cons_show_sent_subs();
        return TRUE;
    }

    if (strcmp(subcmd, "received") == 0) {
        cons_show_received_subs();
        return TRUE;
    }

    if ((window->type != WIN_CHAT) && (jid == NULL)) {
        cons_show("You must specify a contact.");
        return TRUE;
    }

    if (jid == NULL) {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        jid = chatwin->barejid;
    }

    auto_jid Jid* jidp = jid_create(jid);

    if (strcmp(subcmd, "allow") == 0) {
        presence_subscription(jidp->barejid, PRESENCE_SUBSCRIBED);
        cons_show("Accepted subscription for %s", jidp->barejid);
        log_info("Accepted subscription for %s", jidp->barejid);
    } else if (strcmp(subcmd, "deny") == 0) {
        presence_subscription(jidp->barejid, PRESENCE_UNSUBSCRIBED);
        cons_show("Deleted/denied subscription for %s", jidp->barejid);
        log_info("Deleted/denied subscription for %s", jidp->barejid);
    } else if (strcmp(subcmd, "request") == 0) {
        presence_subscription(jidp->barejid, PRESENCE_SUBSCRIBE);
        cons_show("Sent subscription request to %s.", jidp->barejid);
        log_info("Sent subscription request to %s.", jidp->barejid);
    } else if (strcmp(subcmd, "show") == 0) {
        PContact contact = roster_get_contact(jidp->barejid);
        if ((contact == NULL) || (p_contact_subscription(contact) == NULL)) {
            if (window->type == WIN_CHAT) {
                win_println(window, THEME_DEFAULT, "-", "No subscription information for %s.", jidp->barejid);
            } else {
                cons_show("No subscription information for %s.", jidp->barejid);
            }
        } else {
            if (window->type == WIN_CHAT) {
                if (p_contact_pending_out(contact)) {
                    win_println(window, THEME_DEFAULT, "-", "%s subscription status: %s, request pending.",
                                jidp->barejid, p_contact_subscription(contact));
                } else {
                    win_println(window, THEME_DEFAULT, "-", "%s subscription status: %s.", jidp->barejid,
                                p_contact_subscription(contact));
                }
            } else {
                if (p_contact_pending_out(contact)) {
                    cons_show("%s subscription status: %s, request pending.",
                              jidp->barejid, p_contact_subscription(contact));
                } else {
                    cons_show("%s subscription status: %s.", jidp->barejid,
                              p_contact_subscription(contact));
                }
            }
        }
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_disconnect(ProfWin* window, const char* const command, gchar** args)
{
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    cl_ev_disconnect();

    ui_redraw();

    return TRUE;
}

gboolean
cmd_quit(ProfWin* window, const char* const command, gchar** args)
{
    log_info("Profanity is shutting down…");
    exit(0);
    return FALSE;
}

gboolean
cmd_wins_unread(ProfWin* window, const char* const command, gchar** args)
{
    cons_show_wins(TRUE);
    return TRUE;
}

gboolean
cmd_wins_attention(ProfWin* window, const char* const command, gchar** args)
{
    cons_show_wins_attention();
    return TRUE;
}

gboolean
cmd_wins_prune(ProfWin* window, const char* const command, gchar** args)
{
    ui_prune_wins();
    return TRUE;
}

gboolean
cmd_wins_swap(ProfWin* window, const char* const command, gchar** args)
{
    if ((args[1] == NULL) || (args[2] == NULL)) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    int source_win = atoi(args[1]);
    int target_win = atoi(args[2]);

    if ((source_win == 1) || (target_win == 1)) {
        cons_show("Cannot move console window.");
        return TRUE;
    }

    if (source_win == 10 || target_win == 10) {
        cons_show("Window 10 does not exist");
        return TRUE;
    }

    if (source_win == target_win) {
        cons_show("Same source and target window supplied.");
        return TRUE;
    }

    if (wins_get_by_num(source_win) == NULL) {
        cons_show("Window %d does not exist", source_win);
        return TRUE;
    }

    if (wins_get_by_num(target_win) == NULL) {
        cons_show("Window %d does not exist", target_win);
        return TRUE;
    }

    wins_swap(source_win, target_win);
    cons_show("Swapped windows %d <-> %d", source_win, target_win);
    return TRUE;
}

gboolean
cmd_wins(ProfWin* window, const char* const command, gchar** args)
{
    if (args[0] != NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    cons_show_wins(FALSE);
    return TRUE;
}

gboolean
cmd_close(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (g_strcmp0(args[0], "all") == 0) {
        int count = ui_close_all_wins();
        if (count == 0) {
            cons_show("No windows to close.");
        } else if (count == 1) {
            cons_show("Closed 1 window.");
        } else {
            cons_show("Closed %d windows.", count);
        }
        rosterwin_roster();
        return TRUE;
    }

    if (g_strcmp0(args[0], "read") == 0) {
        int count = ui_close_read_wins();
        if (count == 0) {
            cons_show("No windows to close.");
        } else if (count == 1) {
            cons_show("Closed 1 window.");
        } else {
            cons_show("Closed %d windows.", count);
        }
        rosterwin_roster();
        return TRUE;
    }

    gboolean is_num = TRUE;
    int index = 0;
    if (args[0] != NULL) {
        for (int i = 0; i < strlen(args[0]); i++) {
            if (!isdigit((int)args[0][i])) {
                is_num = FALSE;
                break;
            }
        }

        if (is_num) {
            index = atoi(args[0]);
        }
    } else {
        index = wins_get_current_num();
    }

    if (is_num) {
        if (index < 0 || index == 10) {
            cons_show("No such window exists.");
            return TRUE;
        }

        if (index == 1) {
            cons_show("Cannot close console window.");
            return TRUE;
        }

        ProfWin* toclose = wins_get_by_num(index);
        if (!toclose) {
            cons_show("Window is not open.");
            return TRUE;
        }

        // check for unsaved form
        if (ui_win_has_unsaved_form(index)) {
            win_println(window, THEME_DEFAULT, "-", "You have unsaved changes, use /form submit or /form cancel");
            return TRUE;
        }

        // handle leaving rooms, or chat
        if (conn_status == JABBER_CONNECTED) {
            ui_close_connected_win(index);
        }

        // close the window
        ui_close_win(index);
        cons_show("Closed window %d", index);
        wins_tidy();

        rosterwin_roster();
        return TRUE;
    } else {
        if (g_strcmp0(args[0], "console") == 0) {
            cons_show("Cannot close console window.");
            return TRUE;
        }

        ProfWin* toclose = wins_get_by_string(args[0]);
        if (!toclose) {
            cons_show("Window \"%s\" does not exist.", args[0]);
            return TRUE;
        }
        index = wins_get_num(toclose);

        // check for unsaved form
        if (ui_win_has_unsaved_form(index)) {
            win_println(window, THEME_DEFAULT, "-", "You have unsaved changes, use /form submit or /form cancel");
            return TRUE;
        }

        // handle leaving rooms, or chat
        if (conn_status == JABBER_CONNECTED) {
            ui_close_connected_win(index);
        }

        // close the window
        ui_close_win(index);
        cons_show("Closed window %s", args[0]);
        wins_tidy();

        rosterwin_roster();
        return TRUE;
    }
}

gboolean
cmd_win(ProfWin* window, const char* const command, gchar** args)
{
    gboolean is_num = TRUE;
    for (int i = 0; i < strlen(args[0]); i++) {
        if (!isdigit((int)args[0][i])) {
            is_num = FALSE;
            break;
        }
    }

    if (is_num) {
        int num = atoi(args[0]);

        ProfWin* focuswin = wins_get_by_num(num);
        if (!focuswin) {
            cons_show("Window %d does not exist.", num);
        } else {
            ui_focus_win(focuswin);
        }
    } else {
        ProfWin* focuswin = wins_get_by_string(args[0]);
        if (!focuswin) {
            cons_show("Window \"%s\" does not exist.", args[0]);
        } else {
            ui_focus_win(focuswin);
        }
    }

    return TRUE;
}

static void
_cmd_list_commands(GList* commands)
{
    int maxlen = 0;
    GList* curr = commands;
    while (curr) {
        gchar* cmd = curr->data;
        int len = strlen(cmd);
        if (len > maxlen)
            maxlen = len;
        curr = g_list_next(curr);
    }

    GString* cmds = g_string_new("");
    curr = commands;
    int count = 0;
    while (curr) {
        gchar* cmd = curr->data;
        if (count == 5) {
            cons_show(cmds->str);
            g_string_free(cmds, TRUE);
            cmds = g_string_new("");
            count = 0;
        }
        g_string_append_printf(cmds, "%-*s", maxlen + 1, cmd);
        curr = g_list_next(curr);
        count++;
    }
    cons_show(cmds->str);
    g_string_free(cmds, TRUE);
    g_list_free(curr);

    cons_show("");
    cons_show("Use /help [command] without the leading slash, for help on a specific command");
    cons_show("");
}

static void
_cmd_help_cmd_list(const char* const tag)
{
    cons_show("");
    ProfWin* console = wins_get_console();
    if (tag) {
        win_println(console, THEME_HELP_HEADER, "-", "%s commands", tag);
    } else {
        win_println(console, THEME_HELP_HEADER, "-", "All commands");
    }

    GList* ordered_commands = NULL;

    if (g_strcmp0(tag, "plugins") == 0) {
        GList* plugins_cmds = plugins_get_command_names();
        GList* curr = plugins_cmds;
        while (curr) {
            ordered_commands = g_list_insert_sorted(ordered_commands, curr->data, (GCompareFunc)g_strcmp0);
            curr = g_list_next(curr);
        }
        g_list_free(plugins_cmds);
    } else {
        ordered_commands = cmd_get_ordered(tag);

        // add plugins if showing all commands
        if (!tag) {
            GList* plugins_cmds = plugins_get_command_names();
            GList* curr = plugins_cmds;
            while (curr) {
                ordered_commands = g_list_insert_sorted(ordered_commands, curr->data, (GCompareFunc)g_strcmp0);
                curr = g_list_next(curr);
            }
            g_list_free(plugins_cmds);
        }
    }

    _cmd_list_commands(ordered_commands);
    g_list_free(ordered_commands);
}

gboolean
cmd_help(ProfWin* window, const char* const command, gchar** args)
{
    int num_args = g_strv_length(args);
    if (num_args == 0) {
        cons_help();
    } else if (strcmp(args[0], "search_all") == 0) {
        if (args[1] == NULL) {
            cons_bad_cmd_usage(command);
        } else {
            GList* cmds = cmd_search_index_all(args[1]);
            if (cmds == NULL) {
                cons_show("No commands found.");
            } else {
                GList* curr = cmds;
                GList* results = NULL;
                while (curr) {
                    results = g_list_insert_sorted(results, curr->data, (GCompareFunc)g_strcmp0);
                    curr = g_list_next(curr);
                }
                cons_show("Search results:");
                _cmd_list_commands(results);
                g_list_free(results);
            }
            g_list_free(cmds);
        }
    } else if (strcmp(args[0], "search_any") == 0) {
        if (args[1] == NULL) {
            cons_bad_cmd_usage(command);
        } else {
            GList* cmds = cmd_search_index_any(args[1]);
            if (cmds == NULL) {
                cons_show("No commands found.");
            } else {
                GList* curr = cmds;
                GList* results = NULL;
                while (curr) {
                    results = g_list_insert_sorted(results, curr->data, (GCompareFunc)g_strcmp0);
                    curr = g_list_next(curr);
                }
                cons_show("Search results:");
                _cmd_list_commands(results);
                g_list_free(results);
            }
            g_list_free(cmds);
        }
    } else if (strcmp(args[0], "commands") == 0) {
        if (args[1]) {
            if (!cmd_valid_tag(args[1])) {
                cons_bad_cmd_usage(command);
            } else {
                _cmd_help_cmd_list(args[1]);
            }
        } else {
            _cmd_help_cmd_list(NULL);
        }
    } else if (strcmp(args[0], "navigation") == 0) {
        cons_navigation_help();
    } else {
        gchar* cmd = args[0];
        auto_gchar gchar* cmd_with_slash = g_strdup_printf("/%s", cmd);

        Command* command = cmd_get(cmd_with_slash);
        if (command) {
            cons_show_help(cmd_with_slash, &command->help);
        } else {
            CommandHelp* commandHelp = plugins_get_help(cmd_with_slash);
            if (commandHelp) {
                cons_show_help(cmd_with_slash, commandHelp);
            } else {
                cons_show("No such command.");
            }
        }
        cons_show("");
    }

    return TRUE;
}

gboolean
cmd_about(ProfWin* window, const char* const command, gchar** args)
{
    cons_show("");
    cons_about();
    return TRUE;
}

gboolean
cmd_prefs(ProfWin* window, const char* const command, gchar** args)
{
    if (args[0] == NULL) {
        cons_prefs();
        cons_show("Use the /account command for preferences for individual accounts.");
    } else if (strcmp(args[0], "ui") == 0) {
        cons_show("");
        cons_show_ui_prefs();
        cons_show("");
    } else if (strcmp(args[0], "desktop") == 0) {
        cons_show("");
        cons_show_desktop_prefs();
        cons_show("");
    } else if (strcmp(args[0], "chat") == 0) {
        cons_show("");
        cons_show_chat_prefs();
        cons_show("");
    } else if (strcmp(args[0], "log") == 0) {
        cons_show("");
        cons_show_log_prefs();
        cons_show("");
    } else if (strcmp(args[0], "conn") == 0) {
        cons_show("");
        cons_show_connection_prefs();
        cons_show("");
    } else if (strcmp(args[0], "presence") == 0) {
        cons_show("");
        cons_show_presence_prefs();
        cons_show("");
    } else if (strcmp(args[0], "otr") == 0) {
        cons_show("");
        cons_show_otr_prefs();
        cons_show("");
    } else if (strcmp(args[0], "pgp") == 0) {
        cons_show("");
        cons_show_pgp_prefs();
        cons_show("");
    } else if (strcmp(args[0], "omemo") == 0) {
        cons_show("");
        cons_show_omemo_prefs();
        cons_show("");
    } else if (strcmp(args[0], "ox") == 0) {
        cons_show("");
        cons_show_ox_prefs();
        cons_show("");
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_theme(ProfWin* window, const char* const command, gchar** args)
{
    // 'full-load' means to load the theme including the settings (not just [colours])
    gboolean fullload = (g_strcmp0(args[0], "full-load") == 0);

    // list themes
    if (g_strcmp0(args[0], "list") == 0) {
        GSList* themes = theme_list();
        cons_show_themes(themes);
        g_slist_free_full(themes, g_free);

        // load a theme
    } else if (g_strcmp0(args[0], "load") == 0 || fullload) {
        if (args[1] == NULL) {
            cons_bad_cmd_usage(command);
        } else if (theme_load(args[1], fullload)) {
            ui_load_colours();
            prefs_set_string(PREF_THEME, args[1]);
            if (prefs_get_boolean(PREF_ROSTER)) {
                ui_show_roster();
            } else {
                ui_hide_roster();
            }
            if (prefs_get_boolean(PREF_OCCUPANTS)) {
                ui_show_all_room_rosters();
            } else {
                ui_hide_all_room_rosters();
            }
            ui_resize();
            cons_show("Loaded theme: %s", args[1]);
        } else {
            cons_show("Couldn't find theme: %s", args[1]);
        }

        // show colours
    } else if (g_strcmp0(args[0], "colours") == 0) {
        cons_theme_colours();
    } else if (g_strcmp0(args[0], "properties") == 0) {
        cons_theme_properties();
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

static void
_who_room(ProfWin* window, const char* const command, gchar** args)
{
    if ((g_strv_length(args) == 2) && args[1]) {
        cons_show("Argument group is not applicable to chat rooms.");
        return;
    }

    // bad arg
    if (!_string_matches_one_of(NULL, args[0], TRUE, "online", "available", "unavailable", "away", "chat", "xa", "dnd", "any", "moderator", "participant", "visitor", "owner", "admin", "member", "outcast", "none", NULL)) {
        cons_bad_cmd_usage(command);
        return;
    }

    ProfMucWin* mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    // presence filter
    if (_string_matches_one_of(NULL, args[0], TRUE, "online", "available", "unavailable", "away", "chat", "xa", "dnd", "any", NULL)) {

        gchar* presence = args[0];
        GList* occupants = muc_roster(mucwin->roomjid);

        // no arg, show all contacts
        if ((presence == NULL) || (g_strcmp0(presence, "any") == 0)) {
            mucwin_roster(mucwin, occupants, NULL);

            // available
        } else if (strcmp("available", presence) == 0) {
            GList* filtered = NULL;

            while (occupants) {
                Occupant* occupant = occupants->data;
                if (muc_occupant_available(occupant)) {
                    filtered = g_list_append(filtered, occupant);
                }
                occupants = g_list_next(occupants);
            }

            mucwin_roster(mucwin, filtered, "available");

            // unavailable
        } else if (strcmp("unavailable", presence) == 0) {
            GList* filtered = NULL;

            while (occupants) {
                Occupant* occupant = occupants->data;
                if (!muc_occupant_available(occupant)) {
                    filtered = g_list_append(filtered, occupant);
                }
                occupants = g_list_next(occupants);
            }

            mucwin_roster(mucwin, filtered, "unavailable");

            // show specific status
        } else {
            GList* filtered = NULL;

            while (occupants) {
                Occupant* occupant = occupants->data;
                const char* presence_str = string_from_resource_presence(occupant->presence);
                if (strcmp(presence_str, presence) == 0) {
                    filtered = g_list_append(filtered, occupant);
                }
                occupants = g_list_next(occupants);
            }

            mucwin_roster(mucwin, filtered, presence);
        }

        g_list_free(occupants);

        // role or affiliation filter
    } else {
        if (g_strcmp0(args[0], "moderator") == 0) {
            mucwin_show_role_list(mucwin, MUC_ROLE_MODERATOR);
            return;
        }
        if (g_strcmp0(args[0], "participant") == 0) {
            mucwin_show_role_list(mucwin, MUC_ROLE_PARTICIPANT);
            return;
        }
        if (g_strcmp0(args[0], "visitor") == 0) {
            mucwin_show_role_list(mucwin, MUC_ROLE_VISITOR);
            return;
        }

        if (g_strcmp0(args[0], "owner") == 0) {
            mucwin_show_affiliation_list(mucwin, MUC_AFFILIATION_OWNER);
            return;
        }
        if (g_strcmp0(args[0], "admin") == 0) {
            mucwin_show_affiliation_list(mucwin, MUC_AFFILIATION_ADMIN);
            return;
        }
        if (g_strcmp0(args[0], "member") == 0) {
            mucwin_show_affiliation_list(mucwin, MUC_AFFILIATION_MEMBER);
            return;
        }
        if (g_strcmp0(args[0], "outcast") == 0) {
            mucwin_show_affiliation_list(mucwin, MUC_AFFILIATION_OUTCAST);
            return;
        }
        if (g_strcmp0(args[0], "none") == 0) {
            mucwin_show_affiliation_list(mucwin, MUC_AFFILIATION_NONE);
            return;
        }
    }
}

static void
_who_roster(ProfWin* window, const char* const command, gchar** args)
{
    gchar* presence = args[0];

    // bad arg
    if (!_string_matches_one_of(NULL, presence, TRUE, "online", "available", "unavailable", "offline", "away", "chat", "xa", "dnd", "any", NULL)) {
        cons_bad_cmd_usage(command);
        return;
    }

    char* group = NULL;
    if ((g_strv_length(args) == 2) && args[1]) {
        group = args[1];
    }

    cons_show("");
    GSList* list = NULL;
    if (group) {
        list = roster_get_group(group, ROSTER_ORD_NAME);
        if (list == NULL) {
            cons_show("No such group: %s.", group);
            return;
        }
    } else {
        list = roster_get_contacts(ROSTER_ORD_NAME);
        if (list == NULL) {
            cons_show("No contacts in roster.");
            return;
        }
    }

    // no arg, show all contacts
    if ((presence == NULL) || (g_strcmp0(presence, "any") == 0)) {
        if (group) {
            if (list == NULL) {
                cons_show("No contacts in group %s.", group);
            } else {
                cons_show("%s:", group);
                cons_show_contacts(list);
            }
        } else {
            if (list == NULL) {
                cons_show("You have no contacts.");
            } else {
                cons_show("All contacts:");
                cons_show_contacts(list);
            }
        }

        // available
    } else if (strcmp("available", presence) == 0) {
        GSList* filtered = NULL;

        GSList* curr = list;
        while (curr) {
            PContact contact = curr->data;
            if (p_contact_is_available(contact)) {
                filtered = g_slist_append(filtered, contact);
            }
            curr = g_slist_next(curr);
        }

        if (group) {
            if (filtered == NULL) {
                cons_show("No contacts in group %s are %s.", group, presence);
            } else {
                cons_show("%s (%s):", group, presence);
                cons_show_contacts(filtered);
            }
        } else {
            if (filtered == NULL) {
                cons_show("No contacts are %s.", presence);
            } else {
                cons_show("Contacts (%s):", presence);
                cons_show_contacts(filtered);
            }
        }
        g_slist_free(filtered);

        // unavailable
    } else if (strcmp("unavailable", presence) == 0) {
        GSList* filtered = NULL;

        GSList* curr = list;
        while (curr) {
            PContact contact = curr->data;
            if (!p_contact_is_available(contact)) {
                filtered = g_slist_append(filtered, contact);
            }
            curr = g_slist_next(curr);
        }

        if (group) {
            if (filtered == NULL) {
                cons_show("No contacts in group %s are %s.", group, presence);
            } else {
                cons_show("%s (%s):", group, presence);
                cons_show_contacts(filtered);
            }
        } else {
            if (filtered == NULL) {
                cons_show("No contacts are %s.", presence);
            } else {
                cons_show("Contacts (%s):", presence);
                cons_show_contacts(filtered);
            }
        }
        g_slist_free(filtered);

        // online, available resources
    } else if (strcmp("online", presence) == 0) {
        GSList* filtered = NULL;

        GSList* curr = list;
        while (curr) {
            PContact contact = curr->data;
            if (p_contact_has_available_resource(contact)) {
                filtered = g_slist_append(filtered, contact);
            }
            curr = g_slist_next(curr);
        }

        if (group) {
            if (filtered == NULL) {
                cons_show("No contacts in group %s are %s.", group, presence);
            } else {
                cons_show("%s (%s):", group, presence);
                cons_show_contacts(filtered);
            }
        } else {
            if (filtered == NULL) {
                cons_show("No contacts are %s.", presence);
            } else {
                cons_show("Contacts (%s):", presence);
                cons_show_contacts(filtered);
            }
        }
        g_slist_free(filtered);

        // offline, no available resources
    } else if (strcmp("offline", presence) == 0) {
        GSList* filtered = NULL;

        GSList* curr = list;
        while (curr) {
            PContact contact = curr->data;
            if (!p_contact_has_available_resource(contact)) {
                filtered = g_slist_append(filtered, contact);
            }
            curr = g_slist_next(curr);
        }

        if (group) {
            if (filtered == NULL) {
                cons_show("No contacts in group %s are %s.", group, presence);
            } else {
                cons_show("%s (%s):", group, presence);
                cons_show_contacts(filtered);
            }
        } else {
            if (filtered == NULL) {
                cons_show("No contacts are %s.", presence);
            } else {
                cons_show("Contacts (%s):", presence);
                cons_show_contacts(filtered);
            }
        }
        g_slist_free(filtered);

        // show specific status
    } else {
        GSList* filtered = NULL;

        GSList* curr = list;
        while (curr) {
            PContact contact = curr->data;
            if (strcmp(p_contact_presence(contact), presence) == 0) {
                filtered = g_slist_append(filtered, contact);
            }
            curr = g_slist_next(curr);
        }

        if (group) {
            if (filtered == NULL) {
                cons_show("No contacts in group %s are %s.", group, presence);
            } else {
                cons_show("%s (%s):", group, presence);
                cons_show_contacts(filtered);
            }
        } else {
            if (filtered == NULL) {
                cons_show("No contacts are %s.", presence);
            } else {
                cons_show("Contacts (%s):", presence);
                cons_show_contacts(filtered);
            }
        }
        g_slist_free(filtered);
    }

    g_slist_free(list);
}

gboolean
cmd_who(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
    } else if (window->type == WIN_MUC) {
        _who_room(window, command, args);
    } else {
        _who_roster(window, command, args);
    }

    if (window->type != WIN_CONSOLE && window->type != WIN_MUC) {
        status_bar_new(1, WIN_CONSOLE, "console");
    }

    return TRUE;
}

static void
_cmd_msg_chatwin(const char* const barejid, const char* const msg)
{
    ProfChatWin* chatwin = wins_get_chat(barejid);
    if (!chatwin) {
        // NOTE: This will also start the new OMEMO session and send a MAM request.
        chatwin = chatwin_new(barejid);
    }
    ui_focus_win((ProfWin*)chatwin);

    if (msg) {
        // NOTE: In case the message is OMEMO encrypted, we can't be sure
        // whether the key bundles of the recipient have already been
        // received. In the case that *no* bundles have been received yet,
        // the message won't be sent, and an error is shown to the user.
        // Other cases are not handled here.
        cl_ev_send_msg(chatwin, msg, NULL);
    } else {
#ifdef HAVE_LIBOTR
        // Start the OTR session after this (i.e. the first) message was sent
        if (otr_is_secure(barejid)) {
            chatwin_otr_secured(chatwin, otr_is_trusted(barejid));
        }
#endif // HAVE_LIBOTR
    }
}

gboolean
cmd_msg(ProfWin* window, const char* const command, gchar** args)
{
    gchar* usr = args[0];
    gchar* msg = args[1];

    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    // send private message when in MUC room
    if (window->type == WIN_MUC) {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

        Occupant* occupant = muc_roster_item(mucwin->roomjid, usr);
        if (occupant) {
            // in case of non-anon muc send regular chatmessage
            if (muc_anonymity_type(mucwin->roomjid) == MUC_ANONYMITY_TYPE_NONANONYMOUS) {
                auto_jid Jid* jidp = jid_create(occupant->jid);

                _cmd_msg_chatwin(jidp->barejid, msg);
                win_println(window, THEME_DEFAULT, "-", "Starting direct message with occupant \"%s\" from room \"%s\" as \"%s\".", usr, mucwin->roomjid, jidp->barejid);
                cons_show("Starting direct message with occupant \"%s\" from room \"%s\" as \"%s\".", usr, mucwin->roomjid, jidp->barejid);
            } else {
                // otherwise send mucpm
                auto_gchar gchar* full_jid = g_strdup_printf("%s/%s", mucwin->roomjid, usr);
                ProfPrivateWin* privwin = wins_get_private(full_jid);
                if (!privwin) {
                    privwin = (ProfPrivateWin*)wins_new_private(full_jid);
                }
                ui_focus_win((ProfWin*)privwin);

                if (msg) {
                    cl_ev_send_priv_msg(privwin, msg, NULL);
                }
            }

        } else {
            win_println(window, THEME_DEFAULT, "-", "No such participant \"%s\" in room.", usr);
        }

        return TRUE;

        // send chat message
    } else {
        char* barejid = roster_barejid_from_name(usr);
        if (barejid == NULL) {
            barejid = usr;
        }

        _cmd_msg_chatwin(barejid, msg);

        return TRUE;
    }
}

gboolean
cmd_group(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    // list all groups
    if (args[1] == NULL) {
        GList* groups = roster_get_groups();
        GList* curr = groups;
        if (curr) {
            cons_show("Groups:");
            while (curr) {
                cons_show("  %s", curr->data);
                curr = g_list_next(curr);
            }

            g_list_free_full(groups, g_free);
        } else {
            cons_show("No groups.");
        }
        return TRUE;
    }

    // show contacts in group
    if (strcmp(args[1], "show") == 0) {
        gchar* group = args[2];
        if (group == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        GSList* list = roster_get_group(group, ROSTER_ORD_NAME);
        cons_show_roster_group(group, list);
        return TRUE;
    }

    // add contact to group
    if (strcmp(args[1], "add") == 0) {
        gchar* group = args[2];
        gchar* contact = args[3];

        if ((group == NULL) || (contact == NULL)) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        char* barejid = roster_barejid_from_name(contact);
        if (barejid == NULL) {
            barejid = contact;
        }

        PContact pcontact = roster_get_contact(barejid);
        if (pcontact == NULL) {
            cons_show("Contact not found in roster: %s", barejid);
            return TRUE;
        }

        if (p_contact_in_group(pcontact, group)) {
            const char* display_name = p_contact_name_or_jid(pcontact);
            ui_contact_already_in_group(display_name, group);
        } else {
            roster_send_add_to_group(group, pcontact);
        }

        return TRUE;
    }

    // remove contact from group
    if (strcmp(args[1], "remove") == 0) {
        gchar* group = args[2];
        gchar* contact = args[3];

        if ((group == NULL) || (contact == NULL)) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        char* barejid = roster_barejid_from_name(contact);
        if (barejid == NULL) {
            barejid = contact;
        }

        PContact pcontact = roster_get_contact(barejid);
        if (pcontact == NULL) {
            cons_show("Contact not found in roster: %s", barejid);
            return TRUE;
        }

        if (!p_contact_in_group(pcontact, group)) {
            const char* display_name = p_contact_name_or_jid(pcontact);
            ui_contact_not_in_group(display_name, group);
        } else {
            roster_send_remove_from_group(group, pcontact);
        }

        return TRUE;
    }

    cons_bad_cmd_usage(command);
    return TRUE;
}

gboolean
cmd_roster(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    // show roster
    if (args[0] == NULL) {
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }

        GSList* list = roster_get_contacts(ROSTER_ORD_NAME);
        cons_show_roster(list);
        g_slist_free(list);
        return TRUE;

        // show roster, only online contacts
    } else if (g_strcmp0(args[0], "online") == 0) {
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }

        GSList* list = roster_get_contacts_online();
        cons_show_roster(list);
        g_slist_free(list);
        return TRUE;

        // set roster size
    } else if (g_strcmp0(args[0], "size") == 0) {
        if (!args[1]) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
        int intval = 0;
        auto_char char* err_msg = NULL;
        gboolean res = strtoi_range(args[1], &intval, 1, 99, &err_msg);
        if (res) {
            prefs_set_roster_size(intval);
            cons_show("Roster screen size set to: %d%%", intval);
            if (conn_status == JABBER_CONNECTED && prefs_get_boolean(PREF_ROSTER)) {
                wins_resize_all();
            }
            return TRUE;
        } else {
            cons_show(err_msg);
            return TRUE;
        }

        // set line wrapping
    } else if (g_strcmp0(args[0], "wrap") == 0) {
        if (!args[1]) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            _cmd_set_boolean_preference(args[1], "Roster panel line wrap", PREF_ROSTER_WRAP);
            rosterwin_roster();
            return TRUE;
        }

        // header settings
    } else if (g_strcmp0(args[0], "header") == 0) {
        if (g_strcmp0(args[1], "char") == 0) {
            if (!args[2]) {
                cons_bad_cmd_usage(command);
            } else if (g_strcmp0(args[2], "none") == 0) {
                prefs_clear_roster_header_char();
                cons_show("Roster header char removed.");
                rosterwin_roster();
            } else {
                prefs_set_roster_header_char(args[2]);
                cons_show("Roster header char set to %s.", args[2]);
                rosterwin_roster();
            }
        } else {
            cons_bad_cmd_usage(command);
        }
        return TRUE;

        // contact settings
    } else if (g_strcmp0(args[0], "contact") == 0) {
        if (g_strcmp0(args[1], "char") == 0) {
            if (!args[2]) {
                cons_bad_cmd_usage(command);
            } else if (g_strcmp0(args[2], "none") == 0) {
                prefs_clear_roster_contact_char();
                cons_show("Roster contact char removed.");
                rosterwin_roster();
            } else {
                prefs_set_roster_contact_char(args[2]);
                cons_show("Roster contact char set to %s.", args[2]);
                rosterwin_roster();
            }
        } else if (g_strcmp0(args[1], "indent") == 0) {
            if (!args[2]) {
                cons_bad_cmd_usage(command);
            } else {
                int intval = 0;
                auto_char char* err_msg = NULL;
                gboolean res = strtoi_range(args[2], &intval, 0, 10, &err_msg);
                if (res) {
                    prefs_set_roster_contact_indent(intval);
                    cons_show("Roster contact indent set to: %d", intval);
                    rosterwin_roster();
                } else {
                    cons_show(err_msg);
                }
            }
        } else {
            cons_bad_cmd_usage(command);
        }
        return TRUE;

        // resource settings
    } else if (g_strcmp0(args[0], "resource") == 0) {
        if (g_strcmp0(args[1], "char") == 0) {
            if (!args[2]) {
                cons_bad_cmd_usage(command);
            } else if (g_strcmp0(args[2], "none") == 0) {
                prefs_clear_roster_resource_char();
                cons_show("Roster resource char removed.");
                rosterwin_roster();
            } else {
                prefs_set_roster_resource_char(args[2]);
                cons_show("Roster resource char set to %s.", args[2]);
                rosterwin_roster();
            }
        } else if (g_strcmp0(args[1], "indent") == 0) {
            if (!args[2]) {
                cons_bad_cmd_usage(command);
            } else {
                int intval = 0;
                auto_char char* err_msg = NULL;
                gboolean res = strtoi_range(args[2], &intval, 0, 10, &err_msg);
                if (res) {
                    prefs_set_roster_resource_indent(intval);
                    cons_show("Roster resource indent set to: %d", intval);
                    rosterwin_roster();
                } else {
                    cons_show(err_msg);
                }
            }
        } else if (g_strcmp0(args[1], "join") == 0) {
            _cmd_set_boolean_preference(args[2], "Roster join", PREF_ROSTER_RESOURCE_JOIN);
            rosterwin_roster();
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
        }
        return TRUE;

        // presence settings
    } else if (g_strcmp0(args[0], "presence") == 0) {
        if (g_strcmp0(args[1], "indent") == 0) {
            if (!args[2]) {
                cons_bad_cmd_usage(command);
            } else {
                int intval = 0;
                auto_char char* err_msg = NULL;
                gboolean res = strtoi_range(args[2], &intval, -1, 10, &err_msg);
                if (res) {
                    prefs_set_roster_presence_indent(intval);
                    cons_show("Roster presence indent set to: %d", intval);
                    rosterwin_roster();
                } else {
                    cons_show(err_msg);
                }
            }
        } else {
            cons_bad_cmd_usage(command);
        }
        return TRUE;

        // show/hide roster
    } else if ((g_strcmp0(args[0], "show") == 0) || (g_strcmp0(args[0], "hide") == 0)) {
        preference_t pref;
        const char* pref_str;
        if (args[1] == NULL) {
            pref = PREF_ROSTER;
            pref_str = "";
        } else if (g_strcmp0(args[1], "offline") == 0) {
            pref = PREF_ROSTER_OFFLINE;
            pref_str = "offline";
        } else if (g_strcmp0(args[1], "resource") == 0) {
            pref = PREF_ROSTER_RESOURCE;
            pref_str = "resource";
        } else if (g_strcmp0(args[1], "presence") == 0) {
            pref = PREF_ROSTER_PRESENCE;
            pref_str = "presence";
        } else if (g_strcmp0(args[1], "status") == 0) {
            pref = PREF_ROSTER_STATUS;
            pref_str = "status";
        } else if (g_strcmp0(args[1], "empty") == 0) {
            pref = PREF_ROSTER_EMPTY;
            pref_str = "empty";
        } else if (g_strcmp0(args[1], "priority") == 0) {
            pref = PREF_ROSTER_PRIORITY;
            pref_str = "priority";
        } else if (g_strcmp0(args[1], "contacts") == 0) {
            pref = PREF_ROSTER_CONTACTS;
            pref_str = "contacts";
        } else if (g_strcmp0(args[1], "rooms") == 0) {
            pref = PREF_ROSTER_ROOMS;
            pref_str = "rooms";
        } else if (g_strcmp0(args[1], "unsubscribed") == 0) {
            pref = PREF_ROSTER_UNSUBSCRIBED;
            pref_str = "unsubscribed";
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        gboolean val;
        if (g_strcmp0(args[0], "show") == 0) {
            val = TRUE;
        } else { // "hide"
            val = FALSE;
        }

        cons_show("Roster%s%s %s (was %s)", strlen(pref_str) == 0 ? "" : " ", pref_str,
                  val == TRUE ? "enabled" : "disabled",
                  prefs_get_boolean(pref) == TRUE ? "enabled" : "disabled");
        prefs_set_boolean(pref, val);
        if (conn_status == JABBER_CONNECTED) {
            if (pref == PREF_ROSTER) {
                if (val == TRUE) {
                    ui_show_roster();
                } else {
                    ui_hide_roster();
                }
            } else {
                rosterwin_roster();
            }
        }
        return TRUE;

        // roster grouping
    } else if (g_strcmp0(args[0], "by") == 0) {
        if (g_strcmp0(args[1], "group") == 0) {
            cons_show("Grouping roster by roster group");
            prefs_set_string(PREF_ROSTER_BY, "group");
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "presence") == 0) {
            cons_show("Grouping roster by presence");
            prefs_set_string(PREF_ROSTER_BY, "presence");
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "none") == 0) {
            cons_show("Roster grouping disabled");
            prefs_set_string(PREF_ROSTER_BY, "none");
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        // roster item order
    } else if (g_strcmp0(args[0], "order") == 0) {
        if (g_strcmp0(args[1], "name") == 0) {
            cons_show("Ordering roster by name");
            prefs_set_string(PREF_ROSTER_ORDER, "name");
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "presence") == 0) {
            cons_show("Ordering roster by presence");
            prefs_set_string(PREF_ROSTER_ORDER, "presence");
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

    } else if (g_strcmp0(args[0], "count") == 0) {
        if (g_strcmp0(args[1], "zero") == 0) {
            _cmd_set_boolean_preference(args[2], "Roster header zero count", PREF_ROSTER_COUNT_ZERO);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "unread") == 0) {
            cons_show("Roster header count set to unread");
            prefs_set_string(PREF_ROSTER_COUNT, "unread");
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "items") == 0) {
            cons_show("Roster header count set to items");
            prefs_set_string(PREF_ROSTER_COUNT, "items");
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            cons_show("Disabling roster header count");
            prefs_set_string(PREF_ROSTER_COUNT, "off");
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

    } else if (g_strcmp0(args[0], "color") == 0) {
        _cmd_set_boolean_preference(args[1], "Roster consistent colors", PREF_ROSTER_COLOR_NICK);
        ui_show_roster();
        return TRUE;

    } else if (g_strcmp0(args[0], "unread") == 0) {
        if (g_strcmp0(args[1], "before") == 0) {
            cons_show("Roster unread message count: before");
            prefs_set_string(PREF_ROSTER_UNREAD, "before");
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "after") == 0) {
            cons_show("Roster unread message count: after");
            prefs_set_string(PREF_ROSTER_UNREAD, "after");
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            cons_show("Roster unread message count: off");
            prefs_set_string(PREF_ROSTER_UNREAD, "off");
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

    } else if (g_strcmp0(args[0], "private") == 0) {
        if (g_strcmp0(args[1], "char") == 0) {
            if (!args[2]) {
                cons_bad_cmd_usage(command);
            } else if (g_strcmp0(args[2], "none") == 0) {
                prefs_clear_roster_private_char();
                cons_show("Roster private room chat char removed.");
                rosterwin_roster();
            } else {
                prefs_set_roster_private_char(args[2]);
                cons_show("Roster private room chat char set to %s.", args[2]);
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "room") == 0) {
            cons_show("Showing room private chats under room.");
            prefs_set_string(PREF_ROSTER_PRIVATE, "room");
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "group") == 0) {
            cons_show("Showing room private chats as roster group.");
            prefs_set_string(PREF_ROSTER_PRIVATE, "group");
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            cons_show("Hiding room private chats in roster.");
            prefs_set_string(PREF_ROSTER_PRIVATE, "off");
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

    } else if (g_strcmp0(args[0], "room") == 0) {
        if (g_strcmp0(args[1], "char") == 0) {
            if (!args[2]) {
                cons_bad_cmd_usage(command);
            } else if (g_strcmp0(args[2], "none") == 0) {
                prefs_clear_roster_room_char();
                cons_show("Roster room char removed.");
                rosterwin_roster();
            } else {
                prefs_set_roster_room_char(args[2]);
                cons_show("Roster room char set to %s.", args[2]);
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "position") == 0) {
            if (g_strcmp0(args[2], "first") == 0) {
                cons_show("Showing rooms first in roster.");
                prefs_set_string(PREF_ROSTER_ROOMS_POS, "first");
                if (conn_status == JABBER_CONNECTED) {
                    rosterwin_roster();
                }
                return TRUE;
            } else if (g_strcmp0(args[2], "last") == 0) {
                cons_show("Showing rooms last in roster.");
                prefs_set_string(PREF_ROSTER_ROOMS_POS, "last");
                if (conn_status == JABBER_CONNECTED) {
                    rosterwin_roster();
                }
                return TRUE;
            } else {
                cons_bad_cmd_usage(command);
                return TRUE;
            }
        } else if (g_strcmp0(args[1], "order") == 0) {
            if (g_strcmp0(args[2], "name") == 0) {
                cons_show("Ordering roster rooms by name");
                prefs_set_string(PREF_ROSTER_ROOMS_ORDER, "name");
                if (conn_status == JABBER_CONNECTED) {
                    rosterwin_roster();
                }
                return TRUE;
            } else if (g_strcmp0(args[2], "unread") == 0) {
                cons_show("Ordering roster rooms by unread messages");
                prefs_set_string(PREF_ROSTER_ROOMS_ORDER, "unread");
                if (conn_status == JABBER_CONNECTED) {
                    rosterwin_roster();
                }
                return TRUE;
            } else {
                cons_bad_cmd_usage(command);
                return TRUE;
            }
        } else if (g_strcmp0(args[1], "unread") == 0) {
            if (g_strcmp0(args[2], "before") == 0) {
                cons_show("Roster rooms unread message count: before");
                prefs_set_string(PREF_ROSTER_ROOMS_UNREAD, "before");
                if (conn_status == JABBER_CONNECTED) {
                    rosterwin_roster();
                }
                return TRUE;
            } else if (g_strcmp0(args[2], "after") == 0) {
                cons_show("Roster rooms unread message count: after");
                prefs_set_string(PREF_ROSTER_ROOMS_UNREAD, "after");
                if (conn_status == JABBER_CONNECTED) {
                    rosterwin_roster();
                }
                return TRUE;
            } else if (g_strcmp0(args[2], "off") == 0) {
                cons_show("Roster rooms unread message count: off");
                prefs_set_string(PREF_ROSTER_ROOMS_UNREAD, "off");
                if (conn_status == JABBER_CONNECTED) {
                    rosterwin_roster();
                }
                return TRUE;
            } else {
                cons_bad_cmd_usage(command);
                return TRUE;
            }
        } else if (g_strcmp0(args[1], "private") == 0) {
            if (g_strcmp0(args[2], "char") == 0) {
                if (!args[3]) {
                    cons_bad_cmd_usage(command);
                } else if (g_strcmp0(args[3], "none") == 0) {
                    prefs_clear_roster_room_private_char();
                    cons_show("Roster room private char removed.");
                    rosterwin_roster();
                } else {
                    prefs_set_roster_room_private_char(args[3]);
                    cons_show("Roster room private char set to %s.", args[3]);
                    rosterwin_roster();
                }
                return TRUE;
            } else {
                cons_bad_cmd_usage(command);
                return TRUE;
            }
        } else if (g_strcmp0(args[1], "by") == 0) {
            if (g_strcmp0(args[2], "service") == 0) {
                cons_show("Grouping rooms by service");
                prefs_set_string(PREF_ROSTER_ROOMS_BY, "service");
                if (conn_status == JABBER_CONNECTED) {
                    rosterwin_roster();
                }
                return TRUE;
            } else if (g_strcmp0(args[2], "none") == 0) {
                cons_show("Roster room grouping disabled");
                prefs_set_string(PREF_ROSTER_ROOMS_BY, "none");
                if (conn_status == JABBER_CONNECTED) {
                    rosterwin_roster();
                }
                return TRUE;
            } else {
                cons_bad_cmd_usage(command);
                return TRUE;
            }
        } else if (g_strcmp0(args[1], "title") == 0) {
            if (g_strcmp0(args[2], "bookmark") == 0 || g_strcmp0(args[2], "jid") == 0 || g_strcmp0(args[2], "localpart") == 0 || g_strcmp0(args[2], "name") == 0) {
                cons_show("Roster MUCs will display '%s' as their title.", args[2]);
                prefs_set_string(PREF_ROSTER_ROOMS_TITLE, args[2]);
                if (conn_status == JABBER_CONNECTED) {
                    rosterwin_roster();
                }
                return TRUE;
            }
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        // add contact
    } else if (strcmp(args[0], "add") == 0) {
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }
        gchar* jid = args[1];
        if (jid == NULL) {
            cons_bad_cmd_usage(command);
        } else {
            gchar* name = args[2];
            roster_send_add_new(jid, name);
        }
        return TRUE;

        // remove contact
    } else if (strcmp(args[0], "remove") == 0) {
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }
        gchar* usr = args[1];
        if (usr == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
        char* barejid = roster_barejid_from_name(usr);
        if (barejid == NULL) {
            barejid = usr;
        }
        roster_send_remove(barejid);
        return TRUE;
    } else if (strcmp(args[0], "remove_all") == 0) {
        if (g_strcmp0(args[1], "contacts") != 0) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }

        GSList* all = roster_get_contacts(ROSTER_ORD_NAME);
        GSList* curr = all;
        while (curr) {
            PContact contact = curr->data;
            roster_send_remove(p_contact_barejid(contact));
            curr = g_slist_next(curr);
        }

        g_slist_free(all);
        return TRUE;

        // change nickname
    } else if (strcmp(args[0], "nick") == 0) {
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }
        gchar* jid = args[1];
        if (jid == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        gchar* name = args[2];
        if (name == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        // contact does not exist
        PContact contact = roster_get_contact(jid);
        if (contact == NULL) {
            cons_show("Contact not found in roster: %s", jid);
            return TRUE;
        }

        const char* barejid = p_contact_barejid(contact);

        // TODO wait for result stanza before updating
        const char* oldnick = p_contact_name(contact);
        wins_change_nick(barejid, oldnick, name);
        roster_change_name(contact, name);
        GSList* groups = p_contact_groups(contact);
        roster_send_name_change(barejid, name, groups);

        cons_show("Nickname for %s set to: %s.", jid, name);

        return TRUE;

        // remove nickname
    } else if (strcmp(args[0], "clearnick") == 0) {
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }
        gchar* jid = args[1];
        if (jid == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        // contact does not exist
        PContact contact = roster_get_contact(jid);
        if (contact == NULL) {
            cons_show("Contact not found in roster: %s", jid);
            return TRUE;
        }

        const char* barejid = p_contact_barejid(contact);

        // TODO wait for result stanza before updating
        const char* oldnick = p_contact_name(contact);
        wins_remove_nick(barejid, oldnick);
        roster_change_name(contact, NULL);
        GSList* groups = p_contact_groups(contact);
        roster_send_name_change(barejid, NULL, groups);

        cons_show("Nickname for %s removed.", jid);

        return TRUE;
    } else {
        cons_bad_cmd_usage(command);
        return TRUE;
    }
}

gboolean
cmd_blocked(ProfWin* window, const char* const command, gchar** args)
{
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (!connection_supports(XMPP_FEATURE_BLOCKING)) {
        cons_show("Blocking (%s) not supported by server.", XMPP_FEATURE_BLOCKING);
        return TRUE;
    }

    blocked_report br = BLOCKED_NO_REPORT;

    if (g_strcmp0(args[0], "add") == 0) {
        gchar* jid = args[1];

        if (jid == NULL && (window->type == WIN_CHAT)) {
            ProfChatWin* chatwin = (ProfChatWin*)window;
            jid = chatwin->barejid;
        }

        if (jid == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        gboolean res = blocked_add(jid, br, NULL);
        if (!res) {
            cons_show("User %s already blocked.", jid);
        }

        return TRUE;
    }

    if (g_strcmp0(args[0], "remove") == 0) {
        if (args[1] == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        gboolean res = blocked_remove(args[1]);
        if (!res) {
            cons_show("User %s is not currently blocked.", args[1]);
        }

        return TRUE;
    }

    if (args[0] && strncmp(args[0], "report-", 7) == 0) {
        char* jid = NULL;
        char* msg = NULL;
        guint argn = g_strv_length(args);

        if (argn >= 2) {
            jid = args[1];
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        if (argn >= 3) {
            msg = args[2];
        }

        if (args[1] && g_strcmp0(args[0], "report-abuse") == 0) {
            br = BLOCKED_REPORT_ABUSE;
        } else if (args[1] && g_strcmp0(args[0], "report-spam") == 0) {
            br = BLOCKED_REPORT_SPAM;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        if (!connection_supports(XMPP_FEATURE_SPAM_REPORTING)) {
            cons_show("Spam reporting (%s) not supported by server.", XMPP_FEATURE_SPAM_REPORTING);
            return TRUE;
        }

        gboolean res = blocked_add(jid, br, msg);
        if (!res) {
            cons_show("User %s already blocked.", args[1]);
        }
        return TRUE;
    }

    GList* blocked = blocked_list();
    GList* curr = blocked;
    if (curr) {
        cons_show("Blocked users:");
        while (curr) {
            cons_show("  %s", curr->data);
            curr = g_list_next(curr);
        }
    } else {
        cons_show("No blocked users.");
    }

    return TRUE;
}

gboolean
cmd_resource(ProfWin* window, const char* const command, gchar** args)
{
    gchar* cmd = args[0];
    gchar* setting = NULL;
    if (g_strcmp0(cmd, "message") == 0) {
        setting = args[1];
        if (!setting) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            _cmd_set_boolean_preference(setting, "Message resource", PREF_RESOURCE_MESSAGE);
            return TRUE;
        }
    } else if (g_strcmp0(cmd, "title") == 0) {
        setting = args[1];
        if (!setting) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            _cmd_set_boolean_preference(setting, "Title resource", PREF_RESOURCE_TITLE);
            return TRUE;
        }
    }

    if (window->type != WIN_CHAT) {
        cons_show("Resource can only be changed in chat windows.");
        return TRUE;
    }

    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    ProfChatWin* chatwin = (ProfChatWin*)window;

    if (g_strcmp0(cmd, "set") == 0) {
        gchar* resource = args[1];
        if (!resource) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

#ifdef HAVE_LIBOTR
        if (otr_is_secure(chatwin->barejid)) {
            cons_show("Cannot choose resource during an OTR session.");
            return TRUE;
        }
#endif

        PContact contact = roster_get_contact(chatwin->barejid);
        if (!contact) {
            cons_show("Cannot choose resource for contact not in roster.");
            return TRUE;
        }

        if (!p_contact_get_resource(contact, resource)) {
            cons_show("No such resource %s.", resource);
            return TRUE;
        }

        chatwin->resource_override = strdup(resource);
        chat_state_free(chatwin->state);
        chatwin->state = chat_state_new();
        chat_session_resource_override(chatwin->barejid, resource);
        return TRUE;

    } else if (g_strcmp0(cmd, "off") == 0) {
        FREE_SET_NULL(chatwin->resource_override);
        chat_state_free(chatwin->state);
        chatwin->state = chat_state_new();
        chat_session_remove(chatwin->barejid);
        return TRUE;
    } else {
        cons_bad_cmd_usage(command);
        return TRUE;
    }
}

static void
_cmd_status_show_status(char* usr)
{
    char* usr_jid = roster_barejid_from_name(usr);
    if (usr_jid == NULL) {
        usr_jid = usr;
    }
    cons_show_status(usr_jid);
}

gboolean
cmd_status_set(ProfWin* window, const char* const command, gchar** args)
{
    gchar* state = args[1];

    if (g_strcmp0(state, "online") == 0) {
        _update_presence(RESOURCE_ONLINE, "online", args);
    } else if (g_strcmp0(state, "away") == 0) {
        _update_presence(RESOURCE_AWAY, "away", args);
    } else if (g_strcmp0(state, "dnd") == 0) {
        _update_presence(RESOURCE_DND, "dnd", args);
    } else if (g_strcmp0(state, "chat") == 0) {
        _update_presence(RESOURCE_CHAT, "chat", args);
    } else if (g_strcmp0(state, "xa") == 0) {
        _update_presence(RESOURCE_XA, "xa", args);
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_status_get(ProfWin* window, const char* const command, gchar** args)
{
    gchar* usr = args[1];

    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    switch (window->type) {
    case WIN_MUC:
        if (usr) {
            ProfMucWin* mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            Occupant* occupant = muc_roster_item(mucwin->roomjid, usr);
            if (occupant) {
                win_show_occupant(window, occupant);
            } else {
                win_println(window, THEME_DEFAULT, "-", "No such participant \"%s\" in room.", usr);
            }
        } else {
            win_println(window, THEME_DEFAULT, "-", "You must specify a nickname.");
        }
        break;
    case WIN_CHAT:
        if (usr) {
            _cmd_status_show_status(usr);
        } else {
            ProfChatWin* chatwin = (ProfChatWin*)window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
            PContact pcontact = roster_get_contact(chatwin->barejid);
            if (pcontact) {
                win_show_contact(window, pcontact);
            } else {
                win_println(window, THEME_DEFAULT, "-", "Error getting contact info.");
            }
        }
        break;
    case WIN_PRIVATE:
        if (usr) {
            _cmd_status_show_status(usr);
        } else {
            ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
            assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
            auto_jid Jid* jid = jid_create(privatewin->fulljid);
            Occupant* occupant = muc_roster_item(jid->barejid, jid->resourcepart);
            if (occupant) {
                win_show_occupant(window, occupant);
            } else {
                win_println(window, THEME_DEFAULT, "-", "Error getting contact info.");
            }
        }
        break;
    case WIN_CONSOLE:
        if (usr) {
            _cmd_status_show_status(usr);
        } else {
            cons_bad_cmd_usage(command);
        }
        break;
    default:
        break;
    }

    return TRUE;
}

static void
_cmd_info_show_contact(char* usr)
{
    char* usr_jid = roster_barejid_from_name(usr);
    if (usr_jid == NULL) {
        usr_jid = usr;
    }
    PContact pcontact = roster_get_contact(usr_jid);
    if (pcontact) {
        cons_show_info(pcontact);
    } else {
        cons_show("No such contact \"%s\" in roster.", usr);
    }
}

gboolean
cmd_info(ProfWin* window, const char* const command, gchar** args)
{
    gchar* usr = args[0];

    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    switch (window->type) {
    case WIN_MUC:
        if (usr) {
            ProfMucWin* mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            Occupant* occupant = muc_roster_item(mucwin->roomjid, usr);
            if (occupant) {
                win_show_occupant_info(window, mucwin->roomjid, occupant);
            } else {
                win_println(window, THEME_DEFAULT, "-", "No such occupant \"%s\" in room.", usr);
            }
        } else {
            ProfMucWin* mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            iq_room_info_request(mucwin->roomjid, TRUE);
            mucwin_info(mucwin);
            return TRUE;
        }
        break;
    case WIN_CHAT:
        if (usr) {
            _cmd_info_show_contact(usr);
        } else {
            ProfChatWin* chatwin = (ProfChatWin*)window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
            PContact pcontact = roster_get_contact(chatwin->barejid);
            if (pcontact) {
                win_show_info(window, pcontact);
            } else {
                win_println(window, THEME_DEFAULT, "-", "Error getting contact info.");
            }
        }
        break;
    case WIN_PRIVATE:
        if (usr) {
            _cmd_info_show_contact(usr);
        } else {
            ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
            assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
            auto_jid Jid* jid = jid_create(privatewin->fulljid);
            Occupant* occupant = muc_roster_item(jid->barejid, jid->resourcepart);
            if (occupant) {
                win_show_occupant_info(window, jid->barejid, occupant);
            } else {
                win_println(window, THEME_DEFAULT, "-", "Error getting contact info.");
            }
        }
        break;
    case WIN_CONSOLE:
        if (usr) {
            _cmd_info_show_contact(usr);
        } else {
            cons_bad_cmd_usage(command);
        }
        break;
    default:
        break;
    }

    return TRUE;
}

gboolean
cmd_caps(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();
    Occupant* occupant = NULL;

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    switch (window->type) {
    case WIN_MUC:
        if (args[0]) {
            ProfMucWin* mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            occupant = muc_roster_item(mucwin->roomjid, args[0]);
            if (occupant) {
                auto_jid Jid* jidp = jid_create_from_bare_and_resource(mucwin->roomjid, args[0]);
                cons_show_caps(jidp->fulljid, occupant->presence);
            } else {
                cons_show("No such participant \"%s\" in room.", args[0]);
            }
        } else {
            cons_show("No nickname supplied to /caps in chat room.");
        }
        break;
    case WIN_CHAT:
    case WIN_CONSOLE:
        if (args[0]) {
            auto_jid Jid* jid = jid_create(args[0]);

            if (jid->fulljid == NULL) {
                cons_show("You must provide a full jid to the /caps command.");
            } else {
                PContact pcontact = roster_get_contact(jid->barejid);
                if (pcontact == NULL) {
                    cons_show("Contact not found in roster: %s", jid->barejid);
                } else {
                    Resource* resource = p_contact_get_resource(pcontact, jid->resourcepart);
                    if (resource == NULL) {
                        cons_show("Could not find resource %s, for contact %s", jid->barejid, jid->resourcepart);
                    } else {
                        cons_show_caps(jid->fulljid, resource->presence);
                    }
                }
            }
        } else {
            cons_show("You must provide a jid to the /caps command.");
        }
        break;
    case WIN_PRIVATE:
        if (args[0]) {
            cons_show("No parameter needed to /caps when in private chat.");
        } else {
            ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
            assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
            auto_jid Jid* jid = jid_create(privatewin->fulljid);
            if (jid) {
                occupant = muc_roster_item(jid->barejid, jid->resourcepart);
                cons_show_caps(jid->resourcepart, occupant->presence);
            }
        }
        break;
    default:
        break;
    }

    return TRUE;
}

static void
_send_software_version_iq_to_fulljid(char* request)
{
    auto_jid Jid* jid = jid_create(request);

    if (jid == NULL || jid->fulljid == NULL) {
        cons_show("You must provide a full jid to the /software command.");
    } else if (equals_our_barejid(jid->barejid)) {
        cons_show("Cannot request software version for yourself.");
    } else {
        iq_send_software_version(jid->fulljid);
    }
}

gboolean
cmd_software(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    switch (window->type) {
    case WIN_MUC:
        if (args[0]) {
            ProfMucWin* mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            Occupant* occupant = muc_roster_item(mucwin->roomjid, args[0]);
            if (occupant) {
                auto_jid Jid* jid = jid_create_from_bare_and_resource(mucwin->roomjid, args[0]);
                iq_send_software_version(jid->fulljid);
            } else {
                cons_show("No such participant \"%s\" in room.", args[0]);
            }
        } else {
            cons_show("No nickname supplied to /software in chat room.");
        }
        break;
    case WIN_CHAT:
        if (args[0]) {
            _send_software_version_iq_to_fulljid(args[0]);
            break;
        } else {
            ProfChatWin* chatwin = (ProfChatWin*)window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);

            char* resource = NULL;
            ChatSession* session = chat_session_get(chatwin->barejid);
            if (chatwin->resource_override) {
                resource = chatwin->resource_override;
            } else if (session && session->resource) {
                resource = session->resource;
            }

            if (resource) {
                auto_gchar gchar* fulljid = g_strdup_printf("%s/%s", chatwin->barejid, resource);
                iq_send_software_version(fulljid);
            } else {
                win_println(window, THEME_DEFAULT, "-", "Unknown resource for /software command. See /help resource.");
            }
            break;
        }
    case WIN_CONSOLE:
        if (args[0]) {
            _send_software_version_iq_to_fulljid(args[0]);
        } else {
            cons_show("You must provide a jid to the /software command.");
        }
        break;
    case WIN_PRIVATE:
        if (args[0]) {
            cons_show("No parameter needed to /software when in private chat.");
        } else {
            ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
            assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
            iq_send_software_version(privatewin->fulljid);
        }
        break;
    default:
        break;
    }

    return TRUE;
}

gboolean
cmd_serversoftware(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (args[0]) {
        iq_send_software_version(args[0]);
    } else {
        cons_show("You must provide a jid to the /serversoftware command.");
    }

    return TRUE;
}

gboolean
cmd_join(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (args[0] == NULL) {
        char* account_name = session_get_account_name();
        ProfAccount* account = accounts_get_account(account_name);
        if (account && account->muc_service) {
            char* uuid = connection_create_uuid();
            auto_gchar gchar* room_str = g_strdup_printf("private-chat-%s@%s", uuid, account->muc_service);
            connection_free_uuid(uuid);

            presence_join_room(room_str, account->muc_nick, NULL);
            muc_join(room_str, account->muc_nick, NULL, FALSE);
        } else {
            cons_show("Account MUC service property not found.");
        }
        account_free(account);

        return TRUE;
    }

    auto_jid Jid* room_arg = jid_create(args[0]);
    if (room_arg == NULL) {
        cons_show_error("Specified room has incorrect format.");
        cons_show("");
        return TRUE;
    }

    auto_gchar gchar* room = NULL;
    char* nick = NULL;
    char* passwd = NULL;
    char* account_name = session_get_account_name();
    ProfAccount* account = accounts_get_account(account_name);

    // full room jid supplied (room@server)
    if (room_arg->localpart) {
        room = g_strdup(args[0]);

        // server not supplied (room), use account preference
    } else if (account->muc_service) {
        room = g_strdup_printf("%s@%s", args[0], account->muc_service);

        // no account preference
    } else {
        cons_show("Account MUC service property not found.");
        return TRUE;
    }

    // Additional args supplied
    gchar* opt_keys[] = { "nick", "password", NULL };
    gboolean parsed;

    GHashTable* options = parse_options(&args[1], opt_keys, &parsed);
    if (!parsed) {
        cons_bad_cmd_usage(command);
        cons_show("");
        return TRUE;
    }

    nick = g_hash_table_lookup(options, "nick");
    passwd = g_hash_table_lookup(options, "password");

    options_destroy(options);

    // In the case that a nick wasn't provided by the optional args...
    if (!nick) {
        nick = account->muc_nick;
    }

    // When no password, check for invite with password
    if (!passwd) {
        passwd = muc_invite_password(room);
    }

    if (!muc_active(room)) {
        presence_join_room(room, nick, passwd);
        muc_join(room, nick, passwd, FALSE);
        iq_room_affiliation_list(room, "member", false);
        iq_room_affiliation_list(room, "admin", false);
        iq_room_affiliation_list(room, "owner", false);
    } else if (muc_roster_complete(room)) {
        ui_switch_to_room(room);
    }

    account_free(account);

    return TRUE;
}

gboolean
cmd_invite(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (g_strcmp0(args[0], "send") == 0) {
        gchar* contact = args[1];
        gchar* reason = args[2];

        if (window->type != WIN_MUC) {
            cons_show("You must be in a chat room to send an invite.");
            return TRUE;
        }

        char* usr_jid = roster_barejid_from_name(contact);
        if (usr_jid == NULL) {
            usr_jid = contact;
        }

        ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        message_send_invite(mucwin->roomjid, usr_jid, reason);
        if (reason) {
            cons_show("Room invite sent, contact: %s, room: %s, reason: \"%s\".",
                      contact, mucwin->roomjid, reason);
        } else {
            cons_show("Room invite sent, contact: %s, room: %s.",
                      contact, mucwin->roomjid);
        }
    } else if (g_strcmp0(args[0], "list") == 0) {
        GList* invites = muc_invites();
        cons_show_room_invites(invites);
        g_list_free_full(invites, g_free);
    } else if (g_strcmp0(args[0], "decline") == 0) {
        if (!muc_invites_contain(args[1])) {
            cons_show("No such invite exists.");
        } else {
            muc_invites_remove(args[1]);
            cons_show("Declined invite to %s.", args[1]);
        }
    }

    return TRUE;
}

gboolean
cmd_form_field(ProfWin* window, char* tag, gchar** args)
{
    if (window->type != WIN_CONFIG) {
        return TRUE;
    }

    ProfConfWin* confwin = (ProfConfWin*)window;
    DataForm* form = confwin->form;
    if (form) {
        if (!form_tag_exists(form, tag)) {
            win_println(window, THEME_DEFAULT, "-", "Form does not contain a field with tag %s", tag);
            return TRUE;
        }

        form_field_type_t field_type = form_get_field_type(form, tag);
        char* cmd = NULL;
        char* value = NULL;
        gboolean valid = FALSE;
        gboolean added = FALSE;
        gboolean removed = FALSE;

        switch (field_type) {
        case FIELD_BOOLEAN:
            value = args[0];
            if (g_strcmp0(value, "on") == 0) {
                form_set_value(form, tag, "1");
                win_println(window, THEME_DEFAULT, "-", "Field updated…");
                confwin_show_form_field(confwin, form, tag);
            } else if (g_strcmp0(value, "off") == 0) {
                form_set_value(form, tag, "0");
                win_println(window, THEME_DEFAULT, "-", "Field updated…");
                confwin_show_form_field(confwin, form, tag);
            } else {
                win_println(window, THEME_DEFAULT, "-", "Invalid command, usage:");
                confwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, "-", "");
            }
            break;

        case FIELD_TEXT_PRIVATE:
        case FIELD_TEXT_SINGLE:
        case FIELD_JID_SINGLE:
            value = args[0];
            if (value == NULL) {
                win_println(window, THEME_DEFAULT, "-", "Invalid command, usage:");
                confwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, "-", "");
            } else {
                form_set_value(form, tag, value);
                win_println(window, THEME_DEFAULT, "-", "Field updated…");
                confwin_show_form_field(confwin, form, tag);
            }
            break;
        case FIELD_LIST_SINGLE:
            value = args[0];
            if ((value == NULL) || !form_field_contains_option(form, tag, value)) {
                win_println(window, THEME_DEFAULT, "-", "Invalid command, usage:");
                confwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, "-", "");
            } else {
                form_set_value(form, tag, value);
                win_println(window, THEME_DEFAULT, "-", "Field updated…");
                confwin_show_form_field(confwin, form, tag);
            }
            break;

        case FIELD_TEXT_MULTI:
            cmd = args[0];
            if (cmd) {
                value = args[1];
            }
            if (!_string_matches_one_of(NULL, cmd, FALSE, "add", "remove", NULL)) {
                win_println(window, THEME_DEFAULT, "-", "Invalid command, usage:");
                confwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, "-", "");
                break;
            }
            if (value == NULL) {
                win_println(window, THEME_DEFAULT, "-", "Invalid command, usage:");
                confwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, "-", "");
                break;
            }
            if (g_strcmp0(cmd, "add") == 0) {
                form_add_value(form, tag, value);
                win_println(window, THEME_DEFAULT, "-", "Field updated…");
                confwin_show_form_field(confwin, form, tag);
                break;
            }
            if (g_strcmp0(args[0], "remove") == 0) {
                if (!g_str_has_prefix(value, "val")) {
                    win_println(window, THEME_DEFAULT, "-", "Invalid command, usage:");
                    confwin_field_help(confwin, tag);
                    win_println(window, THEME_DEFAULT, "-", "");
                    break;
                }
                if (strlen(value) < 4) {
                    win_println(window, THEME_DEFAULT, "-", "Invalid command, usage:");
                    confwin_field_help(confwin, tag);
                    win_println(window, THEME_DEFAULT, "-", "");
                    break;
                }

                int index = strtol(&value[3], NULL, 10);
                if ((index < 1) || (index > form_get_value_count(form, tag))) {
                    win_println(window, THEME_DEFAULT, "-", "Invalid command, usage:");
                    confwin_field_help(confwin, tag);
                    win_println(window, THEME_DEFAULT, "-", "");
                    break;
                }

                removed = form_remove_text_multi_value(form, tag, index);
                if (removed) {
                    win_println(window, THEME_DEFAULT, "-", "Field updated…");
                    confwin_show_form_field(confwin, form, tag);
                } else {
                    win_println(window, THEME_DEFAULT, "-", "Could not remove %s from %s", value, tag);
                }
            }
            break;
        case FIELD_LIST_MULTI:
            cmd = args[0];
            if (cmd) {
                value = args[1];
            }
            if (!_string_matches_one_of(NULL, cmd, FALSE, "add", "remove", NULL)) {
                win_println(window, THEME_DEFAULT, "-", "Invalid command, usage:");
                confwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, "-", "");
                break;
            }
            if (value == NULL) {
                win_println(window, THEME_DEFAULT, "-", "Invalid command, usage:");
                confwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, "-", "");
                break;
            }
            if (g_strcmp0(args[0], "add") == 0) {
                valid = form_field_contains_option(form, tag, value);
                if (valid) {
                    added = form_add_unique_value(form, tag, value);
                    if (added) {
                        win_println(window, THEME_DEFAULT, "-", "Field updated…");
                        confwin_show_form_field(confwin, form, tag);
                    } else {
                        win_println(window, THEME_DEFAULT, "-", "Value %s already selected for %s", value, tag);
                    }
                } else {
                    win_println(window, THEME_DEFAULT, "-", "Invalid command, usage:");
                    confwin_field_help(confwin, tag);
                    win_println(window, THEME_DEFAULT, "-", "");
                }
                break;
            }
            if (g_strcmp0(args[0], "remove") == 0) {
                valid = form_field_contains_option(form, tag, value);
                if (valid == TRUE) {
                    removed = form_remove_value(form, tag, value);
                    if (removed) {
                        win_println(window, THEME_DEFAULT, "-", "Field updated…");
                        confwin_show_form_field(confwin, form, tag);
                    } else {
                        win_println(window, THEME_DEFAULT, "-", "Value %s is not currently set for %s", value, tag);
                    }
                } else {
                    win_println(window, THEME_DEFAULT, "-", "Invalid command, usage:");
                    confwin_field_help(confwin, tag);
                    win_println(window, THEME_DEFAULT, "-", "");
                }
            }
            break;
        case FIELD_JID_MULTI:
            cmd = args[0];
            if (cmd) {
                value = args[1];
            }
            if (!_string_matches_one_of(NULL, cmd, FALSE, "add", "remove", NULL)) {
                win_println(window, THEME_DEFAULT, "-", "Invalid command, usage:");
                confwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, "-", "");
                break;
            }
            if (value == NULL) {
                win_println(window, THEME_DEFAULT, "-", "Invalid command, usage:");
                confwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, "-", "");
                break;
            }
            if (g_strcmp0(args[0], "add") == 0) {
                added = form_add_unique_value(form, tag, value);
                if (added) {
                    win_println(window, THEME_DEFAULT, "-", "Field updated…");
                    confwin_show_form_field(confwin, form, tag);
                } else {
                    win_println(window, THEME_DEFAULT, "-", "JID %s already exists in %s", value, tag);
                }
                break;
            }
            if (g_strcmp0(args[0], "remove") == 0) {
                removed = form_remove_value(form, tag, value);
                if (removed) {
                    win_println(window, THEME_DEFAULT, "-", "Field updated…");
                    confwin_show_form_field(confwin, form, tag);
                } else {
                    win_println(window, THEME_DEFAULT, "-", "Field %s does not contain %s", tag, value);
                }
            }
            break;

        default:
            break;
        }
    }

    return TRUE;
}

gboolean
cmd_form(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (window->type != WIN_CONFIG) {
        cons_show("Command '/form' does not apply to this window.");
        return TRUE;
    }

    if (!_string_matches_one_of(NULL, args[0], FALSE, "submit", "cancel", "show", "help", NULL)) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    ProfConfWin* confwin = (ProfConfWin*)window;
    assert(confwin->memcheck == PROFCONFWIN_MEMCHECK);

    if (g_strcmp0(args[0], "show") == 0) {
        confwin_show_form(confwin);
        return TRUE;
    }

    if (g_strcmp0(args[0], "help") == 0) {
        gchar* tag = args[1];
        if (tag) {
            confwin_field_help(confwin, tag);
        } else {
            confwin_form_help(confwin);

            gchar** help_text = NULL;
            Command* command = cmd_get("/form");

            if (command) {
                help_text = command->help.synopsis;
            }

            ui_show_lines((ProfWin*)confwin, help_text);
        }
        win_println(window, THEME_DEFAULT, "-", "");
        return TRUE;
    }

    if (g_strcmp0(args[0], "submit") == 0 && confwin->submit != NULL) {
        confwin->submit(confwin);
    }

    if (g_strcmp0(args[0], "cancel") == 0 && confwin->cancel != NULL) {
        confwin->cancel(confwin);
    }

    if ((g_strcmp0(args[0], "submit") == 0) || (g_strcmp0(args[0], "cancel") == 0)) {
        if (confwin->form) {
            cmd_ac_remove_form_fields(confwin->form);
        }

        int num = wins_get_num(window);

        ProfWin* new_current = (ProfWin*)wins_get_muc(confwin->roomjid);
        if (!new_current) {
            new_current = wins_get_console();
        }
        ui_focus_win(new_current);
        wins_close_by_num(num);
        wins_tidy();
    }

    return TRUE;
}

gboolean
cmd_kick(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (window->type != WIN_MUC) {
        cons_show("Command '/kick' only applies in chat rooms.");
        return TRUE;
    }

    ProfMucWin* mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    gchar* nick = args[0];
    if (nick) {
        if (muc_roster_contains_nick(mucwin->roomjid, nick)) {
            gchar* reason = args[1];
            iq_room_kick_occupant(mucwin->roomjid, nick, reason);
        } else {
            win_println(window, THEME_DEFAULT, "!", "Occupant does not exist: %s", nick);
        }
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_ban(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (window->type != WIN_MUC) {
        cons_show("Command '/ban' only applies in chat rooms.");
        return TRUE;
    }

    ProfMucWin* mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    gchar* jid = args[0];
    if (jid) {
        gchar* reason = args[1];
        iq_room_affiliation_set(mucwin->roomjid, jid, "outcast", reason);
    } else {
        cons_bad_cmd_usage(command);
    }
    return TRUE;
}

gboolean
cmd_subject(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (window->type != WIN_MUC) {
        cons_show("Command '/room' does not apply to this window.");
        return TRUE;
    }

    ProfMucWin* mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    if (args[0] == NULL) {
        char* subject = muc_subject(mucwin->roomjid);
        if (subject) {
            win_print(window, THEME_ROOMINFO, "!", "Room subject: ");
            win_appendln(window, THEME_DEFAULT, "%s", subject);
        } else {
            win_println(window, THEME_ROOMINFO, "!", "Room has no subject");
        }
        return TRUE;
    }

    if (g_strcmp0(args[0], "set") == 0) {
        if (args[1]) {
            message_send_groupchat_subject(mucwin->roomjid, args[1]);
        } else {
            cons_bad_cmd_usage(command);
        }
        return TRUE;
    }

    if (g_strcmp0(args[0], "edit") == 0) {
        if (args[1]) {
            message_send_groupchat_subject(mucwin->roomjid, args[1]);
        } else {
            cons_bad_cmd_usage(command);
        }
        return TRUE;
    }

    if (g_strcmp0(args[0], "editor") == 0) {
        gchar* message = NULL;
        char* subject = muc_subject(mucwin->roomjid);

        if (get_message_from_editor(subject, &message)) {
            return TRUE;
        }

        if (message) {
            message_send_groupchat_subject(mucwin->roomjid, message);
        } else {
            cons_bad_cmd_usage(command);
        }
        return TRUE;
    }

    if (g_strcmp0(args[0], "prepend") == 0) {
        if (args[1]) {
            char* old_subject = muc_subject(mucwin->roomjid);
            if (old_subject) {
                auto_gchar gchar* new_subject = g_strdup_printf("%s%s", args[1], old_subject);
                message_send_groupchat_subject(mucwin->roomjid, new_subject);
            } else {
                win_print(window, THEME_ROOMINFO, "!", "Room does not have a subject, use /subject set <subject>");
            }
        } else {
            cons_bad_cmd_usage(command);
        }
        return TRUE;
    }

    if (g_strcmp0(args[0], "append") == 0) {
        if (args[1]) {
            char* old_subject = muc_subject(mucwin->roomjid);
            if (old_subject) {
                auto_gchar gchar* new_subject = g_strdup_printf("%s%s", old_subject, args[1]);
                message_send_groupchat_subject(mucwin->roomjid, new_subject);
            } else {
                win_print(window, THEME_ROOMINFO, "!", "Room does not have a subject, use /subject set <subject>");
            }
        } else {
            cons_bad_cmd_usage(command);
        }
        return TRUE;
    }

    if (g_strcmp0(args[0], "clear") == 0) {
        message_send_groupchat_subject(mucwin->roomjid, NULL);
        return TRUE;
    }

    cons_bad_cmd_usage(command);
    return TRUE;
}

gboolean
cmd_affiliation(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (window->type != WIN_MUC) {
        cons_show("Command '/affiliation' does not apply to this window.");
        return TRUE;
    }

    gchar* cmd = args[0];
    if (cmd == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    gchar* affiliation = args[1];
    if (!_string_matches_one_of(NULL, affiliation, TRUE, "owner", "admin", "member", "none", "outcast", NULL)) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    ProfMucWin* mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    if (g_strcmp0(cmd, "list") == 0) {
        if (!affiliation) {
            iq_room_affiliation_list(mucwin->roomjid, "owner", true);
            iq_room_affiliation_list(mucwin->roomjid, "admin", true);
            iq_room_affiliation_list(mucwin->roomjid, "member", true);
            iq_room_affiliation_list(mucwin->roomjid, "outcast", true);
        } else if (g_strcmp0(affiliation, "none") == 0) {
            win_println(window, THEME_DEFAULT, "!", "Cannot list users with no affiliation.");
        } else {
            iq_room_affiliation_list(mucwin->roomjid, affiliation, true);
        }
        return TRUE;
    }

    if (g_strcmp0(cmd, "set") == 0) {
        if (!affiliation) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        gchar* jid = args[2];
        if (jid == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            gchar* reason = args[3];
            iq_room_affiliation_set(mucwin->roomjid, jid, affiliation, reason);
            return TRUE;
        }
    }

    if (g_strcmp0(cmd, "request") == 0) {
        message_request_voice(mucwin->roomjid);
        return TRUE;
    }

    if (g_strcmp0(cmd, "register") == 0) {
        iq_muc_register_nick(mucwin->roomjid);
        return TRUE;
    }

    cons_bad_cmd_usage(command);
    return TRUE;
}

gboolean
cmd_role(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (window->type != WIN_MUC) {
        cons_show("Command '/role' does not apply to this window.");
        return TRUE;
    }

    gchar* cmd = args[0];
    if (cmd == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    gchar* role = args[1];
    if (!_string_matches_one_of(NULL, role, TRUE, "visitor", "participant", "moderator", "none", NULL)) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    ProfMucWin* mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    if (g_strcmp0(cmd, "list") == 0) {
        if (!role) {
            iq_room_role_list(mucwin->roomjid, "moderator");
            iq_room_role_list(mucwin->roomjid, "participant");
            iq_room_role_list(mucwin->roomjid, "visitor");
        } else if (g_strcmp0(role, "none") == 0) {
            win_println(window, THEME_DEFAULT, "!", "Cannot list users with no role.");
        } else {
            iq_room_role_list(mucwin->roomjid, role);
        }
        return TRUE;
    }

    if (g_strcmp0(cmd, "set") == 0) {
        if (!role) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        char* nick = args[2];
        if (nick == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            char* reason = args[3];
            iq_room_role_set(mucwin->roomjid, nick, role, reason);
            return TRUE;
        }
    }

    cons_bad_cmd_usage(command);
    return TRUE;
}

gboolean
cmd_room(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (window->type != WIN_MUC) {
        cons_show("Command '/room' does not apply to this window.");
        return TRUE;
    }

    ProfMucWin* mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    if (g_strcmp0(args[0], "accept") == 0) {
        gboolean requires_config = muc_requires_config(mucwin->roomjid);
        if (!requires_config) {
            win_println(window, THEME_ROOMINFO, "!", "Current room does not require configuration.");
            return TRUE;
        } else {
            iq_confirm_instant_room(mucwin->roomjid);
            muc_set_requires_config(mucwin->roomjid, FALSE);
            win_println(window, THEME_ROOMINFO, "!", "Room unlocked.");
            return TRUE;
        }
    } else if (g_strcmp0(args[0], "destroy") == 0) {
        iq_destroy_room(mucwin->roomjid);
        return TRUE;
    } else if (g_strcmp0(args[0], "config") == 0) {
        ProfConfWin* confwin = wins_get_conf(mucwin->roomjid);

        if (confwin) {
            ui_focus_win((ProfWin*)confwin);
        } else {
            iq_request_room_config_form(mucwin->roomjid);
        }
        return TRUE;
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_occupants(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strcmp0(args[0], "size") == 0) {
        if (!args[1]) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            int intval = 0;
            auto_char char* err_msg = NULL;
            gboolean res = strtoi_range(args[1], &intval, 1, 99, &err_msg);
            if (res) {
                prefs_set_occupants_size(intval);
                cons_show("Occupants screen size set to: %d%%", intval);
                wins_resize_all();
                return TRUE;
            } else {
                cons_show(err_msg);
                return TRUE;
            }
        }
    }

    if (g_strcmp0(args[0], "indent") == 0) {
        if (!args[1]) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            int intval = 0;
            auto_char char* err_msg = NULL;
            gboolean res = strtoi_range(args[1], &intval, 0, 10, &err_msg);
            if (res) {
                prefs_set_occupants_indent(intval);
                cons_show("Occupants indent set to: %d", intval);

                occupantswin_occupants_all();
            } else {
                cons_show(err_msg);
            }
            return TRUE;
        }
    }

    if (g_strcmp0(args[0], "wrap") == 0) {
        if (!args[1]) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            _cmd_set_boolean_preference(args[1], "Occupants panel line wrap", PREF_OCCUPANTS_WRAP);
            occupantswin_occupants_all();
            return TRUE;
        }
    }

    if (g_strcmp0(args[0], "char") == 0) {
        if (!args[1]) {
            cons_bad_cmd_usage(command);
        } else if (g_strcmp0(args[1], "none") == 0) {
            prefs_clear_occupants_char();
            cons_show("Occupants char removed.");

            occupantswin_occupants_all();
        } else {
            prefs_set_occupants_char(args[1]);
            cons_show("Occupants char set to %s.", args[1]);

            occupantswin_occupants_all();
        }
        return TRUE;
    }

    if (g_strcmp0(args[0], "color") == 0) {
        _cmd_set_boolean_preference(args[1], "Occupants consistent colors", PREF_OCCUPANTS_COLOR_NICK);
        occupantswin_occupants_all();
        return TRUE;
    }

    if (g_strcmp0(args[0], "default") == 0) {
        if (g_strcmp0(args[1], "show") == 0) {
            if (g_strcmp0(args[2], "jid") == 0) {
                cons_show("Occupant jids enabled.");
                prefs_set_boolean(PREF_OCCUPANTS_JID, TRUE);
            } else if (g_strcmp0(args[2], "offline") == 0) {
                cons_show("Occupants offline enabled.");
                prefs_set_boolean(PREF_OCCUPANTS_OFFLINE, TRUE);
            } else {
                cons_show("Occupant list enabled.");
                prefs_set_boolean(PREF_OCCUPANTS, TRUE);
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "hide") == 0) {
            if (g_strcmp0(args[2], "jid") == 0) {
                cons_show("Occupant jids disabled.");
                prefs_set_boolean(PREF_OCCUPANTS_JID, FALSE);
            } else if (g_strcmp0(args[2], "offline") == 0) {
                cons_show("Occupants offline disabled.");
                prefs_set_boolean(PREF_OCCUPANTS_OFFLINE, FALSE);
            } else {
                cons_show("Occupant list disabled.");
                prefs_set_boolean(PREF_OCCUPANTS, FALSE);
            }
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    }

    // header settings
    if (g_strcmp0(args[0], "header") == 0) {
        if (g_strcmp0(args[1], "char") == 0) {
            if (!args[2]) {
                cons_bad_cmd_usage(command);
            } else if (g_strcmp0(args[2], "none") == 0) {
                prefs_clear_occupants_header_char();
                cons_show("Occupants header char removed.");

                occupantswin_occupants_all();
            } else {
                prefs_set_occupants_header_char(args[2]);
                cons_show("Occupants header char set to %s.", args[2]);

                occupantswin_occupants_all();
            }
        } else {
            cons_bad_cmd_usage(command);
        }
        return TRUE;
    }

    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (window->type != WIN_MUC) {
        cons_show("Cannot apply setting when not in chat room.");
        return TRUE;
    }

    ProfMucWin* mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    if (g_strcmp0(args[0], "show") == 0) {
        if (g_strcmp0(args[1], "jid") == 0) {
            mucwin->showjid = TRUE;
            mucwin_update_occupants(mucwin);
        } else if (g_strcmp0(args[1], "offline") == 0) {
            mucwin->showoffline = TRUE;
            mucwin_update_occupants(mucwin);
        } else {
            mucwin_show_occupants(mucwin);
        }
    } else if (g_strcmp0(args[0], "hide") == 0) {
        if (g_strcmp0(args[1], "jid") == 0) {
            mucwin->showjid = FALSE;
            mucwin_update_occupants(mucwin);
        } else if (g_strcmp0(args[1], "offline") == 0) {
            mucwin->showoffline = FALSE;
            mucwin_update_occupants(mucwin);
        } else {
            mucwin_hide_occupants(mucwin);
        }
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_rooms(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    auto_gchar gchar* service = NULL;
    auto_gchar gchar* filter = NULL;
    if (args[0] != NULL) {
        if (g_strcmp0(args[0], "service") == 0) {
            if (args[1] == NULL) {
                cons_bad_cmd_usage(command);
                cons_show("");
                return TRUE;
            }
            service = g_strdup(args[1]);
        } else if (g_strcmp0(args[0], "filter") == 0) {
            if (args[1] == NULL) {
                cons_bad_cmd_usage(command);
                cons_show("");
                return TRUE;
            }
            filter = g_strdup(args[1]);
        } else if (g_strcmp0(args[0], "cache") == 0) {
            if (g_strv_length(args) != 2) {
                cons_bad_cmd_usage(command);
                cons_show("");
                return TRUE;
            } else if (g_strcmp0(args[1], "on") == 0) {
                prefs_set_boolean(PREF_ROOM_LIST_CACHE, TRUE);
                cons_show("Rooms list cache enabled.");
                return TRUE;
            } else if (g_strcmp0(args[1], "off") == 0) {
                prefs_set_boolean(PREF_ROOM_LIST_CACHE, FALSE);
                cons_show("Rooms list cache disabled.");
                return TRUE;
            } else if (g_strcmp0(args[1], "clear") == 0) {
                iq_rooms_cache_clear();
                cons_show("Rooms list cache cleared.");
                return TRUE;
            } else {
                cons_bad_cmd_usage(command);
                cons_show("");
                return TRUE;
            }
        } else {
            cons_bad_cmd_usage(command);
            cons_show("");
            return TRUE;
        }
    }
    if (g_strv_length(args) >= 3) {
        if (g_strcmp0(args[2], "service") == 0) {
            if (args[3] == NULL) {
                cons_bad_cmd_usage(command);
                cons_show("");
                return TRUE;
            }
            g_free(service);
            service = g_strdup(args[3]);
        } else if (g_strcmp0(args[2], "filter") == 0) {
            if (args[3] == NULL) {
                cons_bad_cmd_usage(command);
                cons_show("");
                return TRUE;
            }
            g_free(filter);
            filter = g_strdup(args[3]);
        } else {
            cons_bad_cmd_usage(command);
            cons_show("");
            return TRUE;
        }
    }

    if (service == NULL) {
        ProfAccount* account = accounts_get_account(session_get_account_name());
        if (account->muc_service) {
            service = g_strdup(account->muc_service);
            account_free(account);
        } else {
            cons_show("Account MUC service property not found.");
            account_free(account);
            return TRUE;
        }
    }

    cons_show("");
    if (filter) {
        cons_show("Room list request sent: %s, filter: '%s'", service, filter);
    } else {
        cons_show("Room list request sent: %s", service);
    }
    iq_room_list_request(service, filter);

    return TRUE;
}

gboolean
cmd_bookmark(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        cons_alert(NULL);
        return TRUE;
    }

    int num_args = g_strv_length(args);
    gchar* cmd = args[0];
    if (window->type == WIN_MUC
        && num_args < 2
        && (cmd == NULL || g_strcmp0(cmd, "add") == 0)) {
        // default to current nickname, password, and autojoin "on"
        ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        char* nick = muc_nick(mucwin->roomjid);
        char* password = muc_password(mucwin->roomjid);
        gboolean added = bookmark_add(mucwin->roomjid, nick, password, "on", NULL);
        if (added) {
            win_println(window, THEME_DEFAULT, "!", "Bookmark added for %s.", mucwin->roomjid);
        } else {
            win_println(window, THEME_DEFAULT, "!", "Bookmark already exists for %s.", mucwin->roomjid);
        }
        return TRUE;
    }

    if (window->type == WIN_MUC
        && num_args < 2
        && g_strcmp0(cmd, "remove") == 0) {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        gboolean removed = bookmark_remove(mucwin->roomjid);
        if (removed) {
            win_println(window, THEME_DEFAULT, "!", "Bookmark removed for %s.", mucwin->roomjid);
        } else {
            win_println(window, THEME_DEFAULT, "!", "Bookmark does not exist for %s.", mucwin->roomjid);
        }
        return TRUE;
    }

    if (cmd == NULL) {
        cons_bad_cmd_usage(command);
        cons_alert(NULL);
        return TRUE;
    }

    if (strcmp(cmd, "invites") == 0) {
        if (g_strcmp0(args[1], "on") == 0) {
            prefs_set_boolean(PREF_BOOKMARK_INVITE, TRUE);
            cons_show("Auto bookmarking accepted invites enabled.");
        } else if (g_strcmp0(args[1], "off") == 0) {
            prefs_set_boolean(PREF_BOOKMARK_INVITE, FALSE);
            cons_show("Auto bookmarking accepted invites disabled.");
        } else {
            cons_bad_cmd_usage(command);
            cons_show("");
        }
        cons_alert(NULL);
        return TRUE;
    }

    if (strcmp(cmd, "list") == 0) {
        char* bookmark_jid = args[1];
        if (bookmark_jid == NULL) {
            // list all bookmarks
            GList* bookmarks = bookmark_get_list();
            cons_show_bookmarks(bookmarks);
            g_list_free(bookmarks);
        } else {
            // list one bookmark
            Bookmark* bookmark = bookmark_get_by_jid(bookmark_jid);
            cons_show_bookmark(bookmark);
        }

        return TRUE;
    }

    char* jid = args[1];
    if (jid == NULL) {
        cons_bad_cmd_usage(command);
        cons_show("");
        cons_alert(NULL);
        return TRUE;
    }
    if (strchr(jid, '@') == NULL) {
        cons_show("Invalid room, must be of the form room@domain.tld");
        cons_show("");
        cons_alert(NULL);
        return TRUE;
    }

    if (strcmp(cmd, "remove") == 0) {
        gboolean removed = bookmark_remove(jid);
        if (removed) {
            cons_show("Bookmark removed for %s.", jid);
        } else {
            cons_show("No bookmark exists for %s.", jid);
        }
        cons_alert(NULL);
        return TRUE;
    }

    if (strcmp(cmd, "join") == 0) {
        gboolean joined = bookmark_join(jid);
        if (!joined) {
            cons_show("No bookmark exists for %s.", jid);
        }
        cons_alert(NULL);
        return TRUE;
    }

    gchar* opt_keys[] = { "autojoin", "nick", "password", "name", NULL };
    gboolean parsed;

    GHashTable* options = parse_options(&args[2], opt_keys, &parsed);
    if (!parsed) {
        cons_bad_cmd_usage(command);
        cons_show("");
        cons_alert(NULL);
        return TRUE;
    }

    char* autojoin = g_hash_table_lookup(options, "autojoin");

    if (autojoin && ((strcmp(autojoin, "on") != 0) && (strcmp(autojoin, "off") != 0))) {
        cons_bad_cmd_usage(command);
        cons_show("");
        options_destroy(options);
        cons_alert(NULL);
        return TRUE;
    }

    char* nick = g_hash_table_lookup(options, "nick");
    char* password = g_hash_table_lookup(options, "password");
    char* name = g_hash_table_lookup(options, "name");

    if (strcmp(cmd, "add") == 0) {
        gboolean added = bookmark_add(jid, nick, password, autojoin, name);
        if (added) {
            cons_show("Bookmark added for %s.", jid);
        } else {
            cons_show("Bookmark already exists, use /bookmark update to edit.");
        }
        options_destroy(options);
        cons_alert(NULL);
        return TRUE;
    }

    if (strcmp(cmd, "update") == 0) {
        gboolean updated = bookmark_update(jid, nick, password, autojoin, name);
        if (updated) {
            cons_show("Bookmark updated.");
        } else {
            cons_show("No bookmark exists for %s.", jid);
        }
        options_destroy(options);
        cons_alert(NULL);
        return TRUE;
    }

    cons_bad_cmd_usage(command);
    options_destroy(options);
    cons_alert(NULL);

    return TRUE;
}

gboolean
cmd_bookmark_ignore(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        cons_alert(NULL);
        return TRUE;
    }

    // `/bookmark ignore` lists them
    if (args[1] == NULL) {
        gsize len = 0;
        auto_gcharv gchar** list = bookmark_ignore_list(&len);
        cons_show_bookmarks_ignore(list, len);
        return TRUE;
    }

    if (strcmp(args[1], "add") == 0 && args[2] != NULL) {
        bookmark_ignore_add(args[2]);
        cons_show("Autojoin for bookmark %s added to ignore list.", args[2]);
        return TRUE;
    }

    if (strcmp(args[1], "remove") == 0 && args[2] != NULL) {
        bookmark_ignore_remove(args[2]);
        cons_show("Autojoin for bookmark %s removed from ignore list.", args[2]);
        return TRUE;
    }

    cons_bad_cmd_usage(command);
    return TRUE;
}

gboolean
cmd_disco(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    auto_gchar gchar* jid = g_strdup_printf("%s", args[1] ?: connection_get_jid()->domainpart);

    if (g_strcmp0(args[0], "info") == 0) {
        iq_disco_info_request(jid);
    } else {
        iq_disco_items_request(jid);
    }

    return TRUE;
}

// TODO: Move this into its own tools such as HTTPUpload or AESGCMDownload.
#ifdef HAVE_OMEMO
char*
_add_omemo_stream(int* fd, FILE** fh, char** err)
{
    // Create temporary file for writing ciphertext.
    int tmpfd;
    auto_char char* tmpname = NULL;
    if ((tmpfd = g_file_open_tmp("profanity.XXXXXX", &tmpname, NULL)) == -1) {
        *err = "Unable to create temporary file for encrypted transfer.";
        return NULL;
    }
    FILE* tmpfh = fdopen(tmpfd, "w+b");

    // The temporary ciphertext file should be removed after it has
    // been closed.
    remove(tmpname);

    int crypt_res;
    char* fragment;
    fragment = omemo_encrypt_file(*fh, tmpfh, file_size(*fd), &crypt_res);
    if (crypt_res != 0) {
        fclose(tmpfh);
        return NULL;
    }

    // Force flush as the upload will read from the same stream.
    fflush(tmpfh);
    rewind(tmpfh);

    fclose(*fh); // Also closes descriptor.

    // Switch original stream with temporary ciphertext stream.
    *fd = tmpfd;
    *fh = tmpfh;

    return fragment;
}
#endif

gboolean
cmd_sendfile(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();
    // expand ~ to $HOME
    auto_gchar gchar* filename = get_expanded_path(args[0]);
    char* alt_scheme = NULL;
    char* alt_fragment = NULL;

    if (access(filename, R_OK) != 0) {
        cons_show_error("Uploading '%s' failed: File not found!", filename);
        goto out;
    }

    if (!is_regular_file(filename)) {
        cons_show_error("Uploading '%s' failed: Not a file!", filename);
        goto out;
    }

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        goto out;
    }

    if (window->type != WIN_CHAT && window->type != WIN_PRIVATE && window->type != WIN_MUC) {
        cons_show_error("Unsupported window for file transmission.");
        goto out;
    }

    int fd;
    if ((fd = open(filename, O_RDONLY)) == -1) {
        cons_show_error("Unable to open file descriptor for '%s'.", filename);
        goto out;
    }

    FILE* fh = fdopen(fd, "rb");

    gboolean omemo_enabled = FALSE;
    gboolean sendfile_enabled = TRUE;

    switch (window->type) {
    case WIN_MUC:
    {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        omemo_enabled = mucwin->is_omemo == TRUE;
        break;
    }
    case WIN_CHAT:
    {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        omemo_enabled = chatwin->is_omemo == TRUE;
        sendfile_enabled = !((chatwin->pgp_send == TRUE && !prefs_get_boolean(PREF_PGP_SENDFILE))
                             || (chatwin->is_otr == TRUE && !prefs_get_boolean(PREF_OTR_SENDFILE)));
        break;
    }

    case WIN_PRIVATE: // We don't support encryption in private MUC windows.
    default:
        cons_show_error("Unsupported window for file transmission.");
        goto out;
    }

    if (!sendfile_enabled) {
        cons_show_error("Uploading unencrypted files disabled. See /otr sendfile or /pgp sendfile.");
        win_println(window, THEME_ERROR, "-", "Sending encrypted files via http_upload is not possible yet.");
        goto out;
    }

    if (omemo_enabled) {
#ifdef HAVE_OMEMO
        char* err = NULL;
        alt_scheme = OMEMO_AESGCM_URL_SCHEME;
        alt_fragment = _add_omemo_stream(&fd, &fh, &err);
        if (err != NULL) {
            cons_show_error(err);
            win_println(window, THEME_ERROR, "-", err);
            goto out;
        }
#endif
    }

    HTTPUpload* upload = malloc(sizeof(HTTPUpload));
    upload->window = window;

    upload->filename = strdup(filename);
    upload->filehandle = fh;
    upload->filesize = file_size(fd);
    upload->mime_type = file_mime_type(filename);

    if (alt_scheme != NULL) {
        upload->alt_scheme = strdup(alt_scheme);
    } else {
        upload->alt_scheme = NULL;
    }

    if (alt_fragment != NULL) {
        upload->alt_fragment = strdup(alt_fragment);
    } else {
        upload->alt_fragment = NULL;
    }

    iq_http_upload_request(upload);

out:
#ifdef HAVE_OMEMO
    if (alt_fragment != NULL)
        omemo_free(alt_fragment);
#endif

    return TRUE;
}

gboolean
cmd_lastactivity(ProfWin* window, const char* const command, gchar** args)
{
    if ((g_strcmp0(args[0], "set") == 0)) {
        if ((g_strcmp0(args[1], "on") == 0) || (g_strcmp0(args[1], "off") == 0)) {
            _cmd_set_boolean_preference(args[1], "Last activity", PREF_LASTACTIVITY);
            if (g_strcmp0(args[1], "on") == 0) {
                caps_add_feature(XMPP_FEATURE_LASTACTIVITY);
            }
            if (g_strcmp0(args[1], "off") == 0) {
                caps_remove_feature(XMPP_FEATURE_LASTACTIVITY);
            }
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    }

    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if ((g_strcmp0(args[0], "get") == 0)) {
        if (args[1] == NULL) {
            iq_last_activity_request(connection_get_jid()->domainpart);
        } else {
            iq_last_activity_request(args[1]);
        }
        return TRUE;
    }

    cons_bad_cmd_usage(command);
    return TRUE;
}

gboolean
cmd_nick(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }
    if (window->type != WIN_MUC) {
        cons_show("You can only change your nickname in a chat room window.");
        return TRUE;
    }

    ProfMucWin* mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
    char* nick = args[0];
    presence_change_room_nick(mucwin->roomjid, nick);

    return TRUE;
}

gboolean
cmd_alias(ProfWin* window, const char* const command, gchar** args)
{
    char* subcmd = args[0];

    if (strcmp(subcmd, "add") == 0) {
        char* alias = args[1];
        if (alias == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            if (strchr(alias, ' ')) {
                cons_bad_cmd_usage(command);
                return TRUE;
            }
            char* alias_p = alias;
            auto_gchar gchar* ac_value = NULL;
            if (alias[0] == '/') {
                ac_value = g_strdup_printf("%s", alias);
                alias_p = &alias[1];
            } else {
                ac_value = g_strdup_printf("/%s", alias);
            }

            char* value = args[2];
            if (value == NULL) {
                cons_bad_cmd_usage(command);
            } else if (cmd_ac_exists(ac_value)) {
                cons_show("Command or alias '%s' already exists.", ac_value);
            } else {
                prefs_add_alias(alias_p, value);
                cmd_ac_add(ac_value);
                cmd_ac_add_alias_value(alias_p);
                cons_show("Command alias added %s -> %s", ac_value, value);
            }
            return TRUE;
        }
    } else if (strcmp(subcmd, "remove") == 0) {
        char* alias = args[1];
        if (alias == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            if (alias[0] == '/') {
                alias = &alias[1];
            }
            gboolean removed = prefs_remove_alias(alias);
            if (!removed) {
                cons_show("No such command alias /%s", alias);
            } else {
                auto_gchar gchar* ac_value = g_strdup_printf("/%s", alias);
                cmd_ac_remove(ac_value);
                cmd_ac_remove_alias_value(alias);
                cons_show("Command alias removed -> /%s", alias);
            }
            return TRUE;
        }
    } else if (strcmp(subcmd, "list") == 0) {
        GList* aliases = prefs_get_aliases();
        cons_show_aliases(aliases);
        prefs_free_aliases(aliases);
        return TRUE;
    } else {
        cons_bad_cmd_usage(command);
        return TRUE;
    }
}

gboolean
cmd_clear(ProfWin* window, const char* const command, gchar** args)
{
    if (args[0] == NULL) {
        win_clear(window);
        return TRUE;
    } else {
        if ((g_strcmp0(args[0], "persist_history") == 0)) {

            if (args[1] != NULL) {
                if ((g_strcmp0(args[1], "on") == 0) || (g_strcmp0(args[1], "off") == 0)) {
                    _cmd_set_boolean_preference(args[1], "Persistent history", PREF_CLEAR_PERSIST_HISTORY);
                    return TRUE;
                }
            } else {
                if (prefs_get_boolean(PREF_CLEAR_PERSIST_HISTORY)) {
                    win_println(window, THEME_DEFAULT, "!", "  Persistently clear screen  : ON");
                } else {
                    win_println(window, THEME_DEFAULT, "!", "  Persistently clear screen  : OFF");
                }
                return TRUE;
            }
        }
    }
    cons_bad_cmd_usage(command);

    return TRUE;
}

gboolean
cmd_privileges(ProfWin* window, const char* const command, gchar** args)
{
    _cmd_set_boolean_preference(args[0], "MUC privileges", PREF_MUC_PRIVILEGES);

    ui_redraw_all_room_rosters();

    return TRUE;
}

gboolean
cmd_charset(ProfWin* window, const char* const command, gchar** args)
{
    char* codeset = nl_langinfo(CODESET);
    char* lang = getenv("LANG");

    cons_show("Charset information:");

    if (lang) {
        cons_show("  LANG:       %s", lang);
    }
    if (codeset) {
        cons_show("  CODESET:    %s", codeset);
    }
    cons_show("  MB_CUR_MAX: %d", MB_CUR_MAX);
    cons_show("  MB_LEN_MAX: %d", MB_LEN_MAX);

    return TRUE;
}

gboolean
cmd_beep(ProfWin* window, const char* const command, gchar** args)
{
    _cmd_set_boolean_preference(args[0], "Sound", PREF_BEEP);
    return TRUE;
}

gboolean
cmd_console(ProfWin* window, const char* const command, gchar** args)
{
    gboolean isMuc = (g_strcmp0(args[0], "muc") == 0);

    if (!_string_matches_one_of(NULL, args[0], FALSE, "chat", "private", NULL) && !isMuc) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    gchar* setting = args[1];
    if (!_string_matches_one_of(NULL, setting, FALSE, "all", "first", "none", NULL)) {
        if (!(isMuc && (g_strcmp0(setting, "mention") == 0))) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    }

    if (g_strcmp0(args[0], "chat") == 0) {
        prefs_set_string(PREF_CONSOLE_CHAT, setting);
        cons_show("Console chat messages set: %s", setting);
        return TRUE;
    }

    if (g_strcmp0(args[0], "muc") == 0) {
        prefs_set_string(PREF_CONSOLE_MUC, setting);
        cons_show("Console MUC messages set: %s", setting);
        return TRUE;
    }

    if (g_strcmp0(args[0], "private") == 0) {
        prefs_set_string(PREF_CONSOLE_PRIVATE, setting);
        cons_show("Console private room messages set: %s", setting);
        return TRUE;
    }

    return TRUE;
}

gboolean
cmd_presence(ProfWin* window, const char* const command, gchar** args)
{
    if (strcmp(args[0], "console") != 0 && strcmp(args[0], "chat") != 0 && strcmp(args[0], "room") != 0 && strcmp(args[0], "titlebar") != 0) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (strcmp(args[0], "titlebar") == 0) {
        _cmd_set_boolean_preference(args[1], "Contact presence", PREF_PRESENCE);
        return TRUE;
    }

    if (strcmp(args[1], "all") != 0 && strcmp(args[1], "online") != 0 && strcmp(args[1], "none") != 0) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (strcmp(args[0], "console") == 0) {
        prefs_set_string(PREF_STATUSES_CONSOLE, args[1]);
        if (strcmp(args[1], "all") == 0) {
            cons_show("All presence updates will appear in the console.");
        } else if (strcmp(args[1], "online") == 0) {
            cons_show("Only online/offline presence updates will appear in the console.");
        } else {
            cons_show("Presence updates will not appear in the console.");
        }
    }

    if (strcmp(args[0], "chat") == 0) {
        prefs_set_string(PREF_STATUSES_CHAT, args[1]);
        if (strcmp(args[1], "all") == 0) {
            cons_show("All presence updates will appear in chat windows.");
        } else if (strcmp(args[1], "online") == 0) {
            cons_show("Only online/offline presence updates will appear in chat windows.");
        } else {
            cons_show("Presence updates will not appear in chat windows.");
        }
    }

    if (strcmp(args[0], "room") == 0) {
        prefs_set_string(PREF_STATUSES_MUC, args[1]);
        if (strcmp(args[1], "all") == 0) {
            cons_show("All presence updates will appear in chat room windows.");
        } else if (strcmp(args[1], "online") == 0) {
            cons_show("Only join/leave presence updates will appear in chat room windows.");
        } else {
            cons_show("Presence updates will not appear in chat room windows.");
        }
    }

    return TRUE;
}

gboolean
cmd_wrap(ProfWin* window, const char* const command, gchar** args)
{
    _cmd_set_boolean_preference(args[0], "Word wrap", PREF_WRAP);

    wins_resize_all();

    return TRUE;
}

gboolean
cmd_time(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strcmp0(args[0], "lastactivity") == 0) {
        if (args[1] == NULL) {
            auto_gchar gchar* format = prefs_get_string(PREF_TIME_LASTACTIVITY);
            cons_show("Last activity time format: '%s'.", format);
            return TRUE;
        } else if (g_strcmp0(args[1], "set") == 0 && args[2] != NULL) {
            prefs_set_string(PREF_TIME_LASTACTIVITY, args[2]);
            cons_show("Last activity time format set to '%s'.", args[2]);
            ui_redraw();
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            cons_show("Last activity time cannot be disabled.");
            ui_redraw();
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    } else if (g_strcmp0(args[0], "statusbar") == 0) {
        if (args[1] == NULL) {
            auto_gchar gchar* format = prefs_get_string(PREF_TIME_STATUSBAR);
            cons_show("Status bar time format: '%s'.", format);
            return TRUE;
        } else if (g_strcmp0(args[1], "set") == 0 && args[2] != NULL) {
            prefs_set_string(PREF_TIME_STATUSBAR, args[2]);
            cons_show("Status bar time format set to '%s'.", args[2]);
            ui_redraw();
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            prefs_set_string(PREF_TIME_STATUSBAR, "off");
            cons_show("Status bar time display disabled.");
            ui_redraw();
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    } else if (g_strcmp0(args[0], "console") == 0) {
        if (args[1] == NULL) {
            auto_gchar gchar* format = prefs_get_string(PREF_TIME_CONSOLE);
            cons_show("Console time format: '%s'.", format);
            return TRUE;
        } else if (g_strcmp0(args[1], "set") == 0 && args[2] != NULL) {
            prefs_set_string(PREF_TIME_CONSOLE, args[2]);
            cons_show("Console time format set to '%s'.", args[2]);
            wins_resize_all();
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            prefs_set_string(PREF_TIME_CONSOLE, "off");
            cons_show("Console time display disabled.");
            wins_resize_all();
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    } else if (g_strcmp0(args[0], "chat") == 0) {
        if (args[1] == NULL) {
            auto_gchar gchar* format = prefs_get_string(PREF_TIME_CHAT);
            cons_show("Chat time format: '%s'.", format);
            return TRUE;
        } else if (g_strcmp0(args[1], "set") == 0 && args[2] != NULL) {
            prefs_set_string(PREF_TIME_CHAT, args[2]);
            cons_show("Chat time format set to '%s'.", args[2]);
            wins_resize_all();
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            prefs_set_string(PREF_TIME_CHAT, "off");
            cons_show("Chat time display disabled.");
            wins_resize_all();
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    } else if (g_strcmp0(args[0], "muc") == 0) {
        if (args[1] == NULL) {
            auto_gchar gchar* format = prefs_get_string(PREF_TIME_MUC);
            cons_show("MUC time format: '%s'.", format);
            return TRUE;
        } else if (g_strcmp0(args[1], "set") == 0 && args[2] != NULL) {
            prefs_set_string(PREF_TIME_MUC, args[2]);
            cons_show("MUC time format set to '%s'.", args[2]);
            wins_resize_all();
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            prefs_set_string(PREF_TIME_MUC, "off");
            cons_show("MUC time display disabled.");
            wins_resize_all();
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    } else if (g_strcmp0(args[0], "config") == 0) {
        if (args[1] == NULL) {
            auto_gchar gchar* format = prefs_get_string(PREF_TIME_CONFIG);
            cons_show("config time format: '%s'.", format);
            return TRUE;
        } else if (g_strcmp0(args[1], "set") == 0 && args[2] != NULL) {
            prefs_set_string(PREF_TIME_CONFIG, args[2]);
            cons_show("config time format set to '%s'.", args[2]);
            wins_resize_all();
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            prefs_set_string(PREF_TIME_CONFIG, "off");
            cons_show("config time display disabled.");
            wins_resize_all();
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    } else if (g_strcmp0(args[0], "private") == 0) {
        if (args[1] == NULL) {
            auto_gchar gchar* format = prefs_get_string(PREF_TIME_PRIVATE);
            cons_show("Private chat time format: '%s'.", format);
            return TRUE;
        } else if (g_strcmp0(args[1], "set") == 0 && args[2] != NULL) {
            prefs_set_string(PREF_TIME_PRIVATE, args[2]);
            cons_show("Private chat time format set to '%s'.", args[2]);
            wins_resize_all();
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            prefs_set_string(PREF_TIME_PRIVATE, "off");
            cons_show("Private chat time display disabled.");
            wins_resize_all();
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    } else if (g_strcmp0(args[0], "xml") == 0) {
        if (args[1] == NULL) {
            auto_gchar gchar* format = prefs_get_string(PREF_TIME_XMLCONSOLE);
            cons_show("XML Console time format: '%s'.", format);
            return TRUE;
        } else if (g_strcmp0(args[1], "set") == 0 && args[2] != NULL) {
            prefs_set_string(PREF_TIME_XMLCONSOLE, args[2]);
            cons_show("XML Console time format set to '%s'.", args[2]);
            wins_resize_all();
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            prefs_set_string(PREF_TIME_XMLCONSOLE, "off");
            cons_show("XML Console time display disabled.");
            wins_resize_all();
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    } else if (g_strcmp0(args[0], "all") == 0) {
        if (args[1] == NULL) {
            cons_time_setting();
            return TRUE;
        } else if (g_strcmp0(args[1], "set") == 0 && args[2] != NULL) {
            prefs_set_string(PREF_TIME_CONSOLE, args[2]);
            cons_show("Console time format set to '%s'.", args[2]);
            prefs_set_string(PREF_TIME_CHAT, args[2]);
            cons_show("Chat time format set to '%s'.", args[2]);
            prefs_set_string(PREF_TIME_MUC, args[2]);
            cons_show("MUC time format set to '%s'.", args[2]);
            prefs_set_string(PREF_TIME_CONFIG, args[2]);
            cons_show("config time format set to '%s'.", args[2]);
            prefs_set_string(PREF_TIME_PRIVATE, args[2]);
            cons_show("Private chat time format set to '%s'.", args[2]);
            prefs_set_string(PREF_TIME_XMLCONSOLE, args[2]);
            cons_show("XML Console time format set to '%s'.", args[2]);
            wins_resize_all();
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            prefs_set_string(PREF_TIME_CONSOLE, "off");
            cons_show("Console time display disabled.");
            prefs_set_string(PREF_TIME_CHAT, "off");
            cons_show("Chat time display disabled.");
            prefs_set_string(PREF_TIME_MUC, "off");
            cons_show("MUC time display disabled.");
            prefs_set_string(PREF_TIME_CONFIG, "off");
            cons_show("config time display disabled.");
            prefs_set_string(PREF_TIME_PRIVATE, "off");
            cons_show("config time display disabled.");
            prefs_set_string(PREF_TIME_XMLCONSOLE, "off");
            cons_show("XML Console time display disabled.");
            ui_redraw();
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    } else if (g_strcmp0(args[0], "vcard") == 0) {
        if (args[1] == NULL) {
            auto_gchar gchar* format = prefs_get_string(PREF_TIME_VCARD);
            cons_show("vCard time format: %s", format);
            return TRUE;
        } else if (g_strcmp0(args[1], "set") == 0 && args[2] != NULL) {
            prefs_set_string(PREF_TIME_VCARD, args[2]);
            cons_show("vCard time format set to '%s'.", args[2]);
            ui_redraw();
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            cons_show("vCard time cannot be disabled.");
            ui_redraw();
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    } else {
        cons_bad_cmd_usage(command);
        return TRUE;
    }
}

gboolean
cmd_states(ProfWin* window, const char* const command, gchar** args)
{
    if (args[0] == NULL) {
        return TRUE;
    }

    _cmd_set_boolean_preference(args[0], "Sending chat states", PREF_STATES);

    // if disabled, disable outtype and gone
    if (strcmp(args[0], "off") == 0) {
        prefs_set_boolean(PREF_OUTTYPE, FALSE);
        prefs_set_gone(0);
    }

    return TRUE;
}

gboolean
cmd_wintitle(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strcmp0(args[0], "show") != 0 && g_strcmp0(args[0], "goodbye") != 0) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }
    if (g_strcmp0(args[0], "show") == 0 && g_strcmp0(args[1], "off") == 0) {
        ui_clear_win_title();
    }
    if (g_strcmp0(args[0], "show") == 0) {
        _cmd_set_boolean_preference(args[1], "Window title show", PREF_WINTITLE_SHOW);
    } else {
        _cmd_set_boolean_preference(args[1], "Window title goodbye", PREF_WINTITLE_GOODBYE);
    }

    return TRUE;
}

gboolean
cmd_outtype(ProfWin* window, const char* const command, gchar** args)
{
    if (args[0] == NULL) {
        return TRUE;
    }

    _cmd_set_boolean_preference(args[0], "Sending typing notifications", PREF_OUTTYPE);

    // if enabled, enable states
    if (strcmp(args[0], "on") == 0) {
        prefs_set_boolean(PREF_STATES, TRUE);
    }

    return TRUE;
}

gboolean
cmd_gone(ProfWin* window, const char* const command, gchar** args)
{
    char* value = args[0];

    gint period = atoi(value);
    prefs_set_gone(period);
    if (period == 0) {
        cons_show("Automatic leaving conversations after period disabled.");
    } else if (period == 1) {
        cons_show("Leaving conversations after 1 minute of inactivity.");
    } else {
        cons_show("Leaving conversations after %d minutes of inactivity.", period);
    }

    // if enabled, enable states
    if (period > 0) {
        prefs_set_boolean(PREF_STATES, TRUE);
    }

    return TRUE;
}

gboolean
cmd_notify(ProfWin* window, const char* const command, gchar** args)
{
    if (!args[0]) {
        ProfWin* current = wins_get_current();
        if (current->type == WIN_MUC) {
            win_println(current, THEME_DEFAULT, "-", "");
            ProfMucWin* mucwin = (ProfMucWin*)current;

            win_println(window, THEME_DEFAULT, "!", "Notification settings for %s:", mucwin->roomjid);
            if (prefs_has_room_notify(mucwin->roomjid)) {
                if (prefs_get_room_notify(mucwin->roomjid)) {
                    win_println(window, THEME_DEFAULT, "!", "  Message  : ON");
                } else {
                    win_println(window, THEME_DEFAULT, "!", "  Message  : OFF");
                }
            } else {
                if (prefs_get_boolean(PREF_NOTIFY_ROOM)) {
                    win_println(window, THEME_DEFAULT, "!", "  Message  : ON (global setting)");
                } else {
                    win_println(window, THEME_DEFAULT, "!", "  Message  : OFF (global setting)");
                }
            }
            if (prefs_has_room_notify_mention(mucwin->roomjid)) {
                if (prefs_get_room_notify_mention(mucwin->roomjid)) {
                    win_println(window, THEME_DEFAULT, "!", "  Mention  : ON");
                } else {
                    win_println(window, THEME_DEFAULT, "!", "  Mention  : OFF");
                }
            } else {
                if (prefs_get_boolean(PREF_NOTIFY_ROOM_MENTION)) {
                    win_println(window, THEME_DEFAULT, "!", "  Mention  : ON (global setting)");
                } else {
                    win_println(window, THEME_DEFAULT, "!", "  Mention  : OFF (global setting)");
                }
            }
            if (prefs_has_room_notify_trigger(mucwin->roomjid)) {
                if (prefs_get_room_notify_trigger(mucwin->roomjid)) {
                    win_println(window, THEME_DEFAULT, "!", "  Triggers : ON");
                } else {
                    win_println(window, THEME_DEFAULT, "!", "  Triggers : OFF");
                }
            } else {
                if (prefs_get_boolean(PREF_NOTIFY_ROOM_TRIGGER)) {
                    win_println(window, THEME_DEFAULT, "!", "  Triggers : ON (global setting)");
                } else {
                    win_println(window, THEME_DEFAULT, "!", "  Triggers : OFF (global setting)");
                }
            }
            win_println(current, THEME_DEFAULT, "-", "");
        } else {
            cons_show("");
            cons_notify_setting();
            cons_bad_cmd_usage(command);
        }
        return TRUE;
    }

    // chat settings
    if (g_strcmp0(args[0], "chat") == 0) {
        if (g_strcmp0(args[1], "on") == 0) {
            cons_show("Chat notifications enabled.");
            prefs_set_boolean(PREF_NOTIFY_CHAT, TRUE);
        } else if (g_strcmp0(args[1], "off") == 0) {
            cons_show("Chat notifications disabled.");
            prefs_set_boolean(PREF_NOTIFY_CHAT, FALSE);
        } else if (g_strcmp0(args[1], "current") == 0) {
            if (g_strcmp0(args[2], "on") == 0) {
                cons_show("Current window chat notifications enabled.");
                prefs_set_boolean(PREF_NOTIFY_CHAT_CURRENT, TRUE);
            } else if (g_strcmp0(args[2], "off") == 0) {
                cons_show("Current window chat notifications disabled.");
                prefs_set_boolean(PREF_NOTIFY_CHAT_CURRENT, FALSE);
            } else {
                cons_show("Usage: /notify chat current on|off");
            }
        } else if (g_strcmp0(args[1], "text") == 0) {
            if (g_strcmp0(args[2], "on") == 0) {
                cons_show("Showing text in chat notifications enabled.");
                prefs_set_boolean(PREF_NOTIFY_CHAT_TEXT, TRUE);
            } else if (g_strcmp0(args[2], "off") == 0) {
                cons_show("Showing text in chat notifications disabled.");
                prefs_set_boolean(PREF_NOTIFY_CHAT_TEXT, FALSE);
            } else {
                cons_show("Usage: /notify chat text on|off");
            }
        }

        // chat room settings
    } else if (g_strcmp0(args[0], "room") == 0) {
        if (g_strcmp0(args[1], "on") == 0) {
            cons_show("Room notifications enabled.");
            prefs_set_boolean(PREF_NOTIFY_ROOM, TRUE);
        } else if (g_strcmp0(args[1], "off") == 0) {
            cons_show("Room notifications disabled.");
            prefs_set_boolean(PREF_NOTIFY_ROOM, FALSE);
        } else if (g_strcmp0(args[1], "mention") == 0) {
            if (g_strcmp0(args[2], "on") == 0) {
                cons_show("Room notifications with mention enabled.");
                prefs_set_boolean(PREF_NOTIFY_ROOM_MENTION, TRUE);
            } else if (g_strcmp0(args[2], "off") == 0) {
                cons_show("Room notifications with mention disabled.");
                prefs_set_boolean(PREF_NOTIFY_ROOM_MENTION, FALSE);
            } else if (g_strcmp0(args[2], "case_sensitive") == 0) {
                cons_show("Room mention matching set to case sensitive.");
                prefs_set_boolean(PREF_NOTIFY_MENTION_CASE_SENSITIVE, TRUE);
            } else if (g_strcmp0(args[2], "case_insensitive") == 0) {
                cons_show("Room mention matching set to case insensitive.");
                prefs_set_boolean(PREF_NOTIFY_MENTION_CASE_SENSITIVE, FALSE);
            } else if (g_strcmp0(args[2], "word_whole") == 0) {
                cons_show("Room mention matching set to whole word.");
                prefs_set_boolean(PREF_NOTIFY_MENTION_WHOLE_WORD, TRUE);
            } else if (g_strcmp0(args[2], "word_part") == 0) {
                cons_show("Room mention matching set to partial word.");
                prefs_set_boolean(PREF_NOTIFY_MENTION_WHOLE_WORD, FALSE);
            } else {
                cons_show("Usage: /notify room mention on|off");
            }
        } else if (g_strcmp0(args[1], "offline") == 0) {
            if (g_strcmp0(args[2], "on") == 0) {
                cons_show("Room notifications for offline messages enabled.");
                prefs_set_boolean(PREF_NOTIFY_ROOM_OFFLINE, TRUE);
            } else if (g_strcmp0(args[2], "off") == 0) {
                cons_show("Room notifications for offline messages disabled.");
                prefs_set_boolean(PREF_NOTIFY_ROOM_OFFLINE, FALSE);
            } else {
                cons_show("Usage: /notify room offline on|off");
            }
        } else if (g_strcmp0(args[1], "current") == 0) {
            if (g_strcmp0(args[2], "on") == 0) {
                cons_show("Current window chat room message notifications enabled.");
                prefs_set_boolean(PREF_NOTIFY_ROOM_CURRENT, TRUE);
            } else if (g_strcmp0(args[2], "off") == 0) {
                cons_show("Current window chat room message notifications disabled.");
                prefs_set_boolean(PREF_NOTIFY_ROOM_CURRENT, FALSE);
            } else {
                cons_show("Usage: /notify room current on|off");
            }
        } else if (g_strcmp0(args[1], "text") == 0) {
            if (g_strcmp0(args[2], "on") == 0) {
                cons_show("Showing text in chat room message notifications enabled.");
                prefs_set_boolean(PREF_NOTIFY_ROOM_TEXT, TRUE);
            } else if (g_strcmp0(args[2], "off") == 0) {
                cons_show("Showing text in chat room message notifications disabled.");
                prefs_set_boolean(PREF_NOTIFY_ROOM_TEXT, FALSE);
            } else {
                cons_show("Usage: /notify room text on|off");
            }
        } else if (g_strcmp0(args[1], "trigger") == 0) {
            if (g_strcmp0(args[2], "add") == 0) {
                if (!args[3]) {
                    cons_bad_cmd_usage(command);
                } else {
                    gboolean res = prefs_add_room_notify_trigger(args[3]);
                    if (res) {
                        cons_show("Adding room notification trigger: %s", args[3]);
                    } else {
                        cons_show("Room notification trigger already exists: %s", args[3]);
                    }
                }
            } else if (g_strcmp0(args[2], "remove") == 0) {
                if (!args[3]) {
                    cons_bad_cmd_usage(command);
                } else {
                    gboolean res = prefs_remove_room_notify_trigger(args[3]);
                    if (res) {
                        cons_show("Removing room notification trigger: %s", args[3]);
                    } else {
                        cons_show("Room notification trigger does not exist: %s", args[3]);
                    }
                }
            } else if (g_strcmp0(args[2], "list") == 0) {
                GList* triggers = prefs_get_room_notify_triggers();
                GList* curr = triggers;
                if (curr) {
                    cons_show("Room notification triggers:");
                } else {
                    cons_show("No room notification triggers");
                }
                while (curr) {
                    cons_show("  %s", curr->data);
                    curr = g_list_next(curr);
                }
                g_list_free_full(triggers, free);
            } else if (g_strcmp0(args[2], "on") == 0) {
                cons_show("Enabling room notification triggers");
                prefs_set_boolean(PREF_NOTIFY_ROOM_TRIGGER, TRUE);
            } else if (g_strcmp0(args[2], "off") == 0) {
                cons_show("Disabling room notification triggers");
                prefs_set_boolean(PREF_NOTIFY_ROOM_TRIGGER, FALSE);
            } else {
                cons_bad_cmd_usage(command);
            }
        } else {
            cons_show("Usage: /notify room on|off|mention");
        }

        // typing settings
    } else if (g_strcmp0(args[0], "typing") == 0) {
        if (g_strcmp0(args[1], "on") == 0) {
            cons_show("Typing notifications enabled.");
            prefs_set_boolean(PREF_NOTIFY_TYPING, TRUE);
        } else if (g_strcmp0(args[1], "off") == 0) {
            cons_show("Typing notifications disabled.");
            prefs_set_boolean(PREF_NOTIFY_TYPING, FALSE);
        } else if (g_strcmp0(args[1], "current") == 0) {
            if (g_strcmp0(args[2], "on") == 0) {
                cons_show("Current window typing notifications enabled.");
                prefs_set_boolean(PREF_NOTIFY_TYPING_CURRENT, TRUE);
            } else if (g_strcmp0(args[2], "off") == 0) {
                cons_show("Current window typing notifications disabled.");
                prefs_set_boolean(PREF_NOTIFY_TYPING_CURRENT, FALSE);
            } else {
                cons_show("Usage: /notify typing current on|off");
            }
        } else {
            cons_show("Usage: /notify typing on|off");
        }

        // invite settings
    } else if (g_strcmp0(args[0], "invite") == 0) {
        if (g_strcmp0(args[1], "on") == 0) {
            cons_show("Chat room invite notifications enabled.");
            prefs_set_boolean(PREF_NOTIFY_INVITE, TRUE);
        } else if (g_strcmp0(args[1], "off") == 0) {
            cons_show("Chat room invite notifications disabled.");
            prefs_set_boolean(PREF_NOTIFY_INVITE, FALSE);
        } else {
            cons_show("Usage: /notify invite on|off");
        }

        // subscription settings
    } else if (g_strcmp0(args[0], "sub") == 0) {
        if (g_strcmp0(args[1], "on") == 0) {
            cons_show("Subscription notifications enabled.");
            prefs_set_boolean(PREF_NOTIFY_SUB, TRUE);
        } else if (g_strcmp0(args[1], "off") == 0) {
            cons_show("Subscription notifications disabled.");
            prefs_set_boolean(PREF_NOTIFY_SUB, FALSE);
        } else {
            cons_show("Usage: /notify sub on|off");
        }

        // remind settings
    } else if (g_strcmp0(args[0], "remind") == 0) {
        if (!args[1]) {
            cons_bad_cmd_usage(command);
        } else {
            gint period = atoi(args[1]);
            prefs_set_notify_remind(period);
            if (period == 0) {
                cons_show("Message reminders disabled.");
            } else if (period == 1) {
                cons_show("Message reminder period set to 1 second.");
            } else {
                cons_show("Message reminder period set to %d seconds.", period);
            }
        }

        // current chat room settings
    } else if (g_strcmp0(args[0], "on") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();

        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
        } else {
            ProfWin* window = wins_get_current();
            if (window->type != WIN_MUC) {
                cons_show("You must be in a chat room.");
            } else {
                ProfMucWin* mucwin = (ProfMucWin*)window;
                prefs_set_room_notify(mucwin->roomjid, TRUE);
                win_println(window, THEME_DEFAULT, "!", "Notifications enabled for %s", mucwin->roomjid);
            }
        }
    } else if (g_strcmp0(args[0], "off") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();

        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
        } else {
            ProfWin* window = wins_get_current();
            if (window->type != WIN_MUC) {
                cons_show("You must be in a chat room.");
            } else {
                ProfMucWin* mucwin = (ProfMucWin*)window;
                prefs_set_room_notify(mucwin->roomjid, FALSE);
                win_println(window, THEME_DEFAULT, "!", "Notifications disabled for %s", mucwin->roomjid);
            }
        }
    } else if (g_strcmp0(args[0], "mention") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();

        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
        } else {
            if (g_strcmp0(args[1], "on") == 0) {
                ProfWin* window = wins_get_current();
                if (window->type != WIN_MUC) {
                    cons_show("You must be in a chat room.");
                } else {
                    ProfMucWin* mucwin = (ProfMucWin*)window;
                    prefs_set_room_notify_mention(mucwin->roomjid, TRUE);
                    win_println(window, THEME_DEFAULT, "!", "Mention notifications enabled for %s", mucwin->roomjid);
                }
            } else if (g_strcmp0(args[1], "off") == 0) {
                ProfWin* window = wins_get_current();
                if (window->type != WIN_MUC) {
                    cons_show("You must be in a chat rooms.");
                } else {
                    ProfMucWin* mucwin = (ProfMucWin*)window;
                    prefs_set_room_notify_mention(mucwin->roomjid, FALSE);
                    win_println(window, THEME_DEFAULT, "!", "Mention notifications disabled for %s", mucwin->roomjid);
                }
            } else {
                cons_bad_cmd_usage(command);
            }
        }
    } else if (g_strcmp0(args[0], "trigger") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();

        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
        } else {
            if (g_strcmp0(args[1], "on") == 0) {
                ProfWin* window = wins_get_current();
                if (window->type != WIN_MUC) {
                    cons_show("You must be in a chat room.");
                } else {
                    ProfMucWin* mucwin = (ProfMucWin*)window;
                    prefs_set_room_notify_trigger(mucwin->roomjid, TRUE);
                    win_println(window, THEME_DEFAULT, "!", "Custom trigger notifications enabled for %s", mucwin->roomjid);
                }
            } else if (g_strcmp0(args[1], "off") == 0) {
                ProfWin* window = wins_get_current();
                if (window->type != WIN_MUC) {
                    cons_show("You must be in a chat rooms.");
                } else {
                    ProfMucWin* mucwin = (ProfMucWin*)window;
                    prefs_set_room_notify_trigger(mucwin->roomjid, FALSE);
                    win_println(window, THEME_DEFAULT, "!", "Custom trigger notifications disabled for %s", mucwin->roomjid);
                }
            } else {
                cons_bad_cmd_usage(command);
            }
        }
    } else if (g_strcmp0(args[0], "reset") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();

        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
        } else {
            ProfWin* window = wins_get_current();
            if (window->type != WIN_MUC) {
                cons_show("You must be in a chat room.");
            } else {
                ProfMucWin* mucwin = (ProfMucWin*)window;
                gboolean res = prefs_reset_room_notify(mucwin->roomjid);
                if (res) {
                    win_println(window, THEME_DEFAULT, "!", "Notification settings set to global defaults for %s", mucwin->roomjid);
                } else {
                    win_println(window, THEME_DEFAULT, "!", "No custom notification settings for %s", mucwin->roomjid);
                }
            }
        }
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_inpblock(ProfWin* window, const char* const command, gchar** args)
{
    char* subcmd = args[0];
    char* value = args[1];

    if (g_strcmp0(subcmd, "timeout") == 0) {
        if (value == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        int intval = 0;
        auto_char char* err_msg = NULL;
        gboolean res = strtoi_range(value, &intval, 1, 1000, &err_msg);
        if (res) {
            cons_show("Input blocking set to %d milliseconds.", intval);
            prefs_set_inpblock(intval);
            inp_nonblocking(FALSE);
        } else {
            cons_show(err_msg);
        }

        return TRUE;
    }

    if (g_strcmp0(subcmd, "dynamic") == 0) {
        if (value == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        if (g_strcmp0(value, "on") != 0 && g_strcmp0(value, "off") != 0) {
            cons_show("Dynamic must be one of 'on' or 'off'");
            return TRUE;
        }

        _cmd_set_boolean_preference(value, "Dynamic input blocking", PREF_INPBLOCK_DYNAMIC);
        return TRUE;
    }

    cons_bad_cmd_usage(command);

    return TRUE;
}

gboolean
cmd_titlebar(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strcmp0(args[0], "up") == 0) {
        gboolean result = prefs_titlebar_pos_up();
        if (result) {
            ui_resize();
            cons_show("Title bar moved up.");
        } else {
            cons_show("Could not move title bar up.");
        }

        return TRUE;
    }
    if (g_strcmp0(args[0], "down") == 0) {
        gboolean result = prefs_titlebar_pos_down();
        if (result) {
            ui_resize();
            cons_show("Title bar moved down.");
        } else {
            cons_show("Could not move title bar down.");
        }

        return TRUE;
    }
    if (g_strcmp0(args[0], "room") == 0) {
        if (g_strcmp0(args[1], "title") == 0) {
            if (g_strcmp0(args[2], "bookmark") == 0 || g_strcmp0(args[2], "jid") == 0 || g_strcmp0(args[2], "localpart") == 0 || g_strcmp0(args[2], "name") == 0) {
                cons_show("MUC windows will display '%s' as the window title.", args[2]);
                prefs_set_string(PREF_TITLEBAR_MUC_TITLE, args[2]);
                return TRUE;
            }
        }
    }

    cons_bad_cmd_usage(command);

    return TRUE;
}

gboolean
cmd_titlebar_show_hide(ProfWin* window, const char* const command, gchar** args)
{
    if (args[1] != NULL) {
        if (g_strcmp0(args[0], "show") == 0) {

            if (g_strcmp0(args[1], "tls") == 0) {
                cons_show("TLS titlebar indicator enabled.");
                prefs_set_boolean(PREF_TLS_SHOW, TRUE);
            } else if (g_strcmp0(args[1], "encwarn") == 0) {
                cons_show("Encryption warning titlebar indicator enabled.");
                prefs_set_boolean(PREF_ENC_WARN, TRUE);
            } else if (g_strcmp0(args[1], "resource") == 0) {
                cons_show("Showing resource in titlebar enabled.");
                prefs_set_boolean(PREF_RESOURCE_TITLE, TRUE);
            } else if (g_strcmp0(args[1], "presence") == 0) {
                cons_show("Showing contact presence in titlebar enabled.");
                prefs_set_boolean(PREF_PRESENCE, TRUE);
            } else {
                cons_bad_cmd_usage(command);
            }
        } else if (g_strcmp0(args[0], "hide") == 0) {

            if (g_strcmp0(args[1], "tls") == 0) {
                cons_show("TLS titlebar indicator disabled.");
                prefs_set_boolean(PREF_TLS_SHOW, FALSE);
            } else if (g_strcmp0(args[1], "encwarn") == 0) {
                cons_show("Encryption warning titlebar indicator disabled.");
                prefs_set_boolean(PREF_ENC_WARN, FALSE);
            } else if (g_strcmp0(args[1], "resource") == 0) {
                cons_show("Showing resource in titlebar disabled.");
                prefs_set_boolean(PREF_RESOURCE_TITLE, FALSE);
            } else if (g_strcmp0(args[1], "presence") == 0) {
                cons_show("Showing contact presence in titlebar disabled.");
                prefs_set_boolean(PREF_PRESENCE, FALSE);
            } else {
                cons_bad_cmd_usage(command);
            }
        } else {
            cons_bad_cmd_usage(command);
        }
    }

    return TRUE;
}

gboolean
cmd_mainwin(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strcmp0(args[0], "up") == 0) {
        gboolean result = prefs_mainwin_pos_up();
        if (result) {
            ui_resize();
            cons_show("Main window moved up.");
        } else {
            cons_show("Could not move main window up.");
        }

        return TRUE;
    }
    if (g_strcmp0(args[0], "down") == 0) {
        gboolean result = prefs_mainwin_pos_down();
        if (result) {
            ui_resize();
            cons_show("Main window moved down.");
        } else {
            cons_show("Could not move main window down.");
        }

        return TRUE;
    }

    cons_bad_cmd_usage(command);

    return TRUE;
}

gboolean
cmd_statusbar(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strcmp0(args[0], "show") == 0) {
        if (g_strcmp0(args[1], "name") == 0) {
            prefs_set_boolean(PREF_STATUSBAR_SHOW_NAME, TRUE);
            cons_show("Enabled showing tab names.");
            ui_resize();
            return TRUE;
        }
        if (g_strcmp0(args[1], "number") == 0) {
            prefs_set_boolean(PREF_STATUSBAR_SHOW_NUMBER, TRUE);
            cons_show("Enabled showing tab numbers.");
            ui_resize();
            return TRUE;
        }
        if (g_strcmp0(args[1], "read") == 0) {
            prefs_set_boolean(PREF_STATUSBAR_SHOW_READ, TRUE);
            cons_show("Enabled showing inactive tabs.");
            ui_resize();
            return TRUE;
        }
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (g_strcmp0(args[0], "hide") == 0) {
        if (g_strcmp0(args[1], "name") == 0) {
            if (prefs_get_boolean(PREF_STATUSBAR_SHOW_NUMBER) == FALSE) {
                cons_show("Cannot disable both names and numbers in statusbar.");
                cons_show("Use '/statusbar maxtabs 0' to hide tabs.");
                return TRUE;
            }
            prefs_set_boolean(PREF_STATUSBAR_SHOW_NAME, FALSE);
            cons_show("Disabled showing tab names.");
            ui_resize();
            return TRUE;
        }
        if (g_strcmp0(args[1], "number") == 0) {
            if (prefs_get_boolean(PREF_STATUSBAR_SHOW_NAME) == FALSE) {
                cons_show("Cannot disable both names and numbers in statusbar.");
                cons_show("Use '/statusbar maxtabs 0' to hide tabs.");
                return TRUE;
            }
            prefs_set_boolean(PREF_STATUSBAR_SHOW_NUMBER, FALSE);
            cons_show("Disabled showing tab numbers.");
            ui_resize();
            return TRUE;
        }
        if (g_strcmp0(args[1], "read") == 0) {
            prefs_set_boolean(PREF_STATUSBAR_SHOW_READ, FALSE);
            cons_show("Disabled showing inactive tabs.");
            ui_resize();
            return TRUE;
        }
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (g_strcmp0(args[0], "maxtabs") == 0) {
        if (args[1] == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        char* value = args[1];
        int intval = 0;
        auto_char char* err_msg = NULL;
        gboolean res = strtoi_range(value, &intval, 0, INT_MAX, &err_msg);
        if (res) {
            if (intval < 0 || intval > 10) {
                cons_bad_cmd_usage(command);
                return TRUE;
            }

            prefs_set_statusbartabs(intval);
            if (intval == 0) {
                cons_show("Status bar tabs disabled.");
            } else {
                cons_show("Status bar tabs set to %d.", intval);
            }
            ui_resize();
            return TRUE;
        } else {
            cons_show(err_msg);
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    }

    if (g_strcmp0(args[0], "tablen") == 0) {
        if (args[1] == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        char* value = args[1];
        int intval = 0;
        auto_char char* err_msg = NULL;
        gboolean res = strtoi_range(value, &intval, 0, INT_MAX, &err_msg);
        if (res) {
            if (intval < 0) {
                cons_bad_cmd_usage(command);
                return TRUE;
            }

            prefs_set_statusbartablen(intval);
            if (intval == 0) {
                cons_show("Maximum tab length disabled.");
            } else {
                cons_show("Maximum tab length set to %d.", intval);
            }
            ui_resize();
            return TRUE;
        } else {
            cons_show(err_msg);
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    }

    if (g_strcmp0(args[0], "tabmode") == 0) {
        if ((g_strcmp0(args[1], "default") != 0) && (g_strcmp0(args[1], "actlist") != 0) && (g_strcmp0(args[1], "dynamic") != 0)) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
        char* tabmode = args[1];
        prefs_set_string(PREF_STATUSBAR_TABMODE, tabmode);
        cons_show("Using \"%s\" tabmode for statusbar.", tabmode);
        ui_resize();
        return TRUE;
    }

    if (g_strcmp0(args[0], "self") == 0) {
        if (g_strcmp0(args[1], "barejid") == 0) {
            prefs_set_string(PREF_STATUSBAR_SELF, "barejid");
            cons_show("Using barejid for statusbar title.");
            ui_resize();
            return TRUE;
        }
        if (g_strcmp0(args[1], "fulljid") == 0) {
            prefs_set_string(PREF_STATUSBAR_SELF, "fulljid");
            cons_show("Using fulljid for statusbar title.");
            ui_resize();
            return TRUE;
        }
        if (g_strcmp0(args[1], "user") == 0) {
            prefs_set_string(PREF_STATUSBAR_SELF, "user");
            cons_show("Using user for statusbar title.");
            ui_resize();
            return TRUE;
        }
        if (g_strcmp0(args[1], "off") == 0) {
            prefs_set_string(PREF_STATUSBAR_SELF, "off");
            cons_show("Disabling statusbar title.");
            ui_resize();
            return TRUE;
        }
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (g_strcmp0(args[0], "chat") == 0) {
        if (g_strcmp0(args[1], "jid") == 0) {
            prefs_set_string(PREF_STATUSBAR_CHAT, "jid");
            cons_show("Using jid for chat tabs.");
            ui_resize();
            return TRUE;
        }
        if (g_strcmp0(args[1], "user") == 0) {
            prefs_set_string(PREF_STATUSBAR_CHAT, "user");
            cons_show("Using user for chat tabs.");
            ui_resize();
            return TRUE;
        }
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (g_strcmp0(args[0], "room") == 0) {
        if (g_strcmp0(args[1], "title") == 0) {
            if (g_strcmp0(args[2], "bookmark") == 0 || g_strcmp0(args[2], "jid") == 0 || g_strcmp0(args[2], "localpart") == 0 || g_strcmp0(args[2], "name") == 0) {
                prefs_set_string(PREF_STATUSBAR_ROOM_TITLE, args[2]);
                cons_show("Displaying '%s' as the title for MUC tabs.", args[2]);
                ui_resize();
                return TRUE;
            }
        }
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (g_strcmp0(args[0], "up") == 0) {
        gboolean result = prefs_statusbar_pos_up();
        if (result) {
            ui_resize();
            cons_show("Status bar moved up");
        } else {
            cons_show("Could not move status bar up.");
        }

        return TRUE;
    }
    if (g_strcmp0(args[0], "down") == 0) {
        gboolean result = prefs_statusbar_pos_down();
        if (result) {
            ui_resize();
            cons_show("Status bar moved down.");
        } else {
            cons_show("Could not move status bar down.");
        }

        return TRUE;
    }

    cons_bad_cmd_usage(command);

    return TRUE;
}

gboolean
cmd_inputwin(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strcmp0(args[0], "up") == 0) {
        gboolean result = prefs_inputwin_pos_up();
        if (result) {
            ui_resize();
            cons_show("Input window moved up.");
        } else {
            cons_show("Could not move input window up.");
        }

        return TRUE;
    }
    if (g_strcmp0(args[0], "down") == 0) {
        gboolean result = prefs_inputwin_pos_down();
        if (result) {
            ui_resize();
            cons_show("Input window moved down.");
        } else {
            cons_show("Could not move input window down.");
        }

        return TRUE;
    }

    cons_bad_cmd_usage(command);

    return TRUE;
}

gboolean
cmd_log(ProfWin* window, const char* const command, gchar** args)
{
    char* subcmd = args[0];
    char* value = args[1];

    if (strcmp(subcmd, "where") == 0) {
        cons_show("Log file: %s", get_log_file_location());
        return TRUE;
    }

    if (value == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (strcmp(subcmd, "maxsize") == 0) {
        int intval = 0;
        auto_char char* err_msg = NULL;
        gboolean res = strtoi_range(value, &intval, PREFS_MIN_LOG_SIZE, INT_MAX, &err_msg);
        if (res) {
            prefs_set_max_log_size(intval);
            cons_show("Log maximum size set to %d bytes", intval);
        } else {
            cons_show(err_msg);
        }
        return TRUE;
    }

    if (strcmp(subcmd, "rotate") == 0) {
        _cmd_set_boolean_preference(value, "Log rotate", PREF_LOG_ROTATE);
        return TRUE;
    }

    if (strcmp(subcmd, "shared") == 0) {
        _cmd_set_boolean_preference(value, "Shared log", PREF_LOG_SHARED);
        cons_show("Setting only takes effect after saving and restarting Profanity.");
        return TRUE;
    }

    if (strcmp(subcmd, "level") == 0) {
        log_level_t prof_log_level;
        if (log_level_from_string(value, &prof_log_level) == 0) {
            log_close();
            log_init(prof_log_level, NULL);

            cons_show("Log level changed to: %s.", value);
            return TRUE;
        }
    }

    cons_bad_cmd_usage(command);

    return TRUE;
}

gboolean
cmd_reconnect(ProfWin* window, const char* const command, gchar** args)
{
    char* value = args[0];

    int intval = 0;
    auto_char char* err_msg = NULL;
    if (g_strcmp0(value, "now") == 0) {
        cl_ev_reconnect();
    } else if (strtoi_range(value, &intval, 0, INT_MAX, &err_msg)) {
        prefs_set_reconnect(intval);
        if (intval == 0) {
            cons_show("Reconnect disabled.", intval);
        } else {
            cons_show("Reconnect interval set to %d seconds.", intval);
        }
    } else {
        cons_show(err_msg);
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_autoping(ProfWin* window, const char* const command, gchar** args)
{
    char* cmd = args[0];
    char* value = args[1];

    if (g_strcmp0(cmd, "set") == 0) {
        int intval = 0;
        auto_char char* err_msg = NULL;
        gboolean res = strtoi_range(value, &intval, 0, INT_MAX, &err_msg);
        if (res) {
            prefs_set_autoping(intval);
            iq_set_autoping(intval);
            if (intval == 0) {
                cons_show("Autoping disabled.");
            } else {
                cons_show("Autoping interval set to %d seconds.", intval);
            }
        } else {
            cons_show(err_msg);
            cons_bad_cmd_usage(command);
        }

    } else if (g_strcmp0(cmd, "timeout") == 0) {
        int intval = 0;
        auto_char char* err_msg = NULL;
        gboolean res = strtoi_range(value, &intval, 0, INT_MAX, &err_msg);
        if (res) {
            prefs_set_autoping_timeout(intval);
            if (intval == 0) {
                cons_show("Autoping timeout disabled.");
            } else {
                cons_show("Autoping timeout set to %d seconds.", intval);
            }
        } else {
            cons_show(err_msg);
            cons_bad_cmd_usage(command);
        }

    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_ping(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (args[0] == NULL && connection_supports(XMPP_FEATURE_PING) == FALSE) {
        cons_show("Server does not support ping requests (%s).", XMPP_FEATURE_PING);
        return TRUE;
    }

    if (args[0] != NULL && caps_jid_has_feature(args[0], XMPP_FEATURE_PING) == FALSE) {
        cons_show("%s does not support ping requests.", args[0]);
        return TRUE;
    }

    iq_send_ping(args[0]);

    if (args[0] == NULL) {
        cons_show("Pinged server…");
    } else {
        cons_show("Pinged %s…", args[0]);
    }
    return TRUE;
}

gboolean
cmd_autoaway(ProfWin* window, const char* const command, gchar** args)
{
    if (!_string_matches_one_of("Setting", args[0], FALSE, "mode", "time", "message", "check", NULL)) {
        return TRUE;
    }

    if (g_strcmp0(args[0], "mode") == 0) {
        if (_string_matches_one_of("Mode", args[1], FALSE, "idle", "away", "off", NULL)) {
            prefs_set_string(PREF_AUTOAWAY_MODE, args[1]);
            cons_show("Auto away mode set to: %s.", args[1]);
        }

        return TRUE;
    }

    if ((g_strcmp0(args[0], "time") == 0) && (args[2] != NULL)) {
        if (g_strcmp0(args[1], "away") == 0) {
            int minutesval = 0;
            auto_char char* err_msg = NULL;
            gboolean res = strtoi_range(args[2], &minutesval, 1, INT_MAX, &err_msg);
            if (res) {
                prefs_set_autoaway_time(minutesval);
                if (minutesval == 1) {
                    cons_show("Auto away time set to: 1 minute.");
                } else {
                    cons_show("Auto away time set to: %d minutes.", minutesval);
                }
            } else {
                cons_show(err_msg);
            }

            return TRUE;
        } else if (g_strcmp0(args[1], "xa") == 0) {
            int minutesval = 0;
            auto_char char* err_msg = NULL;
            gboolean res = strtoi_range(args[2], &minutesval, 0, INT_MAX, &err_msg);
            if (res) {
                int away_time = prefs_get_autoaway_time();
                if (minutesval != 0 && minutesval <= away_time) {
                    cons_show("Auto xa time must be larger than auto away time.");
                } else {
                    prefs_set_autoxa_time(minutesval);
                    if (minutesval == 0) {
                        cons_show("Auto xa time disabled.");
                    } else if (minutesval == 1) {
                        cons_show("Auto xa time set to: 1 minute.");
                    } else {
                        cons_show("Auto xa time set to: %d minutes.", minutesval);
                    }
                }
            } else {
                cons_show(err_msg);
            }

            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    }

    if (g_strcmp0(args[0], "message") == 0) {
        if (g_strcmp0(args[1], "away") == 0 && args[2] != NULL) {
            if (g_strcmp0(args[2], "off") == 0) {
                prefs_set_string(PREF_AUTOAWAY_MESSAGE, NULL);
                cons_show("Auto away message cleared.");
            } else {
                prefs_set_string(PREF_AUTOAWAY_MESSAGE, args[2]);
                cons_show("Auto away message set to: \"%s\".", args[2]);
            }

            return TRUE;
        } else if (g_strcmp0(args[1], "xa") == 0 && args[2] != NULL) {
            if (g_strcmp0(args[2], "off") == 0) {
                prefs_set_string(PREF_AUTOXA_MESSAGE, NULL);
                cons_show("Auto xa message cleared.");
            } else {
                prefs_set_string(PREF_AUTOXA_MESSAGE, args[2]);
                cons_show("Auto xa message set to: \"%s\".", args[2]);
            }

            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    }

    if (g_strcmp0(args[0], "check") == 0) {
        _cmd_set_boolean_preference(args[1], "Online check", PREF_AUTOAWAY_CHECK);
        return TRUE;
    }

    cons_bad_cmd_usage(command);
    return TRUE;
}

gboolean
cmd_priority(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    char* value = args[0];

    int intval = 0;
    auto_char char* err_msg = NULL;
    gboolean res = strtoi_range(value, &intval, -128, 127, &err_msg);
    if (res) {
        accounts_set_priority_all(session_get_account_name(), intval);
        resource_presence_t last_presence = accounts_get_last_presence(session_get_account_name());
        cl_ev_presence_send(last_presence, 0);
        cons_show("Priority set to %d.", intval);
    } else {
        cons_show(err_msg);
    }

    return TRUE;
}

gboolean
cmd_vercheck(ProfWin* window, const char* const command, gchar** args)
{
    int num_args = g_strv_length(args);

    if (num_args == 0) {
        cons_check_version(TRUE);
        return TRUE;
    } else {
        _cmd_set_boolean_preference(args[0], "Version checking", PREF_VERCHECK);
        return TRUE;
    }
}

gboolean
cmd_xmlconsole(ProfWin* window, const char* const command, gchar** args)
{
    ProfXMLWin* xmlwin = wins_get_xmlconsole();
    if (xmlwin) {
        ui_focus_win((ProfWin*)xmlwin);
    } else {
        ProfWin* window = wins_new_xmlconsole();
        ui_focus_win(window);
    }

    return TRUE;
}

gboolean
cmd_flash(ProfWin* window, const char* const command, gchar** args)
{
    _cmd_set_boolean_preference(args[0], "Screen flash", PREF_FLASH);
    return TRUE;
}

gboolean
cmd_tray(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_GTK
    if (g_strcmp0(args[0], "timer") == 0) {
        if (args[1] == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        if (prefs_get_boolean(PREF_TRAY) == FALSE) {
            cons_show("Tray icon not currently enabled, see /help tray");
            return TRUE;
        }

        int intval = 0;
        auto_char char* err_msg = NULL;
        gboolean res = strtoi_range(args[1], &intval, 1, 10, &err_msg);
        if (res) {
            if (intval == 1) {
                cons_show("Tray timer set to 1 second.");
            } else {
                cons_show("Tray timer set to %d seconds.", intval);
            }
            prefs_set_tray_timer(intval);
            if (prefs_get_boolean(PREF_TRAY)) {
                tray_set_timer(intval);
            }
        } else {
            cons_show(err_msg);
        }

        return TRUE;
    } else if (g_strcmp0(args[0], "read") == 0) {
        if (prefs_get_boolean(PREF_TRAY) == FALSE) {
            cons_show("Tray icon not currently enabled, see /help tray");
        } else if (g_strcmp0(args[1], "on") == 0) {
            prefs_set_boolean(PREF_TRAY_READ, TRUE);
            cons_show("Tray icon enabled when no unread messages.");
        } else if (g_strcmp0(args[1], "off") == 0) {
            prefs_set_boolean(PREF_TRAY_READ, FALSE);
            cons_show("Tray icon disabled when no unread messages.");
        } else {
            cons_bad_cmd_usage(command);
        }

        return TRUE;
    } else {
        gboolean old = prefs_get_boolean(PREF_TRAY);
        _cmd_set_boolean_preference(args[0], "Tray icon", PREF_TRAY);
        gboolean new = prefs_get_boolean(PREF_TRAY);
        if (old != new) {
            if (new) {
                tray_enable();
            } else {
                tray_disable();
            }
        }

        return TRUE;
    }
#else
    cons_show("This version of Profanity has not been built with GTK Tray Icon support enabled");
    return TRUE;
#endif
}

gboolean
cmd_intype(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strcmp0(args[0], "console") == 0) {
        _cmd_set_boolean_preference(args[1], "Show contact typing in console", PREF_INTYPE_CONSOLE);
    } else if (g_strcmp0(args[0], "titlebar") == 0) {
        _cmd_set_boolean_preference(args[1], "Show contact typing in titlebar", PREF_INTYPE);
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_splash(ProfWin* window, const char* const command, gchar** args)
{
    _cmd_set_boolean_preference(args[0], "Splash screen", PREF_SPLASH);
    return TRUE;
}

gboolean
cmd_autoconnect(ProfWin* window, const char* const command, gchar** args)
{
    if (strcmp(args[0], "off") == 0) {
        prefs_set_string(PREF_CONNECT_ACCOUNT, NULL);
        cons_show("Autoconnect account disabled.");
    } else if (strcmp(args[0], "set") == 0) {
        if (args[1] == NULL || strlen(args[1]) == 0) {
            cons_bad_cmd_usage(command);
        } else {
            if (accounts_account_exists(args[1])) {
                prefs_set_string(PREF_CONNECT_ACCOUNT, args[1]);
                cons_show("Autoconnect account set to: %s.", args[1]);
            } else {
                cons_show_error("Account '%s' does not exist.", args[1]);
            }
        }
    } else {
        cons_bad_cmd_usage(command);
    }
    return TRUE;
}

gboolean
cmd_privacy(ProfWin* window, const char* const command, gchar** args)
{
    if (args[0] == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (g_strcmp0(args[0], "logging") == 0) {
        gchar* arg = args[1];
        if (arg == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        if (g_strcmp0(arg, "on") == 0) {
            cons_show("Logging enabled.");
            prefs_set_string(PREF_DBLOG, arg);
            prefs_set_boolean(PREF_CHLOG, TRUE);
            prefs_set_boolean(PREF_HISTORY, TRUE);
        } else if (g_strcmp0(arg, "off") == 0) {
            cons_show("Logging disabled.");
            prefs_set_string(PREF_DBLOG, arg);
            prefs_set_boolean(PREF_CHLOG, FALSE);
            prefs_set_boolean(PREF_HISTORY, FALSE);
        } else if (g_strcmp0(arg, "redact") == 0) {
            cons_show("Messages are going to be redacted.");
            prefs_set_string(PREF_DBLOG, arg);
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
        return TRUE;
    }

    return TRUE;
}

gboolean
cmd_logging(ProfWin* window, const char* const command, gchar** args)
{
    if (args[0] == NULL) {
        cons_logging_setting();
        return TRUE;
    }

    if (strcmp(args[0], "chat") == 0 && args[1] != NULL) {
        _cmd_set_boolean_preference(args[1], "Chat logging", PREF_CHLOG);

        // if set to off, disable history
        if (strcmp(args[1], "off") == 0) {
            prefs_set_boolean(PREF_HISTORY, FALSE);
        }

        return TRUE;
    } else if (g_strcmp0(args[0], "group") == 0 && args[1] != NULL) {
        _cmd_set_boolean_preference(args[1], "Groupchat logging", PREF_GRLOG);
        return TRUE;
    }

    cons_bad_cmd_usage(command);
    return TRUE;
}

gboolean
cmd_history(ProfWin* window, const char* const command, gchar** args)
{
    if (args[0] == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    _cmd_set_boolean_preference(args[0], "Chat history", PREF_HISTORY);

    // if set to on, set chlog (/logging chat on)
    if (strcmp(args[0], "on") == 0) {
        prefs_set_boolean(PREF_CHLOG, TRUE);
    }

    return TRUE;
}

gboolean
cmd_carbons(ProfWin* window, const char* const command, gchar** args)
{
    if (args[0] == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    _cmd_set_boolean_preference(args[0], "Message carbons preference", PREF_CARBONS);

    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status == JABBER_CONNECTED) {
        // enable carbons
        if (strcmp(args[0], "on") == 0) {
            iq_enable_carbons();
        } else if (strcmp(args[0], "off") == 0) {
            iq_disable_carbons();
        }
    }

    return TRUE;
}

gboolean
cmd_receipts(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strcmp0(args[0], "send") == 0) {
        _cmd_set_boolean_preference(args[1], "Send delivery receipts", PREF_RECEIPTS_SEND);
        if (g_strcmp0(args[1], "on") == 0) {
            caps_add_feature(XMPP_FEATURE_RECEIPTS);
        }
        if (g_strcmp0(args[1], "off") == 0) {
            caps_remove_feature(XMPP_FEATURE_RECEIPTS);
        }
    } else if (g_strcmp0(args[0], "request") == 0) {
        _cmd_set_boolean_preference(args[1], "Request delivery receipts", PREF_RECEIPTS_REQUEST);
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

static gboolean
_is_correct_plugin_extension(gchar* plugin)
{
    return g_str_has_suffix(plugin, ".py") || g_str_has_suffix(plugin, ".so");
}

static gboolean
_http_based_uri_scheme(const char* scheme)
{
    return scheme != NULL && (g_strcmp0(scheme, "http") == 0 || g_strcmp0(scheme, "https") == 0);
}

gboolean
cmd_plugins_install(ProfWin* window, const char* const command, gchar** args)
{
    auto_gchar gchar* path = NULL;

    if (args[1] == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    auto_gchar gchar* scheme = g_uri_parse_scheme(args[1]);
    if (_http_based_uri_scheme(scheme)) {
        if (!_is_correct_plugin_extension(args[1])) {
            cons_show("Please, use url ending with correct file name. Plugins must have one of the following extensions: \".py\" or \".so\".");
            return TRUE;
        }
        _download_install_plugin(window, args[1], NULL);
        return TRUE;
    }

    if (strchr(args[1], '/')) {
        path = get_expanded_path(args[1]);
    } else {
        if (g_str_has_suffix(args[1], ".py")) {
            path = g_strdup_printf("%s/%s", GLOBAL_PYTHON_PLUGINS_PATH, args[1]);
        } else if (g_str_has_suffix(args[1], ".so")) {
            path = g_strdup_printf("%s/%s", GLOBAL_C_PLUGINS_PATH, args[1]);
        } else {
            cons_show("Plugins must have one of the following extensions: \".py\" or \".so\".");
            return TRUE;
        }
    }

    if (access(path, R_OK) != 0) {
        cons_show("Cannot access: %s", path);
        return TRUE;
    }

    if (is_regular_file(path)) {
        if (!_is_correct_plugin_extension(args[1])) {
            cons_show("Plugins must have one of the following extensions: \".py\" or \".so\".");
            return TRUE;
        }

        GString* error_message = g_string_new(NULL);
        auto_gchar gchar* plugin_name = g_path_get_basename(path);
        gboolean result = plugins_install(plugin_name, path, error_message);
        if (result) {
            cons_show("Plugin installed and loaded: %s", plugin_name);
        } else {
            cons_show("Failed to install plugin: %s. %s", plugin_name, error_message->str);
        }
        g_string_free(error_message, TRUE);
        return TRUE;
    } else if (is_dir(path)) {
        PluginsInstallResult* result = plugins_install_all(path);
        if (result->installed || result->failed) {
            if (result->installed) {
                GSList* curr = result->installed;
                cons_show("");
                cons_show("Installed and loaded plugins (%u):", g_slist_length(curr));
                while (curr) {
                    cons_show("  %s", curr->data);
                    curr = g_slist_next(curr);
                }
            }
            if (result->failed) {
                GSList* curr = result->failed;
                cons_show("");
                cons_show("Failed installs (%u):", g_slist_length(curr));
                while (curr) {
                    cons_show("  %s", curr->data);
                    curr = g_slist_next(curr);
                }
            }
        } else {
            cons_show("No plugins found in: %s", path);
        }
        plugins_free_install_result(result);
        return TRUE;
    } else {
        cons_show("Argument must be a file or directory.");
    }

    return TRUE;
}

gboolean
cmd_plugins_update(ProfWin* window, const char* const command, gchar** args)
{
    if (args[1] == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    auto_gchar gchar* scheme = g_uri_parse_scheme(args[1]);
    if (_http_based_uri_scheme(scheme)) {
        auto_char char* plugin_name = basename_from_url(args[1]);
        if (!_is_correct_plugin_extension(plugin_name)) {
            cons_show("Please, use url ending with correct file name. Plugins must have one of the following extensions: \".py\" or \".so\".");
            return TRUE;
        }

        if (!plugins_uninstall(plugin_name)) {
            cons_show("Failed to uninstall plugin: %s.", plugin_name);
            return TRUE;
        }

        _download_install_plugin(window, args[1], NULL);
        return TRUE;
    }

    auto_gchar gchar* path = get_expanded_path(args[1]);

    if (access(path, R_OK) != 0) {
        cons_show("File not found: %s", path);
        return TRUE;
    }

    if (!is_regular_file(path)) {
        cons_show("Argument must be a file.");
        return TRUE;
    }

    if (!g_str_has_suffix(path, ".py") && !g_str_has_suffix(path, ".so")) {
        cons_show("Plugins must have one of the following extensions: '.py' or '.so'");
        return TRUE;
    }

    auto_gchar gchar* plugin_name = g_path_get_basename(path);
    if (!plugins_uninstall(plugin_name)) {
        cons_show("Failed to uninstall plugin: %s.", plugin_name);
        return TRUE;
    }

    GString* error_message = g_string_new(NULL);
    if (plugins_install(plugin_name, path, error_message)) {
        cons_show("Plugin installed: %s", plugin_name);
    } else {
        cons_show("Failed to install plugin: %s. %s", plugin_name, error_message->str);
    }

    g_string_free(error_message, TRUE);
    return TRUE;
}

gboolean
cmd_plugins_uninstall(ProfWin* window, const char* const command, gchar** args)
{
    if (args[1] == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    gboolean res = plugins_uninstall(args[1]);
    if (res) {
        cons_show("Uninstalled plugin: %s", args[1]);
    } else {
        cons_show("Failed to uninstall plugin: %s", args[1]);
    }

    return TRUE;
}

gboolean
cmd_plugins_load(ProfWin* window, const char* const command, gchar** args)
{
    if (args[1] == NULL) {
        GSList* loaded = plugins_load_all();
        if (loaded) {
            cons_show("Loaded plugins:");
            GSList* curr = loaded;
            while (curr) {
                cons_show("  %s", curr->data);
                curr = g_slist_next(curr);
            }
            g_slist_free_full(loaded, g_free);
        } else {
            cons_show("No plugins loaded.");
        }
        return TRUE;
    }

    GString* error_message = g_string_new(NULL);
    gboolean res = plugins_load(args[1], error_message);
    if (res) {
        cons_show("Loaded plugin: %s", args[1]);
    } else {
        cons_show("Failed to load plugin: %s. %s", args[1], error_message->str);
    }
    g_string_free(error_message, TRUE);

    return TRUE;
}

gboolean
cmd_plugins_unload(ProfWin* window, const char* const command, gchar** args)
{
    if (args[1] == NULL) {
        gboolean res = plugins_unload_all();
        if (res) {
            cons_show("Unloaded all plugins.");
        } else {
            cons_show("No plugins unloaded.");
        }
        return TRUE;
    }

    gboolean res = plugins_unload(args[1]);
    if (res) {
        cons_show("Unloaded plugin: %s", args[1]);
    } else {
        cons_show("Failed to unload plugin: %s", args[1]);
    }

    return TRUE;
}

gboolean
cmd_plugins_reload(ProfWin* window, const char* const command, gchar** args)
{
    if (args[1] == NULL) {
        plugins_reload_all();
        cons_show("Reloaded all plugins");
        return TRUE;
    }

    GString* error_message = g_string_new(NULL);
    gboolean res = plugins_reload(args[1], error_message);
    if (res) {
        cons_show("Reloaded plugin: %s", args[1]);
    } else {
        cons_show("Failed to reload plugin: %s, %s.", args[1], error_message->str);
    }
    g_string_free(error_message, TRUE);

    return TRUE;
}

gboolean
cmd_plugins_python_version(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_PYTHON
    const char* version = python_get_version_string();
    cons_show("Python version:");
    cons_show("%s", version);
#else
    cons_show("This build does not support python plugins.");
#endif
    return TRUE;
}

gboolean
cmd_plugins(ProfWin* window, const char* const command, gchar** args)
{
    GDir* global_pyp_dir = NULL;
    GDir* global_cp_dir = NULL;

    if (access(GLOBAL_PYTHON_PLUGINS_PATH, R_OK) == 0) {
        GError* error = NULL;
        global_pyp_dir = g_dir_open(GLOBAL_PYTHON_PLUGINS_PATH, 0, &error);
        if (error) {
            log_warning("Error when trying to open global plugins path: %s", GLOBAL_PYTHON_PLUGINS_PATH);
            g_error_free(error);
            return TRUE;
        }
    }
    if (access(GLOBAL_C_PLUGINS_PATH, R_OK) == 0) {
        GError* error = NULL;
        global_cp_dir = g_dir_open(GLOBAL_C_PLUGINS_PATH, 0, &error);
        if (error) {
            log_warning("Error when trying to open global plugins path: %s", GLOBAL_C_PLUGINS_PATH);
            g_error_free(error);
            return TRUE;
        }
    }
    if (global_pyp_dir) {
        const gchar* filename;
        cons_show("The following Python plugins are available globally and can be installed:");
        while ((filename = g_dir_read_name(global_pyp_dir))) {
            if (g_str_has_suffix(filename, ".py"))
                cons_show("  %s", filename);
        }
    }
    if (global_cp_dir) {
        const gchar* filename;
        cons_show("The following C plugins are available globally and can be installed:");
        while ((filename = g_dir_read_name(global_cp_dir))) {
            if (g_str_has_suffix(filename, ".so"))
                cons_show("  %s", filename);
        }
    }

    GList* plugins = plugins_loaded_list();
    GSList* unloaded_plugins = plugins_unloaded_list();

    if (plugins == NULL && unloaded_plugins == NULL) {
        cons_show("No plugins installed.");
        return TRUE;
    }

    if (unloaded_plugins) {
        GSList* curr = unloaded_plugins;
        cons_show("The following plugins already installed and can be loaded:");
        while (curr) {
            cons_show("  %s", curr->data);
            curr = g_slist_next(curr);
        }
        g_slist_free_full(unloaded_plugins, g_free);
    }

    if (plugins) {
        GList* curr = plugins;
        cons_show("Loaded plugins:");
        while (curr) {
            cons_show("  %s", curr->data);
            curr = g_list_next(curr);
        }
        g_list_free(plugins);
    }

    return TRUE;
}

gboolean
cmd_pgp(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBGPGME
    if (args[0] == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (strcmp(args[0], "char") == 0) {
        if (args[1] == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else if (g_utf8_strlen(args[1], 4) == 1) {
            if (prefs_set_pgp_char(args[1])) {
                cons_show("PGP char set to %s.", args[1]);
            } else {
                cons_show_error("Could not set PGP char: %s.", args[1]);
            }
            return TRUE;
        }
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (g_strcmp0(args[0], "log") == 0) {
        char* choice = args[1];
        if (g_strcmp0(choice, "on") == 0) {
            prefs_set_string(PREF_PGP_LOG, "on");
            cons_show("PGP messages will be logged as plaintext.");
            if (!prefs_get_boolean(PREF_CHLOG)) {
                cons_show("Chat logging is currently disabled, use '/logging chat on' to enable.");
            }
        } else if (g_strcmp0(choice, "off") == 0) {
            prefs_set_string(PREF_PGP_LOG, "off");
            cons_show("PGP message logging disabled.");
        } else if (g_strcmp0(choice, "redact") == 0) {
            prefs_set_string(PREF_PGP_LOG, "redact");
            cons_show("PGP messages will be logged as '[redacted]'.");
            if (!prefs_get_boolean(PREF_CHLOG)) {
                cons_show("Chat logging is currently disabled, use '/logging chat on' to enable.");
            }
        } else {
            cons_bad_cmd_usage(command);
        }
        return TRUE;
    }

    if (g_strcmp0(args[0], "autoimport") == 0) {
        _cmd_set_boolean_preference(args[1], "PGP keys autoimport from messages", PREF_PGP_PUBKEY_AUTOIMPORT);
        return TRUE;
    }

    if (g_strcmp0(args[0], "keys") == 0) {
        GHashTable* keys = p_gpg_list_keys();
        if (!keys || g_hash_table_size(keys) == 0) {
            cons_show("No keys found");
            return TRUE;
        }

        cons_show("PGP keys:");
        GList* keylist = g_hash_table_get_keys(keys);
        GList* curr = keylist;
        while (curr) {
            ProfPGPKey* key = g_hash_table_lookup(keys, curr->data);
            cons_show("  %s", key->name);
            cons_show("    ID          : %s", key->id);
            auto_char char* format_fp = p_gpg_format_fp_str(key->fp);
            cons_show("    Fingerprint : %s", format_fp);
            if (key->secret) {
                cons_show("    Type        : PUBLIC, PRIVATE");
            } else {
                cons_show("    Type        : PUBLIC");
            }
            curr = g_list_next(curr);
        }
        g_list_free(keylist);
        p_gpg_free_keys(keys);
        return TRUE;
    }

    if (g_strcmp0(args[0], "setkey") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }

        char* jid = args[1];
        if (!args[1]) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        char* keyid = args[2];
        if (!args[2]) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        gboolean res = p_gpg_addkey(jid, keyid);
        if (!res) {
            cons_show("Key ID not found.");
        } else {
            cons_show("Key %s set for %s.", keyid, jid);
        }

        return TRUE;
    }

    if (g_strcmp0(args[0], "contacts") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }
        GHashTable* pubkeys = p_gpg_pubkeys();
        GList* jids = g_hash_table_get_keys(pubkeys);
        if (!jids) {
            cons_show("No contacts found with PGP public keys assigned.");
            return TRUE;
        }

        cons_show("Assigned PGP public keys:");
        GList* curr = jids;
        while (curr) {
            char* jid = curr->data;
            ProfPGPPubKeyId* pubkeyid = g_hash_table_lookup(pubkeys, jid);
            if (pubkeyid->received) {
                cons_show("  %s: %s (received)", jid, pubkeyid->id);
            } else {
                cons_show("  %s: %s (stored)", jid, pubkeyid->id);
            }
            curr = g_list_next(curr);
        }
        g_list_free(jids);
        return TRUE;
    }

    if (g_strcmp0(args[0], "libver") == 0) {
        const char* libver = p_gpg_libver();
        if (!libver) {
            cons_show("Could not get libgpgme version");
            return TRUE;
        }

        cons_show("Using libgpgme version %s", libver);

        return TRUE;
    }

    if (g_strcmp0(args[0], "start") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You must be connected to start PGP encryption.");
            return TRUE;
        }

        if (window->type != WIN_CHAT && args[1] == NULL) {
            cons_show("You must set recipient in an argument or be in a regular chat window to start PGP encryption.");
            return TRUE;
        }

        ProfChatWin* chatwin = NULL;

        if (args[1]) {
            char* contact = args[1];
            char* barejid = roster_barejid_from_name(contact);
            if (barejid == NULL) {
                barejid = contact;
            }

            chatwin = wins_get_chat(barejid);
            if (!chatwin) {
                chatwin = chatwin_new(barejid);
            }
            ui_focus_win((ProfWin*)chatwin);
        } else {
            chatwin = (ProfChatWin*)window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        }

        if (chatwin->is_otr) {
            win_println(window, THEME_DEFAULT, "!", "You must end the OTR session to start PGP encryption.");
            return TRUE;
        }

        if (chatwin->pgp_send) {
            win_println(window, THEME_DEFAULT, "!", "You have already started PGP encryption.");
            return TRUE;
        }

        if (chatwin->is_omemo) {
            win_println(window, THEME_DEFAULT, "!", "You must disable OMEMO before starting an PGP encrypted session.");
            return TRUE;
        }

        ProfAccount* account = accounts_get_account(session_get_account_name());
        if (account->pgp_keyid == NULL) {
            win_println(window, THEME_DEFAULT, "!", "Couldn't start PGP session. Please, set your PGP key using /account set %s pgpkeyid <pgpkeyid>. To list pgp keys, use /pgp keys.", account->name);
            account_free(account);
            return TRUE;
        }
        auto_char char* err_str = NULL;
        if (!p_gpg_valid_key(account->pgp_keyid, &err_str)) {
            win_println(window, THEME_DEFAULT, "!", "Invalid PGP key ID %s: %s, cannot start PGP encryption.", account->pgp_keyid, err_str);
            account_free(account);
            return TRUE;
        }
        account_free(account);

        if (!p_gpg_available(chatwin->barejid)) {
            win_println(window, THEME_DEFAULT, "!", "No PGP key found for %s.", chatwin->barejid);
            return TRUE;
        }

        chatwin->pgp_send = TRUE;
        accounts_add_pgp_state(session_get_account_name(), chatwin->barejid, TRUE);
        win_println(window, THEME_DEFAULT, "!", "PGP encryption enabled.");
        return TRUE;
    }

    if (g_strcmp0(args[0], "end") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }

        if (window->type != WIN_CHAT) {
            cons_show("You must be in a regular chat window to end PGP encryption.");
            return TRUE;
        }

        ProfChatWin* chatwin = (ProfChatWin*)window;
        if (chatwin->pgp_send == FALSE) {
            win_println(window, THEME_DEFAULT, "!", "PGP encryption is not currently enabled.");
            return TRUE;
        }

        chatwin->pgp_send = FALSE;
        accounts_add_pgp_state(session_get_account_name(), chatwin->barejid, FALSE);
        win_println(window, THEME_DEFAULT, "!", "PGP encryption disabled.");
        return TRUE;
    }

    if (g_strcmp0(args[0], "sendfile") == 0) {
        _cmd_set_boolean_preference(args[1], "Sending unencrypted files using /sendfile while otherwise using PGP", PREF_PGP_SENDFILE);
        return TRUE;
    }

    if (g_strcmp0(args[0], "sendpub") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You must be connected to share your PGP public key.");
            return TRUE;
        }

        if (window->type != WIN_CHAT && args[1] == NULL) {
            cons_show("You must set recipient in an argument or use this command in a regular chat window to share your PGP key.");
            return TRUE;
        }

        ProfAccount* account = accounts_get_account(session_get_account_name());

        if (account->pgp_keyid == NULL) {
            cons_show_error("Please, set the PGP key first using /account set %s pgpkeyid <pgpkeyid>. To list pgp keys, use /pgp keys.", account->name);
            account_free(account);
            return TRUE;
        }
        auto_char char* pubkey = p_gpg_get_pubkey(account->pgp_keyid);
        if (pubkey == NULL) {
            cons_show_error("Couldn't get your PGP public key. Please, check error logs.");
            account_free(account);
            return TRUE;
        }

        ProfChatWin* chatwin = NULL;

        if (args[1]) {
            char* contact = args[1];
            char* barejid = roster_barejid_from_name(contact);
            if (barejid == NULL) {
                barejid = contact;
            }

            chatwin = wins_get_chat(barejid);
            if (!chatwin) {
                chatwin = chatwin_new(barejid);
            }
            ui_focus_win((ProfWin*)chatwin);
        } else {
            chatwin = (ProfChatWin*)window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        }

        cl_ev_send_msg(chatwin, pubkey, NULL);
        win_update_entry_message((ProfWin*)chatwin, chatwin->last_msg_id, "[you shared your PGP key]");
        cons_show("PGP key has been shared with %s.", chatwin->barejid);
        account_free(account);
        return TRUE;
    }

    cons_bad_cmd_usage(command);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with PGP support enabled");
    return TRUE;
#endif
}

#ifdef HAVE_LIBGPGME

gboolean
cmd_ox(ProfWin* window, const char* const command, gchar** args)
{
    if (args[0] == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (strcmp(args[0], "char") == 0) {
        if (args[1] == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else if (g_utf8_strlen(args[1], 4) == 1) {
            if (prefs_set_ox_char(args[1])) {
                cons_show("OX char set to %s.", args[1]);
            } else {
                cons_show_error("Could not set OX char: %s.", args[1]);
            }
            return TRUE;
        }
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    // The '/ox keys' command - same like in pgp
    // Should we move this to a common command
    // e.g. '/openpgp keys'?.
    else if (g_strcmp0(args[0], "keys") == 0) {
        GHashTable* keys = p_gpg_list_keys();
        if (!keys || g_hash_table_size(keys) == 0) {
            cons_show("No keys found");
            return TRUE;
        }

        cons_show("OpenPGP keys:");
        GList* keylist = g_hash_table_get_keys(keys);
        GList* curr = keylist;
        while (curr) {
            ProfPGPKey* key = g_hash_table_lookup(keys, curr->data);
            cons_show("  %s", key->name);
            cons_show("    ID          : %s", key->id);
            auto_char char* format_fp = p_gpg_format_fp_str(key->fp);
            cons_show("    Fingerprint : %s", format_fp);
            if (key->secret) {
                cons_show("    Type        : PUBLIC, PRIVATE");
            } else {
                cons_show("    Type        : PUBLIC");
            }
            curr = g_list_next(curr);
        }
        g_list_free(keylist);
        p_gpg_free_keys(keys);
        return TRUE;
    }

    else if (g_strcmp0(args[0], "contacts") == 0) {
        GHashTable* keys = ox_gpg_public_keys();
        cons_show("OpenPGP keys:");
        GList* keylist = g_hash_table_get_keys(keys);
        GList* curr = keylist;

        GSList* roster_list = NULL;
        jabber_conn_status_t conn_status = connection_get_status();
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
        } else {
            roster_list = roster_get_contacts(ROSTER_ORD_NAME);
        }

        while (curr) {
            ProfPGPKey* key = g_hash_table_lookup(keys, curr->data);
            PContact contact = NULL;
            if (roster_list) {
                GSList* curr_c = roster_list;
                while (!contact && curr_c) {
                    contact = curr_c->data;
                    auto_gchar gchar* xmppuri = g_strdup_printf("xmpp:%s", p_contact_barejid(contact));
                    if (g_strcmp0(key->name, xmppuri)) {
                        contact = NULL;
                    }
                    curr_c = g_slist_next(curr_c);
                }
            }

            if (contact) {
                cons_show("%s - %s", key->fp, key->name);
            } else {
                cons_show("%s - %s (not in roster)", key->fp, key->name);
            }
            curr = g_list_next(curr);
        }

    } else if (g_strcmp0(args[0], "start") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You must be connected to start OX encryption.");
            return TRUE;
        }

        if (window->type != WIN_CHAT && args[1] == NULL) {
            cons_show("You must be in a regular chat window to start OX encryption.");
            return TRUE;
        }

        ProfChatWin* chatwin = NULL;

        if (args[1]) {
            char* contact = args[1];
            char* barejid = roster_barejid_from_name(contact);
            if (barejid == NULL) {
                barejid = contact;
            }

            chatwin = wins_get_chat(barejid);
            if (!chatwin) {
                chatwin = chatwin_new(barejid);
            }
            ui_focus_win((ProfWin*)chatwin);
        } else {
            chatwin = (ProfChatWin*)window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        }

        if (chatwin->is_otr) {
            win_println(window, THEME_DEFAULT, "!", "You must end the OTR session to start OX encryption.");
            return TRUE;
        }

        if (chatwin->pgp_send) {
            win_println(window, THEME_DEFAULT, "!", "You must end the PGP session to start OX encryption.");
            return TRUE;
        }

        if (chatwin->is_ox) {
            win_println(window, THEME_DEFAULT, "!", "You have already started an OX encrypted session.");
            return TRUE;
        }

        ProfAccount* account = accounts_get_account(session_get_account_name());

        if (!ox_is_private_key_available(account->jid)) {
            win_println(window, THEME_DEFAULT, "!", "No private OpenPGP found, cannot start OX encryption.");
            account_free(account);
            return TRUE;
        }
        account_free(account);

        if (!ox_is_public_key_available(chatwin->barejid)) {
            win_println(window, THEME_DEFAULT, "!", "No OX-OpenPGP key found for %s.", chatwin->barejid);
            return TRUE;
        }

        chatwin->is_ox = TRUE;
        accounts_add_ox_state(session_get_account_name(), chatwin->barejid, TRUE);
        win_println(window, THEME_DEFAULT, "!", "OX encryption enabled.");
        return TRUE;
    } else if (g_strcmp0(args[0], "end") == 0) {
        if (window->type != WIN_CHAT && args[1] == NULL) {
            cons_show("You must be in a regular chat window to stop OX encryption.");
            return TRUE;
        }

        ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);

        if (!chatwin->is_ox) {
            win_println(window, THEME_DEFAULT, "!", "No OX session has been started.");
        } else {
            chatwin->is_ox = FALSE;
            accounts_add_ox_state(session_get_account_name(), chatwin->barejid, FALSE);
            win_println(window, THEME_DEFAULT, "!", "OX encryption disabled.");
        }
        return TRUE;
    } else if (g_strcmp0(args[0], "announce") == 0) {
        if (args[1]) {
            auto_gchar gchar* filename = get_expanded_path(args[1]);

            if (access(filename, R_OK) != 0) {
                cons_show_error("File not found: %s", filename);
                return TRUE;
            }

            if (!is_regular_file(filename)) {
                cons_show_error("Not a file: %s", filename);
                return TRUE;
            }

            ox_announce_public_key(filename);
        } else {
            cons_show("Filename is required");
        }
    } else if (g_strcmp0(args[0], "discover") == 0) {
        if (args[1]) {
            ox_discover_public_key(args[1]);
        } else {
            cons_show("To discover the OpenPGP keys of an user, the JID is required");
        }
    } else if (g_strcmp0(args[0], "request") == 0) {
        if (args[1] && args[2]) {
            ox_request_public_key(args[1], args[2]);
        } else {
            cons_show("JID and OpenPGP Key ID are required");
        }
    } else {
        cons_bad_cmd_usage(command);
    }
    return TRUE;
}

gboolean
cmd_ox_log(ProfWin* window, const char* const command, gchar** args)
{
    char* choice = args[1];
    if (g_strcmp0(choice, "on") == 0) {
        prefs_set_string(PREF_OX_LOG, "on");
        cons_show("OX messages will be logged as plaintext.");
        if (!prefs_get_boolean(PREF_CHLOG)) {
            cons_show("Chat logging is currently disabled, use '/logging chat on' to enable.");
        }
    } else if (g_strcmp0(choice, "off") == 0) {
        prefs_set_string(PREF_OX_LOG, "off");
        cons_show("OX message logging disabled.");
    } else if (g_strcmp0(choice, "redact") == 0) {
        prefs_set_string(PREF_OX_LOG, "redact");
        cons_show("OX messages will be logged as '[redacted]'.");
        if (!prefs_get_boolean(PREF_CHLOG)) {
            cons_show("Chat logging is currently disabled, use '/logging chat on' to enable.");
        }
    } else {
        cons_bad_cmd_usage(command);
    }
    return TRUE;
}
#endif // HAVE_LIBGPGME

gboolean
cmd_otr_char(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    if (args[1] == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    } else if (g_utf8_strlen(args[1], 4) == 1) {
        if (prefs_set_otr_char(args[1])) {
            cons_show("OTR char set to %s.", args[1]);
        } else {
            cons_show_error("Could not set OTR char: %s.", args[1]);
        }
        return TRUE;
    }
    cons_bad_cmd_usage(command);
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
#endif
    return TRUE;
}

gboolean
cmd_otr_log(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    char* choice = args[1];
    if (g_strcmp0(choice, "on") == 0) {
        prefs_set_string(PREF_OTR_LOG, "on");
        cons_show("OTR messages will be logged as plaintext.");
        if (!prefs_get_boolean(PREF_CHLOG)) {
            cons_show("Chat logging is currently disabled, use '/logging chat on' to enable.");
        }
    } else if (g_strcmp0(choice, "off") == 0) {
        prefs_set_string(PREF_OTR_LOG, "off");
        cons_show("OTR message logging disabled.");
    } else if (g_strcmp0(choice, "redact") == 0) {
        prefs_set_string(PREF_OTR_LOG, "redact");
        cons_show("OTR messages will be logged as '[redacted]'.");
        if (!prefs_get_boolean(PREF_CHLOG)) {
            cons_show("Chat logging is currently disabled, use '/logging chat on' to enable.");
        }
    } else {
        cons_bad_cmd_usage(command);
    }
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_libver(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    char* version = otr_libotr_version();
    cons_show("Using libotr version %s", version);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_policy(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    if (args[1] == NULL) {
        auto_gchar gchar* policy = prefs_get_string(PREF_OTR_POLICY);
        cons_show("OTR policy is now set to: %s", policy);
        return TRUE;
    }

    char* choice = args[1];
    if (!_string_matches_one_of("OTR policy", choice, FALSE, "manual", "opportunistic", "always", NULL)) {
        return TRUE;
    }

    char* contact = args[2];
    if (contact == NULL) {
        prefs_set_string(PREF_OTR_POLICY, choice);
        cons_show("OTR policy is now set to: %s", choice);
        return TRUE;
    }

    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected to set the OTR policy for a contact.");
        return TRUE;
    }

    char* contact_jid = roster_barejid_from_name(contact);
    if (contact_jid == NULL) {
        contact_jid = contact;
    }
    accounts_add_otr_policy(session_get_account_name(), contact_jid, choice);
    cons_show("OTR policy for %s set to: %s", contact_jid, choice);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_gen(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    ProfAccount* account = accounts_get_account(session_get_account_name());
    otr_keygen(account);
    account_free(account);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_myfp(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (!otr_key_loaded()) {
        win_println(window, THEME_DEFAULT, "!", "You have not generated or loaded a private key, use '/otr gen'");
        return TRUE;
    }

    auto_char char* fingerprint = otr_get_my_fingerprint();
    win_println(window, THEME_DEFAULT, "!", "Your OTR fingerprint: %s", fingerprint);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_theirfp(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (window->type != WIN_CHAT) {
        win_println(window, THEME_DEFAULT, "-", "You must be in a regular chat window to view a recipient's fingerprint.");
        return TRUE;
    }

    ProfChatWin* chatwin = (ProfChatWin*)window;
    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
    if (chatwin->is_otr == FALSE) {
        win_println(window, THEME_DEFAULT, "!", "You are not currently in an OTR session.");
        return TRUE;
    }

    auto_char char* fingerprint = otr_get_their_fingerprint(chatwin->barejid);
    win_println(window, THEME_DEFAULT, "!", "%s's OTR fingerprint: %s", chatwin->barejid, fingerprint);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_start(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    // recipient supplied
    if (args[1]) {
        char* contact = args[1];
        char* barejid = roster_barejid_from_name(contact);
        if (barejid == NULL) {
            barejid = contact;
        }

        ProfChatWin* chatwin = wins_get_chat(barejid);
        if (!chatwin) {
            chatwin = chatwin_new(barejid);
        }
        ui_focus_win((ProfWin*)chatwin);

        if (chatwin->pgp_send) {
            win_println(window, THEME_DEFAULT, "!", "You must disable PGP encryption before starting an OTR session.");
            return TRUE;
        }

        if (chatwin->is_omemo) {
            win_println(window, THEME_DEFAULT, "!", "You must disable OMEMO before starting an OTR session.");
            return TRUE;
        }

        if (chatwin->is_otr) {
            win_println(window, THEME_DEFAULT, "!", "You are already in an OTR session.");
            return TRUE;
        }

        if (!otr_key_loaded()) {
            win_println(window, THEME_DEFAULT, "!", "You have not generated or loaded a private key, use '/otr gen'");
            return TRUE;
        }

        if (!otr_is_secure(barejid)) {
            char* otr_query_message = otr_start_query();
            free(message_send_chat_otr(barejid, otr_query_message, FALSE, NULL));
            return TRUE;
        }

        chatwin_otr_secured(chatwin, otr_is_trusted(barejid));
        return TRUE;

        // no recipient, use current chat
    } else {
        if (window->type != WIN_CHAT) {
            win_println(window, THEME_DEFAULT, "-", "You must be in a regular chat window to start an OTR session.");
            return TRUE;
        }

        ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        if (chatwin->pgp_send) {
            win_println(window, THEME_DEFAULT, "!", "You must disable PGP encryption before starting an OTR session.");
            return TRUE;
        }

        if (chatwin->is_otr) {
            win_println(window, THEME_DEFAULT, "!", "You are already in an OTR session.");
            return TRUE;
        }

        if (!otr_key_loaded()) {
            win_println(window, THEME_DEFAULT, "!", "You have not generated or loaded a private key, use '/otr gen'");
            return TRUE;
        }

        char* otr_query_message = otr_start_query();
        free(message_send_chat_otr(chatwin->barejid, otr_query_message, FALSE, NULL));

        return TRUE;
    }
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_end(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (window->type != WIN_CHAT) {
        win_println(window, THEME_DEFAULT, "-", "You must be in a regular chat window to use OTR.");
        return TRUE;
    }

    ProfChatWin* chatwin = (ProfChatWin*)window;
    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
    if (chatwin->is_otr == FALSE) {
        win_println(window, THEME_DEFAULT, "!", "You are not currently in an OTR session.");
        return TRUE;
    }

    chatwin_otr_unsecured(chatwin);
    otr_end_session(chatwin->barejid);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_trust(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (window->type != WIN_CHAT) {
        win_println(window, THEME_DEFAULT, "-", "You must be in an OTR session to trust a recipient.");
        return TRUE;
    }

    ProfChatWin* chatwin = (ProfChatWin*)window;
    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
    if (chatwin->is_otr == FALSE) {
        win_println(window, THEME_DEFAULT, "!", "You are not currently in an OTR session.");
        return TRUE;
    }

    chatwin_otr_trust(chatwin);
    otr_trust(chatwin->barejid);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_untrust(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (window->type != WIN_CHAT) {
        win_println(window, THEME_DEFAULT, "-", "You must be in an OTR session to untrust a recipient.");
        return TRUE;
    }

    ProfChatWin* chatwin = (ProfChatWin*)window;
    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
    if (chatwin->is_otr == FALSE) {
        win_println(window, THEME_DEFAULT, "!", "You are not currently in an OTR session.");
        return TRUE;
    }

    chatwin_otr_untrust(chatwin);
    otr_untrust(chatwin->barejid);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_secret(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (window->type != WIN_CHAT) {
        win_println(window, THEME_DEFAULT, "-", "You must be in an OTR session to trust a recipient.");
        return TRUE;
    }

    ProfChatWin* chatwin = (ProfChatWin*)window;
    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
    if (chatwin->is_otr == FALSE) {
        win_println(window, THEME_DEFAULT, "!", "You are not currently in an OTR session.");
        return TRUE;
    }

    char* secret = args[1];
    if (secret == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    otr_smp_secret(chatwin->barejid, secret);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_question(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    char* question = args[1];
    char* answer = args[2];
    if (question == NULL || answer == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (window->type != WIN_CHAT) {
        win_println(window, THEME_DEFAULT, "-", "You must be in an OTR session to trust a recipient.");
        return TRUE;
    }

    ProfChatWin* chatwin = (ProfChatWin*)window;
    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
    if (chatwin->is_otr == FALSE) {
        win_println(window, THEME_DEFAULT, "!", "You are not currently in an OTR session.");
        return TRUE;
    }

    otr_smp_question(chatwin->barejid, question, answer);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_answer(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (window->type != WIN_CHAT) {
        win_println(window, THEME_DEFAULT, "-", "You must be in an OTR session to trust a recipient.");
        return TRUE;
    }

    ProfChatWin* chatwin = (ProfChatWin*)window;
    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
    if (chatwin->is_otr == FALSE) {
        win_println(window, THEME_DEFAULT, "!", "You are not currently in an OTR session.");
        return TRUE;
    }

    char* answer = args[1];
    if (answer == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    otr_smp_answer(chatwin->barejid, answer);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_sendfile(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_LIBOTR
    _cmd_set_boolean_preference(args[1], "Sending unencrypted files in an OTR session via /sendfile", PREF_OTR_SENDFILE);

    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_command_list(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (connection_supports(XMPP_FEATURE_COMMANDS) == FALSE) {
        cons_show("Server does not support ad hoc commands (%s).", XMPP_FEATURE_COMMANDS);
        return TRUE;
    }

    const char* jid = args[1];
    if (jid == NULL) {
        switch (window->type) {
        case WIN_MUC:
        {
            ProfMucWin* mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            jid = mucwin->roomjid;
            break;
        }
        case WIN_CHAT:
        {
            ProfChatWin* chatwin = (ProfChatWin*)window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
            jid = chatwin->barejid;
            break;
        }
        case WIN_PRIVATE:
        {
            ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
            assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
            jid = privatewin->fulljid;
            break;
        }
        case WIN_CONSOLE:
        {
            jid = connection_get_domain();
            break;
        }
        default:
            cons_show("Cannot send ad hoc commands.");
            return TRUE;
        }
    }

    iq_command_list(jid);

    cons_show("List available ad hoc commands");
    return TRUE;
}

gboolean
cmd_command_exec(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (connection_supports(XMPP_FEATURE_COMMANDS) == FALSE) {
        cons_show("Server does not support ad hoc commands (%s).", XMPP_FEATURE_COMMANDS);
        return TRUE;
    }

    if (args[1] == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    const char* jid = args[2];
    if (jid == NULL) {
        switch (window->type) {
        case WIN_MUC:
        {
            ProfMucWin* mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            jid = mucwin->roomjid;
            break;
        }
        case WIN_CHAT:
        {
            ProfChatWin* chatwin = (ProfChatWin*)window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
            jid = chatwin->barejid;
            break;
        }
        case WIN_PRIVATE:
        {
            ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
            assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
            jid = privatewin->fulljid;
            break;
        }
        case WIN_CONSOLE:
        {
            jid = connection_get_domain();
            break;
        }
        default:
            cons_show("Cannot send ad hoc commands.");
            return TRUE;
        }
    }

    iq_command_exec(jid, args[1]);

    cons_show("Execute %s…", args[1]);
    return TRUE;
}

static gboolean
_cmd_execute(ProfWin* window, const char* const command, const char* const inp)
{
    if (g_str_has_prefix(command, "/field") && window->type == WIN_CONFIG) {
        gboolean result = FALSE;
        auto_gcharv gchar** args = parse_args_with_freetext(inp, 1, 2, &result);
        if (!result) {
            win_println(window, THEME_DEFAULT, "!", "Invalid command, see /form help");
            result = TRUE;
        } else {
            auto_gcharv gchar** tokens = g_strsplit(inp, " ", 2);
            char* field = tokens[0] + 1;
            result = cmd_form_field(window, field, args);
        }

        return result;
    }

    Command* cmd = cmd_get(command);
    gboolean result = FALSE;

    if (cmd) {
        auto_gcharv gchar** args = cmd->parser(inp, cmd->min_args, cmd->max_args, &result);
        if (result == FALSE) {
            ui_invalid_command_usage(cmd->cmd, cmd->setting_func);
            return TRUE;
        }
        if (args[0] && cmd->sub_funcs[0].cmd) {
            int i = 0;
            while (cmd->sub_funcs[i].cmd) {
                if (g_strcmp0(args[0], (char*)cmd->sub_funcs[i].cmd) == 0) {
                    return cmd->sub_funcs[i].func(window, command, args);
                }
                i++;
            }
        }
        if (!cmd->func) {
            ui_invalid_command_usage(cmd->cmd, cmd->setting_func);
            return TRUE;
        }
        result = cmd->func(window, command, args);

        return result;
    } else if (plugins_run_command(inp)) {
        return TRUE;
    } else {
        gboolean ran_alias = FALSE;
        gboolean alias_result = _cmd_execute_alias(window, inp, &ran_alias);
        if (!ran_alias) {
            return _cmd_execute_default(window, inp);
        } else {
            return alias_result;
        }
    }
}

static gboolean
_cmd_execute_default(ProfWin* window, const char* inp)
{
    // handle escaped commands - treat as normal message
    if (g_str_has_prefix(inp, "//")) {
        inp++;

        // handle unknown commands
    } else if ((inp[0] == '/') && (!g_str_has_prefix(inp, "/me "))) {
        cons_show("Unknown command: %s", inp);
        cons_alert(NULL);
        return TRUE;
    }

    // handle non commands in non chat or plugin windows
    if (window->type != WIN_CHAT && window->type != WIN_MUC && window->type != WIN_PRIVATE && window->type != WIN_PLUGIN && window->type != WIN_XML) {
        cons_show("Unknown command: %s", inp);
        cons_alert(NULL);
        return TRUE;
    }

    // handle plugin window
    if (window->type == WIN_PLUGIN) {
        ProfPluginWin* pluginwin = (ProfPluginWin*)window;
        plugins_win_process_line(pluginwin->tag, inp);
        return TRUE;
    }

    jabber_conn_status_t status = connection_get_status();
    if (status != JABBER_CONNECTED) {
        win_println(window, THEME_DEFAULT, "-", "You are not currently connected.");
        return TRUE;
    }

    switch (window->type) {
    case WIN_CHAT:
    {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        cl_ev_send_msg(chatwin, inp, NULL);
        break;
    }
    case WIN_PRIVATE:
    {
        ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
        assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
        cl_ev_send_priv_msg(privatewin, inp, NULL);
        break;
    }
    case WIN_MUC:
    {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        cl_ev_send_muc_msg(mucwin, inp, NULL);
        break;
    }
    case WIN_XML:
    {
        connection_send_stanza(inp);
        break;
    }
    default:
        break;
    }

    return TRUE;
}

static gboolean
_cmd_execute_alias(ProfWin* window, const char* const inp, gboolean* ran)
{
    if (inp[0] != '/') {
        *ran = FALSE;
        return TRUE;
    }

    auto_char char* alias = strdup(inp + 1);
    auto_gcharv char** alias_parts = g_strsplit(alias, " ", 2);
    auto_gchar gchar* value = prefs_get_alias(alias_parts[0]);

    if (!value) {
        *ran = FALSE;
        return TRUE;
    }

    char* params = alias_parts[1];
    auto_gchar gchar* full_cmd = params ? g_strdup_printf("%s %s", value, params) : g_strdup(value);

    *ran = TRUE;
    gboolean result = cmd_process_input(window, full_cmd);
    return result;
}

// helper function for status change commands
static void
_update_presence(const resource_presence_t resource_presence,
                 const char* const show, gchar** args)
{
    char* msg = NULL;
    int num_args = g_strv_length(args);

    // if no message, use status as message
    if (num_args == 2) {
        msg = args[1];
    } else {
        msg = args[2];
    }

    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
    } else {
        connection_set_presence_msg(msg);
        cl_ev_presence_send(resource_presence, 0);
        ui_update_presence(resource_presence, msg, show);
    }
}

/**
 * Sets a boolean preference based on the provided argument.
 *
 * @param arg        The argument value specifying the preference state ("on" or "off").
 * @param display    The display (UI) name of the preference being set.
 * @param preference The preference to be changed.
 *
 * @return TRUE if the preference was successfully set, FALSE otherwise.
 */
static gboolean
_cmd_set_boolean_preference(gchar* arg, const char* const display, preference_t preference)
{
    gboolean prev_state = prefs_get_boolean(preference);
    if (arg == NULL) {
        cons_show("%s is %s.", display, prev_state ? "enabled" : "disabled");
        return FALSE;
    }

    if (g_strcmp0(arg, "on") == 0) {
        cons_show("%s %senabled.", display, prev_state ? "is already " : "");
        prefs_set_boolean(preference, TRUE);
    } else if (g_strcmp0(arg, "off") == 0) {
        cons_show("%s %sdisabled.", display, !prev_state ? "is already " : "");
        prefs_set_boolean(preference, FALSE);
    } else {
        cons_show_error("Invalid argument value. Expected 'on' or 'off'.");
        return FALSE;
    }

    return TRUE;
}

gboolean
cmd_omemo_gen(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_OMEMO
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to initialize OMEMO.");
        return TRUE;
    }

    if (omemo_loaded()) {
        cons_show("OMEMO cryptographic materials have already been generated.");
        return TRUE;
    }

    cons_show("Generating OMEMO cryptographic materials, it may take a while…");
    ui_update();
    ProfAccount* account = accounts_get_account(session_get_account_name());
    omemo_generate_crypto_materials(account);
    cons_show("OMEMO cryptographic materials generated. Your Device ID is %d.", omemo_device_id());
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OMEMO support enabled");
    return TRUE;
#endif
}

gboolean
cmd_omemo_start(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_OMEMO
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OMEMO information.");
        return TRUE;
    }

    if (!omemo_loaded()) {
        win_println(window, THEME_DEFAULT, "!", "You have not generated or loaded a cryptographic materials, use '/omemo gen'");
        return TRUE;
    }

    ProfChatWin* chatwin = NULL;

    // recipient supplied
    if (args[1]) {
        char* contact = args[1];
        char* barejid = roster_barejid_from_name(contact);
        if (barejid == NULL) {
            barejid = contact;
        }

        chatwin = wins_get_chat(barejid);
        if (!chatwin) {
            chatwin = chatwin_new(barejid);
        }
        ui_focus_win((ProfWin*)chatwin);
    } else {
        if (window->type == WIN_CHAT) {
            chatwin = (ProfChatWin*)window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        }
    }

    if (chatwin) {
        if (chatwin->pgp_send) {
            win_println((ProfWin*)chatwin, THEME_DEFAULT, "!", "You must disable PGP encryption before starting an OMEMO session.");
            return TRUE;
        }

        if (chatwin->is_otr) {
            win_println((ProfWin*)chatwin, THEME_DEFAULT, "!", "You must disable OTR encryption before starting an OMEMO session.");
            return TRUE;
        }

        if (chatwin->is_omemo) {
            win_println((ProfWin*)chatwin, THEME_DEFAULT, "!", "You are already in an OMEMO session.");
            return TRUE;
        }

        accounts_add_omemo_state(session_get_account_name(), chatwin->barejid, TRUE);
        omemo_start_session(chatwin->barejid);
        chatwin->is_omemo = TRUE;
    } else if (window->type == WIN_MUC) {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

        if (muc_anonymity_type(mucwin->roomjid) == MUC_ANONYMITY_TYPE_NONANONYMOUS
            && muc_member_type(mucwin->roomjid) == MUC_MEMBER_TYPE_MEMBERS_ONLY) {
            accounts_add_omemo_state(session_get_account_name(), mucwin->roomjid, TRUE);
            omemo_start_muc_sessions(mucwin->roomjid);
            mucwin->is_omemo = TRUE;
        } else {
            win_println(window, THEME_DEFAULT, "!", "MUC must be non-anonymous (i.e. be configured to present real jid to anyone) and members-only in order to support OMEMO.");
        }
    } else {
        win_println(window, THEME_DEFAULT, "-", "You must be in a regular chat window to start an OMEMO session.");
    }

    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OMEMO support enabled");
    return TRUE;
#endif
}

gboolean
cmd_omemo_trust_mode(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_OMEMO

    if (!args[1]) {
        cons_show("Current trust mode is %s", prefs_get_string(PREF_OMEMO_TRUST_MODE));
        return TRUE;
    }

    if (g_strcmp0(args[1], "manual") == 0) {
        cons_show("Current trust mode is %s - setting to %s", prefs_get_string(PREF_OMEMO_TRUST_MODE), args[1]);
        cons_show("You need to trust all OMEMO fingerprints manually");
    } else if (g_strcmp0(args[1], "firstusage") == 0) {
        cons_show("Current trust mode is %s - setting to %s", prefs_get_string(PREF_OMEMO_TRUST_MODE), args[1]);
        cons_show("The first seen OMEMO fingerprints will be trusted automatically - new keys must be trusted manually");
    } else if (g_strcmp0(args[1], "blind") == 0) {
        cons_show("Current trust mode is %s - setting to %s", prefs_get_string(PREF_OMEMO_TRUST_MODE), args[1]);
        cons_show("ALL OMEMO fingerprints will be trusted automatically");
    } else {
        cons_bad_cmd_usage(command);
        return TRUE;
    }
    prefs_set_string(PREF_OMEMO_TRUST_MODE, args[1]);

#else
    cons_show("This version of Profanity has not been built with OMEMO support enabled");
#endif
    return TRUE;
}

gboolean
cmd_omemo_char(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_OMEMO
    if (args[1] == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    } else if (g_utf8_strlen(args[1], 4) == 1) {
        if (prefs_set_omemo_char(args[1])) {
            cons_show("OMEMO char set to %s.", args[1]);
        } else {
            cons_show_error("Could not set OMEMO char: %s.", args[1]);
        }
        return TRUE;
    }
    cons_bad_cmd_usage(command);
#else
    cons_show("This version of Profanity has not been built with OMEMO support enabled");
#endif
    return TRUE;
}

gboolean
cmd_omemo_log(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_OMEMO
    char* choice = args[1];
    if (g_strcmp0(choice, "on") == 0) {
        prefs_set_string(PREF_OMEMO_LOG, "on");
        cons_show("OMEMO messages will be logged as plaintext.");
        if (!prefs_get_boolean(PREF_CHLOG)) {
            cons_show("Chat logging is currently disabled, use '/logging chat on' to enable.");
        }
    } else if (g_strcmp0(choice, "off") == 0) {
        prefs_set_string(PREF_OMEMO_LOG, "off");
        cons_show("OMEMO message logging disabled.");
    } else if (g_strcmp0(choice, "redact") == 0) {
        prefs_set_string(PREF_OMEMO_LOG, "redact");
        cons_show("OMEMO messages will be logged as '[redacted]'.");
        if (!prefs_get_boolean(PREF_CHLOG)) {
            cons_show("Chat logging is currently disabled, use '/logging chat on' to enable.");
        }
    } else {
        cons_bad_cmd_usage(command);
    }
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OMEMO support enabled");
    return TRUE;
#endif
}

gboolean
cmd_omemo_end(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_OMEMO
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OMEMO information.");
        return TRUE;
    }

    if (window->type == WIN_CHAT) {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);

        if (!chatwin->is_omemo) {
            win_println(window, THEME_DEFAULT, "!", "You are not currently in an OMEMO session.");
            return TRUE;
        }

        chatwin->is_omemo = FALSE;
        accounts_add_omemo_state(session_get_account_name(), chatwin->barejid, FALSE);
    } else if (window->type == WIN_MUC) {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

        if (!mucwin->is_omemo) {
            win_println(window, THEME_DEFAULT, "!", "You are not currently in an OMEMO session.");
            return TRUE;
        }

        mucwin->is_omemo = FALSE;
        accounts_add_omemo_state(session_get_account_name(), mucwin->roomjid, FALSE);
    } else {
        win_println(window, THEME_DEFAULT, "-", "You must be in a regular chat window to start an OMEMO session.");
        return TRUE;
    }

    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OMEMO support enabled");
    return TRUE;
#endif
}

gboolean
cmd_omemo_fingerprint(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_OMEMO
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OMEMO information.");
        return TRUE;
    }

    if (!omemo_loaded()) {
        win_println(window, THEME_DEFAULT, "!", "You have not generated or loaded a cryptographic materials, use '/omemo gen'");
        return TRUE;
    }

    auto_jid Jid* jid = NULL;
    if (!args[1]) {
        if (window->type == WIN_CONSOLE) {
            auto_char char* fingerprint = omemo_own_fingerprint(TRUE);
            cons_show("Your OMEMO fingerprint: %s", fingerprint);
            jid = jid_create(connection_get_fulljid());
        } else if (window->type == WIN_CHAT) {
            ProfChatWin* chatwin = (ProfChatWin*)window;
            jid = jid_create(chatwin->barejid);
        } else {
            win_println(window, THEME_DEFAULT, "-", "You must be in a regular chat window to print fingerprint without providing the contact.");
            return TRUE;
        }
    } else {
        char* barejid = roster_barejid_from_name(args[1]);
        if (barejid) {
            jid = jid_create(barejid);
        } else {
            jid = jid_create(args[1]);
            if (!jid) {
                cons_show("%s is not a valid jid", args[1]);
                return TRUE;
            }
        }
    }

    GList* fingerprints = omemo_known_device_identities(jid->barejid);

    if (!fingerprints) {
        win_println(window, THEME_DEFAULT, "-", "There is no known fingerprints for %s", jid->barejid);
        return TRUE;
    }

    for (GList* fingerprint = fingerprints; fingerprint != NULL; fingerprint = fingerprint->next) {
        auto_char char* formatted_fingerprint = omemo_format_fingerprint(fingerprint->data);
        gboolean trusted = omemo_is_trusted_identity(jid->barejid, fingerprint->data);

        win_println(window, THEME_DEFAULT, "-", "%s's OMEMO fingerprint: %s%s", jid->barejid, formatted_fingerprint, trusted ? " (trusted)" : "");
    }

    g_list_free(fingerprints);

    win_println(window, THEME_DEFAULT, "-", "You can trust it with '/omemo trust [<contact>] <fingerprint>'");
    win_println(window, THEME_DEFAULT, "-", "You can untrust it with '/omemo untrust [<contact>] <fingerprint>'");

    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OMEMO support enabled");
    return TRUE;
#endif
}

gboolean
cmd_omemo_trust(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_OMEMO
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OMEMO information.");
        return TRUE;
    }

    if (!args[1]) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (!omemo_loaded()) {
        win_println(window, THEME_DEFAULT, "!", "You have not generated or loaded a cryptographic materials, use '/omemo gen'");
        return TRUE;
    }

    char* fingerprint;
    char* barejid;

    /* Contact not provided */
    if (!args[2]) {
        fingerprint = args[1];

        if (window->type != WIN_CHAT) {
            win_println(window, THEME_DEFAULT, "-", "You must be in a regular chat window to trust a device without providing the contact. To trust your own JID, use /omemo trust %s %s", connection_get_barejid(), fingerprint);
            return TRUE;
        }

        ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        barejid = chatwin->barejid;
    } else {
        fingerprint = args[2];
        char* contact = args[1];
        barejid = roster_barejid_from_name(contact);
        if (barejid == NULL) {
            barejid = contact;
        }
    }

    omemo_trust(barejid, fingerprint);

    auto_char char* unformatted_fingerprint = malloc(strlen(fingerprint));
    int i;
    int j;
    for (i = 0, j = 0; fingerprint[i] != '\0'; i++) {
        if (!g_ascii_isxdigit(fingerprint[i])) {
            continue;
        }
        unformatted_fingerprint[j++] = fingerprint[i];
    }

    unformatted_fingerprint[j] = '\0';
    gboolean trusted = omemo_is_trusted_identity(barejid, unformatted_fingerprint);

    win_println(window, THEME_DEFAULT, "-", "%s's OMEMO fingerprint: %s%s", barejid, fingerprint, trusted ? " (trusted)" : "");

    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OMEMO support enabled");
    return TRUE;
#endif
}

gboolean
cmd_omemo_untrust(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_OMEMO
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OMEMO information.");
        return TRUE;
    }

    if (!args[1]) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (!omemo_loaded()) {
        win_println(window, THEME_DEFAULT, "!", "You have not generated or loaded a cryptographic materials, use '/omemo gen'");
        return TRUE;
    }

    char* fingerprint;
    char* barejid;

    /* Contact not provided */
    if (!args[2]) {
        fingerprint = args[1];

        if (window->type != WIN_CHAT) {
            win_println(window, THEME_DEFAULT, "-", "You must be in a regular chat window to trust a device without providing the contact.");
            return TRUE;
        }

        ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        barejid = chatwin->barejid;
    } else {
        fingerprint = args[2];
        char* contact = args[1];
        barejid = roster_barejid_from_name(contact);
        if (barejid == NULL) {
            barejid = contact;
        }
    }

    omemo_untrust(barejid, fingerprint);

    auto_char char* unformatted_fingerprint = malloc(strlen(fingerprint));
    int i, j;
    for (i = 0, j = 0; fingerprint[i] != '\0'; i++) {
        if (!g_ascii_isxdigit(fingerprint[i])) {
            continue;
        }
        unformatted_fingerprint[j++] = fingerprint[i];
    }

    unformatted_fingerprint[j] = '\0';
    gboolean trusted = omemo_is_trusted_identity(barejid, unformatted_fingerprint);

    win_println(window, THEME_DEFAULT, "-", "%s's OMEMO fingerprint: %s%s", barejid, fingerprint, trusted ? " (trusted)" : "");

    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OMEMO support enabled");
    return TRUE;
#endif
}

gboolean
cmd_omemo_clear_device_list(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_OMEMO
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to initialize OMEMO.");
        return TRUE;
    }

    omemo_devicelist_publish(NULL);
    cons_show("Cleared OMEMO device list");
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OMEMO support enabled");
    return TRUE;
#endif
}

gboolean
cmd_omemo_policy(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_OMEMO
    if (args[1] == NULL) {
        auto_gchar gchar* policy = prefs_get_string(PREF_OMEMO_POLICY);
        cons_show("OMEMO policy is now set to: %s", policy);
        return TRUE;
    }

    char* choice = args[1];
    if (!_string_matches_one_of("OMEMO policy", choice, FALSE, "manual", "automatic", "always", NULL)) {
        return TRUE;
    }

    prefs_set_string(PREF_OMEMO_POLICY, choice);
    cons_show("OMEMO policy is now set to: %s", choice);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OMEMO support enabled");
    return TRUE;
#endif
}

gboolean
cmd_omemo_qrcode(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_OMEMO
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OMEMO information.");
        return TRUE;
    }

    if (!omemo_loaded()) {
        win_println(window, THEME_DEFAULT, "!", "You have not generated or loaded a cryptographic materials, use '/omemo gen'");
        return TRUE;
    }

    auto_char char* qrstr = omemo_qrcode_str();
    cons_show_qrcode(qrstr);

    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OMEMO support enabled");
    return TRUE;
#endif
}

gboolean
cmd_save(ProfWin* window, const char* const command, gchar** args)
{
    log_info("Saving preferences to configuration file");
    cons_show("Saving preferences.");
    prefs_save();
    return TRUE;
}

gboolean
cmd_reload(ProfWin* window, const char* const command, gchar** args)
{
    log_info("Reloading preferences");
    cons_show("Reloading preferences.");
    prefs_reload();
    return TRUE;
}

gboolean
cmd_paste(ProfWin* window, const char* const command, gchar** args)
{
#ifdef HAVE_GTK
    auto_char char* clipboard_buffer = clipboard_get();

    if (clipboard_buffer) {
        switch (window->type) {
        case WIN_MUC:
        {
            ProfMucWin* mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            cl_ev_send_muc_msg(mucwin, clipboard_buffer, NULL);
            break;
        }
        case WIN_CHAT:
        {
            ProfChatWin* chatwin = (ProfChatWin*)window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
            cl_ev_send_msg(chatwin, clipboard_buffer, NULL);
            break;
        }
        case WIN_PRIVATE:
        {
            ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
            assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
            cl_ev_send_priv_msg(privatewin, clipboard_buffer, NULL);
            break;
        }
        case WIN_CONSOLE:
        case WIN_XML:
        default:
            cons_bad_cmd_usage(command);
            break;
        }
    }
#else
    cons_show("This version of Profanity has not been built with GTK support enabled. It is needed for the clipboard feature to work.");
#endif

    return TRUE;
}

gboolean
cmd_stamp(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strv_length(args) == 0) {
        auto_gchar gchar* def_incoming = prefs_get_string(PREF_OUTGOING_STAMP);
        if (def_incoming) {
            cons_show("The outgoing stamp is: %s", def_incoming);
        } else {
            cons_show("The default outgoing stamp is used.");
        }
        auto_gchar gchar* def_outgoing = prefs_get_string(PREF_INCOMING_STAMP);
        if (def_outgoing) {
            cons_show("The incoming stamp is: %s", def_outgoing);
        } else {
            cons_show("The default incoming stamp is used.");
        }
        return TRUE;
    }

    if (g_strv_length(args) == 1) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (g_strv_length(args) == 2) {
        if (g_strcmp0(args[0], "outgoing") == 0) {
            prefs_set_string(PREF_OUTGOING_STAMP, args[1]);
            cons_show("Outgoing stamp set to: %s", args[1]);
        } else if (g_strcmp0(args[0], "incoming") == 0) {
            prefs_set_string(PREF_INCOMING_STAMP, args[1]);
            cons_show("Incoming stamp set to: %s", args[1]);
        } else if (g_strcmp0(args[0], "unset") == 0) {
            if (g_strcmp0(args[1], "incoming") == 0) {
                prefs_set_string(PREF_INCOMING_STAMP, NULL);
                cons_show("Incoming stamp unset");
            } else if (g_strcmp0(args[1], "outgoing") == 0) {
                prefs_set_string(PREF_OUTGOING_STAMP, NULL);
                cons_show("Outgoing stamp unset");
            } else {
                cons_bad_cmd_usage(command);
            }
        } else {
            cons_bad_cmd_usage(command);
        }
    }

    return TRUE;
}

gboolean
cmd_color(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strcmp0(args[0], "on") == 0) {
        prefs_set_string(PREF_COLOR_NICK, "true");
    } else if (g_strcmp0(args[0], "off") == 0) {
        prefs_set_string(PREF_COLOR_NICK, "false");
    } else if (g_strcmp0(args[0], "redgreen") == 0) {
        prefs_set_string(PREF_COLOR_NICK, "redgreen");
    } else if (g_strcmp0(args[0], "blue") == 0) {
        prefs_set_string(PREF_COLOR_NICK, "blue");
    } else if (g_strcmp0(args[0], "own") == 0) {
        if (g_strcmp0(args[1], "on") == 0) {
            _cmd_set_boolean_preference(args[1], "Color generation for own nick", PREF_COLOR_NICK_OWN);
        }
    } else {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    cons_show("Consistent color generation for nicks set to: %s", args[0]);

    auto_gchar gchar* theme = prefs_get_string(PREF_THEME);
    if (theme) {
        gboolean res = theme_load(theme, false);

        if (res) {
            cons_show("Theme reloaded: %s", theme);
        } else {
            theme_load("default", false);
        }
    }

    return TRUE;
}

gboolean
cmd_avatar(ProfWin* window, const char* const command, gchar** args)
{
    if (args[1] == NULL) {
        if (g_strcmp0(args[0], "disable") == 0) {
            if (avatar_publishing_disable()) {
                cons_show("Avatar publishing disabled. To enable avatar publishing, use '/avatar set <path>'.");
            } else {
                cons_show("Failed to disable avatar publishing.");
            }
        } else {
            cons_bad_cmd_usage(command);
        }
    } else {
        if (g_strcmp0(args[0], "set") == 0) {
#ifdef HAVE_PIXBUF
            if (avatar_set(args[1])) {
                cons_show("Avatar updated successfully");
            }
#else
            cons_show("Profanity has not been built with GDK Pixbuf support enabled which is needed to scale the avatar when uploading.");
#endif
        } else if (g_strcmp0(args[0], "get") == 0) {
            avatar_get_by_nick(args[1], false);
        } else if (g_strcmp0(args[0], "open") == 0) {
            avatar_get_by_nick(args[1], true);
        } else {
            cons_bad_cmd_usage(command);
        }
    }

    return TRUE;
}

gboolean
cmd_os(ProfWin* window, const char* const command, gchar** args)
{
    _cmd_set_boolean_preference(args[1], "Revealing OS name", PREF_REVEAL_OS);

    return TRUE;
}

gboolean
cmd_correction(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strcmp0(args[0], "on") == 0) {
        _cmd_set_boolean_preference(args[0], "Last Message Correction", PREF_CORRECTION_ALLOW);
        caps_add_feature(XMPP_FEATURE_LAST_MESSAGE_CORRECTION);
        return TRUE;
    } else if (g_strcmp0(args[0], "off") == 0) {
        _cmd_set_boolean_preference(args[0], "Last Message Correction", PREF_CORRECTION_ALLOW);
        caps_remove_feature(XMPP_FEATURE_LAST_MESSAGE_CORRECTION);
        return TRUE;
    }

    if (g_strcmp0(args[0], "char") == 0) {
        if (args[1] == NULL || g_utf8_strlen(args[1], 4) != 1) {
            cons_bad_cmd_usage(command);
        } else {
            prefs_set_correction_char(args[1]);
            cons_show("LMC char set to %s.", args[1]);
        }
    }

    return TRUE;
}

gboolean
_can_correct(ProfWin* window)
{
    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are currently not connected.");
        return FALSE;
    } else if (!prefs_get_boolean(PREF_CORRECTION_ALLOW)) {
        win_println(window, THEME_DEFAULT, "!", "Corrections not enabled. See /help correction.");
        return FALSE;
    } else if (window->type == WIN_CHAT) {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);

        if (chatwin->last_msg_id == NULL || chatwin->last_message == NULL) {
            win_println(window, THEME_DEFAULT, "!", "No last message to correct.");
            return FALSE;
        }
    } else if (window->type == WIN_MUC) {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

        if (mucwin->last_msg_id == NULL || mucwin->last_message == NULL) {
            win_println(window, THEME_DEFAULT, "!", "No last message to correct.");
            return FALSE;
        }
    } else {
        win_println(window, THEME_DEFAULT, "!", "Command /correct-editor only valid in regular chat windows.");
        return FALSE;
    }

    return TRUE;
}

gboolean
cmd_correct(ProfWin* window, const char* const command, gchar** args)
{
    if (!_can_correct(window)) {
        return TRUE;
    }

    if (window->type == WIN_CHAT) {
        ProfChatWin* chatwin = (ProfChatWin*)window;

        // send message again, with replace flag
        auto_gchar gchar* message = g_strjoinv(" ", args);
        cl_ev_send_msg_correct(chatwin, message, FALSE, TRUE);
    } else if (window->type == WIN_MUC) {
        ProfMucWin* mucwin = (ProfMucWin*)window;

        // send message again, with replace flag
        auto_gchar gchar* message = g_strjoinv(" ", args);
        cl_ev_send_muc_msg_corrected(mucwin, message, FALSE, TRUE);
    }

    return TRUE;
}

gboolean
cmd_slashguard(ProfWin* window, const char* const command, gchar** args)
{
    if (args[0] == NULL) {
        return FALSE;
    }

    _cmd_set_boolean_preference(args[0], "Slashguard", PREF_SLASH_GUARD);

    return TRUE;
}

gchar*
_prepare_filename(gchar* url, gchar* path)
{
    // Ensure that the downloads directory exists for saving cleartexts.
    auto_gchar gchar* downloads_dir = path ? get_expanded_path(path) : files_get_data_path(DIR_DOWNLOADS);
    if (g_mkdir_with_parents(downloads_dir, S_IRWXU) != 0) {
        cons_show_error("Failed to create download directory "
                        "at '%s' with error '%s'",
                        downloads_dir, strerror(errno));
        return NULL;
    }

    // Generate an unique filename from the URL that should be stored in the
    // downloads directory.
    return unique_filename_from_url(url, downloads_dir);
}

#ifdef HAVE_OMEMO
void
_url_aesgcm_method(ProfWin* window, const char* cmd_template, gchar* url, gchar* path)
{
    auto_gchar gchar* filename = _prepare_filename(url, path);
    if (!filename)
        return;
    auto_char char* id = get_random_string(4);
    AESGCMDownload* download = malloc(sizeof(AESGCMDownload));
    download->window = window;
    download->url = strdup(url);
    download->filename = strdup(filename);
    download->id = strdup(id);
    if (cmd_template != NULL) {
        download->cmd_template = strdup(cmd_template);
    } else {
        download->cmd_template = NULL;
    }

    pthread_create(&(download->worker), NULL, &aesgcm_file_get, download);
    aesgcm_download_add_download(download);
}
#endif

static gboolean
_download_install_plugin(ProfWin* window, gchar* url, gchar* path)
{
    auto_gchar gchar* filename = _prepare_filename(url, path);
    if (!filename)
        return FALSE;
    HTTPDownload* download = malloc(sizeof(HTTPDownload));
    download->window = window;
    download->url = strdup(url);
    download->filename = strdup(filename);
    download->id = get_random_string(4);
    download->cmd_template = NULL;

    pthread_create(&(download->worker), NULL, &plugin_download_install, download);
    plugin_download_add_download(download);
    return TRUE;
}

void
_url_http_method(ProfWin* window, const char* cmd_template, gchar* url, gchar* path)
{
    auto_gchar gchar* filename = _prepare_filename(url, path);
    if (!filename)
        return;
    auto_char char* id = get_random_string(4);
    HTTPDownload* download = malloc(sizeof(HTTPDownload));
    download->window = window;
    download->url = strdup(url);
    download->filename = strdup(filename);
    download->id = strdup(id);
    download->cmd_template = cmd_template ? strdup(cmd_template) : NULL;

    pthread_create(&(download->worker), NULL, &http_file_get, download);
    http_download_add_download(download);
}

void
_url_external_method(const char* cmd_template, const char* url, gchar* filename)
{
    auto_gcharv gchar** argv = format_call_external_argv(cmd_template, url, filename);

    if (!call_external(argv)) {
        cons_show_error("Unable to call external executable for url: check the logs for more information.");
    } else {
        cons_show("URL '%s' has been called with '%s'.", url, cmd_template);
    }
}

gboolean
cmd_url_open(ProfWin* window, const char* const command, gchar** args)
{
    if (window->type != WIN_CHAT && window->type != WIN_MUC && window->type != WIN_PRIVATE) {
        cons_show_error("url open not supported in this window");
        return TRUE;
    }

    gchar* url = args[1];
    if (url == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    // reset autocompletion to start from latest url and not where we left of
    autocomplete_reset(window->urls_ac);

    auto_gchar gchar* scheme = g_uri_parse_scheme(url);
    if (scheme == NULL) {
        cons_show_error("URL '%s' is not valid.", args[1]);
        return TRUE;
    }

    auto_gchar gchar* cmd_template = prefs_get_string(PREF_URL_OPEN_CMD);
    if (cmd_template == NULL) {
        cons_show_error("No default `url open` command found in executables preferences.");
        return TRUE;
    }

#ifdef HAVE_OMEMO
    // OMEMO URLs (aesgcm://) must be saved and decrypted before being opened.
    if (g_strcmp0(scheme, "aesgcm") == 0) {
        // Download, decrypt and open the cleartext version of the AESGCM
        // encrypted file.
        _url_aesgcm_method(window, cmd_template, url, NULL);
        return TRUE;
    }
#endif

    _url_external_method(cmd_template, url, NULL);
    return TRUE;
}

gboolean
cmd_url_save(ProfWin* window, const char* const command, gchar** args)
{
    if (window->type != WIN_CHAT && window->type != WIN_MUC && window->type != WIN_PRIVATE) {
        cons_show_error("`/url save` is not supported in this window.");
        return TRUE;
    }

    if (args[1] == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    gchar* url = args[1];
    gchar* path = args[2]; // might be NULL, intentionally skip NULL check

    // reset autocompletion to start from latest url and not where we left of
    autocomplete_reset(window->urls_ac);

    auto_gchar gchar* scheme = g_uri_parse_scheme(url);
    if (scheme == NULL) {
        cons_show_error("URL '%s' is not valid.", url);
        return TRUE;
    }

    auto_gchar gchar* cmd_template = prefs_get_string(PREF_URL_SAVE_CMD);
    if (cmd_template == NULL && (g_strcmp0(scheme, "http") == 0 || g_strcmp0(scheme, "https") == 0)) {
        _url_http_method(window, cmd_template, url, path);
#ifdef HAVE_OMEMO
    } else if (g_strcmp0(scheme, "aesgcm") == 0) {
        _url_aesgcm_method(window, cmd_template, url, path);
#endif
    } else if (cmd_template != NULL) {
        auto_gchar gchar* filename = _prepare_filename(url, NULL);
        if (!filename)
            return TRUE;
        _url_external_method(cmd_template, url, filename);
    } else {
        cons_show_error("No download method defined for the scheme '%s'.", scheme);
    }

    return TRUE;
}

gboolean
_cmd_executable_template(const preference_t setting, const char* command, gchar** args)
{
    guint num_args = g_strv_length(args);
    if (num_args < 2) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (g_strcmp0(args[1], "set") == 0 && num_args >= 3) {
        auto_gchar gchar* str = g_strjoinv(" ", &args[2]);
        prefs_set_string(setting, str);
        cons_show("`%s` command set to invoke '%s'", command, str);
    } else if (g_strcmp0(args[1], "default") == 0) {
        prefs_set_string(setting, NULL);
        auto_gchar gchar* def = prefs_get_string(setting);
        if (def == NULL) {
            def = g_strdup("built-in method");
        }
        cons_show("`%s` command set to invoke %s (default)", command, def);
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_executable_avatar(ProfWin* window, const char* const command, gchar** args)
{
    return _cmd_executable_template(PREF_AVATAR_CMD, command, args);
}

gboolean
cmd_executable_urlopen(ProfWin* window, const char* const command, gchar** args)
{
    return _cmd_executable_template(PREF_URL_OPEN_CMD, command, args);
}

gboolean
cmd_executable_urlsave(ProfWin* window, const char* const command, gchar** args)
{
    return _cmd_executable_template(PREF_URL_SAVE_CMD, command, args);
}

gboolean
cmd_executable_editor(ProfWin* window, const char* const command, gchar** args)
{
    return _cmd_executable_template(PREF_COMPOSE_EDITOR, command, args);
}

gboolean
cmd_executable_vcard_photo(ProfWin* window, const char* const command, gchar** args)
{
    return _cmd_executable_template(PREF_VCARD_PHOTO_CMD, command, args);
}

gboolean
cmd_mam(ProfWin* window, const char* const command, gchar** args)
{
    _cmd_set_boolean_preference(args[0], "Message Archive Management", PREF_MAM);

    return TRUE;
}

gboolean
cmd_change_password(ProfWin* window, const char* const command, gchar** args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    auto_char char* passwd = ui_ask_password(false);
    auto_char char* confirm_passwd = ui_ask_password(true);

    if (g_strcmp0(passwd, confirm_passwd) == 0) {
        iq_register_change_password(connection_get_user(), passwd);
    } else {
        cons_show("Aborted! The new password and the confirmed password do not match.");
    }

    return TRUE;
}

gboolean
cmd_editor(ProfWin* window, const char* const command, gchar** args)
{
    auto_gchar gchar* message = NULL;

    if (get_message_from_editor(NULL, &message)) {
        return TRUE;
    }

    rl_insert_text(message);
    ui_resize();
    rl_point = rl_end;
    rl_forced_update_display();

    return TRUE;
}

gboolean
cmd_correct_editor(ProfWin* window, const char* const command, gchar** args)
{
    if (!_can_correct(window)) {
        return TRUE;
    }

    gchar* initial_message = win_get_last_sent_message(window);

    auto_gchar gchar* message = NULL;
    if (get_message_from_editor(initial_message, &message)) {
        return TRUE;
    }

    if (window->type == WIN_CHAT) {
        ProfChatWin* chatwin = (ProfChatWin*)window;

        cl_ev_send_msg_correct(chatwin, message, FALSE, TRUE);
    } else if (window->type == WIN_MUC) {
        ProfMucWin* mucwin = (ProfMucWin*)window;

        cl_ev_send_muc_msg_corrected(mucwin, message, FALSE, TRUE);
    }

    return TRUE;
}

gboolean
cmd_redraw(ProfWin* window, const char* const command, gchar** args)
{
    ui_resize();

    return TRUE;
}

gboolean
cmd_silence(ProfWin* window, const char* const command, gchar** args)
{
    _cmd_set_boolean_preference(args[0], "Block all messages from JIDs that are not in the roster", PREF_SILENCE_NON_ROSTER);

    return TRUE;
}

gboolean
cmd_register(ProfWin* window, const char* const command, gchar** args)
{
    gchar* opt_keys[] = { "port", "tls", "auth", NULL };
    gboolean parsed;

    GHashTable* options = parse_options(&args[2], opt_keys, &parsed);
    if (!parsed) {
        cons_bad_cmd_usage(command);
        cons_show("");
        options_destroy(options);
        return TRUE;
    }

    char* tls_policy = g_hash_table_lookup(options, "tls");
    if (!_string_matches_one_of("TLS policy", tls_policy, TRUE, "force", "allow", "trust", "disable", "legacy", NULL)) {
        cons_bad_cmd_usage(command);
        cons_show("");
        options_destroy(options);
        return TRUE;
    }

    int port = 0;
    if (g_hash_table_contains(options, "port")) {
        char* port_str = g_hash_table_lookup(options, "port");
        auto_char char* err_msg = NULL;
        gboolean res = strtoi_range(port_str, &port, 1, 65535, &err_msg);
        if (!res) {
            cons_show(err_msg);
            cons_show("");
            port = 0;
            options_destroy(options);
            return TRUE;
        }
    }

    char* username = args[0];
    char* server = args[1];

    auto_char char* passwd = ui_ask_password(false);
    auto_char char* confirm_passwd = ui_ask_password(true);

    if (g_strcmp0(passwd, confirm_passwd) == 0) {
        log_info("Attempting to register account %s on server %s.", username, server);
        connection_register(server, port, tls_policy, username, passwd);
    } else {
        cons_show("The two passwords do not match.");
    }

    if (connection_get_status() == JABBER_DISCONNECTED) {
        cons_show_error("Connection attempt to server %s port %d failed.", server, port);
        log_info("Connection attempt to server %s port %d failed.", server, port);
        return TRUE;
    }

    options_destroy(options);

    log_info("we are leaving the registration process");
    return TRUE;
}

gboolean
cmd_mood(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strcmp0(args[0], "on") == 0) {
        _cmd_set_boolean_preference(args[0], "User mood", PREF_MOOD);
        caps_add_feature(STANZA_NS_MOOD_NOTIFY);
    } else if (g_strcmp0(args[0], "off") == 0) {
        _cmd_set_boolean_preference(args[0], "User mood", PREF_MOOD);
        caps_remove_feature(STANZA_NS_MOOD_NOTIFY);
    } else if (g_strcmp0(args[0], "set") == 0) {
        if (args[1]) {
            cons_show("Your mood: %s", args[1]);
            if (args[2]) {
                publish_user_mood(args[1], args[2]);
            } else {
                publish_user_mood(args[1], args[1]);
            }
        }
    } else if (g_strcmp0(args[0], "clear") == 0) {
        cons_show("Clearing the user mood.");
        publish_user_mood(NULL, NULL);
    }

    return TRUE;
}

gboolean
cmd_strophe(ProfWin* window, const char* const command, gchar** args)
{
    if (g_strcmp0(args[0], "verbosity") == 0) {
        int verbosity;
        auto_gchar gchar* err_msg = NULL;
        if (string_to_verbosity(args[1], &verbosity, &err_msg)) {
            xmpp_ctx_set_verbosity(connection_get_ctx(), verbosity);
            prefs_set_string(PREF_STROPHE_VERBOSITY, args[1]);
            return TRUE;
        } else {
            cons_show(err_msg);
        }
    } else if (g_strcmp0(args[0], "sm") == 0) {
        if (g_strcmp0(args[1], "no-resend") == 0) {
            cons_show("Stream Management set to 'no-resend'.");
            prefs_set_boolean(PREF_STROPHE_SM_ENABLED, TRUE);
            prefs_set_boolean(PREF_STROPHE_SM_RESEND, FALSE);
            return TRUE;
        } else if (g_strcmp0(args[1], "on") == 0) {
            cons_show("Stream Management enabled.");
            prefs_set_boolean(PREF_STROPHE_SM_ENABLED, TRUE);
            prefs_set_boolean(PREF_STROPHE_SM_RESEND, TRUE);
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            cons_show("Stream Management disabled.");
            prefs_set_boolean(PREF_STROPHE_SM_ENABLED, FALSE);
            prefs_set_boolean(PREF_STROPHE_SM_RESEND, FALSE);
            return TRUE;
        }
    }
    cons_bad_cmd_usage(command);
    return TRUE;
}

gboolean
cmd_vcard(ProfWin* window, const char* const command, gchar** args)
{
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    ProfVcardWin* vcardwin = wins_get_vcard();

    if (vcardwin) {
        ui_focus_win((ProfWin*)vcardwin);
    } else {
        vcardwin = (ProfVcardWin*)vcard_user_create_win();
        ui_focus_win((ProfWin*)vcardwin);
    }
    vcardwin_update();
    return TRUE;
}

gboolean
cmd_vcard_add(ProfWin* window, const char* const command, gchar** args)
{
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    vcard_element_t* element = calloc(1, sizeof(vcard_element_t));
    if (!element) {
        cons_show_error("Memory allocation failed.");
        return TRUE;
    }

    struct tm tm;
    gchar* type = args[1];
    gchar* value = args[2];

    if (g_strcmp0(type, "nickname") == 0) {
        element->type = VCARD_NICKNAME;

        element->nickname = strdup(value);
    } else if (g_strcmp0(type, "birthday") == 0) {
        element->type = VCARD_BIRTHDAY;

        memset(&tm, 0, sizeof(struct tm));
        if (!strptime(value, "%Y-%m-%d", &tm)) {
            cons_show_error("Error parsing ISO8601 date.");
            free(element);
            return TRUE;
        }
        element->birthday = g_date_time_new_local(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 0, 0, 0);
    } else if (g_strcmp0(type, "tel") == 0) {
        element->type = VCARD_TELEPHONE;
        if (value) {
            element->telephone.number = strdup(value);
        }
    } else if (g_strcmp0(type, "address") == 0) {
        element->type = VCARD_ADDRESS;
    } else if (g_strcmp0(type, "email") == 0) {
        element->type = VCARD_EMAIL;
        if (value) {
            element->email.userid = strdup(value);
        }
    } else if (g_strcmp0(type, "jid") == 0) {
        element->type = VCARD_JID;
        if (value) {
            element->jid = strdup(value);
        }
    } else if (g_strcmp0(type, "title") == 0) {
        element->type = VCARD_TITLE;
        if (value) {
            element->title = strdup(value);
        }
    } else if (g_strcmp0(type, "role") == 0) {
        element->type = VCARD_ROLE;
        if (value) {
            element->role = strdup(value);
        }
    } else if (g_strcmp0(type, "note") == 0) {
        element->type = VCARD_NOTE;
        if (value) {
            element->note = strdup(value);
        }
    } else if (g_strcmp0(type, "url") == 0) {
        element->type = VCARD_URL;
        if (value) {
            element->url = strdup(value);
        }
    } else {
        cons_bad_cmd_usage(command);
        free(element);
        return TRUE;
    }

    vcard_user_add_element(element);
    vcardwin_update();
    return TRUE;
}

gboolean
cmd_vcard_remove(ProfWin* window, const char* const command, gchar** args)
{
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (args[1]) {
        vcard_user_remove_element(atoi(args[1]));
        cons_show("Removed element at index %d", atoi(args[1]));
        vcardwin_update();
    } else {
        cons_bad_cmd_usage(command);
    }
    return TRUE;
}

gboolean
cmd_vcard_get(ProfWin* window, const char* const command, gchar** args)
{
    char* user = args[1];
    xmpp_ctx_t* const ctx = connection_get_ctx();

    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (user) {
        // get the JID when in MUC window
        if (window->type == WIN_MUC) {
            ProfMucWin* mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

            if (muc_anonymity_type(mucwin->roomjid) == MUC_ANONYMITY_TYPE_NONANONYMOUS) {
                // non-anon muc: get the user's jid and send vcard request to them
                Occupant* occupant = muc_roster_item(mucwin->roomjid, user);
                auto_jid Jid* jid_occupant = jid_create(occupant->jid);

                vcard_print(ctx, window, jid_occupant->barejid);
            } else {
                // anon muc: send the vcard request through the MUC's server
                auto_gchar gchar* full_jid = g_strdup_printf("%s/%s", mucwin->roomjid, user);
                vcard_print(ctx, window, full_jid);
            }
        } else {
            char* jid = roster_barejid_from_name(user);
            if (!jid) {
                cons_bad_cmd_usage(command);
                return TRUE;
            }

            vcard_print(ctx, window, jid);
        }
    } else {
        if (window->type == WIN_CHAT) {
            ProfChatWin* chatwin = (ProfChatWin*)window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);

            vcard_print(ctx, window, chatwin->barejid);
        } else {
            vcard_print(ctx, window, NULL);
        }
    }

    return TRUE;
}

gboolean
cmd_vcard_photo(ProfWin* window, const char* const command, gchar** args)
{
    char* operation = args[1];
    char* user = args[2];

    xmpp_ctx_t* const ctx = connection_get_ctx();

    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    gboolean jidless = (g_strcmp0(operation, "open-self") == 0 || g_strcmp0(operation, "save-self") == 0);

    if (!operation || (!jidless && !user)) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    auto_gchar gchar* jid = NULL;
    char* filepath = NULL;
    int index = 0;

    if (!jidless) {
        if (window->type == WIN_MUC) {
            ProfMucWin* mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

            if (muc_anonymity_type(mucwin->roomjid) == MUC_ANONYMITY_TYPE_NONANONYMOUS) {
                // non-anon muc: get the user's jid and send vcard request to them
                Occupant* occupant = muc_roster_item(mucwin->roomjid, user);
                auto_jid Jid* jid_occupant = jid_create(occupant->jid);

                jid = g_strdup(jid_occupant->barejid);
            } else {
                // anon muc: send the vcard request through the MUC's server
                jid = g_strdup_printf("%s/%s", mucwin->roomjid, user);
            }
        } else {
            char* jid_temp = roster_barejid_from_name(user);
            if (!jid_temp) {
                cons_bad_cmd_usage(command);
                return TRUE;
            } else {
                jid = g_strdup(jid_temp);
            }
        }
    }
    if (!g_strcmp0(operation, "open")) {
        // if an index is provided
        if (args[3]) {
            vcard_photo(ctx, jid, NULL, atoi(args[3]), TRUE);
        } else {
            vcard_photo(ctx, jid, NULL, -1, TRUE);
        }
    } else if (!g_strcmp0(operation, "save")) {
        // arguments
        if (g_strv_length(args) > 2) {
            gchar* opt_keys[] = { "output", "index", NULL };
            gboolean parsed;

            GHashTable* options = parse_options(&args[3], opt_keys, &parsed);
            if (!parsed) {
                cons_bad_cmd_usage(command);
                options_destroy(options);
                return TRUE;
            }

            filepath = g_hash_table_lookup(options, "output");
            if (!filepath) {
                filepath = NULL;
            }

            char* index_str = g_hash_table_lookup(options, "index");
            if (!index_str) {
                index = -1;
            } else {
                index = atoi(index_str);
            }

            options_destroy(options);
        } else {
            filepath = NULL;
            index = -1;
        }

        vcard_photo(ctx, jid, filepath, index, FALSE);
    } else if (!g_strcmp0(operation, "open-self")) {
        // if an index is provided
        if (args[2]) {
            vcard_photo(ctx, NULL, NULL, atoi(args[2]), TRUE);
        } else {
            vcard_photo(ctx, NULL, NULL, -1, TRUE);
        }
    } else if (!g_strcmp0(operation, "save-self")) {
        // arguments
        if (g_strv_length(args) > 2) {
            gchar* opt_keys[] = { "output", "index", NULL };
            gboolean parsed;

            GHashTable* options = parse_options(&args[2], opt_keys, &parsed);
            if (!parsed) {
                cons_bad_cmd_usage(command);
                options_destroy(options);
                return TRUE;
            }

            filepath = g_hash_table_lookup(options, "output");
            if (!filepath) {
                filepath = NULL;
            }

            char* index_str = g_hash_table_lookup(options, "index");
            if (!index_str) {
                index = -1;
            } else {
                index = atoi(index_str);
            }

            options_destroy(options);
        } else {
            filepath = NULL;
            index = -1;
        }

        vcard_photo(ctx, NULL, filepath, index, FALSE);
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_vcard_refresh(ProfWin* window, const char* const command, gchar** args)
{
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    vcard_user_refresh();
    vcardwin_update();
    return TRUE;
}

gboolean
cmd_vcard_set(ProfWin* window, const char* const command, gchar** args)
{
    char* key = args[1];
    char* value = args[2];

    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (!key) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    gboolean is_num = TRUE;
    for (int i = 0; i < strlen(key); i++) {
        if (!isdigit((int)key[i])) {
            is_num = FALSE;
            break;
        }
    }

    if (g_strcmp0(key, "fullname") == 0 && value) {
        vcard_user_set_fullname(value);
        cons_show("User vCard's full name has been set");
    } else if (g_strcmp0(key, "name") == 0 && value) {
        char* value2 = args[3];

        if (!value2) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        if (g_strcmp0(value, "family") == 0) {
            vcard_user_set_name_family(value2);
            cons_show("User vCard's family name has been set");
        } else if (g_strcmp0(value, "given") == 0) {
            vcard_user_set_name_given(value2);
            cons_show("User vCard's given name has been set");
        } else if (g_strcmp0(value, "middle") == 0) {
            vcard_user_set_name_middle(value2);
            cons_show("User vCard's middle name has been set");
        } else if (g_strcmp0(value, "prefix") == 0) {
            vcard_user_set_name_prefix(value2);
            cons_show("User vCard's prefix name has been set");
        } else if (g_strcmp0(value, "suffix") == 0) {
            vcard_user_set_name_suffix(value2);
            cons_show("User vCard's suffix name has been set");
        }
    } else if (is_num) {
        char* value2 = args[3];
        struct tm tm;

        vcard_element_t* element = vcard_user_get_element_index(atoi(key));

        if (!element) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        if (!value2 || !value) {
            // Set the main field of element at index <key> to <value>, or from an editor

            switch (element->type) {
            case VCARD_NICKNAME:
                if (!value) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->nickname, &editor_value)) {
                        return TRUE;
                    }

                    if (element->nickname) {
                        free(element->nickname);
                    }
                    element->nickname = editor_value;
                } else {
                    if (element->nickname) {
                        free(element->nickname);
                    }
                    element->nickname = strdup(value);
                }
                break;
            case VCARD_BIRTHDAY:
                memset(&tm, 0, sizeof(struct tm));
                if (!strptime(value, "%Y-%m-%d", &tm)) {
                    cons_show_error("Error parsing ISO8601 date.");
                    return TRUE;
                }

                if (element->birthday) {
                    g_date_time_unref(element->birthday);
                }
                element->birthday = g_date_time_new_local(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 0, 0, 0);
                break;
            case VCARD_TELEPHONE:
                if (!value) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->telephone.number, &editor_value)) {
                        return TRUE;
                    }

                    if (element->telephone.number) {
                        free(element->telephone.number);
                    }
                    element->telephone.number = editor_value;
                } else {
                    if (element->telephone.number) {
                        free(element->telephone.number);
                    }
                    element->telephone.number = strdup(value);
                }

                break;
            case VCARD_EMAIL:
                if (!value) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->email.userid, &editor_value)) {
                        return TRUE;
                    }

                    if (element->email.userid) {
                        free(element->email.userid);
                    }
                    element->email.userid = editor_value;
                } else {
                    if (element->email.userid) {
                        free(element->email.userid);
                    }
                    element->email.userid = strdup(value);
                }
                break;
            case VCARD_JID:
                if (!value) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->jid, &editor_value)) {
                        return TRUE;
                    }

                    if (element->jid) {
                        free(element->jid);
                    }
                    element->jid = editor_value;
                } else {
                    if (element->jid) {
                        free(element->jid);
                    }
                    element->jid = strdup(value);
                }
                break;
            case VCARD_TITLE:
                if (!value) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->title, &editor_value)) {
                        return TRUE;
                    }

                    if (element->title) {
                        free(element->title);
                    }
                    element->title = editor_value;
                } else {
                    if (element->title) {
                        free(element->title);
                    }
                    element->title = strdup(value);
                }
                break;
            case VCARD_ROLE:
                if (!value) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->role, &editor_value)) {
                        return TRUE;
                    }

                    if (element->role) {
                        free(element->role);
                    }
                    element->role = editor_value;
                } else {
                    if (element->role) {
                        free(element->role);
                    }
                    element->role = strdup(value);
                }
                break;
            case VCARD_NOTE:
                if (!value) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->note, &editor_value)) {
                        return TRUE;
                    }

                    if (element->note) {
                        free(element->note);
                    }
                    element->note = editor_value;
                } else {
                    if (element->note) {
                        free(element->note);
                    }
                    element->note = strdup(value);
                }
                break;
            case VCARD_URL:
                if (!value) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->url, &editor_value)) {
                        return TRUE;
                    }

                    if (element->url) {
                        free(element->url);
                    }
                    element->url = editor_value;
                } else {
                    if (element->url) {
                        free(element->url);
                    }
                    element->url = strdup(value);
                }
                break;
            default:
                cons_show_error("Element unsupported");
            }
        } else if (value) {
            if (g_strcmp0(value, "pobox") == 0 && element->type == VCARD_ADDRESS) {
                if (!value2) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->address.pobox, &editor_value)) {
                        return TRUE;
                    }

                    if (element->address.pobox) {
                        free(element->address.pobox);
                    }
                    element->address.pobox = editor_value;
                } else {
                    if (element->address.pobox) {
                        free(element->address.pobox);
                    }
                    element->address.pobox = strdup(value2);
                }
            } else if (g_strcmp0(value, "extaddr") == 0 && element->type == VCARD_ADDRESS) {
                if (!value2) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->address.extaddr, &editor_value)) {
                        return TRUE;
                    }

                    if (element->address.extaddr) {
                        free(element->address.extaddr);
                    }
                    element->address.extaddr = editor_value;
                } else {
                    if (element->address.extaddr) {
                        free(element->address.extaddr);
                    }
                    element->address.extaddr = strdup(value2);
                }
            } else if (g_strcmp0(value, "street") == 0 && element->type == VCARD_ADDRESS) {
                if (!value2) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->address.street, &editor_value)) {
                        return TRUE;
                    }

                    if (element->address.street) {
                        free(element->address.street);
                    }
                    element->address.street = editor_value;
                } else {
                    if (element->address.street) {
                        free(element->address.street);
                    }
                    element->address.street = strdup(value2);
                }
            } else if (g_strcmp0(value, "locality") == 0 && element->type == VCARD_ADDRESS) {
                if (!value2) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->address.locality, &editor_value)) {
                        return TRUE;
                    }

                    if (element->address.locality) {
                        free(element->address.locality);
                    }
                    element->address.locality = editor_value;
                } else {
                    if (element->address.locality) {
                        free(element->address.locality);
                    }
                    element->address.locality = strdup(value2);
                }
            } else if (g_strcmp0(value, "region") == 0 && element->type == VCARD_ADDRESS) {
                if (!value2) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->address.region, &editor_value)) {
                        return TRUE;
                    }

                    if (element->address.region) {
                        free(element->address.region);
                    }
                    element->address.region = editor_value;
                } else {
                    if (element->address.region) {
                        free(element->address.region);
                    }
                    element->address.region = strdup(value2);
                }
            } else if (g_strcmp0(value, "pocode") == 0 && element->type == VCARD_ADDRESS) {
                if (!value2) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->address.pcode, &editor_value)) {
                        return TRUE;
                    }

                    if (element->address.pcode) {
                        free(element->address.pcode);
                    }
                    element->address.pcode = editor_value;
                } else {
                    if (element->address.pcode) {
                        free(element->address.pcode);
                    }
                    element->address.pcode = strdup(value2);
                }
            } else if (g_strcmp0(value, "country") == 0 && element->type == VCARD_ADDRESS) {
                if (!value2) {
                    gchar* editor_value;
                    if (get_message_from_editor(element->address.country, &editor_value)) {
                        return TRUE;
                    }

                    if (element->address.country) {
                        free(element->address.country);
                    }
                    element->address.country = editor_value;
                } else {
                    if (element->address.country) {
                        free(element->address.country);
                    }
                    element->address.country = strdup(value2);
                }
            } else if (g_strcmp0(value, "type") == 0 && element->type == VCARD_ADDRESS) {
                if (g_strcmp0(value2, "domestic") == 0) {
                    element->address.options &= ~VCARD_INTL;
                    element->address.options |= VCARD_DOM;
                } else if (g_strcmp0(value2, "international") == 0) {
                    element->address.options &= ~VCARD_DOM;
                    element->address.options |= VCARD_INTL;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "home") == 0) {
                switch (element->type) {
                case VCARD_ADDRESS:
                    if (g_strcmp0(value2, "on") == 0) {
                        element->address.options |= VCARD_HOME;
                    } else if (g_strcmp0(value2, "off") == 0) {
                        element->address.options &= ~VCARD_HOME;
                    } else {
                        cons_bad_cmd_usage(command);
                        return TRUE;
                    }
                    break;
                case VCARD_TELEPHONE:
                    if (g_strcmp0(value2, "on") == 0) {
                        element->telephone.options |= VCARD_HOME;
                    } else if (g_strcmp0(value2, "off") == 0) {
                        element->telephone.options &= ~VCARD_HOME;
                    } else {
                        cons_bad_cmd_usage(command);
                        return TRUE;
                    }
                    break;
                case VCARD_EMAIL:
                    if (g_strcmp0(value2, "on") == 0) {
                        element->email.options |= VCARD_HOME;
                    } else if (g_strcmp0(value2, "off") == 0) {
                        element->email.options &= ~VCARD_HOME;
                    } else {
                        cons_bad_cmd_usage(command);
                        return TRUE;
                    }
                    break;
                default:
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "work") == 0) {
                switch (element->type) {
                case VCARD_ADDRESS:
                    if (g_strcmp0(value2, "on") == 0) {
                        element->address.options |= VCARD_WORK;
                    } else if (g_strcmp0(value2, "off") == 0) {
                        element->address.options &= ~VCARD_WORK;
                    } else {
                        cons_bad_cmd_usage(command);
                        return TRUE;
                    }
                    break;
                case VCARD_TELEPHONE:
                    if (g_strcmp0(value2, "on") == 0) {
                        element->telephone.options |= VCARD_WORK;
                    } else if (g_strcmp0(value2, "off") == 0) {
                        element->telephone.options &= ~VCARD_WORK;
                    } else {
                        cons_bad_cmd_usage(command);
                        return TRUE;
                    }
                    break;
                case VCARD_EMAIL:
                    if (g_strcmp0(value2, "on") == 0) {
                        element->email.options |= VCARD_WORK;
                    } else if (g_strcmp0(value2, "off") == 0) {
                        element->email.options &= ~VCARD_WORK;
                    } else {
                        cons_bad_cmd_usage(command);
                        return TRUE;
                    }
                    break;
                default:
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "voice") == 0 && element->type == VCARD_TELEPHONE) {
                if (g_strcmp0(value2, "on") == 0) {
                    element->telephone.options |= VCARD_TEL_VOICE;
                } else if (g_strcmp0(value2, "off") == 0) {
                    element->telephone.options &= ~VCARD_TEL_VOICE;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "fax") == 0 && element->type == VCARD_TELEPHONE) {
                if (g_strcmp0(value2, "on") == 0) {
                    element->telephone.options |= VCARD_TEL_FAX;
                } else if (g_strcmp0(value2, "off") == 0) {
                    element->telephone.options &= ~VCARD_TEL_FAX;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "pager") == 0 && element->type == VCARD_TELEPHONE) {
                if (g_strcmp0(value2, "on") == 0) {
                    element->telephone.options |= VCARD_TEL_PAGER;
                } else if (g_strcmp0(value2, "off") == 0) {
                    element->telephone.options &= ~VCARD_TEL_PAGER;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "msg") == 0 && element->type == VCARD_TELEPHONE) {
                if (g_strcmp0(value2, "on") == 0) {
                    element->telephone.options |= VCARD_TEL_MSG;
                } else if (g_strcmp0(value2, "off") == 0) {
                    element->telephone.options &= ~VCARD_TEL_MSG;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "cell") == 0 && element->type == VCARD_TELEPHONE) {
                if (g_strcmp0(value2, "on") == 0) {
                    element->telephone.options |= VCARD_TEL_CELL;
                } else if (g_strcmp0(value2, "off") == 0) {
                    element->telephone.options &= ~VCARD_TEL_CELL;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "video") == 0 && element->type == VCARD_TELEPHONE) {
                if (g_strcmp0(value2, "on") == 0) {
                    element->telephone.options |= VCARD_TEL_VIDEO;
                } else if (g_strcmp0(value2, "off") == 0) {
                    element->telephone.options &= ~VCARD_TEL_VIDEO;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "bbs") == 0 && element->type == VCARD_TELEPHONE) {
                if (g_strcmp0(value2, "on") == 0) {
                    element->telephone.options |= VCARD_TEL_BBS;
                } else if (g_strcmp0(value2, "off") == 0) {
                    element->telephone.options &= ~VCARD_TEL_BBS;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "modem") == 0 && element->type == VCARD_TELEPHONE) {
                if (g_strcmp0(value2, "on") == 0) {
                    element->telephone.options |= VCARD_TEL_MODEM;
                } else if (g_strcmp0(value2, "off") == 0) {
                    element->telephone.options &= ~VCARD_TEL_MODEM;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "isdn") == 0 && element->type == VCARD_TELEPHONE) {
                if (g_strcmp0(value2, "on") == 0) {
                    element->telephone.options |= VCARD_TEL_ISDN;
                } else if (g_strcmp0(value2, "off") == 0) {
                    element->telephone.options &= ~VCARD_TEL_ISDN;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "pcs") == 0 && element->type == VCARD_TELEPHONE) {
                if (g_strcmp0(value2, "on") == 0) {
                    element->telephone.options |= VCARD_TEL_PCS;
                } else if (g_strcmp0(value2, "off") == 0) {
                    element->telephone.options &= ~VCARD_TEL_PCS;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "preferred") == 0) {
                switch (element->type) {
                case VCARD_ADDRESS:
                    if (g_strcmp0(value2, "on") == 0) {
                        element->address.options |= VCARD_PREF;
                    } else if (g_strcmp0(value2, "off") == 0) {
                        element->address.options &= ~VCARD_PREF;
                    } else {
                        cons_bad_cmd_usage(command);
                        return TRUE;
                    }
                    break;
                case VCARD_TELEPHONE:
                    if (g_strcmp0(value2, "on") == 0) {
                        element->telephone.options |= VCARD_PREF;
                    } else if (g_strcmp0(value2, "off") == 0) {
                        element->telephone.options &= ~VCARD_PREF;
                    } else {
                        cons_bad_cmd_usage(command);
                        return TRUE;
                    }
                    break;
                case VCARD_EMAIL:
                    if (g_strcmp0(value2, "on") == 0) {
                        element->email.options |= VCARD_PREF;
                    } else if (g_strcmp0(value2, "off") == 0) {
                        element->email.options &= ~VCARD_PREF;
                    } else {
                        cons_bad_cmd_usage(command);
                        return TRUE;
                    }
                    break;
                default:
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "parcel") == 0 && element->type == VCARD_ADDRESS) {
                if (g_strcmp0(value2, "on") == 0) {
                    element->address.options |= VCARD_PARCEL;
                } else if (g_strcmp0(value2, "off") == 0) {
                    element->address.options &= ~VCARD_PARCEL;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "postal") == 0 && element->type == VCARD_ADDRESS) {
                if (g_strcmp0(value2, "on") == 0) {
                    element->address.options |= VCARD_POSTAL;
                } else if (g_strcmp0(value2, "off") == 0) {
                    element->address.options &= ~VCARD_POSTAL;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "internet") == 0 && element->type == VCARD_EMAIL) {
                if (g_strcmp0(value2, "on") == 0) {
                    element->email.options |= VCARD_EMAIL_INTERNET;
                } else if (g_strcmp0(value2, "off") == 0) {
                    element->email.options &= ~VCARD_EMAIL_INTERNET;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else if (g_strcmp0(value, "x400") == 0 && element->type == VCARD_EMAIL) {
                if (g_strcmp0(value2, "on") == 0) {
                    element->email.options |= VCARD_EMAIL_X400;
                } else if (g_strcmp0(value2, "off") == 0) {
                    element->email.options &= ~VCARD_EMAIL_X400;
                } else {
                    cons_bad_cmd_usage(command);
                    return TRUE;
                }
            } else {
                cons_bad_cmd_usage(command);
                return TRUE;
            }
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    } else {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    vcardwin_update();
    return TRUE;
}

gboolean
cmd_vcard_save(ProfWin* window, const char* const command, gchar** args)
{
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    vcard_user_save();
    cons_show("User vCard uploaded");
    return TRUE;
}
