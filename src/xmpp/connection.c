/*
 * connection.c
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_LIBMESODE
#include <mesode.h>
#endif
#ifdef HAVE_LIBSTROPHE
#include <strophe.h>
#endif

#include "log.h"
#include "config/preferences.h"
#include "event/server_events.h"
#include "xmpp/connection.h"
#include "xmpp/session.h"

typedef struct prof_conn_t {
    xmpp_log_t *xmpp_log;
    xmpp_ctx_t *xmpp_ctx;
    xmpp_conn_t *xmpp_conn;
    jabber_conn_status_t conn_status;
    char *presence_message;
    int priority;
    char *domain;
    GHashTable *available_resources;
} ProfConnection;

static ProfConnection conn;

static xmpp_log_t* _xmpp_get_file_logger(void);
static xmpp_log_level_t _get_xmpp_log_level(void);
static void _xmpp_file_logger(void *const userdata, const xmpp_log_level_t level, const char *const area, const char *const msg);
static log_level_t _get_log_level(const xmpp_log_level_t xmpp_level);
static void _connection_handler(xmpp_conn_t *const xmpp_conn, const xmpp_conn_event_t status, const int error,
    xmpp_stream_error_t *const stream_error, void *const userdata);

#ifdef HAVE_LIBMESODE
static int _connection_certfail_cb(xmpp_tlscert_t *xmpptlscert, const char *const errormsg);
#endif

void connection_init(void)
{
    conn.conn_status = JABBER_STARTED;
    conn.presence_message = NULL;
    conn.xmpp_conn = NULL;
    conn.xmpp_ctx = NULL;
    conn.domain = NULL;
    conn.available_resources = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)resource_destroy);
}

jabber_conn_status_t
connection_connect(const char *const fulljid, const char *const passwd, const char *const altdomain, int port,
    const char *const tls_policy)
{
    assert(fulljid != NULL);
    assert(passwd != NULL);

    Jid *jid = jid_create(fulljid);
    if (jid == NULL) {
        log_error("Malformed JID not able to connect: %s", fulljid);
        conn.conn_status = JABBER_DISCONNECTED;
        return conn.conn_status;
    } else if (jid->fulljid == NULL) {
        log_error("Full JID required to connect, received: %s", fulljid);
        conn.conn_status = JABBER_DISCONNECTED;
        jid_destroy(jid);
        return conn.conn_status;
    }
    jid_destroy(jid);

    log_info("Connecting as %s", fulljid);

    if (conn.xmpp_log) {
        free(conn.xmpp_log);
    }
    conn.xmpp_log = _xmpp_get_file_logger();

    if (conn.xmpp_conn) {
        xmpp_conn_release(conn.xmpp_conn);
    }
    if (conn.xmpp_ctx) {
        xmpp_ctx_free(conn.xmpp_ctx);
    }
    conn.xmpp_ctx = xmpp_ctx_new(NULL, conn.xmpp_log);
    if (conn.xmpp_ctx == NULL) {
        log_warning("Failed to get libstrophe ctx during connect");
        return JABBER_DISCONNECTED;
    }
    conn.xmpp_conn = xmpp_conn_new(conn.xmpp_ctx);
    if (conn.xmpp_conn == NULL) {
        log_warning("Failed to get libstrophe conn during connect");
        return JABBER_DISCONNECTED;
    }
    xmpp_conn_set_jid(conn.xmpp_conn, fulljid);
    xmpp_conn_set_pass(conn.xmpp_conn, passwd);

    if (!tls_policy || (g_strcmp0(tls_policy, "force") == 0)) {
        xmpp_conn_set_flags(conn.xmpp_conn, XMPP_CONN_FLAG_MANDATORY_TLS);
    } else if (g_strcmp0(tls_policy, "disable") == 0) {
        xmpp_conn_set_flags(conn.xmpp_conn, XMPP_CONN_FLAG_DISABLE_TLS);
    }

#ifdef HAVE_LIBMESODE
    char *cert_path = prefs_get_string(PREF_TLS_CERTPATH);
    if (cert_path) {
        xmpp_conn_tlscert_path(conn.xmpp_conn, cert_path);
    }
    prefs_free_string(cert_path);

    int connect_status = xmpp_connect_client(
        conn.xmpp_conn,
        altdomain,
        port,
        _connection_certfail_cb,
        _connection_handler,
        conn.xmpp_ctx);
#else
    int connect_status = xmpp_connect_client(
        conn.xmpp_conn,
        altdomain,
        port,
        _connection_handler,
        conn.xmpp_ctx);
#endif

    if (connect_status == 0) {
        conn.conn_status = JABBER_CONNECTING;
    } else {
        conn.conn_status = JABBER_DISCONNECTED;
    }

    return conn.conn_status;
}

jabber_conn_status_t
connection_get_status(void)
{
    return conn.conn_status;
}

void
connection_set_status(jabber_conn_status_t status)
{
    conn.conn_status = status;
}

xmpp_conn_t*
connection_get_conn(void)
{
    return conn.xmpp_conn;
}

xmpp_ctx_t*
connection_get_ctx(void)
{
    return conn.xmpp_ctx;
}

const char*
connection_get_fulljid(void)
{
    return xmpp_conn_get_jid(conn.xmpp_conn);
}

GList*
connection_get_available_resources(void)
{
    return g_hash_table_get_values(conn.available_resources);
}

void
connection_add_available_resource(Resource *resource)
{
    g_hash_table_replace(conn.available_resources, strdup(resource->name), resource);
}

void
connection_remove_available_resource(const char *const resource)
{
    g_hash_table_remove(conn.available_resources, resource);
}

void
connection_remove_all_available_resources(void)
{
    g_hash_table_remove_all(conn.available_resources);
}

char*
connection_create_uuid(void)
{
    return xmpp_uuid_gen(conn.xmpp_ctx);
}

void
connection_free_uuid(char *uuid)
{
    if (uuid) {
        xmpp_free(conn.xmpp_ctx, uuid);
    }
}

char *
connection_get_domain(void)
{
    return conn.domain;
}

char *
connection_get_presence_msg(void)
{
    return conn.presence_message;
}

void
connection_free_conn(void)
{
    if (conn.xmpp_conn) {
        xmpp_conn_release(conn.xmpp_conn);
        conn.xmpp_conn = NULL;
    }
}

void
connection_free_ctx(void)
{
    if (conn.xmpp_ctx) {
        xmpp_ctx_free(conn.xmpp_ctx);
        conn.xmpp_ctx = NULL;
    }
}

void
connection_free_presence_msg(void)
{
    FREE_SET_NULL(conn.presence_message);
}

void
connection_set_presence_msg(const char *const message)
{
    FREE_SET_NULL(conn.presence_message);
    if (message) {
        conn.presence_message = strdup(message);
    }
}

void
connection_free_domain(void)
{
    FREE_SET_NULL(conn.domain);
}

void
connection_free_log(void)
{
    free(conn.xmpp_log);
    conn.xmpp_log = NULL;
}

void
connection_set_priority(const int priority)
{
    conn.priority = priority;
}

void
connection_set_domain(char *domain)
{
    conn.domain = strdup(domain);
}

#ifdef HAVE_LIBMESODE
TLSCertificate*
connection_get_tls_peer_cert(void)
{
    xmpp_tlscert_t *xmpptlscert = xmpp_conn_tls_peer_cert(conn.xmpp_conn);
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

    xmpp_conn_free_tlscert(conn.xmpp_ctx, xmpptlscert);

    return cert;
}
#endif

gboolean
connection_is_secured(void)
{
    if (conn.conn_status == JABBER_CONNECTED) {
        return xmpp_conn_is_secured(conn.xmpp_conn) == 0 ? FALSE : TRUE;
    } else {
        return FALSE;
    }
}

gboolean
connection_send_stanza(const char *const stanza)
{
    if (conn.conn_status != JABBER_CONNECTED) {
        return FALSE;
    } else {
        xmpp_send_raw_string(conn.xmpp_conn, "%s", stanza);
        return TRUE;
    }
}

static void
_connection_handler(xmpp_conn_t *const xmpp_conn, const xmpp_conn_event_t status, const int error,
    xmpp_stream_error_t *const stream_error, void *const userdata)
{
    // login success
    if (status == XMPP_CONN_CONNECT) {
        log_debug("Connection handler: XMPP_CONN_CONNECT");
        conn.conn_status = JABBER_CONNECTED;

        session_login_success(connection_is_secured());

    } else if (status == XMPP_CONN_DISCONNECT) {
        log_debug("Connection handler: XMPP_CONN_DISCONNECT");

        // lost connection for unknown reason
        if (conn.conn_status == JABBER_CONNECTED) {
            log_debug("Connection handler: Lost connection for unknown reason");
            session_lost_connection();

        // login attempt failed
        } else if (conn.conn_status != JABBER_DISCONNECTING) {
            log_debug("Connection handler: Login failed");
            session_login_failed();
        }

        // close stream response from server after disconnect is handled too
        conn.conn_status = JABBER_DISCONNECTED;
    } else if (status == XMPP_CONN_FAIL) {
        log_debug("Connection handler: XMPP_CONN_FAIL");
    } else {
        log_error("Connection handler: Unknown status");
    }
}

#ifdef HAVE_LIBMESODE
static int
_connection_certfail_cb(xmpp_tlscert_t *xmpptlscert, const char *const errormsg)
{
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
    int res = sv_ev_certfail(errormsg, cert);
    tlscerts_free(cert);

    return res;
}
#endif

static xmpp_log_t*
_xmpp_get_file_logger(void)
{
    xmpp_log_level_t level = _get_xmpp_log_level();
    xmpp_log_t *file_log = malloc(sizeof(xmpp_log_t));

    file_log->handler = _xmpp_file_logger;
    file_log->userdata = &level;

    return file_log;
}

static xmpp_log_level_t
_get_xmpp_log_level(void)
{
    log_level_t prof_level = log_get_filter();

    if (prof_level == PROF_LEVEL_DEBUG) {
        return XMPP_LEVEL_DEBUG;
    } else if (prof_level == PROF_LEVEL_INFO) {
        return XMPP_LEVEL_INFO;
    } else if (prof_level == PROF_LEVEL_WARN) {
        return XMPP_LEVEL_WARN;
    } else {
        return XMPP_LEVEL_ERROR;
    }
}

static void
_xmpp_file_logger(void *const userdata, const xmpp_log_level_t level, const char *const area, const char *const msg)
{
    log_level_t prof_level = _get_log_level(level);
    log_msg(prof_level, area, msg);
    if ((g_strcmp0(area, "xmpp") == 0) || (g_strcmp0(area, "conn")) == 0) {
        sv_ev_xmpp_stanza(msg);
    }
}

static log_level_t
_get_log_level(const xmpp_log_level_t xmpp_level)
{
    if (xmpp_level == XMPP_LEVEL_DEBUG) {
        return PROF_LEVEL_DEBUG;
    } else if (xmpp_level == XMPP_LEVEL_INFO) {
        return PROF_LEVEL_INFO;
    } else if (xmpp_level == XMPP_LEVEL_WARN) {
        return PROF_LEVEL_WARN;
    } else {
        return PROF_LEVEL_ERROR;
    }
}
