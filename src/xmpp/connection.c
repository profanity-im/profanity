/*
 * connection.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2018 - 2019 Michael Vetter <jubalh@idoru.org>
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
#include <assert.h>

#include <glib.h>
#include <glib/gstdio.h>

#ifdef HAVE_LIBMESODE
#include <mesode.h>
#endif

#ifdef HAVE_LIBSTROPHE
#include <strophe.h>
#endif

#include "common.h"
#include "log.h"
#include "config/files.h"
#include "config/preferences.h"
#include "event/server_events.h"
#include "xmpp/connection.h"
#include "xmpp/session.h"
#include "xmpp/iq.h"

typedef struct prof_conn_t {
    xmpp_log_t *xmpp_log;
    xmpp_ctx_t *xmpp_ctx;
    xmpp_conn_t *xmpp_conn;
    gboolean xmpp_in_event_loop;
    jabber_conn_status_t conn_status;
    xmpp_conn_event_t conn_last_event;
    char *presence_message;
    int priority;
    char *domain;
    GHashTable *available_resources;
    GHashTable *features_by_jid;
    GHashTable *requested_features;
} ProfConnection;

static ProfConnection conn;
static gchar *profanity_instance_id = NULL;
static gchar *prof_identifier = NULL;

static xmpp_log_t* _xmpp_get_file_logger(void);
static void _xmpp_file_logger(void *const userdata, const xmpp_log_level_t level, const char *const area, const char *const msg);

static void _connection_handler(xmpp_conn_t *const xmpp_conn, const xmpp_conn_event_t status, const int error,
    xmpp_stream_error_t *const stream_error, void *const userdata);

#ifdef HAVE_LIBMESODE
TLSCertificate* _xmppcert_to_profcert(xmpp_tlscert_t *xmpptlscert);
static int _connection_certfail_cb(xmpp_tlscert_t *xmpptlscert, const char *const errormsg);
#endif

static void _random_bytes_init(void);
static void _random_bytes_close(void);
static void _compute_identifier(const char *barejid);

void
connection_init(void)
{
    xmpp_initialize();
    conn.xmpp_conn = NULL;
    conn.xmpp_ctx = NULL;
    conn.xmpp_in_event_loop = FALSE;
    conn.conn_status = JABBER_DISCONNECTED;
    conn.conn_last_event = XMPP_CONN_DISCONNECT;
    conn.presence_message = NULL;
    conn.domain = NULL;
    conn.features_by_jid = NULL;
    conn.available_resources = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)resource_destroy);
    conn.requested_features = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);

	_random_bytes_init();
}

void
connection_check_events(void)
{
    conn.xmpp_in_event_loop = TRUE;
    xmpp_run_once(conn.xmpp_ctx, 10);
    conn.xmpp_in_event_loop = FALSE;
}

void
connection_shutdown(void)
{
    connection_clear_data();
    xmpp_shutdown();

    free(conn.xmpp_log);
    conn.xmpp_log = NULL;

	_random_bytes_close();
}

jabber_conn_status_t
connection_connect(const char *const jid, const char *const passwd, const char *const altdomain, int port,
    const char *const tls_policy)
{
    assert(jid != NULL);
    assert(passwd != NULL);

    Jid *jidp = jid_create(jid);
    if (jidp == NULL) {
        log_error("Malformed JID not able to connect: %s", jid);
        conn.conn_status = JABBER_DISCONNECTED;
        return conn.conn_status;
    }

    _compute_identifier(jidp->barejid);
    jid_destroy(jidp);

    log_info("Connecting as %s", jid);

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
    xmpp_conn_set_jid(conn.xmpp_conn, jid);
    xmpp_conn_set_pass(conn.xmpp_conn, passwd);

    if (!tls_policy || (g_strcmp0(tls_policy, "force") == 0)) {
        xmpp_conn_set_flags(conn.xmpp_conn, XMPP_CONN_FLAG_MANDATORY_TLS);
    } else if (g_strcmp0(tls_policy, "trust") == 0) {
        xmpp_conn_set_flags(conn.xmpp_conn, XMPP_CONN_FLAG_MANDATORY_TLS);
        xmpp_conn_set_flags(conn.xmpp_conn, XMPP_CONN_FLAG_TRUST_TLS);
    } else if (g_strcmp0(tls_policy, "disable") == 0) {
        xmpp_conn_set_flags(conn.xmpp_conn, XMPP_CONN_FLAG_DISABLE_TLS);
    } else if (g_strcmp0(tls_policy, "legacy") == 0) {
        xmpp_conn_set_flags(conn.xmpp_conn, XMPP_CONN_FLAG_LEGACY_SSL);
    }

#ifdef HAVE_LIBMESODE
    char *cert_path = prefs_get_tls_certpath();
    if (cert_path) {
        xmpp_conn_tlscert_path(conn.xmpp_conn, cert_path);
        free(cert_path);
    }

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

void
connection_disconnect(void)
{
    // don't disconnect already disconnected connection,
    // or we get infinite loop otherwise
    if (conn.conn_last_event == XMPP_CONN_CONNECT) {
        conn.conn_status = JABBER_DISCONNECTING;
        xmpp_disconnect(conn.xmpp_conn);

        while (conn.conn_status == JABBER_DISCONNECTING) {
            session_process_events();
        }
    } else {
        conn.conn_status = JABBER_DISCONNECTED;
    }

    // can't free libstrophe objects while we're in the event loop
    if (!conn.xmpp_in_event_loop) {
        if (conn.xmpp_conn) {
            xmpp_conn_release(conn.xmpp_conn);
            conn.xmpp_conn = NULL;
        }

        if (conn.xmpp_ctx) {
            xmpp_ctx_free(conn.xmpp_ctx);
            conn.xmpp_ctx = NULL;
        }
    }

    free(prof_identifier);
    prof_identifier = NULL;
}

void
connection_set_disconnected(void)
{
    FREE_SET_NULL(conn.presence_message);
    FREE_SET_NULL(conn.domain);
    conn.conn_status = JABBER_DISCONNECTED;
}

void
connection_clear_data(void)
{
    if (conn.features_by_jid) {
        g_hash_table_destroy(conn.features_by_jid);
        conn.features_by_jid = NULL;
    }

    if (conn.available_resources) {
        g_hash_table_remove_all(conn.available_resources);
    }

    if (conn.requested_features) {
        g_hash_table_remove_all(conn.requested_features);
    }
}

#ifdef HAVE_LIBMESODE
TLSCertificate*
connection_get_tls_peer_cert(void)
{
    xmpp_tlscert_t *xmpptlscert = xmpp_conn_tls_peer_cert(conn.xmpp_conn);
    TLSCertificate *cert = _xmppcert_to_profcert(xmpptlscert);
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

gboolean
connection_supports(const char *const feature)
{
    gboolean ret = FALSE;
    GList *jids = g_hash_table_get_keys(conn.features_by_jid);

    GList *curr = jids;
    while (curr) {
        char *jid = curr->data;
        GHashTable *features = g_hash_table_lookup(conn.features_by_jid, jid);
        if (features && g_hash_table_lookup(features, feature)) {
            ret = TRUE;
            break;
        }

        curr = g_list_next(curr);
    }

    g_list_free(jids);

    return ret;
}

char*
connection_jid_for_feature(const char *const feature)
{
    if (conn.features_by_jid == NULL) {
        return NULL;
    }

    GList *jids = g_hash_table_get_keys(conn.features_by_jid);

    GList *curr = jids;
    while (curr) {
        char *jid = curr->data;
        GHashTable *features = g_hash_table_lookup(conn.features_by_jid, jid);
        if (features && g_hash_table_lookup(features, feature)) {
            g_list_free(jids);
            return jid;
        }

        curr = g_list_next(curr);
    }

    g_list_free(jids);

    return NULL;
}

void
connection_request_features(void)
{
    /* We don't record it as a requested feature to avoid triggering th
     * sv_ev_connection_features_received too soon */
    iq_disco_info_request_onconnect(conn.domain);
}

