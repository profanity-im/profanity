/*
 * session.c
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_LIBMESODE
#include <mesode.h>
#endif
#ifdef HAVE_LIBSTROPHE
#include <strophe.h>
#endif

#include "chat_session.h"
#include "common.h"
#include "config/preferences.h"
#include "jid.h"
#include "log.h"
#include "muc.h"
#include "plugins/plugins.h"
#include "profanity.h"
#include "event/server_events.h"
#include "xmpp/bookmark.h"
#include "xmpp/blocking.h"
#include "xmpp/connection.h"
#include "xmpp/capabilities.h"
#include "xmpp/session.h"
#include "xmpp/iq.h"
#include "xmpp/message.h"
#include "xmpp/presence.h"
#include "xmpp/roster.h"
#include "xmpp/stanza.h"
#include "xmpp/xmpp.h"

static GHashTable *available_resources;
static GSList *disco_items;

// for auto reconnect
static struct {
    char *name;
    char *passwd;
} saved_account;

static struct {
    char *name;
    char *jid;
    char *passwd;
    char *altdomain;
    int port;
    char *tls_policy;
} saved_details;

static GTimer *reconnect_timer;

static jabber_conn_status_t _jabber_connect(const char *const fulljid, const char *const passwd,
    const char *const altdomain, int port, const char *const tls_policy);

static void _jabber_reconnect(void);

void _connection_free_saved_account(void);
void _connection_free_saved_details(void);
void _connection_free_session_data(void);

static void
_info_destroy(DiscoInfo *info)
{
    if (info) {
        free(info->item);
        if (info->features) {
            g_hash_table_destroy(info->features);
        }
        free(info);
    }
}

void
jabber_init(void)
{
    log_info("Initialising XMPP");
    connection_init();
    presence_sub_requests_init();
    caps_init();
    available_resources = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)resource_destroy);
    disco_items = NULL;
    xmpp_initialize();
}

jabber_conn_status_t
jabber_connect_with_account(const ProfAccount *const account)
{
    assert(account != NULL);

    log_info("Connecting using account: %s", account->name);

    // save account name and password for reconnect
    if (saved_account.name) {
        free(saved_account.name);
    }
    saved_account.name = strdup(account->name);
    if (saved_account.passwd) {
        free(saved_account.passwd);
    }
    saved_account.passwd = strdup(account->password);

    // connect with fulljid
    Jid *jidp = jid_create_from_bare_and_resource(account->jid, account->resource);
    jabber_conn_status_t result =
        _jabber_connect(jidp->fulljid, account->password, account->server, account->port, account->tls_policy);
    jid_destroy(jidp);

    return result;
}

jabber_conn_status_t
jabber_connect_with_details(const char *const jid, const char *const passwd, const char *const altdomain,
    const int port, const char *const tls_policy)
{
    assert(jid != NULL);
    assert(passwd != NULL);

    // save details for reconnect, remember name for account creating on success
    saved_details.name = strdup(jid);
    saved_details.passwd = strdup(passwd);
    if (altdomain) {
        saved_details.altdomain = strdup(altdomain);
    } else {
        saved_details.altdomain = NULL;
    }
    if (port != 0) {
        saved_details.port = port;
    } else {
        saved_details.port = 0;
    }
    if (tls_policy) {
        saved_details.tls_policy = strdup(tls_policy);
    } else {
        saved_details.tls_policy = NULL;
    }

    // use 'profanity' when no resourcepart in provided jid
    Jid *jidp = jid_create(jid);
    if (jidp->resourcepart == NULL) {
        jid_destroy(jidp);
        jidp = jid_create_from_bare_and_resource(jid, "profanity");
        saved_details.jid = strdup(jidp->fulljid);
    } else {
        saved_details.jid = strdup(jid);
    }
    jid_destroy(jidp);

    // connect with fulljid
    log_info("Connecting without account, JID: %s", saved_details.jid);

    return _jabber_connect(
        saved_details.jid,
        passwd,
        saved_details.altdomain,
        saved_details.port,
        saved_details.tls_policy);
}

void
connection_autoping_fail(void)
{
    if (connection_get_status() == JABBER_CONNECTED) {
        log_info("Closing connection");
        char *account_name = jabber_get_account_name();
        const char *fulljid = jabber_get_fulljid();
        plugins_on_disconnect(account_name, fulljid);
        accounts_set_last_activity(jabber_get_account_name());
        connection_set_status(JABBER_DISCONNECTING);
        xmpp_disconnect(connection_get_conn());

        while (connection_get_status() == JABBER_DISCONNECTING) {
            jabber_process_events(10);
        }

        connection_free_conn();
        connection_free_ctx();
    }

    connection_free_presence_msg();
    connection_free_domain();

    connection_set_status(JABBER_DISCONNECTED);

    jabber_lost_connection();
}

void
jabber_disconnect(void)
{
    // if connected, send end stream and wait for response
    if (connection_get_status() == JABBER_CONNECTED) {
        char *account_name = jabber_get_account_name();
        const char *fulljid = jabber_get_fulljid();
        plugins_on_disconnect(account_name, fulljid);
        log_info("Closing connection");
        accounts_set_last_activity(jabber_get_account_name());
        connection_set_status(JABBER_DISCONNECTING);
        xmpp_disconnect(connection_get_conn());

        while (connection_get_status() == JABBER_DISCONNECTING) {
            jabber_process_events(10);
        }
        _connection_free_saved_account();
        _connection_free_saved_details();
        _connection_free_session_data();

        connection_free_conn();
        connection_free_ctx();
    }

    connection_free_presence_msg();
    connection_free_domain();

    connection_set_status(JABBER_STARTED);
}

void
jabber_shutdown(void)
{
    _connection_free_saved_account();
    _connection_free_saved_details();
    _connection_free_session_data();
    xmpp_shutdown();
    connection_free_log();
}

void
jabber_process_events(int millis)
{
    int reconnect_sec;

    jabber_conn_status_t conn_status = connection_get_status();
    switch (conn_status)
    {
        case JABBER_CONNECTED:
        case JABBER_CONNECTING:
        case JABBER_DISCONNECTING:
            xmpp_run_once(connection_get_ctx(), millis);
            break;
        case JABBER_DISCONNECTED:
            reconnect_sec = prefs_get_reconnect();
            if ((reconnect_sec != 0) && reconnect_timer) {
                int elapsed_sec = g_timer_elapsed(reconnect_timer, NULL);
                if (elapsed_sec > reconnect_sec) {
                    _jabber_reconnect();
                }
            }
            break;
        default:
            break;
    }
}

GList*
jabber_get_available_resources(void)
{
    return g_hash_table_get_values(available_resources);
}

GSList*
connection_get_disco_items(void)
{
    return (disco_items);
}

gboolean
jabber_service_supports(const char *const feature)
{
    DiscoInfo *disco_info;
    while (disco_items) {
        disco_info = disco_items->data;
        if (g_hash_table_lookup_extended(disco_info->features, feature, NULL, NULL)) {
            return TRUE;
        }
        disco_items = g_slist_next(disco_items);
    }

    return FALSE;
}

void
connection_set_disco_items(GSList *_disco_items)
{
    disco_items = _disco_items;
}

const char*
jabber_get_fulljid(void)
{
    return xmpp_conn_get_jid(connection_get_conn());
}

char*
jabber_get_account_name(void)
{
    return saved_account.name;
}

char*
jabber_create_uuid(void)
{
    return xmpp_uuid_gen(connection_get_ctx());
}

void
jabber_free_uuid(char *uuid)
{
    if (uuid) {
        xmpp_free(connection_get_ctx(), uuid);
    }
}

void
connection_add_available_resource(Resource *resource)
{
    g_hash_table_replace(available_resources, strdup(resource->name), resource);
}

void
connection_remove_available_resource(const char *const resource)
{
    g_hash_table_remove(available_resources, resource);
}

void
_connection_free_saved_account(void)
{
    FREE_SET_NULL(saved_account.name);
    FREE_SET_NULL(saved_account.passwd);
}

void
_connection_free_saved_details(void)
{
    FREE_SET_NULL(saved_details.name);
    FREE_SET_NULL(saved_details.jid);
    FREE_SET_NULL(saved_details.passwd);
    FREE_SET_NULL(saved_details.altdomain);
    FREE_SET_NULL(saved_details.tls_policy);
}

void
_connection_free_session_data(void)
{
    g_slist_free_full(disco_items, (GDestroyNotify)_info_destroy);
    disco_items = NULL;
    g_hash_table_remove_all(available_resources);
    chat_sessions_clear();
    presence_clear_sub_requests();
}

void
jabber_login_success(int secured)
{
    // logged in with account
    if (saved_account.name) {
        log_debug("Connection handler: logged in with account name: %s", saved_account.name);
        sv_ev_login_account_success(saved_account.name, secured);

    // logged in without account, use details to create new account
    } else {
        log_debug("Connection handler: logged in with jid: %s", saved_details.name);
        accounts_add(saved_details.name, saved_details.altdomain, saved_details.port, saved_details.tls_policy);
        accounts_set_jid(saved_details.name, saved_details.jid);

        sv_ev_login_account_success(saved_details.name, secured);
        saved_account.name = strdup(saved_details.name);
        saved_account.passwd = strdup(saved_details.passwd);

        _connection_free_saved_details();
    }

    Jid *my_jid = jid_create(jabber_get_fulljid());
    connection_set_domain(my_jid->domainpart);
    jid_destroy(my_jid);

    chat_sessions_init();

    message_handlers_init();
    presence_handlers_init();
    iq_handlers_init();

    roster_request();
    bookmark_request();
    blocking_request();

    // items discovery
    DiscoInfo *info = malloc(sizeof(struct disco_info_t));
    info->item = strdup(connection_get_domain());
    info->features = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
    disco_items = g_slist_append(disco_items, info);
    iq_disco_info_request_onconnect(info->item);
    iq_disco_items_request_onconnect(connection_get_domain());

    if (prefs_get_boolean(PREF_CARBONS)){
        iq_enable_carbons();
    }

    if ((prefs_get_reconnect() != 0) && reconnect_timer) {
        g_timer_destroy(reconnect_timer);
        reconnect_timer = NULL;
    }
}

void
jabber_login_failed(void)
{
    if (reconnect_timer == NULL) {
        log_debug("Connection handler: No reconnect timer");
        sv_ev_failed_login();
        _connection_free_saved_account();
        _connection_free_saved_details();
        _connection_free_session_data();
    } else {
        log_debug("Connection handler: Restarting reconnect timer");
        if (prefs_get_reconnect() != 0) {
            g_timer_start(reconnect_timer);
        }
        // free resources but leave saved_user untouched
        _connection_free_session_data();
    }
}

#ifdef HAVE_LIBMESODE
TLSCertificate*
jabber_get_tls_peer_cert(void)
{
    xmpp_tlscert_t *xmpptlscert = xmpp_conn_tls_peer_cert(connection_get_conn());
    int version = xmpp_conn_tlscert_version(xmpptlscert);
    char *serialnumber = xmpp_conn_tlscert_serialnumber(xmpptlscert);
    char *subjectname = xmpp_conn_tlscert_subjectname(xmpptlscert);
    char *issuername = xmpp_conn_tlscert_issuername(xmpptlscert);
    char *fingerprint = xmpp_conn_tlscert_fingerprint(xmpptlscert);
    char *notbefore = xmpp_conn_tlscert_notbefore(xmpptlscert);
    char *notafter = xmpp_conn_tlscert_notafter(xmpptlscert);
    char *key_alg = xmpp_conn_tlscert_key_algorithm(xmpptlscert);
    char *signature_alg = xmpp_conn_tlscert_signature_algorithm(xmpptlscert);

    TLSCertificate *cert = tlscerts_new(fingerprint, version, serialnumber, subjectname, issuername, notbefore,
        notafter, key_alg, signature_alg);

    xmpp_conn_free_tlscert(connection_get_ctx(), xmpptlscert);

    return cert;
}
#endif

gboolean
jabber_conn_is_secured(void)
{
    if (connection_get_status() == JABBER_CONNECTED) {
        return xmpp_conn_is_secured(connection_get_conn()) == 0 ? FALSE : TRUE;
    } else {
        return FALSE;
    }
}

gboolean
jabber_send_stanza(const char *const stanza)
{
    if (connection_get_status() != JABBER_CONNECTED) {
        return FALSE;
    } else {
        xmpp_send_raw_string(connection_get_conn(), "%s", stanza);
        return TRUE;
    }
}

static jabber_conn_status_t
_jabber_connect(const char *const fulljid, const char *const passwd, const char *const altdomain, int port,
    const char *const tls_policy)
{
    assert(fulljid != NULL);
    assert(passwd != NULL);

    Jid *jid = jid_create(fulljid);

    if (jid == NULL) {
        log_error("Malformed JID not able to connect: %s", fulljid);
        connection_set_status(JABBER_DISCONNECTED);
        return connection_get_status();
    } else if (jid->fulljid == NULL) {
        log_error("Full JID required to connect, received: %s", fulljid);
        connection_set_status(JABBER_DISCONNECTED);
        jid_destroy(jid);
        return connection_get_status();
    }

    jid_destroy(jid);

    log_info("Connecting as %s", fulljid);
    char *cert_path = prefs_get_string(PREF_TLS_CERTPATH);
    jabber_conn_status_t status = connection_connect(fulljid, passwd, altdomain, port, tls_policy, cert_path);
    prefs_free_string(cert_path);

    return status;
}

static void
_jabber_reconnect(void)
{
    // reconnect with account.
    ProfAccount *account = accounts_get_account(saved_account.name);

    if (account == NULL) {
        log_error("Unable to reconnect, account no longer exists: %s", saved_account.name);
    } else {
        char *fulljid = create_fulljid(account->jid, account->resource);
        log_debug("Attempting reconnect with account %s", account->name);
        _jabber_connect(fulljid, saved_account.passwd, account->server, account->port, account->tls_policy);
        free(fulljid);
        g_timer_start(reconnect_timer);
    }
}

void
jabber_lost_connection(void)
{
    sv_ev_lost_connection();
    if (prefs_get_reconnect() != 0) {
        assert(reconnect_timer == NULL);
        reconnect_timer = g_timer_new();
    } else {
        _connection_free_saved_account();
        _connection_free_saved_details();
    }
    _connection_free_session_data();
}
