/*
 * cmd_funcs.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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

#define _GNU_SOURCE 1

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <glib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <langinfo.h>
#include <ctype.h>

#include "profanity.h"
#include "log.h"
#include "common.h"
#include "command/cmd_funcs.h"
#include "command/cmd_defs.h"
#include "command/cmd_ac.h"
#include "config/accounts.h"
#include "config/account.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "config/tlscerts.h"
#include "config/scripts.h"
#include "event/client_events.h"
#include "tools/http_upload.h"
#include "tools/autocomplete.h"
#include "tools/parser.h"
#include "tools/tinyurl.h"
#include "plugins/plugins.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/xmpp.h"
#include "xmpp/contact.h"
#include "xmpp/roster_list.h"
#include "xmpp/jid.h"
#include "xmpp/muc.h"
#include "xmpp/chat_session.h"

#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif

#ifdef HAVE_LIBGPGME
#include "pgp/gpg.h"
#endif

#ifdef HAVE_GTK
#include "ui/tray.h"
#endif

#ifdef HAVE_PYTHON
#include "plugins/python_plugins.h"
#endif

static void _update_presence(const resource_presence_t presence,
    const char *const show, gchar **args);
static void _cmd_set_boolean_preference(gchar *arg, const char *const command,
    const char *const display, preference_t pref);
static void _who_room(ProfWin *window, const char *const command, gchar **args);
static void _who_roster(ProfWin *window, const char *const command, gchar **args);
static gboolean _cmd_execute(ProfWin *window, const char *const command, const char *const inp);
static gboolean _cmd_execute_default(ProfWin *window, const char *inp);
static gboolean _cmd_execute_alias(ProfWin *window, const char *const inp, gboolean *ran);

/*
 * Take a line of input and process it, return TRUE if profanity is to
 * continue, FALSE otherwise
 */
gboolean
cmd_process_input(ProfWin *window, char *inp)
{
    log_debug("Input received: %s", inp);
    gboolean result = FALSE;
    g_strchomp(inp);

    // just carry on if no input
    if (strlen(inp) == 0) {
        result = TRUE;

    // handle command if input starts with a '/'
    } else if (inp[0] == '/') {
        char *inp_cpy = strdup(inp);
        char *command = strtok(inp_cpy, " ");
        char *question_mark = strchr(command, '?');
        if (question_mark) {
            *question_mark = '\0';
            char *fakeinp;
            if (asprintf(&fakeinp, "/help %s", command+1)) {
                result = _cmd_execute(window, "/help", fakeinp);
                free(fakeinp);
            }
        } else {
            result = _cmd_execute(window, command, inp);
        }
        free(inp_cpy);

    // call a default handler if input didn't start with '/'
    } else {
        result = _cmd_execute_default(window, inp);
    }

    return result;
}

// Command execution

void
cmd_execute_connect(ProfWin *window, const char *const account)
{
    GString *command = g_string_new("/connect ");
    g_string_append(command, account);
    cmd_process_input(window, command->str);
    g_string_free(command, TRUE);
}

gboolean
cmd_tls_certpath(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBMESODE
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
        char *path = prefs_get_tls_certpath();
        if (path) {
            cons_show("Trusted certificate path: %s", path);
            free(path);
        } else {
            cons_show("No trusted certificate path set.");
        }
        return TRUE;
    } else {
        cons_bad_cmd_usage(command);
        return TRUE;
    }
#else
    cons_show("Certificate path setting only supported when built with libmesode.");
    return TRUE;
#endif

}

gboolean
cmd_tls_trust(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBMESODE
    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }
    if (!connection_is_secured()) {
        cons_show("No TLS connection established");
        return TRUE;
    }
    TLSCertificate *cert = connection_get_tls_peer_cert();
    if (!cert) {
        cons_show("Error getting TLS certificate.");
        return TRUE;
    }
    if (tlscerts_exists(cert->fingerprint)) {
        cons_show("Certificate %s already trusted.", cert->fingerprint);
        tlscerts_free(cert);
        return TRUE;
    }
    cons_show("Adding %s to trusted certificates.", cert->fingerprint);
    tlscerts_add(cert);
    tlscerts_free(cert);
    return TRUE;
#else
    cons_show("Manual certificate trust only supported when built with libmesode.");
    return TRUE;
#endif
}

gboolean
cmd_tls_trusted(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBMESODE
    GList *certs = tlscerts_list();
    GList *curr = certs;

    if (curr) {
        cons_show("Trusted certificates:");
        cons_show("");
    } else {
        cons_show("No trusted certificates found.");
    }
    while (curr) {
        TLSCertificate *cert = curr->data;
        cons_show_tlscert_summary(cert);
        cons_show("");
        curr = g_list_next(curr);
    }
    g_list_free_full(certs, (GDestroyNotify)tlscerts_free);
    return TRUE;
#else
    cons_show("Manual certificate trust only supported when built with libmesode.");
    return TRUE;
#endif
}

gboolean
cmd_tls_revoke(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBMESODE
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
#else
    cons_show("Manual certificate trust only supported when built with libmesode.");
    return TRUE;
#endif
}

gboolean
cmd_tls_show(ProfWin *window, const char *const command, gchar **args)
{
    _cmd_set_boolean_preference(args[1], command, "TLS titlebar indicator", PREF_TLS_SHOW);
    return TRUE;
}

gboolean
cmd_tls_cert(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBMESODE
    if (args[1]) {
        TLSCertificate *cert = tlscerts_get_trusted(args[1]);
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
        TLSCertificate *cert = connection_get_tls_peer_cert();
        if (!cert) {
            cons_show("Error getting TLS certificate.");
            return TRUE;
        }
        cons_show_tlscert(cert);
        cons_show("");
        tlscerts_free(cert);
        return TRUE;
    }
#else
    cons_show("Certificate fetching not supported.");
    return TRUE;
#endif
}

gboolean
cmd_connect(ProfWin *window, const char *const command, gchar **args)
{
    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status != JABBER_DISCONNECTED) {
        cons_show("You are either connected already, or a login is in process.");
        return TRUE;
    }

    gchar *opt_keys[] = { "server", "port", "tls", NULL };
    gboolean parsed;

    GHashTable *options = parse_options(&args[args[0] ? 1 : 0], opt_keys, &parsed);
    if (!parsed) {
        cons_bad_cmd_usage(command);
        cons_show("");
        return TRUE;
    }

    char *altdomain = g_hash_table_lookup(options, "server");

    char *tls_policy = g_hash_table_lookup(options, "tls");
    if (tls_policy &&
            (g_strcmp0(tls_policy, "force") != 0) &&
            (g_strcmp0(tls_policy, "allow") != 0) &&
            (g_strcmp0(tls_policy, "disable") != 0) &&
            (g_strcmp0(tls_policy, "legacy") != 0)) {
        cons_bad_cmd_usage(command);
        cons_show("");
        return TRUE;
    }

    int port = 0;
    if (g_hash_table_contains(options, "port")) {
        char *port_str = g_hash_table_lookup(options, "port");
        char *err_msg = NULL;
        gboolean res = strtoi_range(port_str, &port, 1, 65535, &err_msg);
        if (!res) {
            cons_show(err_msg);
            cons_show("");
            free(err_msg);
            port = 0;
            return TRUE;
        }
    }

    char *user = args[0];
    char *def = prefs_get_string(PREF_DEFAULT_ACCOUNT);
    if (!user) {
        if (def) {
            user = def;
            cons_show("Using default account %s.", user);
        } else {
            cons_show("No default account.");
            g_free(def);
            return TRUE;
        }
    }

    char *lower = g_utf8_strdown(user, -1);
    char *jid;
    g_free(def);

    // connect with account
    ProfAccount *account = accounts_get_account(lower);
    if (account) {
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
                g_free(lower);
                account_free(account);
                return TRUE;
            }

        // no account password setting, prompt
        } else {
            account->password = ui_ask_password();
            conn_status = cl_ev_connect_account(account);
            free(account->password);
            account->password = NULL;
        }

        jid = account_create_connect_jid(account);
        account_free(account);

    // connect with JID
    } else {
        jid = strdup(lower);
        char *passwd = ui_ask_password();
        conn_status = cl_ev_connect_jid(jid, passwd, altdomain, port, tls_policy);
        free(passwd);
    }

    if (conn_status == JABBER_DISCONNECTED) {
        cons_show_error("Connection attempt for %s failed.", jid);
        log_info("Connection attempt for %s failed", jid);
    }

    options_destroy(options);
    g_free(lower);
    free(jid);

    return TRUE;
}

gboolean
cmd_account_list(ProfWin *window, const char *const command, gchar **args)
{
    gchar **accounts = accounts_get_list();
    cons_show_account_list(accounts);
    g_strfreev(accounts);

    return TRUE;
}