void
connection_set_disco_items(GSList *items)
{
    GSList *curr = items;
    while (curr) {
        DiscoItem *item = curr->data;
        g_hash_table_insert(conn.requested_features, strdup(item->jid), NULL);
        g_hash_table_insert(conn.features_by_jid, strdup(item->jid),
            g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL));

        iq_disco_info_request_onconnect(item->jid);

        curr = g_slist_next(curr);
    }
}

jabber_conn_status_t
connection_get_status(void)
{
    return conn.conn_status;
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
    const char *jid = xmpp_conn_get_bound_jid(conn.xmpp_conn);
    if (jid) {
        return jid;
    } else {
        return xmpp_conn_get_jid(conn.xmpp_conn);
    }
}

void
connection_features_received(const char *const jid)
{
    if (g_hash_table_remove(conn.requested_features, jid) && g_hash_table_size(conn.requested_features) == 0) {
        sv_ev_connection_features_received();
    }
}

GHashTable*
connection_get_features(const char *const jid)
{
    return g_hash_table_lookup(conn.features_by_jid, jid);
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

char*
connection_create_stanza_id(void)
{
    char *uuid = connection_create_uuid();

    assert(uuid != NULL);

    gchar *hmac = g_compute_hmac_for_string(G_CHECKSUM_SHA256,
            (guchar*)prof_identifier, strlen(prof_identifier),
            uuid, strlen(uuid));

    GString *signature = g_string_new("");
    g_string_printf(signature, "%s%s", uuid, hmac);

    free(uuid);
    g_free(hmac);

    char *b64 = g_base64_encode((unsigned char*)signature->str, signature->len);
    g_string_free(signature, TRUE);

    assert(b64 != NULL);

    return b64;
}

char*
connection_get_domain(void)
{
    return conn.domain;
}

char*
connection_get_presence_msg(void)
{
    return conn.presence_message;
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
connection_set_priority(const int priority)
{
    conn.priority = priority;
}

static void
_connection_handler(xmpp_conn_t *const xmpp_conn, const xmpp_conn_event_t status, const int error,
    xmpp_stream_error_t *const stream_error, void *const userdata)
{
    conn.conn_last_event = status;

    switch (status) {

    // login success
    case XMPP_CONN_CONNECT:
        log_debug("Connection handler: XMPP_CONN_CONNECT");
        conn.conn_status = JABBER_CONNECTED;

        Jid *my_jid = jid_create(xmpp_conn_get_jid(conn.xmpp_conn));
        conn.domain = strdup(my_jid->domainpart);
        jid_destroy(my_jid);

        conn.features_by_jid = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)g_hash_table_destroy);
        g_hash_table_insert(conn.features_by_jid, strdup(conn.domain), g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL));

        session_login_success(connection_is_secured());

        break;

    // disconnected
    case XMPP_CONN_DISCONNECT:
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

        // close stream response from server after disconnect is handled
        conn.conn_status = JABBER_DISCONNECTED;

        break;

    // connection failed
    case XMPP_CONN_FAIL:
        log_debug("Connection handler: XMPP_CONN_FAIL");
        break;

    // unknown state
    default:
        log_error("Connection handler: Unknown status");
        break;
    }
}

#ifdef HAVE_LIBMESODE
static int
_connection_certfail_cb(xmpp_tlscert_t *xmpptlscert, const char *const errormsg)
{
    TLSCertificate *cert = _xmppcert_to_profcert(xmpptlscert);

    int res = sv_ev_certfail(errormsg, cert);
    tlscerts_free(cert);

    return res;
}

TLSCertificate*
_xmppcert_to_profcert(xmpp_tlscert_t *xmpptlscert)
{
    return tlscerts_new(
        xmpp_conn_tlscert_fingerprint(xmpptlscert),
        xmpp_conn_tlscert_version(xmpptlscert),
        xmpp_conn_tlscert_serialnumber(xmpptlscert),
        xmpp_conn_tlscert_subjectname(xmpptlscert),
        xmpp_conn_tlscert_issuername(xmpptlscert),
        xmpp_conn_tlscert_notbefore(xmpptlscert),
        xmpp_conn_tlscert_notafter(xmpptlscert),
        xmpp_conn_tlscert_key_algorithm(xmpptlscert),
        xmpp_conn_tlscert_signature_algorithm(xmpptlscert));
}
#endif

static xmpp_log_t*
_xmpp_get_file_logger(void)
{
    log_level_t prof_level = log_get_filter();
    xmpp_log_level_t xmpp_level = XMPP_LEVEL_ERROR;

    switch (prof_level) {
    case PROF_LEVEL_DEBUG:  xmpp_level = XMPP_LEVEL_DEBUG; break;
    case PROF_LEVEL_INFO:   xmpp_level = XMPP_LEVEL_INFO;  break;
    case PROF_LEVEL_WARN:   xmpp_level = XMPP_LEVEL_WARN;  break;
    default:                xmpp_level = XMPP_LEVEL_ERROR; break;
    }

    xmpp_log_t *file_log = malloc(sizeof(xmpp_log_t));
    file_log->handler = _xmpp_file_logger;
    file_log->userdata = &xmpp_level;

    return file_log;
}