gboolean
cmd_account_show(ProfWin *window, const char *const command, gchar **args)
{
    char *account_name = args[1];
    if (account_name == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    ProfAccount *account = accounts_get_account(account_name);
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
cmd_account_add(ProfWin *window, const char *const command, gchar **args)
{
    char *account_name = args[1];
    if (account_name == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    accounts_add(account_name, NULL, 0, NULL);
    cons_show("Account created.");
    cons_show("");

    return TRUE;
}

gboolean
cmd_account_remove(ProfWin *window, const char *const command, gchar **args)
{
    char *account_name = args[1];
    if(!account_name) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    char *def = prefs_get_string(PREF_DEFAULT_ACCOUNT);
    if(accounts_remove(account_name)){
        cons_show("Account %s removed.", account_name);
        if(def && strcmp(def, account_name) == 0){
            prefs_set_string(PREF_DEFAULT_ACCOUNT, NULL);
            cons_show("Default account removed because the corresponding account was removed.");
        }
    } else {
        cons_show("Failed to remove account %s.", account_name);
        cons_show("Either the account does not exist, or an unknown error occurred.");
    }
    cons_show("");
    g_free(def);

    return TRUE;
}

gboolean
cmd_account_enable(ProfWin *window, const char *const command, gchar **args)
{
    char *account_name = args[1];
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
cmd_account_disable(ProfWin *window, const char *const command, gchar **args)
{
    char *account_name = args[1];
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
cmd_account_rename(ProfWin *window, const char *const command, gchar **args)
{
    if (g_strv_length(args) != 3) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    char *account_name = args[1];
    char *new_name = args[2];

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
cmd_account_default(ProfWin *window, const char *const command, gchar **args)
{
    if (g_strv_length(args) == 1) {
        char *def = prefs_get_string(PREF_DEFAULT_ACCOUNT);
        if (def) {
            cons_show("The default account is %s.", def);
            free(def);
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
            ProfAccount *account_p = accounts_get_account(args[2]);
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
_account_set_jid(char *account_name, char *jid)
{
    Jid *jidp = jid_create(jid);
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
    jid_destroy(jidp);

    return TRUE;
}

gboolean
_account_set_server(char *account_name, char *server)
{
    accounts_set_server(account_name, server);
    cons_show("Updated server for account %s: %s", account_name, server);
    cons_show("");
    return TRUE;
}

gboolean
_account_set_port(char *account_name, char *port)
{
    int porti;
    char *err_msg = NULL;
    gboolean res = strtoi_range(port, &porti, 1, 65535, &err_msg);
    if (!res) {
        cons_show(err_msg);
        cons_show("");
        free(err_msg);
    } else {
        accounts_set_port(account_name, porti);
        cons_show("Updated port for account %s: %s", account_name, port);
        cons_show("");
    }
    return TRUE;
}

gboolean
_account_set_resource(char *account_name, char *resource)
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
_account_set_password(char *account_name, char *password)
{
    ProfAccount *account = accounts_get_account(account_name);
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
_account_set_eval_password(char *account_name, char *eval_password)
{
    ProfAccount *account = accounts_get_account(account_name);
    if(account->password) {
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
_account_set_muc(char *account_name, char *muc)
{
    accounts_set_muc_service(account_name, muc);
    cons_show("Updated muc service for account %s: %s", account_name, muc);
    cons_show("");
    return TRUE;
}

gboolean
_account_set_nick(char *account_name, char *nick)
{
    accounts_set_muc_nick(account_name, nick);
    cons_show("Updated muc nick for account %s: %s", account_name, nick);
    cons_show("");
    return TRUE;
}

gboolean
_account_set_otr(char *account_name, char *policy)
{
    if ((g_strcmp0(policy, "manual") != 0)
            && (g_strcmp0(policy, "opportunistic") != 0)
            && (g_strcmp0(policy, "always") != 0)) {
        cons_show("OTR policy must be one of: manual, opportunistic or always.");
    } else {
        accounts_set_otr_policy(account_name, policy);
        cons_show("Updated OTR policy for account %s: %s", account_name, policy);
        cons_show("");
    }
    return TRUE;
}

gboolean
_account_set_status(char *account_name, char *status)
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
_account_set_pgpkeyid(char *account_name, char *pgpkeyid)
{
#ifdef HAVE_LIBGPGME
    char *err_str = NULL;
    if (!p_gpg_valid_key(pgpkeyid, &err_str)) {
        cons_show("Invalid PGP key ID specified: %s, see /pgp keys", err_str);
    } else {
        accounts_set_pgp_keyid(account_name, pgpkeyid);
        cons_show("Updated PGP key ID for account %s: %s", account_name, pgpkeyid);
    }
    free(err_str);
#else
    cons_show("PGP support is not included in this build.");
#endif
    cons_show("");
    return TRUE;
}

gboolean
_account_set_startscript(char *account_name, char *script)
{
    accounts_set_script_start(account_name, script);
    cons_show("Updated start script for account %s: %s", account_name, script);
    return TRUE;
}

gboolean
_account_set_theme(char *account_name, char *theme)
{
    if (!theme_exists(theme)) {
        cons_show("Theme does not exist: %s", theme);
        return TRUE;
    }

    accounts_set_theme(account_name, theme);
    if (connection_get_status() == JABBER_CONNECTED) {
        ProfAccount *account = accounts_get_account(session_get_account_name());
        if (account) {
            if (g_strcmp0(account->name, account_name) == 0) {
                theme_load(theme);
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
_account_set_tls(char *account_name, char *policy)
{
    if ((g_strcmp0(policy, "force") != 0)
            && (g_strcmp0(policy, "allow") != 0)
            && (g_strcmp0(policy, "disable") != 0)
            && (g_strcmp0(policy, "legacy") != 0)) {
        cons_show("TLS policy must be one of: force, allow, legacy or disable.");
    } else {
        accounts_set_tls_policy(account_name, policy);
        cons_show("Updated TLS policy for account %s: %s", account_name, policy);
        cons_show("");
    }
    return TRUE;
}

gboolean
_account_set_presence_priority(char *account_name, char *presence, char *priority)
{
    int intval;
    char *err_msg = NULL;
    gboolean res = strtoi_range(priority, &intval, -128, 127, &err_msg);
    if (!res) {
        cons_show(err_msg);
        free(err_msg);
        return TRUE;
    }

    resource_presence_t presence_type = resource_presence_from_string(presence);
    switch (presence_type)
    {
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
        char *connected_account = session_get_account_name();
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
cmd_account_set(ProfWin *window, const char *const command, gchar **args)
{
    if (g_strv_length(args) != 4) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    char *account_name = args[1];
    if (!accounts_account_exists(account_name)) {
        cons_show("Account %s doesn't exist", account_name);
        cons_show("");
        return TRUE;
    }

    char *property = args[2];
    char *value = args[3];
    if (strcmp(property, "jid") == 0)           return _account_set_jid(account_name, value);
    if (strcmp(property, "server") == 0)        return _account_set_server(account_name, value);
    if (strcmp(property, "port") == 0)          return _account_set_port(account_name, value);
    if (strcmp(property, "resource") == 0)      return _account_set_resource(account_name, value);
    if (strcmp(property, "password") == 0)      return _account_set_password(account_name, value);
    if (strcmp(property, "eval_password") == 0) return _account_set_eval_password(account_name, value);
    if (strcmp(property, "muc") == 0)           return _account_set_muc(account_name, value);
    if (strcmp(property, "nick") == 0)          return _account_set_nick(account_name, value);
    if (strcmp(property, "otr") == 0)           return _account_set_otr(account_name, value);
    if (strcmp(property, "status") == 0)        return _account_set_status(account_name, value);
    if (strcmp(property, "pgpkeyid") == 0)      return _account_set_pgpkeyid(account_name, value);
    if (strcmp(property, "startscript") == 0)   return _account_set_startscript(account_name, value);
    if (strcmp(property, "theme") == 0)         return _account_set_theme(account_name, value);
    if (strcmp(property, "tls") == 0)           return _account_set_tls(account_name, value);

    if (valid_resource_presence_string(property)) {
        return _account_set_presence_priority(account_name, property, value);
    }

    cons_show("Invalid property: %s", property);
    cons_show("");

    return TRUE;
}

gboolean
cmd_account_clear(ProfWin *window, const char *const command, gchar **args)
{
    if (g_strv_length(args) != 3) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    char *account_name = args[1];
    if (!accounts_account_exists(account_name)) {
        cons_show("Account %s doesn't exist", account_name);
        cons_show("");
        return TRUE;
    }

    char *property = args[2];
    if (strcmp(property, "password") == 0) {
        accounts_clear_password(account_name);
        cons_show("Removed password for account %s", account_name);
        cons_show("");
    } else if (strcmp(property, "eval_password") == 0) {
        accounts_clear_eval_password(account_name);
        cons_show("Removed eval password for account %s", account_name);
        cons_show("");
    } else if (strcmp(property, "server") == 0) {
        accounts_clear_server(account_name);
        cons_show("Removed server for account %s", account_name);
        cons_show("");
    } else if (strcmp(property, "port") == 0) {
        accounts_clear_port(account_name);
        cons_show("Removed port for account %s", account_name);
        cons_show("");
    } else if (strcmp(property, "otr") == 0) {
        accounts_clear_otr(account_name);
        cons_show("OTR policy removed for account %s", account_name);
        cons_show("");
    } else if (strcmp(property, "pgpkeyid") == 0) {
        accounts_clear_pgp_keyid(account_name);
        cons_show("Removed PGP key ID for account %s", account_name);
        cons_show("");
    } else if (strcmp(property, "startscript") == 0) {
        accounts_clear_script_start(account_name);
        cons_show("Removed start script for account %s", account_name);
        cons_show("");
    } else if (strcmp(property, "theme") == 0) {
        accounts_clear_theme(account_name);
        cons_show("Removed theme for account %s", account_name);
        cons_show("");
    } else if (strcmp(property, "muc") == 0) {
        accounts_clear_muc(account_name);
        cons_show("Removed MUC service for account %s", account_name);
        cons_show("");
    } else if (strcmp(property, "resource") == 0) {
        accounts_clear_resource(account_name);
        cons_show("Removed resource for account %s", account_name);
        cons_show("");
    } else {
        cons_show("Invalid property: %s", property);
        cons_show("");
    }

    return TRUE;
}

gboolean
cmd_account(ProfWin *window, const char *const command, gchar **args)
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

    ProfAccount *account = accounts_get_account(session_get_account_name());
    cons_show_account(account);
    account_free(account);

    return TRUE;
}

gboolean
cmd_script(ProfWin *window, const char *const command, gchar **args)
{
    if ((g_strcmp0(args[0], "run") == 0) && args[1]) {
        gboolean res = scripts_exec(args[1]);
        if (!res) {
            cons_show("Could not find script %s", args[1]);
        }
    } else if (g_strcmp0(args[0], "list") == 0) {
        GSList *scripts = scripts_list();
        cons_show_scripts(scripts);
        g_slist_free_full(scripts, g_free);
    } else if ((g_strcmp0(args[0], "show") == 0) && args[1]) {
        GSList *commands = scripts_read(args[1]);
        cons_show_script(args[1], commands);
        g_slist_free_full(commands, g_free);
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

/* escape a string into csv and write it to the file descriptor */
static int
_writecsv(int fd, const char *const str)
{
    if (!str) return 0;
    size_t len = strlen(str);
    char *s = malloc(2 * len * sizeof(char));
    char *c = s;
    int i = 0;
    for (; i < strlen(str); i++) {
        if (str[i] != '"') *c++ = str[i];
        else { *c++ = '"'; *c++ = '"'; len++; }
    }
    if (-1 == write(fd, s, len)) {
        cons_show("error: failed to write '%s' to the requested file: %s", s, strerror(errno));
        return -1;
    }
    free(s);
    return 0;
}

gboolean
cmd_export(ProfWin *window, const char *const command, gchar **args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        cons_show("");
        return TRUE;
    } else {
        GString *fname = g_string_new("");
        GSList *list = NULL;
        int fd;

        /* deal with the ~ convention for $HOME */
        if (args[0][0] == '~') {
            fname = g_string_append(fname, getenv("HOME"));
            fname = g_string_append(fname, args[0] + 1);
        } else {
            fname = g_string_append(fname, args[0]);
        }

        fd = open(fname->str, O_WRONLY | O_CREAT, 00600);
        g_string_free(fname, TRUE);

        if (-1 == fd) {
            cons_show("error: cannot open %s: %s", args[0], strerror(errno));
            cons_show("");
            return TRUE;
        }

        if (-1 == write(fd, "jid,name\n", strlen("jid,name\n"))) goto write_error;

        list = roster_get_contacts(ROSTER_ORD_NAME);
        if (list) {
            GSList *curr = list;
            while (curr){
                PContact contact = curr->data;
                const char *jid = p_contact_barejid(contact);
                const char  *name = p_contact_name(contact);

                /* write the data to the file */
                if (-1 == write(fd, "\"", 1)) goto write_error;
                if (-1 == _writecsv(fd, jid)) goto write_error;
                if (-1 == write(fd, "\",\"", 3)) goto write_error;
                if (-1 == _writecsv(fd, name)) goto write_error;
                if (-1 == write(fd, "\"\n", 2)) goto write_error;

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
cmd_sub(ProfWin *window, const char *const command, gchar **args)
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
        ProfChatWin *chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        jid = chatwin->barejid;
    }

    Jid *jidp = jid_create(jid);

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
                win_println(window, THEME_DEFAULT, '-', "No subscription information for %s.", jidp->barejid);
            } else {
                cons_show("No subscription information for %s.", jidp->barejid);
            }
        } else {
            if (window->type == WIN_CHAT) {
                if (p_contact_pending_out(contact)) {
                    win_println(window, THEME_DEFAULT, '-', "%s subscription status: %s, request pending.",
                        jidp->barejid, p_contact_subscription(contact));
                } else {
                    win_println(window, THEME_DEFAULT, '-', "%s subscription status: %s.", jidp->barejid,
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

    jid_destroy(jidp);

    return TRUE;
}

gboolean
cmd_disconnect(ProfWin *window, const char *const command, gchar **args)
{
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    cl_ev_disconnect();

    char *theme = prefs_get_string(PREF_THEME);
    if (theme) {
        gboolean res = theme_load(theme);
        prefs_free_string(theme);
        if (!res) {
            theme_load("default");
        }
    } else {
        theme_load("default");
    }
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

    return TRUE;
}

gboolean
cmd_quit(ProfWin *window, const char *const command, gchar **args)
{
    log_info("Profanity is shutting down...");
    exit(0);
    return FALSE;
}

gboolean
cmd_wins_unread(ProfWin *window, const char *const command, gchar **args)
{
    cons_show_wins(TRUE);
    return TRUE;
}

gboolean
cmd_wins_tidy(ProfWin *window, const char *const command, gchar **args)
{
    if (wins_tidy()) {
        cons_show("Windows tidied.");
    } else {
        cons_show("No tidy needed.");
    }
    return TRUE;
}

gboolean
cmd_wins_prune(ProfWin *window, const char *const command, gchar **args)
{
    ui_prune_wins();
    return TRUE;
}

gboolean
cmd_wins_swap(ProfWin *window, const char *const command, gchar **args)
{
    if ((args[1] == NULL) || (args[2] == NULL)) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    int source_win = atoi(args[1]);
    int target_win = atoi(args[2]);
    if ((source_win == 1) || (target_win == 1)) {
        cons_show("Cannot move console window.");
    } else if (source_win == 10 || target_win == 10) {
        cons_show("Window 10 does not exist");
    } else if (source_win != target_win) {
        gboolean swapped = wins_swap(source_win, target_win);
        if (swapped) {
            cons_show("Swapped windows %d <-> %d", source_win, target_win);
        } else {
            cons_show("Window %d does not exist", source_win);
        }
    } else {
        cons_show("Same source and target window supplied.");
    }

    return TRUE;
}

gboolean
cmd_wins_autotidy(ProfWin *window, const char *const command, gchar **args)
{
    if (g_strcmp0(args[1], "on") == 0) {
        cons_show("Window autotidy enabled");
        prefs_set_boolean(PREF_WINS_AUTO_TIDY, TRUE);
        wins_tidy();
    } else if (g_strcmp0(args[1], "off") == 0) {
        cons_show("Window autotidy disabled");
        prefs_set_boolean(PREF_WINS_AUTO_TIDY, FALSE);
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_wins(ProfWin *window, const char *const command, gchar **args)
{
    if (args[0] != NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    cons_show_wins(FALSE);
    return TRUE;
}

gboolean
cmd_close(ProfWin *window, const char *const command, gchar **args)
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
        int i = 0;
        for (i = 0; i < strlen(args[0]); i++) {
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

        ProfWin *toclose = wins_get_by_num(index);
        if (!toclose) {
            cons_show("Window is not open.");
            return TRUE;
        }

        // check for unsaved form
        if (ui_win_has_unsaved_form(index)) {
            win_println(window, THEME_DEFAULT, '-', "You have unsaved changes, use /form submit or /form cancel");
            return TRUE;
        }

        // handle leaving rooms, or chat
        if (conn_status == JABBER_CONNECTED) {
            ui_close_connected_win(index);
        }

        // close the window
        ui_close_win(index);
        cons_show("Closed window %d", index);

        // Tidy up the window list.
        if (prefs_get_boolean(PREF_WINS_AUTO_TIDY)) {
            wins_tidy();
        }

        rosterwin_roster();
        return TRUE;
    } else {
        if (g_strcmp0(args[0], "console") == 0) {
            cons_show("Cannot close console window.");
            return TRUE;
        }

        ProfWin *toclose = wins_get_by_string(args[0]);
        if (!toclose) {
            cons_show("Window \"%s\" does not exist.", args[0]);
            return TRUE;
        }
        index = wins_get_num(toclose);

        // check for unsaved form
        if (ui_win_has_unsaved_form(index)) {
            win_println(window, THEME_DEFAULT, '-', "You have unsaved changes, use /form submit or /form cancel");
            return TRUE;
        }

        // handle leaving rooms, or chat
        if (conn_status == JABBER_CONNECTED) {
            ui_close_connected_win(index);
        }

        // close the window
        ui_close_win(index);
        cons_show("Closed window %s", args[0]);

        // Tidy up the window list.
        if (prefs_get_boolean(PREF_WINS_AUTO_TIDY)) {
            wins_tidy();
        }

        rosterwin_roster();
        return TRUE;
    }
}

gboolean
cmd_win(ProfWin *window, const char *const command, gchar **args)
{
    gboolean is_num = TRUE;
    int i = 0;
    for (i = 0; i < strlen(args[0]); i++) {
        if (!isdigit((int)args[0][i])) {
            is_num = FALSE;
            break;
        }
    }

    if (is_num) {
        int num = atoi(args[0]);

        ProfWin *focuswin = wins_get_by_num(num);
        if (!focuswin) {
            cons_show("Window %d does not exist.", num);
        } else {
            ui_focus_win(focuswin);
        }
    } else {
        ProfWin *focuswin = wins_get_by_string(args[0]);
        if (!focuswin) {
            cons_show("Window \"%s\" does not exist.", args[0]);
        } else {
            ui_focus_win(focuswin);
        }
    }

    return TRUE;
}

static void
_cmd_list_commands(GList *commands) {
    int maxlen = 0;
    GList *curr = commands;
    while (curr) {
        gchar *cmd = curr->data;
        int len = strlen(cmd);
        if (len > maxlen) maxlen = len;
        curr = g_list_next(curr);
    }

    GString *cmds = g_string_new("");
    curr = commands;
    int count = 0;
    while (curr) {
        gchar *cmd = curr->data;
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
_cmd_help_cmd_list(const char *const tag)
{
    cons_show("");
    ProfWin *console = wins_get_console();
    if (tag) {
        win_println(console, THEME_HELP_HEADER, '-', "%s commands", tag);
    } else {
        win_println(console, THEME_HELP_HEADER, '-', "All commands");
    }

    GList *ordered_commands = NULL;

    if (g_strcmp0(tag, "plugins") == 0) {
        GList *plugins_cmds = plugins_get_command_names();
        GList *curr = plugins_cmds;
        while (curr) {
            ordered_commands = g_list_insert_sorted(ordered_commands, curr->data, (GCompareFunc)g_strcmp0);
            curr = g_list_next(curr);
        }
        g_list_free(plugins_cmds);
    } else {
        ordered_commands = cmd_get_ordered(tag);

        // add plugins if showing all commands
        if (!tag) {
            GList *plugins_cmds = plugins_get_command_names();
            GList *curr = plugins_cmds;
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
cmd_help(ProfWin *window, const char *const command, gchar **args)
{
    int num_args = g_strv_length(args);
    if (num_args == 0) {
        cons_help();
    } else if (strcmp(args[0], "search_all") == 0) {
        if (args[1] == NULL) {
            cons_bad_cmd_usage(command);
        } else {
            GList *cmds = cmd_search_index_all(args[1]);
            if (cmds == NULL) {
                cons_show("No commands found.");
            } else {
                GList *curr = cmds;
                GList *results = NULL;
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
            GList *cmds = cmd_search_index_any(args[1]);
            if (cmds == NULL) {
                cons_show("No commands found.");
            } else {
                GList *curr = cmds;
                GList *results = NULL;
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
        char *cmd = args[0];
        char cmd_with_slash[1 + strlen(cmd) + 1];
        sprintf(cmd_with_slash, "/%s", cmd);

        Command *command = cmd_get(cmd_with_slash);
        if (command) {
            cons_show_help(cmd_with_slash, &command->help);
        } else {
            CommandHelp *commandHelp = plugins_get_help(cmd_with_slash);
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
cmd_about(ProfWin *window, const char *const command, gchar **args)
{
    cons_show("");
    cons_about();
    return TRUE;
}

gboolean
cmd_prefs(ProfWin *window, const char *const command, gchar **args)
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
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_theme(ProfWin *window, const char *const command, gchar **args)
{
    // list themes
    if (g_strcmp0(args[0], "list") == 0) {
        GSList *themes = theme_list();
        cons_show_themes(themes);
        g_slist_free_full(themes, g_free);

    // load a theme
    } else if (g_strcmp0(args[0], "load") == 0) {
        if (args[1] == NULL) {
            cons_bad_cmd_usage(command);
        } else if (theme_load(args[1])) {
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
_who_room(ProfWin *window, const char *const command, gchar **args)
{
    if ((g_strv_length(args) == 2) && args[1]) {
        cons_show("Argument group is not applicable to chat rooms.");
        return;
    }

    // bad arg
    if (args[0] &&
            (g_strcmp0(args[0], "online") != 0) &&
            (g_strcmp0(args[0], "available") != 0) &&
            (g_strcmp0(args[0], "unavailable") != 0) &&
            (g_strcmp0(args[0], "away") != 0) &&
            (g_strcmp0(args[0], "chat") != 0) &&
            (g_strcmp0(args[0], "xa") != 0) &&
            (g_strcmp0(args[0], "dnd") != 0) &&
            (g_strcmp0(args[0], "any") != 0) &&
            (g_strcmp0(args[0], "moderator") != 0) &&
            (g_strcmp0(args[0], "participant") != 0) &&
            (g_strcmp0(args[0], "visitor") != 0) &&
            (g_strcmp0(args[0], "owner") != 0) &&
            (g_strcmp0(args[0], "admin") != 0) &&
            (g_strcmp0(args[0], "member") != 0) &&
            (g_strcmp0(args[0], "outcast") != 0)) {
        cons_bad_cmd_usage(command);
        return;
    }

    ProfMucWin *mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    // presence filter
    if (args[0] == NULL ||
            (g_strcmp0(args[0], "online") == 0) ||
            (g_strcmp0(args[0], "available") == 0) ||
            (g_strcmp0(args[0], "unavailable") == 0) ||
            (g_strcmp0(args[0], "away") == 0) ||
            (g_strcmp0(args[0], "chat") == 0) ||
            (g_strcmp0(args[0], "xa") == 0) ||
            (g_strcmp0(args[0], "dnd") == 0) ||
            (g_strcmp0(args[0], "any") == 0)) {

        char *presence = args[0];
        GList *occupants = muc_roster(mucwin->roomjid);

        // no arg, show all contacts
        if ((presence == NULL) || (g_strcmp0(presence, "any") == 0)) {
            mucwin_roster(mucwin, occupants, NULL);

        // available
        } else if (strcmp("available", presence) == 0) {
            GList *filtered = NULL;

            while (occupants) {
                Occupant *occupant = occupants->data;
                if (muc_occupant_available(occupant)) {
                    filtered = g_list_append(filtered, occupant);
                }
                occupants = g_list_next(occupants);
            }

            mucwin_roster(mucwin, filtered, "available");

        // unavailable
        } else if (strcmp("unavailable", presence) == 0) {
            GList *filtered = NULL;

            while (occupants) {
                Occupant *occupant = occupants->data;
                if (!muc_occupant_available(occupant)) {
                    filtered = g_list_append(filtered, occupant);
                }
                occupants = g_list_next(occupants);
            }

            mucwin_roster(mucwin, filtered, "unavailable");

        // show specific status
        } else {
            GList *filtered = NULL;

            while (occupants) {
                Occupant *occupant = occupants->data;
                const char *presence_str = string_from_resource_presence(occupant->presence);
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
    }
}

static void
_who_roster(ProfWin *window, const char *const command, gchar **args)
{
    char *presence = args[0];

    // bad arg
    if (presence
            && (strcmp(presence, "online") != 0)
            && (strcmp(presence, "available") != 0)
            && (strcmp(presence, "unavailable") != 0)
            && (strcmp(presence, "offline") != 0)
            && (strcmp(presence, "away") != 0)
            && (strcmp(presence, "chat") != 0)
            && (strcmp(presence, "xa") != 0)
            && (strcmp(presence, "dnd") != 0)
            && (strcmp(presence, "any") != 0)) {
        cons_bad_cmd_usage(command);
        return;
    }

    char *group = NULL;
    if ((g_strv_length(args) == 2) && args[1]) {
        group = args[1];
    }

    cons_show("");
    GSList *list = NULL;
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
        GSList *filtered = NULL;

        GSList *curr = list;
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
        GSList *filtered = NULL;

        GSList *curr = list;
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
        GSList *filtered = NULL;

        GSList *curr = list;
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
        GSList *filtered = NULL;

        GSList *curr = list;
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
        GSList *filtered = NULL;

        GSList *curr = list;
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
cmd_who(ProfWin *window, const char *const command, gchar **args)
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
        status_bar_new(1);
    }

    return TRUE;
}

gboolean
cmd_msg(ProfWin *window, const char *const command, gchar **args)
{
    char *usr = args[0];
    char *msg = args[1];

    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    // send private message when in MUC room
    if (window->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        if (muc_roster_contains_nick(mucwin->roomjid, usr)) {
            GString *full_jid = g_string_new(mucwin->roomjid);
            g_string_append(full_jid, "/");
            g_string_append(full_jid, usr);

            ProfPrivateWin *privwin = wins_get_private(full_jid->str);
            if (!privwin) {
                privwin = (ProfPrivateWin*)wins_new_private(full_jid->str);
            }
            ui_focus_win((ProfWin*)privwin);

            if (msg) {
                cl_ev_send_priv_msg(privwin, msg, NULL);
            }

            g_string_free(full_jid, TRUE);

        } else {
            win_println(window, THEME_DEFAULT, '-', "No such participant \"%s\" in room.", usr);
        }

        return TRUE;

    // send chat message
    } else {
        char *barejid = roster_barejid_from_name(usr);
        if (barejid == NULL) {
            barejid = usr;
        }

        ProfChatWin *chatwin = wins_get_chat(barejid);
        if (!chatwin) {
            chatwin = chatwin_new(barejid);
        }
        ui_focus_win((ProfWin*)chatwin);

        if (msg) {
            cl_ev_send_msg(chatwin, msg, NULL);
        } else {
#ifdef HAVE_LIBOTR
            if (otr_is_secure(barejid)) {
                chatwin_otr_secured(chatwin, otr_is_trusted(barejid));
            }
#endif
        }

        return TRUE;
    }
}

gboolean
cmd_group(ProfWin *window, const char *const command, gchar **args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    // list all groups
    if (args[0] == NULL) {
        GList *groups = roster_get_groups();
        GList *curr = groups;
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
    if (strcmp(args[0], "show") == 0) {
        char *group = args[1];
        if (group == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        GSList *list = roster_get_group(group, ROSTER_ORD_NAME);
        cons_show_roster_group(group, list);
        return TRUE;
    }

    // add contact to group
    if (strcmp(args[0], "add") == 0) {
        char *group = args[1];
        char *contact = args[2];

        if ((group == NULL) || (contact == NULL)) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        char *barejid = roster_barejid_from_name(contact);
        if (barejid == NULL) {
            barejid = contact;
        }

        PContact pcontact = roster_get_contact(barejid);
        if (pcontact == NULL) {
            cons_show("Contact not found in roster: %s", barejid);
            return TRUE;
        }

        if (p_contact_in_group(pcontact, group)) {
            const char *display_name = p_contact_name_or_jid(pcontact);
            ui_contact_already_in_group(display_name, group);
        } else {
            roster_send_add_to_group(group, pcontact);
        }

        return TRUE;
    }

    // remove contact from group
    if (strcmp(args[0], "remove") == 0) {
        char *group = args[1];
        char *contact = args[2];

        if ((group == NULL) || (contact == NULL)) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        char *barejid = roster_barejid_from_name(contact);
        if (barejid == NULL) {
            barejid = contact;
        }

        PContact pcontact = roster_get_contact(barejid);
        if (pcontact == NULL) {
            cons_show("Contact not found in roster: %s", barejid);
            return TRUE;
        }

        if (!p_contact_in_group(pcontact, group)) {
            const char *display_name = p_contact_name_or_jid(pcontact);
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
cmd_roster(ProfWin *window, const char *const command, gchar **args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    // show roster
    if (args[0] == NULL) {
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }

        GSList *list = roster_get_contacts(ROSTER_ORD_NAME);
        cons_show_roster(list);
        g_slist_free(list);
        return TRUE;

    // show roster, only online contacts
    } else if(g_strcmp0(args[0], "online") == 0){
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }

        GSList *list = roster_get_contacts_online();
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
        char *err_msg = NULL;
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
            free(err_msg);
            return TRUE;
        }

    // set line wrapping
    } else if (g_strcmp0(args[0], "wrap") == 0) {
        if (!args[1]) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            _cmd_set_boolean_preference(args[1], command, "Roster panel line wrap", PREF_ROSTER_WRAP);
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
                prefs_set_roster_header_char(args[2][0]);
                cons_show("Roster header char set to %c.", args[2][0]);
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
                prefs_set_roster_contact_char(args[2][0]);
                cons_show("Roster contact char set to %c.", args[2][0]);
                rosterwin_roster();
            }
        } else if (g_strcmp0(args[1], "indent") == 0) {
            if (!args[2]) {
                cons_bad_cmd_usage(command);
            } else {
                int intval = 0;
                char *err_msg = NULL;
                gboolean res = strtoi_range(args[2], &intval, 0, 10, &err_msg);
                if (res) {
                    prefs_set_roster_contact_indent(intval);
                    cons_show("Roster contact indent set to: %d", intval);
                    rosterwin_roster();
                } else {
                    cons_show(err_msg);
                    free(err_msg);
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
                prefs_set_roster_resource_char(args[2][0]);
                cons_show("Roster resource char set to %c.", args[2][0]);
                rosterwin_roster();
            }
        } else if (g_strcmp0(args[1], "indent") == 0) {
            if (!args[2]) {
                cons_bad_cmd_usage(command);
            } else {
                int intval = 0;
                char *err_msg = NULL;
                gboolean res = strtoi_range(args[2], &intval, 0, 10, &err_msg);
                if (res) {
                    prefs_set_roster_resource_indent(intval);
                    cons_show("Roster resource indent set to: %d", intval);
                    rosterwin_roster();
                } else {
                    cons_show(err_msg);
                    free(err_msg);
                }
            }
        } else if (g_strcmp0(args[1], "join") == 0) {
            _cmd_set_boolean_preference(args[2], command, "Roster join", PREF_ROSTER_RESOURCE_JOIN);
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
                char *err_msg = NULL;
                gboolean res = strtoi_range(args[2], &intval, -1, 10, &err_msg);
                if (res) {
                    prefs_set_roster_presence_indent(intval);
                    cons_show("Roster presence indent set to: %d", intval);
                    rosterwin_roster();
                } else {
                    cons_show(err_msg);
                    free(err_msg);
                }
            }
        } else {
            cons_bad_cmd_usage(command);
        }
        return TRUE;

    // show/hide roster
    } else if (g_strcmp0(args[0], "show") == 0) {
        if (args[1] == NULL) {
            cons_show("Roster enabled.");
            prefs_set_boolean(PREF_ROSTER, TRUE);
            if (conn_status == JABBER_CONNECTED) {
                ui_show_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "offline") == 0) {
            cons_show("Roster offline enabled");
            prefs_set_boolean(PREF_ROSTER_OFFLINE, TRUE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "resource") == 0) {
            cons_show("Roster resource enabled");
            prefs_set_boolean(PREF_ROSTER_RESOURCE, TRUE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "presence") == 0) {
            cons_show("Roster presence enabled");
            prefs_set_boolean(PREF_ROSTER_PRESENCE, TRUE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "status") == 0) {
            cons_show("Roster status enabled");
            prefs_set_boolean(PREF_ROSTER_STATUS, TRUE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "empty") == 0) {
            cons_show("Roster empty enabled");
            prefs_set_boolean(PREF_ROSTER_EMPTY, TRUE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "priority") == 0) {
            cons_show("Roster priority enabled");
            prefs_set_boolean(PREF_ROSTER_PRIORITY, TRUE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "contacts") == 0) {
            cons_show("Roster contacts enabled");
            prefs_set_boolean(PREF_ROSTER_CONTACTS, TRUE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "rooms") == 0) {
            cons_show("Roster rooms enabled");
            prefs_set_boolean(PREF_ROSTER_ROOMS, TRUE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "unsubscribed") == 0) {
            cons_show("Roster unsubscribed enabled");
            prefs_set_boolean(PREF_ROSTER_UNSUBSCRIBED, TRUE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    } else if (g_strcmp0(args[0], "hide") == 0) {
        if (args[1] == NULL) {
            cons_show("Roster disabled.");
            prefs_set_boolean(PREF_ROSTER, FALSE);
            if (conn_status == JABBER_CONNECTED) {
                ui_hide_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "offline") == 0) {
            cons_show("Roster offline disabled");
            prefs_set_boolean(PREF_ROSTER_OFFLINE, FALSE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "resource") == 0) {
            cons_show("Roster resource disabled");
            prefs_set_boolean(PREF_ROSTER_RESOURCE, FALSE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "presence") == 0) {
            cons_show("Roster presence disabled");
            prefs_set_boolean(PREF_ROSTER_PRESENCE, FALSE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "status") == 0) {
            cons_show("Roster status disabled");
            prefs_set_boolean(PREF_ROSTER_STATUS, FALSE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "empty") == 0) {
            cons_show("Roster empty disabled");
            prefs_set_boolean(PREF_ROSTER_EMPTY, FALSE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "priority") == 0) {
            cons_show("Roster priority disabled");
            prefs_set_boolean(PREF_ROSTER_PRIORITY, FALSE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "contacts") == 0) {
            cons_show("Roster contacts disabled");
            prefs_set_boolean(PREF_ROSTER_CONTACTS, FALSE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "rooms") == 0) {
            cons_show("Roster rooms disabled");
            prefs_set_boolean(PREF_ROSTER_ROOMS, FALSE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "unsubscribed") == 0) {
            cons_show("Roster unsubscribed disabled");
            prefs_set_boolean(PREF_ROSTER_UNSUBSCRIBED, FALSE);
            if (conn_status == JABBER_CONNECTED) {
                rosterwin_roster();
            }
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

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
            _cmd_set_boolean_preference(args[2], command, "Roster header zero count", PREF_ROSTER_COUNT_ZERO);
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
                prefs_set_roster_private_char(args[2][0]);
                cons_show("Roster private room chat char set to %c.", args[2][0]);
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
                prefs_set_roster_room_char(args[2][0]);
                cons_show("Roster room char set to %c.", args[2][0]);
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
                    prefs_set_roster_room_private_char(args[3][0]);
                    cons_show("Roster room private char set to %c.", args[3][0]);
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
        char *jid = args[1];
        if (jid == NULL) {
            cons_bad_cmd_usage(command);
        } else {
            char *name = args[2];
            roster_send_add_new(jid, name);
        }
        return TRUE;

    // remove contact
    } else if (strcmp(args[0], "remove") == 0) {
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }
        char *jid = args[1];
        if (jid == NULL) {
            cons_bad_cmd_usage(command);
        } else {
            roster_send_remove(jid);
        }
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

        GSList *all = roster_get_contacts(ROSTER_ORD_NAME);
        GSList *curr = all;
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
        char *jid = args[1];
        if (jid == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        char *name = args[2];
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

        const char *barejid = p_contact_barejid(contact);

        // TODO wait for result stanza before updating
        const char *oldnick = p_contact_name(contact);
        wins_change_nick(barejid, oldnick, name);
        roster_change_name(contact, name);
        GSList *groups = p_contact_groups(contact);
        roster_send_name_change(barejid, name, groups);


        cons_show("Nickname for %s set to: %s.", jid, name);

        return TRUE;

    // remove nickname
    } else if (strcmp(args[0], "clearnick") == 0) {
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }
        char *jid = args[1];
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

        const char *barejid = p_contact_barejid(contact);

        // TODO wait for result stanza before updating
        const char *oldnick = p_contact_name(contact);
        wins_remove_nick(barejid, oldnick);
        roster_change_name(contact, NULL);
        GSList *groups = p_contact_groups(contact);
        roster_send_name_change(barejid, NULL, groups);

        cons_show("Nickname for %s removed.", jid);

        return TRUE;
    } else {
        cons_bad_cmd_usage(command);
        return TRUE;
    }
}

gboolean
cmd_blocked(ProfWin *window, const char *const command, gchar **args)
{
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (!connection_supports(XMPP_FEATURE_BLOCKING)) {
        cons_show("Blocking not supported by server.");
        return TRUE;
    }

    if (g_strcmp0(args[0], "add") == 0) {
        char *jid = args[1];
        if (jid == NULL && (window->type == WIN_CHAT)) {
            ProfChatWin *chatwin = (ProfChatWin*)window;
            jid = chatwin->barejid;
        }

        if (jid == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        gboolean res = blocked_add(jid);
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

    GList *blocked = blocked_list();
    GList *curr = blocked;
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
cmd_resource(ProfWin *window, const char *const command, gchar **args)
{
    char *cmd = args[0];
    char *setting = NULL;
    if (g_strcmp0(cmd, "message") == 0) {
        setting = args[1];
        if (!setting) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            _cmd_set_boolean_preference(setting, command, "Message resource", PREF_RESOURCE_MESSAGE);
            return TRUE;
        }
    } else if (g_strcmp0(cmd, "title") == 0) {
        setting = args[1];
        if (!setting) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            _cmd_set_boolean_preference(setting, command, "Title resource", PREF_RESOURCE_TITLE);
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

    ProfChatWin *chatwin = (ProfChatWin*)window;

    if (g_strcmp0(cmd, "set") == 0) {
        char *resource = args[1];
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

gboolean
cmd_status(ProfWin *window, const char *const command, gchar **args)
{
    char *usr = args[0];

    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    switch (window->type)
    {
        case WIN_MUC:
            if (usr) {
                ProfMucWin *mucwin = (ProfMucWin*)window;
                assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
                Occupant *occupant = muc_roster_item(mucwin->roomjid, usr);
                if (occupant) {
                    win_show_occupant(window, occupant);
                } else {
                    win_println(window, THEME_DEFAULT, '-', "No such participant \"%s\" in room.", usr);
                }
            } else {
                win_println(window, THEME_DEFAULT, '-', "You must specify a nickname.");
            }
            break;
        case WIN_CHAT:
            if (usr) {
                win_println(window, THEME_DEFAULT, '-', "No parameter required when in chat.");
            } else {
                ProfChatWin *chatwin = (ProfChatWin*)window;
                assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
                PContact pcontact = roster_get_contact(chatwin->barejid);
                if (pcontact) {
                    win_show_contact(window, pcontact);
                } else {
                    win_println(window, THEME_DEFAULT, '-', "Error getting contact info.");
                }
            }
            break;
        case WIN_PRIVATE:
            if (usr) {
                win_println(window, THEME_DEFAULT, '-', "No parameter required when in chat.");
            } else {
                ProfPrivateWin *privatewin = (ProfPrivateWin*)window;
                assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
                Jid *jid = jid_create(privatewin->fulljid);
                Occupant *occupant = muc_roster_item(jid->barejid, jid->resourcepart);
                if (occupant) {
                    win_show_occupant(window, occupant);
                } else {
                    win_println(window, THEME_DEFAULT, '-', "Error getting contact info.");
                }
                jid_destroy(jid);
            }
            break;
        case WIN_CONSOLE:
            if (usr) {
                char *usr_jid = roster_barejid_from_name(usr);
                if (usr_jid == NULL) {
                    usr_jid = usr;
                }
                cons_show_status(usr_jid);
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
cmd_info(ProfWin *window, const char *const command, gchar **args)
{
    char *usr = args[0];

    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    switch (window->type)
    {
        case WIN_MUC:
            if (usr) {
                ProfMucWin *mucwin = (ProfMucWin*)window;
                assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
                Occupant *occupant = muc_roster_item(mucwin->roomjid, usr);
                if (occupant) {
                    win_show_occupant_info(window, mucwin->roomjid, occupant);
                } else {
                    win_println(window, THEME_DEFAULT, '-', "No such occupant \"%s\" in room.", usr);
                }
            } else {
                ProfMucWin *mucwin = (ProfMucWin*)window;
                assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
                iq_room_info_request(mucwin->roomjid, TRUE);
                mucwin_info(mucwin);
                return TRUE;
            }
            break;
        case WIN_CHAT:
            if (usr) {
                win_println(window, THEME_DEFAULT, '-', "No parameter required when in chat.");
            } else {
                ProfChatWin *chatwin = (ProfChatWin*)window;
                assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
                PContact pcontact = roster_get_contact(chatwin->barejid);
                if (pcontact) {
                    win_show_info(window, pcontact);
                } else {
                    win_println(window, THEME_DEFAULT, '-', "Error getting contact info.");
                }
            }
            break;
        case WIN_PRIVATE:
            if (usr) {
                win_println(window, THEME_DEFAULT, '-', "No parameter required when in chat.");
            } else {
                ProfPrivateWin *privatewin = (ProfPrivateWin*)window;
                assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
                Jid *jid = jid_create(privatewin->fulljid);
                Occupant *occupant = muc_roster_item(jid->barejid, jid->resourcepart);
                if (occupant) {
                    win_show_occupant_info(window, jid->barejid, occupant);
                } else {
                    win_println(window, THEME_DEFAULT, '-', "Error getting contact info.");
                }
                jid_destroy(jid);
            }
            break;
        case WIN_CONSOLE:
            if (usr) {
                char *usr_jid = roster_barejid_from_name(usr);
                if (usr_jid == NULL) {
                    usr_jid = usr;
                }
                PContact pcontact = roster_get_contact(usr_jid);
                if (pcontact) {
                    cons_show_info(pcontact);
                } else {
                    cons_show("No such contact \"%s\" in roster.", usr);
                }
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
cmd_caps(ProfWin *window, const char *const command, gchar **args)
{
    jabber_conn_status_t conn_status = connection_get_status();
    Occupant *occupant = NULL;

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    switch (window->type)
    {
        case WIN_MUC:
            if (args[0]) {
                ProfMucWin *mucwin = (ProfMucWin*)window;
                assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
                occupant = muc_roster_item(mucwin->roomjid, args[0]);
                if (occupant) {
                    Jid *jidp = jid_create_from_bare_and_resource(mucwin->roomjid, args[0]);
                    cons_show_caps(jidp->fulljid, occupant->presence);
                    jid_destroy(jidp);
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
                Jid *jid = jid_create(args[0]);

                if (jid->fulljid == NULL) {
                    cons_show("You must provide a full jid to the /caps command.");
                } else {
                    PContact pcontact = roster_get_contact(jid->barejid);
                    if (pcontact == NULL) {
                        cons_show("Contact not found in roster: %s", jid->barejid);
                    } else {
                        Resource *resource = p_contact_get_resource(pcontact, jid->resourcepart);
                        if (resource == NULL) {
                            cons_show("Could not find resource %s, for contact %s", jid->barejid, jid->resourcepart);
                        } else {
                            cons_show_caps(jid->fulljid, resource->presence);
                        }
                    }
                }
                jid_destroy(jid);
            } else {
                cons_show("You must provide a jid to the /caps command.");
            }
            break;
        case WIN_PRIVATE:
            if (args[0]) {
                cons_show("No parameter needed to /caps when in private chat.");
            } else {
                ProfPrivateWin *privatewin = (ProfPrivateWin*)window;
                assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
                Jid *jid = jid_create(privatewin->fulljid);
                if (jid) {
                    occupant = muc_roster_item(jid->barejid, jid->resourcepart);
                    cons_show_caps(jid->resourcepart, occupant->presence);
                    jid_destroy(jid);
                }
            }
            break;
        default:
            break;
    }

    return TRUE;
}


gboolean
cmd_software(ProfWin *window, const char *const command, gchar **args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    switch (window->type)
    {
        case WIN_MUC:
            if (args[0]) {
                ProfMucWin *mucwin = (ProfMucWin*)window;
                assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
                Occupant *occupant = muc_roster_item(mucwin->roomjid, args[0]);
                if (occupant) {
                    Jid *jid = jid_create_from_bare_and_resource(mucwin->roomjid, args[0]);
                    iq_send_software_version(jid->fulljid);
                    jid_destroy(jid);
                } else {
                    cons_show("No such participant \"%s\" in room.", args[0]);
                }
            } else {
                cons_show("No nickname supplied to /software in chat room.");
            }
            break;
        case WIN_CHAT:
            if (args[0]) {
                cons_show("No parameter needed to /software when in chat.");
            } else {
                ProfChatWin *chatwin = (ProfChatWin*)window;
                assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);

                char *resource = NULL;
                ChatSession *session = chat_session_get(chatwin->barejid);
                if (chatwin->resource_override) {
                    resource = chatwin->resource_override;
                } else if (session && session->resource) {
                    resource = session->resource;
                }

                if (resource) {
                    GString *fulljid = g_string_new(chatwin->barejid);
                    g_string_append_printf(fulljid, "/%s", resource);
                    iq_send_software_version(fulljid->str);
                    g_string_free(fulljid, TRUE);
                } else {
                    win_println(window, THEME_DEFAULT, '-', "Unknown resource for /software command.");
                }
            }
            break;
        case WIN_CONSOLE:
            if (args[0]) {
                Jid *myJid = jid_create(connection_get_fulljid());
                Jid *jid = jid_create(args[0]);

                if (jid == NULL || jid->fulljid == NULL) {
                    cons_show("You must provide a full jid to the /software command.");
                } else if (g_strcmp0(jid->barejid, myJid->barejid) == 0) {
                    cons_show("Cannot request software version for yourself.");
                } else {
                    iq_send_software_version(jid->fulljid);
                }
                jid_destroy(myJid);
                jid_destroy(jid);
            } else {
                cons_show("You must provide a jid to the /software command.");
            }
            break;
        case WIN_PRIVATE:
            if (args[0]) {
                cons_show("No parameter needed to /software when in private chat.");
            } else {
                ProfPrivateWin *privatewin = (ProfPrivateWin*)window;
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
cmd_join(ProfWin *window, const char *const command, gchar **args)
{
    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (args[0] == NULL) {
        char *account_name = session_get_account_name();
        ProfAccount *account = accounts_get_account(account_name);
        if (account->muc_service) {
            GString *room_str = g_string_new("");
            char *uuid = connection_create_uuid();
            g_string_append_printf(room_str, "private-chat-%s@%s", uuid, account->muc_service);
            connection_free_uuid(uuid);

            presence_join_room(room_str->str, account->muc_nick, NULL);
            muc_join(room_str->str, account->muc_nick, NULL, FALSE);

            g_string_free(room_str, TRUE);
            account_free(account);
        } else {
            cons_show("Account MUC service property not found.");
        }

        return TRUE;
    }

    Jid *room_arg = jid_create(args[0]);
    if (room_arg == NULL) {
        cons_show_error("Specified room has incorrect format.");
        cons_show("");
        return TRUE;
    }

    char *room = NULL;
    char *nick = NULL;
    char *passwd = NULL;
    char *account_name = session_get_account_name();
    ProfAccount *account = accounts_get_account(account_name);

    // full room jid supplied (room@server)
    if (room_arg->localpart) {
        room = args[0];

    // server not supplied (room), use account preference
    } else if (account->muc_service) {
        GString *room_str = g_string_new("");
        g_string_append(room_str, args[0]);
        g_string_append(room_str, "@");
        g_string_append(room_str, account->muc_service);
        room = room_str->str;
        g_string_free(room_str, FALSE);

    // no account preference
    } else {
        cons_show("Account MUC service property not found.");
        return TRUE;
    }

    // Additional args supplied
    gchar *opt_keys[] = { "nick", "password", NULL };
    gboolean parsed;

    GHashTable *options = parse_options(&args[1], opt_keys, &parsed);
    if (!parsed) {
        cons_bad_cmd_usage(command);
        cons_show("");
        jid_destroy(room_arg);
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
    } else if (muc_roster_complete(room)) {
        ui_switch_to_room(room);
    }

    jid_destroy(room_arg);
    account_free(account);

    return TRUE;
}

gboolean
cmd_invite(ProfWin *window, const char *const command, gchar **args)
{
    char *contact = args[0];
    char *reason = args[1];
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (window->type != WIN_MUC) {
        cons_show("You must be in a chat room to send an invite.");
        return TRUE;
    }

    char *usr_jid = roster_barejid_from_name(contact);
    if (usr_jid == NULL) {
        usr_jid = contact;
    }

    ProfMucWin *mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
    message_send_invite(mucwin->roomjid, usr_jid, reason);
    if (reason) {
        cons_show("Room invite sent, contact: %s, room: %s, reason: \"%s\".",
            contact, mucwin->roomjid, reason);
    } else {
        cons_show("Room invite sent, contact: %s, room: %s.",
            contact, mucwin->roomjid);
    }

    return TRUE;
}

gboolean
cmd_invites(ProfWin *window, const char *const command, gchar **args)
{
    GList *invites = muc_invites();
    cons_show_room_invites(invites);
    g_list_free_full(invites, g_free);
    return TRUE;
}

gboolean
cmd_decline(ProfWin *window, const char *const command, gchar **args)
{
    if (!muc_invites_contain(args[0])) {
        cons_show("No such invite exists.");
    } else {
        muc_invites_remove(args[0]);
        cons_show("Declined invite to %s.", args[0]);
    }

    return TRUE;
}

gboolean
cmd_form_field(ProfWin *window, char *tag, gchar **args)
{
    if (window->type != WIN_MUC_CONFIG) {
        return TRUE;
    }

    ProfMucConfWin *confwin = (ProfMucConfWin*)window;
    DataForm *form = confwin->form;
    if (form) {
        if (!form_tag_exists(form, tag)) {
            win_println(window, THEME_DEFAULT, '-', "Form does not contain a field with tag %s", tag);
            return TRUE;
        }

        form_field_type_t field_type = form_get_field_type(form, tag);
        char *cmd = NULL;
        char *value = NULL;
        gboolean valid = FALSE;
        gboolean added = FALSE;
        gboolean removed = FALSE;

        switch (field_type) {
        case FIELD_BOOLEAN:
            value = args[0];
            if (g_strcmp0(value, "on") == 0) {
                form_set_value(form, tag, "1");
                win_println(window, THEME_DEFAULT, '-', "Field updated...");
                mucconfwin_show_form_field(confwin, form, tag);
            } else if (g_strcmp0(value, "off") == 0) {
                form_set_value(form, tag, "0");
                win_println(window, THEME_DEFAULT, '-', "Field updated...");
                mucconfwin_show_form_field(confwin, form, tag);
            } else {
                win_println(window, THEME_DEFAULT, '-', "Invalid command, usage:");
                mucconfwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, '-', "");
            }
            break;

        case FIELD_TEXT_PRIVATE:
        case FIELD_TEXT_SINGLE:
        case FIELD_JID_SINGLE:
            value = args[0];
            if (value == NULL) {
                win_println(window, THEME_DEFAULT, '-', "Invalid command, usage:");
                mucconfwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, '-', "");
            } else {
                form_set_value(form, tag, value);
                win_println(window, THEME_DEFAULT, '-', "Field updated...");
                mucconfwin_show_form_field(confwin, form, tag);
            }
            break;
        case FIELD_LIST_SINGLE:
            value = args[0];
            if ((value == NULL) || !form_field_contains_option(form, tag, value)) {
                win_println(window, THEME_DEFAULT, '-', "Invalid command, usage:");
                mucconfwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, '-', "");
            } else {
                form_set_value(form, tag, value);
                win_println(window, THEME_DEFAULT, '-', "Field updated...");
                mucconfwin_show_form_field(confwin, form, tag);
            }
            break;

        case FIELD_TEXT_MULTI:
            cmd = args[0];
            if (cmd) {
                value = args[1];
            }
            if ((g_strcmp0(cmd, "add") != 0) && (g_strcmp0(cmd, "remove"))) {
                win_println(window, THEME_DEFAULT, '-', "Invalid command, usage:");
                mucconfwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, '-', "");
                break;
            }
            if (value == NULL) {
                win_println(window, THEME_DEFAULT, '-', "Invalid command, usage:");
                mucconfwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, '-', "");
                break;
            }
            if (g_strcmp0(cmd, "add") == 0) {
                form_add_value(form, tag, value);
                win_println(window, THEME_DEFAULT, '-', "Field updated...");
                mucconfwin_show_form_field(confwin, form, tag);
                break;
            }
            if (g_strcmp0(args[0], "remove") == 0) {
                if (!g_str_has_prefix(value, "val")) {
                    win_println(window, THEME_DEFAULT, '-', "Invalid command, usage:");
                    mucconfwin_field_help(confwin, tag);
                    win_println(window, THEME_DEFAULT, '-', "");
                    break;
                }
                if (strlen(value) < 4) {
                    win_println(window, THEME_DEFAULT, '-', "Invalid command, usage:");
                    mucconfwin_field_help(confwin, tag);
                    win_println(window, THEME_DEFAULT, '-', "");
                    break;
                }

                int index = strtol(&value[3], NULL, 10);
                if ((index < 1) || (index > form_get_value_count(form, tag))) {
                    win_println(window, THEME_DEFAULT, '-', "Invalid command, usage:");
                    mucconfwin_field_help(confwin, tag);
                    win_println(window, THEME_DEFAULT, '-', "");
                    break;
                }

                removed = form_remove_text_multi_value(form, tag, index);
                if (removed) {
                    win_println(window, THEME_DEFAULT, '-', "Field updated...");
                    mucconfwin_show_form_field(confwin, form, tag);
                } else {
                    win_println(window, THEME_DEFAULT, '-', "Could not remove %s from %s", value, tag);
                }
            }
            break;
        case FIELD_LIST_MULTI:
            cmd = args[0];
            if (cmd) {
                value = args[1];
            }
            if ((g_strcmp0(cmd, "add") != 0) && (g_strcmp0(cmd, "remove"))) {
                win_println(window, THEME_DEFAULT, '-', "Invalid command, usage:");
                mucconfwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, '-', "");
                break;
            }
            if (value == NULL) {
                win_println(window, THEME_DEFAULT, '-', "Invalid command, usage:");
                mucconfwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, '-', "");
                break;
            }
            if (g_strcmp0(args[0], "add") == 0) {
                valid = form_field_contains_option(form, tag, value);
                if (valid) {
                    added = form_add_unique_value(form, tag, value);
                    if (added) {
                        win_println(window, THEME_DEFAULT, '-', "Field updated...");
                        mucconfwin_show_form_field(confwin, form, tag);
                    } else {
                        win_println(window, THEME_DEFAULT, '-', "Value %s already selected for %s", value, tag);
                    }
                } else {
                    win_println(window, THEME_DEFAULT, '-', "Invalid command, usage:");
                    mucconfwin_field_help(confwin, tag);
                    win_println(window, THEME_DEFAULT, '-', "");
                }
                break;
            }
            if (g_strcmp0(args[0], "remove") == 0) {
                valid = form_field_contains_option(form, tag, value);
                if (valid == TRUE) {
                    removed = form_remove_value(form, tag, value);
                    if (removed) {
                        win_println(window, THEME_DEFAULT, '-', "Field updated...");
                        mucconfwin_show_form_field(confwin, form, tag);
                    } else {
                        win_println(window, THEME_DEFAULT, '-', "Value %s is not currently set for %s", value, tag);
                    }
                } else {
                    win_println(window, THEME_DEFAULT, '-', "Invalid command, usage:");
                    mucconfwin_field_help(confwin, tag);
                    win_println(window, THEME_DEFAULT, '-', "");
                }
            }
            break;
        case FIELD_JID_MULTI:
            cmd = args[0];
            if (cmd) {
                value = args[1];
            }
            if ((g_strcmp0(cmd, "add") != 0) && (g_strcmp0(cmd, "remove"))) {
                win_println(window, THEME_DEFAULT, '-', "Invalid command, usage:");
                mucconfwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, '-', "");
                break;
            }
            if (value == NULL) {
                win_println(window, THEME_DEFAULT, '-', "Invalid command, usage:");
                mucconfwin_field_help(confwin, tag);
                win_println(window, THEME_DEFAULT, '-', "");
                break;
            }
            if (g_strcmp0(args[0], "add") == 0) {
                added = form_add_unique_value(form, tag, value);
                if (added) {
                    win_println(window, THEME_DEFAULT, '-', "Field updated...");
                    mucconfwin_show_form_field(confwin, form, tag);
                } else {
                    win_println(window, THEME_DEFAULT, '-', "JID %s already exists in %s", value, tag);
                }
                break;
            }
            if (g_strcmp0(args[0], "remove") == 0) {
                removed = form_remove_value(form, tag, value);
                if (removed) {
                    win_println(window, THEME_DEFAULT, '-', "Field updated...");
                    mucconfwin_show_form_field(confwin, form, tag);
                } else {
                    win_println(window, THEME_DEFAULT, '-', "Field %s does not contain %s", tag, value);
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
cmd_form(ProfWin *window, const char *const command, gchar **args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (window->type != WIN_MUC_CONFIG) {
        cons_show("Command '/form' does not apply to this window.");
        return TRUE;
    }

    if ((g_strcmp0(args[0], "submit") != 0) &&
            (g_strcmp0(args[0], "cancel") != 0) &&
            (g_strcmp0(args[0], "show") != 0) &&
            (g_strcmp0(args[0], "help") != 0)) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    ProfMucConfWin *confwin = (ProfMucConfWin*)window;
    assert(confwin->memcheck == PROFCONFWIN_MEMCHECK);

    if (g_strcmp0(args[0], "show") == 0) {
        mucconfwin_show_form(confwin);
        return TRUE;
    }

    if (g_strcmp0(args[0], "help") == 0) {
        char *tag = args[1];
        if (tag) {
            mucconfwin_field_help(confwin, tag);
        } else {
            mucconfwin_form_help(confwin);

            gchar **help_text = NULL;
            Command *command = cmd_get("/form");

            if (command) {
                help_text = command->help.synopsis;
            }

            ui_show_lines((ProfWin*) confwin, help_text);
        }
        win_println(window, THEME_DEFAULT, '-', "");
        return TRUE;
    }

    if (g_strcmp0(args[0], "submit") == 0) {
        iq_submit_room_config(confwin->roomjid, confwin->form);
    }

    if (g_strcmp0(args[0], "cancel") == 0) {
        iq_room_config_cancel(confwin->roomjid);
    }

    if ((g_strcmp0(args[0], "submit") == 0) || (g_strcmp0(args[0], "cancel") == 0)) {
        if (confwin->form) {
            cmd_ac_remove_form_fields(confwin->form);
        }

        int num = wins_get_num(window);

        ProfWin *new_current = (ProfWin*)wins_get_muc(confwin->roomjid);
        if (!new_current) {
            new_current = wins_get_console();
        }
        ui_focus_win(new_current);
        wins_close_by_num(num);
    }

    return TRUE;
}

gboolean
cmd_kick(ProfWin *window, const char *const command, gchar **args)
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

    ProfMucWin *mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    char *nick = args[0];
    if (nick) {
        if (muc_roster_contains_nick(mucwin->roomjid, nick)) {
            char *reason = args[1];
            iq_room_kick_occupant(mucwin->roomjid, nick, reason);
        } else {
            win_println(window, THEME_DEFAULT, '!', "Occupant does not exist: %s", nick);
        }
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_ban(ProfWin *window, const char *const command, gchar **args)
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

    ProfMucWin *mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    char *jid = args[0];
    if (jid) {
        char *reason = args[1];
        iq_room_affiliation_set(mucwin->roomjid, jid, "outcast", reason);
    } else {
        cons_bad_cmd_usage(command);
    }
    return TRUE;
}

gboolean
cmd_subject(ProfWin *window, const char *const command, gchar **args)
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

    ProfMucWin *mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    if (args[0] == NULL) {
        char *subject = muc_subject(mucwin->roomjid);
        if (subject) {
            win_print(window, THEME_ROOMINFO, '!', "Room subject: ");
            win_appendln(window, THEME_DEFAULT, "%s", subject);
        } else {
            win_println(window, THEME_ROOMINFO, '!', "Room has no subject");
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

    if (g_strcmp0(args[0], "prepend") == 0) {
        if (args[1]) {
            char *old_subject = muc_subject(mucwin->roomjid);
            if (old_subject) {
                GString *new_subject = g_string_new(args[1]);
                g_string_append(new_subject, old_subject);
                message_send_groupchat_subject(mucwin->roomjid, new_subject->str);
                g_string_free(new_subject, TRUE);
            } else {
                win_print(window, THEME_ROOMINFO, '!', "Room does not have a subject, use /subject set <subject>");
            }
        } else {
            cons_bad_cmd_usage(command);
        }
        return TRUE;
    }

    if (g_strcmp0(args[0], "append") == 0) {
        if (args[1]) {
            char *old_subject = muc_subject(mucwin->roomjid);
            if (old_subject) {
                GString *new_subject = g_string_new(old_subject);
                g_string_append(new_subject, args[1]);
                message_send_groupchat_subject(mucwin->roomjid, new_subject->str);
                g_string_free(new_subject, TRUE);
            } else {
                win_print(window, THEME_ROOMINFO, '!', "Room does not have a subject, use /subject set <subject>");
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
cmd_affiliation(ProfWin *window, const char *const command, gchar **args)
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

    char *cmd = args[0];
    if (cmd == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    char *affiliation = args[1];
    if (affiliation &&
            (g_strcmp0(affiliation, "owner") != 0) &&
            (g_strcmp0(affiliation, "admin") != 0) &&
            (g_strcmp0(affiliation, "member") != 0) &&
            (g_strcmp0(affiliation, "none") != 0) &&
            (g_strcmp0(affiliation, "outcast") != 0)) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    ProfMucWin *mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    if (g_strcmp0(cmd, "list") == 0) {
        if (!affiliation) {
            iq_room_affiliation_list(mucwin->roomjid, "owner");
            iq_room_affiliation_list(mucwin->roomjid, "admin");
            iq_room_affiliation_list(mucwin->roomjid, "member");
            iq_room_affiliation_list(mucwin->roomjid, "outcast");
        } else if (g_strcmp0(affiliation, "none") == 0) {
            win_println(window, THEME_DEFAULT, '!', "Cannot list users with no affiliation.");
        } else {
            iq_room_affiliation_list(mucwin->roomjid, affiliation);
        }
        return TRUE;
    }

    if (g_strcmp0(cmd, "set") == 0) {
        if (!affiliation) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        char *jid = args[2];
        if (jid == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            char *reason = args[3];
            iq_room_affiliation_set(mucwin->roomjid, jid, affiliation, reason);
            return TRUE;
        }
    }

    cons_bad_cmd_usage(command);
    return TRUE;
}

gboolean
cmd_role(ProfWin *window, const char *const command, gchar **args)
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

    char *cmd = args[0];
    if (cmd == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    char *role = args[1];
    if (role &&
            (g_strcmp0(role, "visitor") != 0) &&
            (g_strcmp0(role, "participant") != 0) &&
            (g_strcmp0(role, "moderator") != 0) &&
            (g_strcmp0(role, "none") != 0)) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    ProfMucWin *mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    if (g_strcmp0(cmd, "list") == 0) {
        if (!role) {
            iq_room_role_list(mucwin->roomjid, "moderator");
            iq_room_role_list(mucwin->roomjid, "participant");
            iq_room_role_list(mucwin->roomjid, "visitor");
        } else if (g_strcmp0(role, "none") == 0) {
            win_println(window, THEME_DEFAULT, '!', "Cannot list users with no role.");
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

        char *nick = args[2];
        if (nick == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            char *reason = args[3];
            iq_room_role_set(mucwin->roomjid, nick, role, reason);
            return TRUE;
        }
    }

    cons_bad_cmd_usage(command);
    return TRUE;
}

gboolean
cmd_room(ProfWin *window, const char *const command, gchar **args)
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

    if ((g_strcmp0(args[0], "accept") != 0) &&
            (g_strcmp0(args[0], "destroy") != 0) &&
            (g_strcmp0(args[0], "config") != 0)) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    ProfMucWin *mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
    int num = wins_get_num(window);

    int ui_index = num;
    if (ui_index == 10) {
        ui_index = 0;
    }

    if (g_strcmp0(args[0], "accept") == 0) {
        gboolean requires_config = muc_requires_config(mucwin->roomjid);
        if (!requires_config) {
            win_println(window, THEME_ROOMINFO, '!', "Current room does not require configuration.");
            return TRUE;
        } else {
            iq_confirm_instant_room(mucwin->roomjid);
            muc_set_requires_config(mucwin->roomjid, FALSE);
            win_println(window, THEME_ROOMINFO, '!', "Room unlocked.");
            return TRUE;
        }
    }

    if (g_strcmp0(args[0], "destroy") == 0) {
        iq_destroy_room(mucwin->roomjid);
        return TRUE;
    }

    if (g_strcmp0(args[0], "config") == 0) {
        ProfMucConfWin *confwin = wins_get_muc_conf(mucwin->roomjid);

        if (confwin) {
            ui_focus_win((ProfWin*)confwin);
        } else {
            iq_request_room_config_form(mucwin->roomjid);
        }
        return TRUE;
    }

    return TRUE;
}

gboolean
cmd_occupants(ProfWin *window, const char *const command, gchar **args)
{
    if (g_strcmp0(args[0], "size") == 0) {
        if (!args[1]) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            int intval = 0;
            char *err_msg = NULL;
            gboolean res = strtoi_range(args[1], &intval, 1, 99, &err_msg);
            if (res) {
                prefs_set_occupants_size(intval);
                cons_show("Occupants screen size set to: %d%%", intval);
                wins_resize_all();
                return TRUE;
            } else {
                cons_show(err_msg);
                free(err_msg);
                return TRUE;
            }
        }
    }

    if (g_strcmp0(args[0], "default") == 0) {
        if (g_strcmp0(args[1], "show") == 0) {
            if (g_strcmp0(args[2], "jid") == 0) {
                cons_show("Occupant jids enabled.");
                prefs_set_boolean(PREF_OCCUPANTS_JID, TRUE);
            } else {
                cons_show("Occupant list enabled.");
                prefs_set_boolean(PREF_OCCUPANTS, TRUE);
            }
            return TRUE;
        } else if (g_strcmp0(args[1], "hide") == 0) {
            if (g_strcmp0(args[2], "jid") == 0) {
                cons_show("Occupant jids disabled.");
                prefs_set_boolean(PREF_OCCUPANTS_JID, FALSE);
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

    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (window->type != WIN_MUC) {
        cons_show("Cannot apply setting when not in chat room.");
        return TRUE;
    }

    ProfMucWin *mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

    if (g_strcmp0(args[0], "show") == 0) {
        if (g_strcmp0(args[1], "jid") == 0) {
            mucwin->showjid = TRUE;
            mucwin_update_occupants(mucwin);
        } else {
            mucwin_show_occupants(mucwin);
        }
    } else if (g_strcmp0(args[0], "hide") == 0) {
        if (g_strcmp0(args[1], "jid") == 0) {
            mucwin->showjid = FALSE;
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
cmd_rooms(ProfWin *window, const char *const command, gchar **args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (args[0]) {
        iq_room_list_request(args[0]);
        return TRUE;
    }

    ProfAccount *account = accounts_get_account(session_get_account_name());
    if (account->muc_service) {
        iq_room_list_request(account->muc_service);
    } else {
        cons_show("Account MUC service property not found.");
    }
    account_free(account);

    return TRUE;
}

gboolean
cmd_bookmark(ProfWin *window, const char *const command, gchar **args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    gchar *cmd = args[0];
    if (window->type == WIN_MUC && (cmd == NULL || g_strcmp0(cmd, "add") == 0)) {
        // default to current nickname, password, and autojoin "on"
        ProfMucWin *mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        char *nick = muc_nick(mucwin->roomjid);
        char *password = muc_password(mucwin->roomjid);
        gboolean added = bookmark_add(mucwin->roomjid, nick, password, "on");
        if (added) {
            win_println(window, THEME_DEFAULT, '!', "Bookmark added for %s.", mucwin->roomjid);
        } else {
            win_println(window, THEME_DEFAULT, '!', "Bookmark already exists for %s.", mucwin->roomjid);
        }
        return TRUE;
    }

    if (window->type == WIN_MUC && g_strcmp0(cmd, "remove") == 0) {
        ProfMucWin *mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        gboolean removed = bookmark_remove(mucwin->roomjid);
        if (removed) {
            win_println(window, THEME_DEFAULT, '!', "Bookmark removed for %s.", mucwin->roomjid);
        } else {
            win_println(window, THEME_DEFAULT, '!', "Bookmark does not exist for %s.", mucwin->roomjid);
        }
        return TRUE;
    }

    if (cmd == NULL) {
        cons_bad_cmd_usage(command);
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
        return TRUE;
    }

    if (strcmp(cmd, "list") == 0) {
        GList *bookmarks = bookmark_get_list();
        cons_show_bookmarks(bookmarks);
        g_list_free(bookmarks);
        return TRUE;
    }

    char *jid = args[1];
    if (jid == NULL) {
        cons_bad_cmd_usage(command);
        cons_show("");
        return TRUE;
    }
    if (strchr(jid, '@') == NULL) {
        cons_show("Invalid room, must be of the form room@domain.tld");
        cons_show("");
        return TRUE;
    }

    if (strcmp(cmd, "remove") == 0) {
        gboolean removed = bookmark_remove(jid);
        if (removed) {
            cons_show("Bookmark removed for %s.", jid);
        } else {
            cons_show("No bookmark exists for %s.", jid);
        }
        return TRUE;
    }

    if (strcmp(cmd, "join") == 0) {
        gboolean joined = bookmark_join(jid);
        if (!joined) {
            cons_show("No bookmark exists for %s.", jid);
        }
        return TRUE;
    }

    gchar *opt_keys[] = { "autojoin", "nick", "password", NULL };
    gboolean parsed;

    GHashTable *options = parse_options(&args[2], opt_keys, &parsed);
    if (!parsed) {
        cons_bad_cmd_usage(command);
        cons_show("");
        return TRUE;
    }

    char *autojoin = g_hash_table_lookup(options, "autojoin");

    if (autojoin && ((strcmp(autojoin, "on") != 0) && (strcmp(autojoin, "off") != 0))) {
        cons_bad_cmd_usage(command);
        cons_show("");
        options_destroy(options);
        return TRUE;
    }

    char *nick = g_hash_table_lookup(options, "nick");
    char *password = g_hash_table_lookup(options, "password");

    if (strcmp(cmd, "add") == 0) {
        gboolean added = bookmark_add(jid, nick, password, autojoin);
        if (added) {
            cons_show("Bookmark added for %s.", jid);
        } else {
            cons_show("Bookmark already exists, use /bookmark update to edit.");
        }
        options_destroy(options);
        return TRUE;
    }

    if (strcmp(cmd, "update") == 0) {
        gboolean updated = bookmark_update(jid, nick, password, autojoin);
        if (updated) {
            cons_show("Bookmark updated.");
        } else {
            cons_show("No bookmark exists for %s.", jid);
        }
        options_destroy(options);
        return TRUE;
    }

    cons_bad_cmd_usage(command);
    options_destroy(options);

    return TRUE;
}

gboolean
cmd_disco(ProfWin *window, const char *const command, gchar **args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    GString *jid = g_string_new("");
    if (args[1]) {
        jid = g_string_append(jid, args[1]);
    } else {
        Jid *jidp = jid_create(connection_get_fulljid());
        jid = g_string_append(jid, jidp->domainpart);
        jid_destroy(jidp);
    }

    if (g_strcmp0(args[0], "info") == 0) {
        iq_disco_info_request(jid->str);
    } else {
        iq_disco_items_request(jid->str);
    }

    g_string_free(jid, TRUE);

    return TRUE;
}

gboolean
cmd_sendfile(ProfWin *window, const char *const command, gchar **args)
{
    jabber_conn_status_t conn_status = connection_get_status();
    char *filename = args[0];

    // expand ~ to $HOME
    if (filename[0] == '~' && filename[1] == '/') {
        if (asprintf(&filename, "%s/%s", getenv("HOME"), filename+2) == -1) {
            return TRUE;
        }
    } else {
        filename = strdup(filename);
    }

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        free(filename);
        return TRUE;
    }

    if (window->type != WIN_CHAT && window->type != WIN_PRIVATE && window->type != WIN_MUC) {
        cons_show_error("Unsupported window for file transmission.");
        free(filename);
        return TRUE;
    }

    if (access(filename, R_OK) != 0) {
        cons_show_error("Uploading '%s' failed: File not found!", filename);
        free(filename);
        return TRUE;
    }

    if (!is_regular_file(filename)) {
        cons_show_error("Uploading '%s' failed: Not a file!", filename);
        free(filename);
        return TRUE;
    }

    HTTPUpload *upload = malloc(sizeof(HTTPUpload));
    upload->window = window;

    upload->filename = filename;
    upload->filesize = file_size(filename);
    upload->mime_type = file_mime_type(filename);

    iq_http_upload_request(upload);

    return TRUE;
}

gboolean
cmd_lastactivity(ProfWin *window, const char *const command, gchar **args)
{
    if ((g_strcmp0(args[0], "on") == 0) || (g_strcmp0(args[0], "off") == 0)) {
        _cmd_set_boolean_preference(args[0], command, "Last activity", PREF_LASTACTIVITY);
        if (g_strcmp0(args[0], "on") == 0) {
            caps_add_feature(XMPP_FEATURE_LASTACTIVITY);
        }
        if (g_strcmp0(args[0], "off") == 0) {
            caps_remove_feature(XMPP_FEATURE_LASTACTIVITY);
        }
        return TRUE;
    }

    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (args[0] == NULL) {
        Jid *jidp = jid_create(connection_get_fulljid());
        GString *jid = g_string_new(jidp->domainpart);

        iq_last_activity_request(jid->str);

        g_string_free(jid, TRUE);
        jid_destroy(jidp);

        return TRUE;
    } else {
        iq_last_activity_request(args[0]);
        return TRUE;
    }
}

gboolean
cmd_nick(ProfWin *window, const char *const command, gchar **args)
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

    ProfMucWin *mucwin = (ProfMucWin*)window;
    assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
    char *nick = args[0];
    presence_change_room_nick(mucwin->roomjid, nick);

    return TRUE;
}

gboolean
cmd_alias(ProfWin *window, const char *const command, gchar **args)
{
    char *subcmd = args[0];

    if (strcmp(subcmd, "add") == 0) {
        char *alias = args[1];
        if (alias == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        } else {
            char *alias_p = alias;
            GString *ac_value = g_string_new("");
            if (alias[0] == '/') {
                g_string_append(ac_value, alias);
                alias_p = &alias[1];
            } else {
                g_string_append(ac_value, "/");
                g_string_append(ac_value, alias);
            }

            char *value = args[2];
            if (value == NULL) {
                cons_bad_cmd_usage(command);
                g_string_free(ac_value, TRUE);
                return TRUE;
            } else if (cmd_ac_exists(ac_value->str)) {
                cons_show("Command or alias '%s' already exists.", ac_value->str);
                g_string_free(ac_value, TRUE);
                return TRUE;
            } else {
                prefs_add_alias(alias_p, value);
                cmd_ac_add(ac_value->str);
                cmd_ac_add_alias_value(alias_p);
                cons_show("Command alias added %s -> %s", ac_value->str, value);
                g_string_free(ac_value, TRUE);
                return TRUE;
            }
        }
    } else if (strcmp(subcmd, "remove") == 0) {
        char *alias = args[1];
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
                GString *ac_value = g_string_new("/");
                g_string_append(ac_value, alias);
                cmd_ac_remove(ac_value->str);
                cmd_ac_remove_alias_value(alias);
                g_string_free(ac_value, TRUE);
                cons_show("Command alias removed -> /%s", alias);
            }
            return TRUE;
        }
    } else if (strcmp(subcmd, "list") == 0) {
        GList *aliases = prefs_get_aliases();
        cons_show_aliases(aliases);
        prefs_free_aliases(aliases);
        return TRUE;
    } else {
        cons_bad_cmd_usage(command);
        return TRUE;
    }
}

gboolean
cmd_tiny(ProfWin *window, const char *const command, gchar **args)
{
    char *url = args[0];

    if (window->type != WIN_CHAT && window->type != WIN_MUC && window->type != WIN_PRIVATE) {
        cons_show("/tiny can only be used in chat windows");
        return TRUE;
    }

    if (!tinyurl_valid(url)) {
        win_println(window, THEME_ERROR, '-', "/tiny, badly formed URL: %s", url);
        return TRUE;
    }

    char *tiny = tinyurl_get(url);
    if (!tiny) {
        win_println(window, THEME_ERROR, '-', "Couldn't create tinyurl.");
        return TRUE;
    }

    switch (window->type){
    case WIN_CHAT:
    {
        ProfChatWin *chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        cl_ev_send_msg(chatwin, tiny, NULL);
        break;
    }
    case WIN_PRIVATE:
    {
        ProfPrivateWin *privatewin = (ProfPrivateWin*)window;
        assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
        cl_ev_send_priv_msg(privatewin, tiny, NULL);
        break;
    }
    case WIN_MUC:
    {
        ProfMucWin *mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        cl_ev_send_muc_msg(mucwin, tiny, NULL);
        break;
    }
    default:
        break;
    }

    free(tiny);

    return TRUE;
}

gboolean
cmd_clear(ProfWin *window, const char *const command, gchar **args)
{
    win_clear(window);
    return TRUE;
}

gboolean
cmd_leave(ProfWin *window, const char *const command, gchar **args)
{
    if (window->type != WIN_MUC && window->type != WIN_CHAT && window->type != WIN_PRIVATE) {
        cons_show("The /leave command is only valid in chat, or chat room windows.");
        cons_alert();
        return TRUE;
    }

    // use /close behaviour
    return cmd_close(window, "/leave", args);
}

gboolean
cmd_privileges(ProfWin *window, const char *const command, gchar **args)
{
    _cmd_set_boolean_preference(args[0], command, "MUC privileges", PREF_MUC_PRIVILEGES);

    ui_redraw_all_room_rosters();

    return TRUE;
}

gboolean
cmd_charset(ProfWin *window, const char *const command, gchar **args)
{
    char *codeset = nl_langinfo(CODESET);
    char *lang = getenv("LANG");

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
cmd_beep(ProfWin *window, const char *const command, gchar **args)
{
    _cmd_set_boolean_preference(args[0], command, "Sound", PREF_BEEP);
    return TRUE;
}

gboolean
cmd_console(ProfWin *window, const char *const command, gchar **args)
{
    if ((g_strcmp0(args[0], "chat") != 0) && (g_strcmp0(args[0], "muc") != 0) && (g_strcmp0(args[0], "private") != 0)) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    char *setting = args[1];
    if ((g_strcmp0(setting, "all") != 0) && (g_strcmp0(setting, "first") != 0) && (g_strcmp0(setting, "none") != 0)) {
        cons_bad_cmd_usage(command);
        return TRUE;
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
cmd_presence(ProfWin *window, const char *const command, gchar **args)
{
    if (strcmp(args[0], "console") != 0 &&
            strcmp(args[0], "chat") != 0 &&
            strcmp(args[0], "room") != 0 &&
            strcmp(args[0], "titlebar") != 0) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (strcmp(args[0], "titlebar") == 0) {
        _cmd_set_boolean_preference(args[1], command, "Contact presence", PREF_PRESENCE);
        return TRUE;
    }

    if (strcmp(args[1], "all") != 0 &&
            strcmp(args[1], "online") != 0 &&
            strcmp(args[1], "none") != 0) {
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
cmd_wrap(ProfWin *window, const char *const command, gchar **args)
{
    _cmd_set_boolean_preference(args[0], command, "Word wrap", PREF_WRAP);

    wins_resize_all();

    return TRUE;
}

gboolean
cmd_time(ProfWin *window, const char *const command, gchar **args)
{
    if (g_strcmp0(args[0], "lastactivity") == 0) {
        if (args[1] == NULL) {
            char *format = prefs_get_string(PREF_TIME_LASTACTIVITY);
            cons_show("Last activity time format: '%s'.", format);
            prefs_free_string(format);
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
            char *format = prefs_get_string(PREF_TIME_STATUSBAR);
            cons_show("Status bar time format: '%s'.", format);
            prefs_free_string(format);
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
            char *format = prefs_get_string(PREF_TIME_CONSOLE);
            cons_show("Console time format: '%s'.", format);
            prefs_free_string(format);
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
            char *format = prefs_get_string(PREF_TIME_CHAT);
            cons_show("Chat time format: '%s'.", format);
            prefs_free_string(format);
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
            char *format = prefs_get_string(PREF_TIME_MUC);
            cons_show("MUC time format: '%s'.", format);
            prefs_free_string(format);
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
    } else if (g_strcmp0(args[0], "mucconfig") == 0) {
        if (args[1] == NULL) {
            char *format = prefs_get_string(PREF_TIME_MUCCONFIG);
            cons_show("MUC config time format: '%s'.", format);
            prefs_free_string(format);
            return TRUE;
        } else if (g_strcmp0(args[1], "set") == 0 && args[2] != NULL) {
            prefs_set_string(PREF_TIME_MUCCONFIG, args[2]);
            cons_show("MUC config time format set to '%s'.", args[2]);
            wins_resize_all();
            return TRUE;
        } else if (g_strcmp0(args[1], "off") == 0) {
            prefs_set_string(PREF_TIME_MUCCONFIG, "off");
            cons_show("MUC config time display disabled.");
            wins_resize_all();
            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    } else if (g_strcmp0(args[0], "private") == 0) {
        if (args[1] == NULL) {
            char *format = prefs_get_string(PREF_TIME_PRIVATE);
            cons_show("Private chat time format: '%s'.", format);
            prefs_free_string(format);
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
            char *format = prefs_get_string(PREF_TIME_XMLCONSOLE);
            cons_show("XML Console time format: '%s'.", format);
            prefs_free_string(format);
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
    } else {
        cons_bad_cmd_usage(command);
        return TRUE;
    }
}

gboolean
cmd_states(ProfWin *window, const char *const command, gchar **args)
{
    _cmd_set_boolean_preference(args[0], command, "Sending chat states", PREF_STATES);

    // if disabled, disable outtype and gone
    if (strcmp(args[0], "off") == 0) {
        prefs_set_boolean(PREF_OUTTYPE, FALSE);
        prefs_set_gone(0);
    }

    return TRUE;
}

gboolean
cmd_wintitle(ProfWin *window, const char *const command, gchar **args)
{
    if (g_strcmp0(args[0], "show") != 0 && g_strcmp0(args[0], "goodbye") != 0) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }
    if (g_strcmp0(args[0], "show") == 0 && g_strcmp0(args[1], "off") == 0) {
        ui_clear_win_title();
    }
    if (g_strcmp0(args[0], "show") == 0) {
        _cmd_set_boolean_preference(args[1], command, "Window title show", PREF_WINTITLE_SHOW);
    } else {
        _cmd_set_boolean_preference(args[1], command, "Window title goodbye", PREF_WINTITLE_GOODBYE);
    }

    return TRUE;
}

gboolean
cmd_outtype(ProfWin *window, const char *const command, gchar **args)
{
    _cmd_set_boolean_preference(args[0], command, "Sending typing notifications", PREF_OUTTYPE);

    // if enabled, enable states
    if (strcmp(args[0], "on") == 0) {
        prefs_set_boolean(PREF_STATES, TRUE);
    }

    return TRUE;
}

gboolean
cmd_gone(ProfWin *window, const char *const command, gchar **args)
{
    char *value = args[0];

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
cmd_notify(ProfWin *window, const char *const command, gchar **args)
{
    if (!args[0]) {
        ProfWin *current = wins_get_current();
        if (current->type == WIN_MUC) {
            win_println(current, THEME_DEFAULT, '-', "");
            ProfMucWin *mucwin = (ProfMucWin *)current;

            win_println(window, THEME_DEFAULT, '!', "Notification settings for %s:", mucwin->roomjid);
            if (prefs_has_room_notify(mucwin->roomjid)) {
                if (prefs_get_room_notify(mucwin->roomjid)) {
                    win_println(window, THEME_DEFAULT, '!', "  Message  : ON");
                } else {
                    win_println(window, THEME_DEFAULT, '!', "  Message  : OFF");
                }
            } else {
                if (prefs_get_boolean(PREF_NOTIFY_ROOM)) {
                    win_println(window, THEME_DEFAULT, '!', "  Message  : ON (global setting)");
                } else {
                    win_println(window, THEME_DEFAULT, '!', "  Message  : OFF (global setting)");
                }
            }
            if (prefs_has_room_notify_mention(mucwin->roomjid)) {
                if (prefs_get_room_notify_mention(mucwin->roomjid)) {
                    win_println(window, THEME_DEFAULT, '!', "  Mention  : ON");
                } else {
                    win_println(window, THEME_DEFAULT, '!', "  Mention  : OFF");
                }
            } else {
                if (prefs_get_boolean(PREF_NOTIFY_ROOM_MENTION)) {
                    win_println(window, THEME_DEFAULT, '!', "  Mention  : ON (global setting)");
                } else {
                    win_println(window, THEME_DEFAULT, '!', "  Mention  : OFF (global setting)");
                }
            }
            if (prefs_has_room_notify_trigger(mucwin->roomjid)) {
                if (prefs_get_room_notify_trigger(mucwin->roomjid)) {
                    win_println(window, THEME_DEFAULT, '!', "  Triggers : ON");
                } else {
                    win_println(window, THEME_DEFAULT, '!', "  Triggers : OFF");
                }
            } else {
                if (prefs_get_boolean(PREF_NOTIFY_ROOM_TRIGGER)) {
                    win_println(window, THEME_DEFAULT, '!', "  Triggers : ON (global setting)");
                } else {
                    win_println(window, THEME_DEFAULT, '!', "  Triggers : OFF (global setting)");
                }
            }
            win_println(current, THEME_DEFAULT, '-', "");
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
                GList *triggers = prefs_get_room_notify_triggers();
                GList *curr = triggers;
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
            ProfWin *window = wins_get_current();
            if (window->type != WIN_MUC) {
                cons_show("You must be in a chat room.");
            } else {
                ProfMucWin *mucwin = (ProfMucWin*)window;
                prefs_set_room_notify(mucwin->roomjid, TRUE);
                win_println(window, THEME_DEFAULT, '!', "Notifications enabled for %s", mucwin->roomjid);
            }
        }
    } else if (g_strcmp0(args[0], "off") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();

        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
        } else {
            ProfWin *window = wins_get_current();
            if (window->type != WIN_MUC) {
                cons_show("You must be in a chat room.");
            } else {
                ProfMucWin *mucwin = (ProfMucWin*)window;
                prefs_set_room_notify(mucwin->roomjid, FALSE);
                win_println(window, THEME_DEFAULT, '!', "Notifications disabled for %s", mucwin->roomjid);
            }
        }
    } else if (g_strcmp0(args[0], "mention") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();

        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
        } else {
            if (g_strcmp0(args[1], "on") == 0) {
                ProfWin *window = wins_get_current();
                if (window->type != WIN_MUC) {
                    cons_show("You must be in a chat room.");
                } else {
                    ProfMucWin *mucwin = (ProfMucWin*)window;
                    prefs_set_room_notify_mention(mucwin->roomjid, TRUE);
                    win_println(window, THEME_DEFAULT, '!', "Mention notifications enabled for %s", mucwin->roomjid);
                }
            } else if (g_strcmp0(args[1], "off") == 0) {
                ProfWin *window = wins_get_current();
                if (window->type != WIN_MUC) {
                    cons_show("You must be in a chat rooms.");
                } else {
                    ProfMucWin *mucwin = (ProfMucWin*)window;
                    prefs_set_room_notify_mention(mucwin->roomjid, FALSE);
                    win_println(window, THEME_DEFAULT, '!', "Mention notifications disabled for %s", mucwin->roomjid);
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
                ProfWin *window = wins_get_current();
                if (window->type != WIN_MUC) {
                    cons_show("You must be in a chat room.");
                } else {
                    ProfMucWin *mucwin = (ProfMucWin*)window;
                    prefs_set_room_notify_trigger(mucwin->roomjid, TRUE);
                    win_println(window, THEME_DEFAULT, '!', "Custom trigger notifications enabled for %s", mucwin->roomjid);
                }
            } else if (g_strcmp0(args[1], "off") == 0) {
                ProfWin *window = wins_get_current();
                if (window->type != WIN_MUC) {
                    cons_show("You must be in a chat rooms.");
                } else {
                    ProfMucWin *mucwin = (ProfMucWin*)window;
                    prefs_set_room_notify_trigger(mucwin->roomjid, FALSE);
                    win_println(window, THEME_DEFAULT, '!', "Custom trigger notifications disabled for %s", mucwin->roomjid);
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
            ProfWin *window = wins_get_current();
            if (window->type != WIN_MUC) {
                cons_show("You must be in a chat room.");
            } else {
                ProfMucWin *mucwin = (ProfMucWin*)window;
                gboolean res = prefs_reset_room_notify(mucwin->roomjid);
                if (res) {
                    win_println(window, THEME_DEFAULT, '!', "Notification settings set to global defaults for %s", mucwin->roomjid);
                } else {
                    win_println(window, THEME_DEFAULT, '!', "No custom notification settings for %s", mucwin->roomjid);
                }
            }
        }
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_inpblock(ProfWin *window, const char *const command, gchar **args)
{
    char *subcmd = args[0];
    char *value = args[1];

    if (g_strcmp0(subcmd, "timeout") == 0) {
        if (value == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        int intval = 0;
        char *err_msg = NULL;
        gboolean res = strtoi_range(value, &intval, 1, 1000, &err_msg);
        if (res) {
            cons_show("Input blocking set to %d milliseconds.", intval);
            prefs_set_inpblock(intval);
            inp_nonblocking(FALSE);
        } else {
            cons_show(err_msg);
            free(err_msg);
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

        _cmd_set_boolean_preference(value, command, "Dynamic input blocking", PREF_INPBLOCK_DYNAMIC);
        return TRUE;
    }

    cons_bad_cmd_usage(command);

    return TRUE;
}

gboolean
cmd_titlebar(ProfWin *window, const char *const command, gchar **args)
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

    cons_bad_cmd_usage(command);

    return TRUE;
}

gboolean
cmd_mainwin(ProfWin *window, const char *const command, gchar **args)
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
cmd_statusbar(ProfWin *window, const char *const command, gchar **args)
{
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
cmd_inputwin(ProfWin *window, const char *const command, gchar **args)
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
cmd_log(ProfWin *window, const char *const command, gchar **args)
{
    char *subcmd = args[0];
    char *value = args[1];

    if (strcmp(subcmd, "maxsize") == 0) {
        if (value == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        int intval = 0;
        char *err_msg = NULL;
        gboolean res = strtoi_range(value, &intval, PREFS_MIN_LOG_SIZE, INT_MAX, &err_msg);
        if (res) {
            prefs_set_max_log_size(intval);
            cons_show("Log maximum size set to %d bytes", intval);
        } else {
            cons_show(err_msg);
            free(err_msg);
        }
        return TRUE;
    }

    if (strcmp(subcmd, "rotate") == 0) {
        if (value == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
        _cmd_set_boolean_preference(value, command, "Log rotate", PREF_LOG_ROTATE);
        return TRUE;
    }

    if (strcmp(subcmd, "shared") == 0) {
        if (value == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
        _cmd_set_boolean_preference(value, command, "Shared log", PREF_LOG_SHARED);
        log_reinit();
        return TRUE;
    }

    if (strcmp(subcmd, "where") == 0) {
        char *logfile = get_log_file_location();
        cons_show("Log file: %s", logfile);
        return TRUE;
    }

    cons_bad_cmd_usage(command);

    /* TODO: make 'level' subcommand for debug level */

    return TRUE;
}

gboolean
cmd_reconnect(ProfWin *window, const char *const command, gchar **args)
{
    char *value = args[0];

    int intval = 0;
    char *err_msg = NULL;
    gboolean res = strtoi_range(value, &intval, 0, INT_MAX, &err_msg);
    if (res) {
        prefs_set_reconnect(intval);
        if (intval == 0) {
            cons_show("Reconnect disabled.", intval);
        } else {
            cons_show("Reconnect interval set to %d seconds.", intval);
        }
    } else {
        cons_show(err_msg);
        cons_bad_cmd_usage(command);
        free(err_msg);
    }

    return TRUE;
}

gboolean
cmd_autoping(ProfWin *window, const char *const command, gchar **args)
{
    char *cmd = args[0];
    char *value = args[1];

    if (g_strcmp0(cmd, "set") == 0) {
        int intval = 0;
        char *err_msg = NULL;
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
            free(err_msg);
        }

    } else if (g_strcmp0(cmd, "timeout") == 0) {
        int intval = 0;
        char *err_msg = NULL;
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
            free(err_msg);
        }

    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_ping(ProfWin *window, const char *const command, gchar **args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    if (args[0] == NULL && connection_supports(XMPP_FEATURE_PING) == FALSE) {
        cons_show("Server does not support ping requests.");
        return TRUE;
    }

    if (args[0] != NULL && caps_jid_has_feature(args[0], XMPP_FEATURE_PING) == FALSE) {
        cons_show("%s does not support ping requests.", args[0]);
        return TRUE;
    }

    iq_send_ping(args[0]);

    if (args[0] == NULL) {
        cons_show("Pinged server...");
    } else {
        cons_show("Pinged %s...", args[0]);
    }
    return TRUE;
}

gboolean
cmd_autoaway(ProfWin *window, const char *const command, gchar **args)
{
    if ((strcmp(args[0], "mode") != 0) && (strcmp(args[0], "time") != 0) &&
            (strcmp(args[0], "message") != 0) && (strcmp(args[0], "check") != 0)) {
        cons_show("Setting must be one of 'mode', 'time', 'message' or 'check'");
        return TRUE;
    }

    if (strcmp(args[0], "mode") == 0) {
        if ((strcmp(args[1], "idle") != 0) && (strcmp(args[1], "away") != 0) &&
                (strcmp(args[1], "off") != 0)) {
            cons_show("Mode must be one of 'idle', 'away' or 'off'");
        } else {
            prefs_set_string(PREF_AUTOAWAY_MODE, args[1]);
            cons_show("Auto away mode set to: %s.", args[1]);
        }

        return TRUE;
    }

    if (strcmp(args[0], "time") == 0) {
        if (g_strcmp0(args[1], "away") == 0) {
            int minutesval = 0;
            char *err_msg = NULL;
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
                free(err_msg);
            }

            return TRUE;
        } else if (g_strcmp0(args[1], "xa") == 0) {
            int minutesval = 0;
            char *err_msg = NULL;
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
                free(err_msg);
            }

            return TRUE;
        } else {
            cons_bad_cmd_usage(command);
            return TRUE;
        }
    }

    if (strcmp(args[0], "message") == 0) {
        if (g_strcmp0(args[1], "away") == 0) {
            if (strcmp(args[2], "off") == 0) {
                prefs_set_string(PREF_AUTOAWAY_MESSAGE, NULL);
                cons_show("Auto away message cleared.");
            } else {
                prefs_set_string(PREF_AUTOAWAY_MESSAGE, args[2]);
                cons_show("Auto away message set to: \"%s\".", args[2]);
            }

            return TRUE;
        } else if (g_strcmp0(args[1], "xa") == 0) {
            if (strcmp(args[2], "off") == 0) {
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

    if (strcmp(args[0], "check") == 0) {
        _cmd_set_boolean_preference(args[1], command, "Online check", PREF_AUTOAWAY_CHECK);
        return TRUE;
    }

    return TRUE;
}

gboolean
cmd_priority(ProfWin *window, const char *const command, gchar **args)
{
    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_show("You are not currently connected.");
        return TRUE;
    }

    char *value = args[0];

    int intval = 0;
    char *err_msg = NULL;
    gboolean res = strtoi_range(value, &intval, -128, 127, &err_msg);
    if (res) {
        accounts_set_priority_all(session_get_account_name(), intval);
        resource_presence_t last_presence = accounts_get_last_presence(session_get_account_name());
        cl_ev_presence_send(last_presence, 0);
        cons_show("Priority set to %d.", intval);
    } else {
        cons_show(err_msg);
        free(err_msg);
    }

    return TRUE;
}

gboolean
cmd_vercheck(ProfWin *window, const char *const command, gchar **args)
{
    int num_args = g_strv_length(args);

    if (num_args == 0) {
        cons_check_version(TRUE);
        return TRUE;
    } else {
        _cmd_set_boolean_preference(args[0], command, "Version checking", PREF_VERCHECK);
        return TRUE;
    }
}

gboolean
cmd_xmlconsole(ProfWin *window, const char *const command, gchar **args)
{
    ProfXMLWin *xmlwin = wins_get_xmlconsole();
    if (xmlwin) {
        ui_focus_win((ProfWin*)xmlwin);
    } else {
        ProfWin *window = wins_new_xmlconsole();
        ui_focus_win(window);
    }

    return TRUE;
}

gboolean
cmd_flash(ProfWin *window, const char *const command, gchar **args)
{
    _cmd_set_boolean_preference(args[0], command, "Screen flash", PREF_FLASH);
    return TRUE;
}

gboolean
cmd_tray(ProfWin *window, const char *const command, gchar **args)
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
        char *err_msg = NULL;
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
            free(err_msg);
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
        _cmd_set_boolean_preference(args[0], command, "Tray icon", PREF_TRAY);
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
cmd_intype(ProfWin *window, const char *const command, gchar **args)
{
    _cmd_set_boolean_preference(args[0], command, "Show contact typing", PREF_INTYPE);
    return TRUE;
}

gboolean
cmd_splash(ProfWin *window, const char *const command, gchar **args)
{
    _cmd_set_boolean_preference(args[0], command, "Splash screen", PREF_SPLASH);
    return TRUE;
}

gboolean
cmd_autoconnect(ProfWin *window, const char *const command, gchar **args)
{
    if (strcmp(args[0], "off") == 0) {
        prefs_set_string(PREF_CONNECT_ACCOUNT, NULL);
        cons_show("Autoconnect account disabled.");
    } else if (strcmp(args[0], "set") == 0) {
        prefs_set_string(PREF_CONNECT_ACCOUNT, args[1]);
        cons_show("Autoconnect account set to: %s.", args[1]);
    } else {
        cons_bad_cmd_usage(command);
    }
    return true;
}

gboolean
cmd_chlog(ProfWin *window, const char *const command, gchar **args)
{
    _cmd_set_boolean_preference(args[0], command, "Chat logging", PREF_CHLOG);

    // if set to off, disable history
    if (strcmp(args[0], "off") == 0) {
        prefs_set_boolean(PREF_HISTORY, FALSE);
    }

    return TRUE;
}

gboolean
cmd_grlog(ProfWin *window, const char *const command, gchar **args)
{
    _cmd_set_boolean_preference(args[0], command, "Groupchat logging", PREF_GRLOG);

    return TRUE;
}

gboolean
cmd_history(ProfWin *window, const char *const command, gchar **args)
{
    _cmd_set_boolean_preference(args[0], command, "Chat history", PREF_HISTORY);

    // if set to on, set chlog
    if (strcmp(args[0], "on") == 0) {
        prefs_set_boolean(PREF_CHLOG, TRUE);
    }

    return TRUE;
}

gboolean
cmd_carbons(ProfWin *window, const char *const command, gchar **args)
{
    _cmd_set_boolean_preference(args[0], command, "Message carbons preference", PREF_CARBONS);

    jabber_conn_status_t conn_status = connection_get_status();

    if (conn_status == JABBER_CONNECTED) {
        // enable carbons
        if (strcmp(args[0], "on") == 0) {
            iq_enable_carbons();
        }
        else if (strcmp(args[0], "off") == 0){
            iq_disable_carbons();
        }
    }

    return TRUE;
}

gboolean
cmd_receipts(ProfWin *window, const char *const command, gchar **args)
{
    if (g_strcmp0(args[0], "send") == 0) {
        _cmd_set_boolean_preference(args[1], command, "Send delivery receipts", PREF_RECEIPTS_SEND);
        if (g_strcmp0(args[1], "on") == 0) {
            caps_add_feature(XMPP_FEATURE_RECEIPTS);
        }
        if (g_strcmp0(args[1], "off") == 0) {
            caps_remove_feature(XMPP_FEATURE_RECEIPTS);
        }
    } else if (g_strcmp0(args[0], "request") == 0) {
        _cmd_set_boolean_preference(args[1], command, "Request delivery receipts", PREF_RECEIPTS_REQUEST);
    } else {
        cons_bad_cmd_usage(command);
    }

    return TRUE;
}

gboolean
cmd_away(ProfWin *window, const char *const command, gchar **args)
{
    _update_presence(RESOURCE_AWAY, "away", args);
    return TRUE;
}

gboolean
cmd_online(ProfWin *window, const char *const command, gchar **args)
{
    _update_presence(RESOURCE_ONLINE, "online", args);
    return TRUE;
}

gboolean
cmd_dnd(ProfWin *window, const char *const command, gchar **args)
{
    _update_presence(RESOURCE_DND, "dnd", args);
    return TRUE;
}

gboolean
cmd_chat(ProfWin *window, const char *const command, gchar **args)
{
    _update_presence(RESOURCE_CHAT, "chat", args);
    return TRUE;
}

gboolean
cmd_xa(ProfWin *window, const char *const command, gchar **args)
{
    _update_presence(RESOURCE_XA, "xa", args);
    return TRUE;
}

gboolean
cmd_plugins_sourcepath(ProfWin *window, const char *const command, gchar **args)
{
    if (args[1] == NULL) {
        char *sourcepath = prefs_get_string(PREF_PLUGINS_SOURCEPATH);
        if (sourcepath) {
            cons_show("Current plugins sourcepath: %s", sourcepath);
            prefs_free_string(sourcepath);
        } else {
            cons_show("Plugins sourcepath not currently set.");
        }
        return TRUE;
    }

    if (g_strcmp0(args[1], "clear") == 0) {
        prefs_set_string(PREF_PLUGINS_SOURCEPATH, NULL);
        cons_show("Plugins sourcepath cleared.");
        return TRUE;
    }

    if (g_strcmp0(args[1], "set") == 0) {
        char *path = args[2];
        if (path == NULL) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        // expand ~ to $HOME
        if (path[0] == '~' && path[1] == '/') {
            if (asprintf(&path, "%s/%s", getenv("HOME"), path+2) == -1) {
                return TRUE;
            }
        } else {
            path = strdup(path);
        }

        if (!is_dir(path)) {
            cons_show("Plugins sourcepath must be a directory.");
            return TRUE;
        }

        cons_show("Setting plugins sourcepath: %s", path);
        prefs_set_string(PREF_PLUGINS_SOURCEPATH, path);
        return TRUE;
    }

    cons_bad_cmd_usage(command);
    return TRUE;
}

gboolean
cmd_plugins_install(ProfWin *window, const char *const command, gchar **args)
{
    char *path = args[1];
    if (path == NULL) {
        char* sourcepath = prefs_get_string(PREF_PLUGINS_SOURCEPATH);
        if (sourcepath) {
            path = strdup(sourcepath);
            prefs_free_string(sourcepath);
        } else {
            cons_show("Either a path must be provided or the sourcepath property must be set, see /help plugins");
            return TRUE;
        }
    } else if (path[0] == '~' && path[1] == '/') {
        if (asprintf(&path, "%s/%s", getenv("HOME"), path+2) == -1) {
            return TRUE;
        }
    } else {
        path = strdup(path);
    }

    if (access(path, R_OK) != 0) {
        cons_show("File not found: %s", path);
        free(path);
        return TRUE;
    }

    if (is_regular_file(path)) {
        if (!g_str_has_suffix(path, ".py") && !g_str_has_suffix(path, ".so")) {
            cons_show("Plugins must have one of the following extensions: '.py' '.so'");
            free(path);
            return TRUE;
        }

        gchar *plugin_name = g_path_get_basename(path);
        gboolean result = plugins_install(plugin_name, path);
        if (result) {
            cons_show("Plugin installed: %s", plugin_name);
        } else {
            cons_show("Failed to install plugin: %s", plugin_name);
        }
        g_free(plugin_name);

        free(path);
        return TRUE;
    }

    if (is_dir(path)) {
        PluginsInstallResult* result = plugins_install_all(path);
        if (result->installed || result->failed) {
            if (result->installed) {
                cons_show("");
                cons_show("Installed plugins:");
                GSList *curr = result->installed;
                while (curr) {
                    cons_show("  %s", curr->data);
                    curr = g_slist_next(curr);
                }
            }
            if (result->failed) {
                cons_show("");
                cons_show("Failed installs:");
                GSList *curr = result->failed;
                while (curr) {
                    cons_show("  %s", curr->data);
                    curr = g_slist_next(curr);
                }
            }
        } else {
            cons_show("No plugins found in: %s", path);
        }
        free(path);
        plugins_free_install_result(result);
        return TRUE;
    }

    cons_show("Argument must be a file or directory.");
    return TRUE;
}

gboolean
cmd_plugins_load(ProfWin *window, const char *const command, gchar **args)
{
    if (args[1] == NULL) {
        GSList *loaded = plugins_load_all();
        if (loaded) {
            cons_show("Loaded plugins:");
            GSList *curr = loaded;
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

    gboolean res = plugins_load(args[1]);
    if (res) {
        cons_show("Loaded plugin: %s", args[1]);
    } else {
        cons_show("Failed to load plugin: %s", args[1]);
    }

    return TRUE;
}

gboolean
cmd_plugins_unload(ProfWin *window, const char *const command, gchar **args)
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
cmd_plugins_reload(ProfWin *window, const char *const command, gchar **args)
{
    if (args[1] == NULL) {
        plugins_reload_all();
        cons_show("Reloaded all plugins");
        return TRUE;
    }

    gboolean res = plugins_reload(args[1]);
    if (res) {
        cons_show("Reloaded plugin: %s", args[1]);
    } else {
        cons_show("Failed to reload plugin: %s", args[1]);
    }

    return TRUE;
}

gboolean
cmd_plugins_python_version(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_PYTHON
    const char *version = python_get_version();
    cons_show("Python version:");
    cons_show("%s", version);
#else
    cons_show("This build does not support pytyon plugins.");
#endif
    return TRUE;
}

gboolean
cmd_plugins(ProfWin *window, const char *const command, gchar **args)
{
    GList *plugins = plugins_loaded_list();
    if (plugins == NULL) {
        cons_show("No plugins installed.");
        return TRUE;
    }

    GList *curr = plugins;
    cons_show("Installed plugins:");
    while (curr) {
        cons_show("  %s", curr->data);
        curr = g_list_next(curr);
    }
    g_list_free(plugins);

    return TRUE;
}

gboolean
cmd_pgp(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBGPGME
    if (args[0] == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (strcmp(args[0], "char") == 0) {
        if (args[1] == NULL) {
            cons_bad_cmd_usage(command);
        } else if (strlen(args[1]) != 1) {
            cons_bad_cmd_usage(command);
        } else {
            prefs_set_pgp_char(args[1][0]);
            cons_show("PGP char set to %c.", args[1][0]);
        }
        return TRUE;
    } else if (g_strcmp0(args[0], "log") == 0) {
        char *choice = args[1];
        if (g_strcmp0(choice, "on") == 0) {
            prefs_set_string(PREF_PGP_LOG, "on");
            cons_show("PGP messages will be logged as plaintext.");
            if (!prefs_get_boolean(PREF_CHLOG)) {
                cons_show("Chat logging is currently disabled, use '/chlog on' to enable.");
            }
        } else if (g_strcmp0(choice, "off") == 0) {
            prefs_set_string(PREF_PGP_LOG, "off");
            cons_show("PGP message logging disabled.");
        } else if (g_strcmp0(choice, "redact") == 0) {
            prefs_set_string(PREF_PGP_LOG, "redact");
            cons_show("PGP messages will be logged as '[redacted]'.");
            if (!prefs_get_boolean(PREF_CHLOG)) {
                cons_show("Chat logging is currently disabled, use '/chlog on' to enable.");
            }
        } else {
            cons_bad_cmd_usage(command);
        }
        return TRUE;
    }

    if (g_strcmp0(args[0], "keys") == 0) {
        GHashTable *keys = p_gpg_list_keys();
        if (!keys || g_hash_table_size(keys) == 0) {
            cons_show("No keys found");
            return TRUE;
        }

        cons_show("PGP keys:");
        GList *keylist = g_hash_table_get_keys(keys);
        GList *curr = keylist;
        while (curr) {
            ProfPGPKey *key = g_hash_table_lookup(keys, curr->data);
            cons_show("  %s", key->name);
            cons_show("    ID          : %s", key->id);
            char *format_fp = p_gpg_format_fp_str(key->fp);
            cons_show("    Fingerprint : %s", format_fp);
            free(format_fp);
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

        char *jid = args[1];
        if (!args[1]) {
            cons_bad_cmd_usage(command);
            return TRUE;
        }

        char *keyid = args[2];
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
        GHashTable *pubkeys = p_gpg_pubkeys();
        GList *jids = g_hash_table_get_keys(pubkeys);
        if (!jids) {
            cons_show("No contacts found with PGP public keys assigned.");
            return TRUE;
        }

        cons_show("Assigned PGP public keys:");
        GList *curr = jids;
        while (curr) {
            char *jid = curr->data;
            ProfPGPPubKeyId *pubkeyid = g_hash_table_lookup(pubkeys, jid);
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
        const char *libver = p_gpg_libver();
        if (!libver) {
            cons_show("Could not get libgpgme version");
            return TRUE;
        }

        GString *fullstr = g_string_new("Using libgpgme version ");
        g_string_append(fullstr, libver);
        cons_show("%s", fullstr->str);
        g_string_free(fullstr, TRUE);

        return TRUE;
    }

    if (g_strcmp0(args[0], "start") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You must be connected to start PGP encrpytion.");
            return TRUE;
        }

        if (window->type != WIN_CHAT && args[1] == NULL) {
            cons_show("You must be in a regular chat window to start PGP encrpytion.");
            return TRUE;
        }

        ProfChatWin *chatwin = NULL;

        if (args[1]) {
            char *contact = args[1];
            char *barejid = roster_barejid_from_name(contact);
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
            win_println(window, THEME_DEFAULT, '!', "You must end the OTR session to start PGP encryption.");
            return TRUE;
        }

        if (chatwin->pgp_send) {
            win_println(window, THEME_DEFAULT, '!', "You have already started PGP encryption.");
            return TRUE;
        }

        ProfAccount *account = accounts_get_account(session_get_account_name());
        char *err_str = NULL;
        if (!p_gpg_valid_key(account->pgp_keyid, &err_str)) {
            win_println(window, THEME_DEFAULT, '!', "Invalid PGP key ID %s: %s, cannot start PGP encryption.", account->pgp_keyid, err_str);
            free(err_str);
            account_free(account);
            return TRUE;
        }
        free(err_str);
        account_free(account);

        if (!p_gpg_available(chatwin->barejid)) {
            win_println(window, THEME_DEFAULT, '!', "No PGP key found for %s.", chatwin->barejid);
            return TRUE;
        }

        chatwin->pgp_send = TRUE;
        win_println(window, THEME_DEFAULT, '!', "PGP encryption enabled.");
        return TRUE;
    }

    if (g_strcmp0(args[0], "end") == 0) {
        jabber_conn_status_t conn_status = connection_get_status();
        if (conn_status != JABBER_CONNECTED) {
            cons_show("You are not currently connected.");
            return TRUE;
        }

        if (window->type != WIN_CHAT) {
            cons_show("You must be in a regular chat window to end PGP encrpytion.");
            return TRUE;
        }

        ProfChatWin *chatwin = (ProfChatWin*)window;
        if (chatwin->pgp_send == FALSE) {
            win_println(window, THEME_DEFAULT, '!', "PGP encryption is not currently enabled.");
            return TRUE;
        }

        chatwin->pgp_send = FALSE;
        win_println(window, THEME_DEFAULT, '!', "PGP encryption disabled.");
        return TRUE;
    }

    cons_bad_cmd_usage(command);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with PGP support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_char(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBOTR
    if (args[1] == NULL) {
        cons_bad_cmd_usage(command);
    } else if (strlen(args[1]) != 1) {
        cons_bad_cmd_usage(command);
    } else {
        prefs_set_otr_char(args[1][0]);
        cons_show("OTR char set to %c.", args[1][0]);
    }
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_log(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBOTR
    char *choice = args[1];
    if (g_strcmp0(choice, "on") == 0) {
        prefs_set_string(PREF_OTR_LOG, "on");
        cons_show("OTR messages will be logged as plaintext.");
        if (!prefs_get_boolean(PREF_CHLOG)) {
            cons_show("Chat logging is currently disabled, use '/chlog on' to enable.");
        }
    } else if (g_strcmp0(choice, "off") == 0) {
        prefs_set_string(PREF_OTR_LOG, "off");
        cons_show("OTR message logging disabled.");
    } else if (g_strcmp0(choice, "redact") == 0) {
        prefs_set_string(PREF_OTR_LOG, "redact");
        cons_show("OTR messages will be logged as '[redacted]'.");
        if (!prefs_get_boolean(PREF_CHLOG)) {
            cons_show("Chat logging is currently disabled, use '/chlog on' to enable.");
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
cmd_otr_libver(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBOTR
    char *version = otr_libotr_version();
    cons_show("Using libotr version %s", version);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_policy(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBOTR
    if (args[1] == NULL) {
        char *policy = prefs_get_string(PREF_OTR_POLICY);
        cons_show("OTR policy is now set to: %s", policy);
        prefs_free_string(policy);
        return TRUE;
    }

    char *choice = args[1];
    if ((g_strcmp0(choice, "manual") != 0) &&
            (g_strcmp0(choice, "opportunistic") != 0) &&
            (g_strcmp0(choice, "always") != 0)) {
        cons_show("OTR policy can be set to: manual, opportunistic or always.");
        return TRUE;
    }

    char *contact = args[2];
    if (contact == NULL) {
        prefs_set_string(PREF_OTR_POLICY, choice);
        cons_show("OTR policy is now set to: %s", choice);
        return TRUE;
    }

    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected to set the OTR policy for a contact.");
        return TRUE;
    }

    char *contact_jid = roster_barejid_from_name(contact);
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
cmd_otr_gen(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    ProfAccount *account = accounts_get_account(session_get_account_name());
    otr_keygen(account);
    account_free(account);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_myfp(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (!otr_key_loaded()) {
        win_println(window, THEME_DEFAULT, '!', "You have not generated or loaded a private key, use '/otr gen'");
        return TRUE;
    }

    char *fingerprint = otr_get_my_fingerprint();
    win_println(window, THEME_DEFAULT, '!', "Your OTR fingerprint: %s", fingerprint);
    free(fingerprint);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_theirfp(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (window->type != WIN_CHAT) {
        win_println(window, THEME_DEFAULT, '-', "You must be in a regular chat window to view a recipient's fingerprint.");
        return TRUE;
    }

    ProfChatWin *chatwin = (ProfChatWin*)window;
    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
    if (chatwin->is_otr == FALSE) {
        win_println(window, THEME_DEFAULT, '!', "You are not currently in an OTR session.");
        return TRUE;
    }

    char *fingerprint = otr_get_their_fingerprint(chatwin->barejid);
    win_println(window, THEME_DEFAULT, '!', "%s's OTR fingerprint: %s", chatwin->barejid, fingerprint);
    free(fingerprint);
    return TRUE;
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_start(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    // recipient supplied
    if (args[1]) {
        char *contact = args[1];
        char *barejid = roster_barejid_from_name(contact);
        if (barejid == NULL) {
            barejid = contact;
        }

        ProfChatWin *chatwin = wins_get_chat(barejid);
        if (!chatwin) {
            chatwin = chatwin_new(barejid);
        }
        ui_focus_win((ProfWin*)chatwin);

        if (chatwin->pgp_send) {
            win_println(window, THEME_DEFAULT, '!', "You must disable PGP encryption before starting an OTR session.");
            return TRUE;
        }

        if (chatwin->is_otr) {
            win_println(window, THEME_DEFAULT, '!', "You are already in an OTR session.");
            return TRUE;
        }

        if (!otr_key_loaded()) {
            win_println(window, THEME_DEFAULT, '!', "You have not generated or loaded a private key, use '/otr gen'");
            return TRUE;
        }

        if (!otr_is_secure(barejid)) {
            char *otr_query_message = otr_start_query();
            char *id = message_send_chat_otr(barejid, otr_query_message, FALSE);
            free(id);
            return TRUE;
        }

        chatwin_otr_secured(chatwin, otr_is_trusted(barejid));
        return TRUE;

    // no recipient, use current chat
    } else {
        if (window->type != WIN_CHAT) {
            win_println(window, THEME_DEFAULT, '-', "You must be in a regular chat window to start an OTR session.");
            return TRUE;
        }

        ProfChatWin *chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        if (chatwin->pgp_send) {
            win_println(window, THEME_DEFAULT, '!', "You must disable PGP encryption before starting an OTR session.");
            return TRUE;
        }

        if (chatwin->is_otr) {
            win_println(window, THEME_DEFAULT, '!', "You are already in an OTR session.");
            return TRUE;
        }

        if (!otr_key_loaded()) {
            win_println(window, THEME_DEFAULT, '!', "You have not generated or loaded a private key, use '/otr gen'");
            return TRUE;
        }

        char *otr_query_message = otr_start_query();
        char *id = message_send_chat_otr(chatwin->barejid, otr_query_message, FALSE);
        free(id);
        return TRUE;
    }
#else
    cons_show("This version of Profanity has not been built with OTR support enabled");
    return TRUE;
#endif
}

gboolean
cmd_otr_end(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (window->type != WIN_CHAT) {
        win_println(window, THEME_DEFAULT, '-', "You must be in a regular chat window to use OTR.");
        return TRUE;
    }

    ProfChatWin *chatwin = (ProfChatWin*)window;
    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
    if (chatwin->is_otr == FALSE) {
        win_println(window, THEME_DEFAULT, '!', "You are not currently in an OTR session.");
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
cmd_otr_trust(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (window->type != WIN_CHAT) {
        win_println(window, THEME_DEFAULT, '-', "You must be in an OTR session to trust a recipient.");
        return TRUE;
    }

    ProfChatWin *chatwin = (ProfChatWin*)window;
    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
    if (chatwin->is_otr == FALSE) {
        win_println(window, THEME_DEFAULT, '!', "You are not currently in an OTR session.");
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
cmd_otr_untrust(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (window->type != WIN_CHAT) {
        win_println(window, THEME_DEFAULT, '-', "You must be in an OTR session to untrust a recipient.");
        return TRUE;
    }

    ProfChatWin *chatwin = (ProfChatWin*)window;
    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
    if (chatwin->is_otr == FALSE) {
        win_println(window, THEME_DEFAULT, '!', "You are not currently in an OTR session.");
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
cmd_otr_secret(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (window->type != WIN_CHAT) {
        win_println(window, THEME_DEFAULT, '-', "You must be in an OTR session to trust a recipient.");
        return TRUE;
    }

    ProfChatWin *chatwin = (ProfChatWin*)window;
    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
    if (chatwin->is_otr == FALSE) {
        win_println(window, THEME_DEFAULT, '!', "You are not currently in an OTR session.");
        return TRUE;
    }

    char *secret = args[1];
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
cmd_otr_question(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    char *question = args[1];
    char *answer = args[2];
    if (question == NULL || answer == NULL) {
        cons_bad_cmd_usage(command);
        return TRUE;
    }

    if (window->type != WIN_CHAT) {
        win_println(window, THEME_DEFAULT, '-', "You must be in an OTR session to trust a recipient.");
        return TRUE;
    }

    ProfChatWin *chatwin = (ProfChatWin*)window;
    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
    if (chatwin->is_otr == FALSE) {
        win_println(window, THEME_DEFAULT, '!', "You are not currently in an OTR session.");
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
cmd_otr_answer(ProfWin *window, const char *const command, gchar **args)
{
#ifdef HAVE_LIBOTR
    if (connection_get_status() != JABBER_CONNECTED) {
        cons_show("You must be connected with an account to load OTR information.");
        return TRUE;
    }

    if (window->type != WIN_CHAT) {
        win_println(window, THEME_DEFAULT, '-', "You must be in an OTR session to trust a recipient.");
        return TRUE;
    }

    ProfChatWin *chatwin = (ProfChatWin*)window;
    assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
    if (chatwin->is_otr == FALSE) {
        win_println(window, THEME_DEFAULT, '!', "You are not currently in an OTR session.");
        return TRUE;
    }

    char *answer = args[1];
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
cmd_encwarn(ProfWin *window, const char *const command, gchar **args)
{
    _cmd_set_boolean_preference(args[0], command, "Encryption warning message", PREF_ENC_WARN);
    return TRUE;
}

static gboolean
_cmd_execute(ProfWin *window, const char *const command, const char *const inp)
{
    if (g_str_has_prefix(command, "/field") && window->type == WIN_MUC_CONFIG) {
        gboolean result = FALSE;
        gchar **args = parse_args_with_freetext(inp, 1, 2, &result);
        if (!result) {
            win_println(window, THEME_DEFAULT, '!', "Invalid command, see /form help");
            result = TRUE;
        } else {
            gchar **tokens = g_strsplit(inp, " ", 2);
            char *field = tokens[0] + 1;
            result = cmd_form_field(window, field, args);
            g_strfreev(tokens);
        }

        g_strfreev(args);
        return result;
    }

    Command *cmd = cmd_get(command);
    gboolean result = FALSE;

    if (cmd) {
        gchar **args = cmd->parser(inp, cmd->min_args, cmd->max_args, &result);
        if (result == FALSE) {
            ui_invalid_command_usage(cmd->cmd, cmd->setting_func);
            return TRUE;
        }
        if (args[0] && cmd->sub_funcs[0][0]) {
            int i = 0;
            while (cmd->sub_funcs[i][0]) {
                if (g_strcmp0(args[0], (char*)cmd->sub_funcs[i][0]) == 0) {
                    gboolean (*func)(ProfWin *window, const char *const command, gchar **args) = cmd->sub_funcs[i][1];
                    gboolean result = func(window, command, args);
                    g_strfreev(args);
                    return result;
                }
                i++;
            }
        }
        if (!cmd->func) {
            ui_invalid_command_usage(cmd->cmd, cmd->setting_func);
            return TRUE;
        }
        gboolean result = cmd->func(window, command, args);
        g_strfreev(args);
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
_cmd_execute_default(ProfWin *window, const char *inp)
{
    // handle escaped commands - treat as normal message
    if (g_str_has_prefix(inp, "//")) {
        inp++;

    // handle unknown commands
    } else if ((inp[0] == '/') && (!g_str_has_prefix(inp, "/me "))) {
        cons_show("Unknown command: %s", inp);
        cons_alert();
        return TRUE;
    }

    // handle non commands in non chat or plugin windows
    if (window->type != WIN_CHAT && window->type != WIN_MUC && window->type != WIN_PRIVATE && window->type != WIN_PLUGIN && window->type != WIN_XML) {
        cons_show("Unknown command: %s", inp);
        return TRUE;
    }

    // handle plugin window
    if (window->type == WIN_PLUGIN) {
        ProfPluginWin *pluginwin = (ProfPluginWin*)window;
        plugins_win_process_line(pluginwin->tag, inp);
        return TRUE;
    }

    jabber_conn_status_t status = connection_get_status();
    if (status != JABBER_CONNECTED) {
        win_println(window, THEME_DEFAULT, '-', "You are not currently connected.");
        return TRUE;
    }

    switch (window->type) {
    case WIN_CHAT:
    {
        ProfChatWin *chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        cl_ev_send_msg(chatwin, inp, NULL);
        break;
    }
    case WIN_PRIVATE:
    {
        ProfPrivateWin *privatewin = (ProfPrivateWin*)window;
        assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
        cl_ev_send_priv_msg(privatewin, inp, NULL);
        break;
    }
    case WIN_MUC:
    {
        ProfMucWin *mucwin = (ProfMucWin*)window;
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
_cmd_execute_alias(ProfWin *window, const char *const inp, gboolean *ran)
{
    if (inp[0] != '/') {
        ran = FALSE;
        return TRUE;
    }

    char *alias = strdup(inp+1);
    char *value = prefs_get_alias(alias);
    free(alias);
    if (value) {
        *ran = TRUE;
        gboolean result = cmd_process_input(window, value);
        prefs_free_string(value);
        return result;
    }

    *ran = FALSE;
    return TRUE;
}

// helper function for status change commands
static void
_update_presence(const resource_presence_t resource_presence,
    const char *const show, gchar **args)
{
    char *msg = NULL;
    int num_args = g_strv_length(args);
    if (num_args == 1) {
        msg = args[0];
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

// helper function for boolean preference commands
static void
_cmd_set_boolean_preference(gchar *arg, const char *const command,
    const char *const display, preference_t pref)
{
    GString *enabled = g_string_new(display);
    g_string_append(enabled, " enabled.");

    GString *disabled = g_string_new(display);
    g_string_append(disabled, " disabled.");

    if (arg == NULL) {
        cons_bad_cmd_usage(command);
    } else if (strcmp(arg, "on") == 0) {
        cons_show(enabled->str);
        prefs_set_boolean(pref, TRUE);
    } else if (strcmp(arg, "off") == 0) {
        cons_show(disabled->str);
        prefs_set_boolean(pref, FALSE);
    } else {
        cons_bad_cmd_usage(command);
    }

    g_string_free(enabled, TRUE);
    g_string_free(disabled, TRUE);
}