static void
_xmpp_file_logger(void *const userdata, const xmpp_log_level_t xmpp_level, const char *const area, const char *const msg)
{
    log_level_t prof_level = PROF_LEVEL_ERROR;

    switch (xmpp_level) {
    case XMPP_LEVEL_DEBUG:  prof_level = PROF_LEVEL_DEBUG; break;
    case XMPP_LEVEL_INFO:   prof_level = PROF_LEVEL_INFO;  break;
    case XMPP_LEVEL_WARN:   prof_level = PROF_LEVEL_WARN;  break;
    default:                prof_level = PROF_LEVEL_ERROR; break;
    }

    log_msg(prof_level, area, msg);

    if ((g_strcmp0(area, "xmpp") == 0) || (g_strcmp0(area, "conn")) == 0) {
        sv_ev_xmpp_stanza(msg);
    }
}

static void _random_bytes_init(void)
{
    char *rndbytes_loc;
    GKeyFile *rndbytes;

    rndbytes_loc = files_get_data_path(FILE_PROFANITY_IDENTIFIER);

    if (g_file_test(rndbytes_loc, G_FILE_TEST_EXISTS)) {
        g_chmod(rndbytes_loc, S_IRUSR | S_IWUSR);
    }

    rndbytes = g_key_file_new();
    g_key_file_load_from_file(rndbytes, rndbytes_loc, G_KEY_FILE_KEEP_COMMENTS, NULL);

    if (g_key_file_has_group(rndbytes, "identifier")) {
        profanity_instance_id = g_key_file_get_string(rndbytes, "identifier", "random_bytes", NULL);
    } else {
        profanity_instance_id = get_random_string(10);
        g_key_file_set_string(rndbytes, "identifier", "random_bytes", profanity_instance_id);

        gsize g_data_size;
        gchar *g_accounts_data = g_key_file_to_data(rndbytes, &g_data_size, NULL);

        gchar *base = g_path_get_basename(rndbytes_loc);
        gchar *true_loc = get_file_or_linked(rndbytes_loc, base);
        g_file_set_contents(true_loc, g_accounts_data, g_data_size, NULL);

        g_free(base);
        free(true_loc);
        g_free(g_accounts_data);
    }

    free(rndbytes_loc);
    g_key_file_free(rndbytes);
}

static void _random_bytes_close(void)
{
    g_free(profanity_instance_id);
}

static void _compute_identifier(const char *barejid)
{
    gchar *hmac = g_compute_hmac_for_string(G_CHECKSUM_SHA256,
            (guchar*)profanity_instance_id, strlen(profanity_instance_id),
            barejid, strlen(barejid));

    char *b64 = g_base64_encode((guchar*)hmac, XMPP_SHA1_DIGEST_SIZE);
    assert(b64 != NULL);
    g_free(hmac);

    //in case of reconnect (lost connection)
    free(prof_identifier);

    prof_identifier = b64;
}

const char* connection_get_profanity_identifier(void) {
    return prof_identifier;
}
