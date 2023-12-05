/*
 * connection.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2018 - 2023 Michael Vetter <jubalh@iodoru.org>
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

#include <strophe.h>

#include "common.h"
#include "log.h"
#include "config/files.h"
#include "config/preferences.h"
#include "event/server_events.h"
#include "xmpp/connection.h"
#include "xmpp/session.h"
#include "xmpp/stanza.h"
#include "xmpp/iq.h"
#include "ui/ui.h"

typedef struct prof_conn_t
{
    xmpp_ctx_t* xmpp_ctx;
    xmpp_conn_t* xmpp_conn;
    xmpp_sm_state_t* sm_state;
    char** queued_messages;
    gboolean xmpp_in_event_loop;
    jabber_conn_status_t conn_status;
    xmpp_conn_event_t conn_last_event;
    char* presence_message;
    int priority;
    char* domain;
    Jid* jid;
    GHashTable* available_resources;
    GHashTable* features_by_jid;
    GHashTable* requested_features;
} ProfConnection;

typedef struct
{
    const char* username;
    const char* password;
} prof_reg_t;

static ProfConnection conn;
static gchar* profanity_instance_id = NULL;
static gchar* prof_identifier = NULL;

static void _xmpp_file_logger(void* const userdata, const xmpp_log_level_t level, const char* const area, const char* const msg);

static void _connection_handler(xmpp_conn_t* const xmpp_conn, const xmpp_conn_event_t status, const int error,
                                xmpp_stream_error_t* const stream_error, void* const userdata);

static TLSCertificate* _xmppcert_to_profcert(const xmpp_tlscert_t* xmpptlscert);
static int _connection_certfail_cb(const xmpp_tlscert_t* xmpptlscert, const char* errormsg);

static void _random_bytes_init(void);
static void _random_bytes_close(void);
static void _compute_identifier(const char* barejid);

static void*
_xmalloc(size_t size, void* userdata)
{
    void* ret = malloc(size);
    assert(ret != NULL);
    return ret;
}

static void
_xfree(void* p, void* userdata)
{
    free(p);
}

static void*
_xrealloc(void* p, size_t size, void* userdata)
{
    void* ret = realloc(p, size);
    assert(ret != NULL);
    return ret;
}

static xmpp_mem_t prof_mem = {
    _xmalloc, _xfree, _xrealloc, NULL
};
static xmpp_log_t prof_log = {
    _xmpp_file_logger, NULL
};

void
connection_init(void)
{
    xmpp_initialize();
    conn.sm_state = NULL;
    conn.queued_messages = NULL;
    conn.xmpp_in_event_loop = FALSE;
    conn.conn_status = JABBER_DISCONNECTED;
    conn.conn_last_event = XMPP_CONN_DISCONNECT;
    conn.presence_message = NULL;
    conn.domain = NULL;
    conn.jid = NULL;
    conn.features_by_jid = NULL;
    conn.available_resources = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)resource_destroy);
    conn.requested_features = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);

    conn.xmpp_ctx = xmpp_ctx_new(&prof_mem, &prof_log);
    auto_gchar gchar* v = prefs_get_string(PREF_STROPHE_VERBOSITY);
    auto_gchar gchar* err_msg = NULL;
    int verbosity;
    if (string_to_verbosity(v, &verbosity, &err_msg)) {
        xmpp_ctx_set_verbosity(conn.xmpp_ctx, verbosity);
    } else {
        cons_show(err_msg);
    }
    conn.xmpp_conn = xmpp_conn_new(conn.xmpp_ctx);

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
    if (conn.xmpp_conn) {
        xmpp_conn_release(conn.xmpp_conn);
        conn.xmpp_conn = NULL;
    }
    if (conn.xmpp_ctx) {
        xmpp_ctx_free(conn.xmpp_ctx);
        conn.xmpp_ctx = NULL;
    }
    xmpp_shutdown();

    _random_bytes_close();
}

static gboolean
_conn_apply_settings(const char* const jid, const char* const passwd, const char* const tls_policy, const char* const auth_policy)
{
    auto_jid Jid* jidp = jid_create(jid);
    if (jidp == NULL) {
        log_error("Malformed JID not able to connect: %s", jid);
        conn.conn_status = JABBER_DISCONNECTED;
        return FALSE;
    }

    _compute_identifier(jidp->barejid);

    xmpp_conn_set_jid(conn.xmpp_conn, jid);
    if (passwd)
        xmpp_conn_set_pass(conn.xmpp_conn, passwd);

    long flags = xmpp_conn_get_flags(conn.xmpp_conn);

    /* clear all TLS & auth related flags */
    flags &= ~(XMPP_CONN_FLAG_DISABLE_TLS | XMPP_CONN_FLAG_MANDATORY_TLS
               | XMPP_CONN_FLAG_LEGACY_SSL | XMPP_CONN_FLAG_TRUST_TLS
               | XMPP_CONN_FLAG_LEGACY_AUTH);
    if (!tls_policy || (g_strcmp0(tls_policy, "force") == 0)) {
        flags |= XMPP_CONN_FLAG_MANDATORY_TLS;
    } else if (g_strcmp0(tls_policy, "trust") == 0) {
        flags |= XMPP_CONN_FLAG_MANDATORY_TLS;
        flags |= XMPP_CONN_FLAG_TRUST_TLS;
    } else if (g_strcmp0(tls_policy, "disable") == 0) {
        flags |= XMPP_CONN_FLAG_DISABLE_TLS;
    } else if (g_strcmp0(tls_policy, "legacy") == 0) {
        flags |= XMPP_CONN_FLAG_LEGACY_SSL;
    }

    if (auth_policy && (g_strcmp0(auth_policy, "legacy") == 0)) {
        flags |= XMPP_CONN_FLAG_LEGACY_AUTH;
    }

    /* Print debug logs that can help when users share the logs */
    if (flags != 0) {
        log_debug("Connecting with flags (0x%lx):", flags);
#define LOG_FLAG_IF_SET(name)  \
    if (flags & name) {        \
        log_debug("  " #name); \
    }
        LOG_FLAG_IF_SET(XMPP_CONN_FLAG_MANDATORY_TLS);
        LOG_FLAG_IF_SET(XMPP_CONN_FLAG_TRUST_TLS);
        LOG_FLAG_IF_SET(XMPP_CONN_FLAG_DISABLE_TLS);
        LOG_FLAG_IF_SET(XMPP_CONN_FLAG_LEGACY_SSL);
        LOG_FLAG_IF_SET(XMPP_CONN_FLAG_LEGACY_AUTH);
#undef LOG_FLAG_IF_SET
    }

    if (xmpp_conn_set_flags(conn.xmpp_conn, flags)) {
        log_error("libstrophe doesn't accept this combination of flags: 0x%x", flags);
        conn.conn_status = JABBER_DISCONNECTED;
        return FALSE;
    }

    auto_char char* cert_path = prefs_get_tls_certpath();
    if (cert_path) {
        xmpp_conn_set_capath(conn.xmpp_conn, cert_path);
    }

    xmpp_conn_set_certfail_handler(conn.xmpp_conn, _connection_certfail_cb);
    if (conn.sm_state) {
        if (xmpp_conn_set_sm_state(conn.xmpp_conn, conn.sm_state)) {
            log_warning("Had Stream Management state, but libstrophe didn't accept it");
            xmpp_free_sm_state(conn.sm_state);
        }
        conn.sm_state = NULL;
    }

    return TRUE;
}

jabber_conn_status_t
connection_connect(const char* const jid, const char* const passwd, const char* const altdomain, int port,
                   const char* const tls_policy, const char* const auth_policy)
{
    assert(jid != NULL);
    assert(passwd != NULL);
    log_info("Connecting as %s", jid);

    _conn_apply_settings(jid, passwd, tls_policy, auth_policy);

    int connect_status = xmpp_connect_client(
        conn.xmpp_conn,
        altdomain,
        port,
        _connection_handler,
        conn.xmpp_ctx);

    if (connect_status == 0) {
        conn.conn_status = JABBER_CONNECTING;
    } else {
        conn.conn_status = JABBER_DISCONNECTED;
    }

    return conn.conn_status;
}

static int
iq_reg2_cb(xmpp_conn_t* xmpp_conn, xmpp_stanza_t* stanza, void* userdata)
{
    const char* type;

    (void)userdata;

    type = xmpp_stanza_get_type(stanza);
    if (!type || strcmp(type, "error") == 0) {
        char* error_message = stanza_get_error_message(stanza);
        cons_show_error("Server error: %s", error_message);
        log_debug("Registration error: %s", error_message);
        goto quit;
    }

    if (strcmp(type, "result") != 0) {
        log_debug("Expected type 'result', but got %s.", type);
        goto quit;
    }

    cons_show("Registration successful.");
    log_info("Registration successful.");
    goto quit;

quit:
    xmpp_disconnect(xmpp_conn);

    return 0;
}

static int
iq_reg_cb(xmpp_conn_t* xmpp_conn, xmpp_stanza_t* stanza, void* userdata)
{
    prof_reg_t* reg = (prof_reg_t*)userdata;
    xmpp_stanza_t* registered = NULL;
    xmpp_stanza_t* query;
    const char* type;

    type = xmpp_stanza_get_type(stanza);
    if (!type || strcmp(type, "error") == 0) {
        char* error_message = stanza_get_error_message(stanza);
        cons_show_error("Server error: %s", error_message);
        log_debug("Registration error: %s", error_message);
        xmpp_disconnect(xmpp_conn);
        goto quit;
    }

    if (strcmp(type, "result") != 0) {
        log_debug("Expected type 'result', but got %s.", type);
        xmpp_disconnect(xmpp_conn);
        goto quit;
    }

    query = xmpp_stanza_get_child_by_name(stanza, "query");
    if (query)
        registered = xmpp_stanza_get_child_by_name(query, "registered");
    if (registered != NULL) {
        cons_show_error("Already registered.");
        log_debug("Already registered.");
        xmpp_disconnect(xmpp_conn);
        goto quit;
    }
    xmpp_stanza_t* iq = stanza_register_new_account(conn.xmpp_ctx, reg->username, reg->password);
    xmpp_id_handler_add(xmpp_conn, iq_reg2_cb, xmpp_stanza_get_id(iq), reg);
    xmpp_send(xmpp_conn, iq);

quit:
    return 0;
}

static int
_register_handle_error(xmpp_conn_t* xmpp_conn, xmpp_stanza_t* stanza, void* userdata)
{
    (void)stanza;
    (void)userdata;

    char* error_message = stanza_get_error_message(stanza);
    cons_show_error("Server error: %s", error_message);
    log_debug("Registration error: %s", error_message);
    xmpp_disconnect(xmpp_conn);

    return 0;
}

static int
_register_handle_proceedtls_default(xmpp_conn_t* xmpp_conn,
                                    xmpp_stanza_t* stanza,
                                    void* userdata)
{
    const char* name = xmpp_stanza_get_name(stanza);

    (void)userdata;

    if (strcmp(name, "proceed") == 0) {
        log_debug("Proceeding with TLS.");
        if (xmpp_conn_tls_start(xmpp_conn) == 0) {
            xmpp_handler_delete(xmpp_conn, _register_handle_error);
            xmpp_conn_open_stream_default(xmpp_conn);
        } else {
            log_debug("TLS failed.");
            /* failed tls spoils the connection, so disconnect */
            xmpp_disconnect(xmpp_conn);
        }
    }
    return 0;
}

static int
_register_handle_missing_features(xmpp_conn_t* xmpp_conn, void* userdata)
{
    (void)userdata;

    log_debug("Timeout");
    xmpp_disconnect(xmpp_conn);

    return 0;
}

static int
_register_handle_features(xmpp_conn_t* xmpp_conn, xmpp_stanza_t* stanza, void* userdata)
{
    prof_reg_t* reg = (prof_reg_t*)userdata;
    xmpp_ctx_t* ctx = conn.xmpp_ctx;
    xmpp_stanza_t* child;
    xmpp_stanza_t* iq;
    char* domain;

    xmpp_timed_handler_delete(xmpp_conn, _register_handle_missing_features);

    /* secure connection if possible */
    child = xmpp_stanza_get_child_by_name(stanza, "starttls");
    if (child && (strcmp(xmpp_stanza_get_ns(child), XMPP_NS_TLS) == 0)) {
        log_debug("Server supports TLS. Attempting to establish…");
        child = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(child, "starttls");
        xmpp_stanza_set_ns(child, XMPP_NS_TLS);
        xmpp_handler_add(xmpp_conn, _register_handle_proceedtls_default, XMPP_NS_TLS, NULL,
                         NULL, NULL);
        xmpp_send(xmpp_conn, child);
        xmpp_stanza_release(child);
        return 0;
    }

    /* check whether server supports in-band registration */
    child = xmpp_stanza_get_child_by_name(stanza, "register");
    if (!child) {
        log_debug("Server does not support in-band registration.");
        cons_show_error("Server does not support in-band registration, aborting.");
        xmpp_disconnect(xmpp_conn);
        return 0;
    }

    log_debug("Server supports in-band registration. Attempting registration.");

    domain = strdup(conn.domain);
    iq = xmpp_iq_new(ctx, "get", "reg1");
    xmpp_stanza_set_to(iq, domain);
    child = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(child, "query");
    xmpp_stanza_set_ns(child, STANZA_NS_REGISTER);
    xmpp_stanza_add_child(iq, child);

    xmpp_handler_add(xmpp_conn, iq_reg_cb, STANZA_NS_REGISTER, "iq", NULL, reg);
    xmpp_send(xmpp_conn, iq);

    xmpp_free(ctx, domain);
    xmpp_stanza_release(child);
    xmpp_stanza_release(iq);

    return 0;
}

static void
_register_handler(xmpp_conn_t* xmpp_conn,
                  xmpp_conn_event_t status,
                  int error,
                  xmpp_stream_error_t* stream_error,
                  void* userdata)
{
    conn.conn_last_event = status;

    prof_reg_t* reg = (prof_reg_t*)userdata;
    int secured;

    (void)error;
    (void)stream_error;

    switch (status) {

    case XMPP_CONN_RAW_CONNECT:
        log_debug("Raw connection established.");
        xmpp_conn_open_stream_default(xmpp_conn);
        conn.conn_status = JABBER_RAW_CONNECTED;
        break;

    case XMPP_CONN_CONNECT:
        log_debug("Connected.");
        secured = xmpp_conn_is_secured(xmpp_conn);
        conn.conn_status = JABBER_CONNECTED;
        log_debug("Connection is %s.\n",
                  secured ? "secured" : "NOT secured");

        Jid* my_jid = jid_create(xmpp_conn_get_jid(xmpp_conn));
        conn.domain = strdup(my_jid->domainpart);
        jid_destroy(my_jid);

        xmpp_handler_add(xmpp_conn, _register_handle_error, XMPP_NS_STREAMS, "error", NULL,
                         NULL);
        xmpp_handler_add(xmpp_conn, _register_handle_features, XMPP_NS_STREAMS, "features",
                         NULL, reg);
        xmpp_timed_handler_add(xmpp_conn, _register_handle_missing_features, 5000,
                               NULL);
        break;

    case XMPP_CONN_DISCONNECT:
        log_debug("Disconnected");
        conn.conn_status = JABBER_DISCONNECTED;
        break;

    default:
        break;
    }
}

jabber_conn_status_t
connection_register(const char* const altdomain, int port, const char* const tls_policy,
                    const char* const username, const char* const password)
{
    _conn_apply_settings(altdomain, NULL, tls_policy, NULL);

    prof_reg_t* reg;

    reg = calloc(1, sizeof(*reg));
    if (reg == NULL) {
        log_warning("Failed to allocate registration data struct during connect");
        return JABBER_DISCONNECTED;
    }

    reg->username = strdup(username);
    reg->password = strdup(password);

    int connect_status = xmpp_connect_raw(
        conn.xmpp_conn,
        altdomain,
        port,
        _register_handler,
        reg);

    if (connect_status == 0) {
        conn.conn_status = JABBER_RAW_CONNECTING;
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
            conn.xmpp_conn = xmpp_conn_new(conn.xmpp_ctx);
        }
    }

    g_free(prof_identifier);
    prof_identifier = NULL;

    jid_destroy(conn.jid);
    conn.jid = NULL;
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

TLSCertificate*
connection_get_tls_peer_cert(void)
{
    xmpp_tlscert_t* xmpptlscert = xmpp_conn_get_peer_cert(conn.xmpp_conn);
    TLSCertificate* cert = _xmppcert_to_profcert(xmpptlscert);
    xmpp_tlscert_free(xmpptlscert);

    return cert;
}

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
connection_send_stanza(const char* const stanza)
{
    if (conn.conn_status != JABBER_CONNECTED) {
        return FALSE;
    } else {
        xmpp_send_raw_string(conn.xmpp_conn, "%s", stanza);
        return TRUE;
    }
}

gboolean
connection_supports(const char* const feature)
{
    gboolean ret = FALSE;
    GList* jids = g_hash_table_get_keys(conn.features_by_jid);

    GList* curr = jids;
    while (curr) {
        char* jid = curr->data;
        GHashTable* features = g_hash_table_lookup(conn.features_by_jid, jid);
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
connection_jid_for_feature(const char* const feature)
{
    if (conn.features_by_jid == NULL) {
        return NULL;
    }

    GList* jids = g_hash_table_get_keys(conn.features_by_jid);

    GList* curr = jids;
    while (curr) {
        char* jid = curr->data;
        GHashTable* features = g_hash_table_lookup(conn.features_by_jid, jid);
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
connection_set_disco_items(GSList* items)
{
    GSList* curr = items;
    while (curr) {
        DiscoItem* item = curr->data;
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
    if (!conn.xmpp_conn)
        return NULL;
    const char* jid = xmpp_conn_get_bound_jid(conn.xmpp_conn);
    if (jid) {
        return jid;
    } else {
        return xmpp_conn_get_jid(conn.xmpp_conn);
    }
}

const Jid*
connection_get_jid(void)
{
    static const char* fulljid;
    const char* cur_fulljid = connection_get_fulljid();
    if (!conn.jid || cur_fulljid != fulljid) {
        fulljid = cur_fulljid;
        if (conn.jid)
            jid_destroy(conn.jid);
        conn.jid = jid_create(fulljid);
        if (!conn.jid) {
            log_error("Could not create connection-wide JID object from \"%s\"", STR_MAYBE_NULL(fulljid));
            /* In case we failed to create the jid or we're not yet
             * connected (or whatever else could fail) return the
             * pointer to a zero-initialized static object, so
             * de-referencing that pointer won't fail.
             */
            static const Jid jid = { 0 };
            return &jid;
        }
    }
    return conn.jid;
}

const char*
connection_get_barejid(void)
{
    return connection_get_jid()->barejid;
}

gboolean
equals_our_barejid(const char* cmp)
{
    return g_strcmp0(connection_get_jid()->barejid, cmp) == 0;
}

char*
connection_get_user(void)
{
    const char* jid = connection_get_fulljid();
    if (!jid)
        return NULL;
    char* result = strdup(jid);

    char* split = strchr(result, '@');
    *split = '\0';

    return result;
}

void
connection_features_received(const char* const jid)
{
    log_info("[CONNECTION] connection_features_received %s", jid);
    if (g_hash_table_remove(conn.requested_features, jid) && g_hash_table_size(conn.requested_features) == 0) {
        sv_ev_connection_features_received();
    }
}

GHashTable*
connection_get_features(const char* const jid)
{
    return g_hash_table_lookup(conn.features_by_jid, jid);
}

GList*
connection_get_available_resources(void)
{
    return g_hash_table_get_values(conn.available_resources);
}

int
connection_count_available_resources(void)
{
    return g_hash_table_size(conn.available_resources);
}

void
connection_add_available_resource(Resource* resource)
{
    g_hash_table_replace(conn.available_resources, strdup(resource->name), resource);
}

void
connection_remove_available_resource(const char* const resource)
{
    g_hash_table_remove(conn.available_resources, resource);
}

char*
connection_create_uuid(void)
{
    return xmpp_uuid_gen(conn.xmpp_ctx);
}

void
connection_free_uuid(char* uuid)
{
    if (uuid) {
        xmpp_free(conn.xmpp_ctx, uuid);
    }
}

char*
connection_create_stanza_id(void)
{
    auto_char char* rndid = get_random_string(CON_RAND_ID_LEN);
    assert(rndid != NULL);

    auto_gchar gchar* hmac = g_compute_hmac_for_string(G_CHECKSUM_SHA1,
                                                       (guchar*)prof_identifier, strlen(prof_identifier),
                                                       rndid, strlen(rndid));

    return g_strdup_printf("%s%s", rndid, hmac);
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
connection_set_presence_msg(const char* const message)
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

static xmpp_stanza_t*
_get_soh_error(xmpp_stanza_t* error_stanza)
{
    return xmpp_stanza_get_child_by_path(error_stanza,
                                         XMPP_STANZA_NAME_IN_NS("error", STANZA_NS_STREAMS),
                                         XMPP_STANZA_NAME_IN_NS("see-other-host", STANZA_NS_XMPP_STREAMS),
                                         NULL);
}

#if GLIB_CHECK_VERSION(2, 66, 0)
static gboolean
_split_url(const char* alturi, gchar** host, gint* port)
{
    /* Construct a valid URI with `schema://` as `g_uri_split_network()`
     * requires this to be there.
     */
    auto_gchar gchar* xmpp_uri = g_strdup_printf("xmpp://%s", alturi);
    gboolean ret = g_uri_split_network(xmpp_uri, 0, NULL, host, port, NULL);
    /* fix-up `port` as g_uri_split_network() sets port to `-1` if it's missing
     * in the passed-in URI, but libstrophe expects a "missing port"
     * to be passed as `0` (which then results in connecting to the standard port).
     */
    if (*port == -1)
        *port = 0;
    return ret;
}
#else
/* poor-mans URL splitting */
static gboolean
_split_url(const char* alturi, gchar** host, gint* port)
{
    ptrdiff_t hostlen;
    /* search ':' from start and end
     * if `first` matches `last` it's a `hostname:port` combination
     * if `first` is different than `last` it's `[ip:v6]` or `[ip:v6]:port`
     */
    char* first = strchr(alturi, ':');
    char* last = strrchr(alturi, ':');
    if (first) {
        if (first == last) {
            hostlen = last - alturi + 1;
            if (!strtoi_range(last + 1, port, 1, 65535, NULL))
                return FALSE;
        } else {
            /* IPv6 handling */
            char* bracket = strrchr(alturi, ']');
            if (!bracket)
                return FALSE;
            if (bracket > last) {
                /* `[ip:v6]` */
                hostlen = strlen(alturi) + 1;
                *port = 0;
            } else {
                /* `[ip:v6]:port` */
                hostlen = last - alturi + 1;
                if (!strtoi_range(last + 1, port, 1, 65535, NULL))
                    return FALSE;
            }
        }
    } else {
        hostlen = strlen(alturi) + 1;
        *port = 0;
    }
    gchar* buf = g_malloc(hostlen);
    if (!buf)
        return FALSE;
    memcpy(buf, alturi, hostlen);
    buf[hostlen - 1] = '\0';
    *host = buf;
    return TRUE;
}
#endif

static bool
_get_other_host(xmpp_stanza_t* error_stanza, gchar** host, int* port)
{
    xmpp_stanza_t* soh_error = _get_soh_error(error_stanza);
    if (!soh_error || !xmpp_stanza_get_children(soh_error)) {
        log_debug("_get_other_host: stream-error contains no see-other-host");
        return false;
    }
    const char* alturi = xmpp_stanza_get_text_ptr(xmpp_stanza_get_children(soh_error));
    if (!alturi) {
        log_debug("_get_other_host: see-other-host contains no text");
        return false;
    }

    if (!_split_url(alturi, host, port)) {
        log_debug("_get_other_host: Could not split \"%s\"", alturi);
        return false;
    }
    return true;
}

static void
_connection_handler(xmpp_conn_t* const xmpp_conn, const xmpp_conn_event_t status, const int error,
                    xmpp_stream_error_t* const stream_error, void* const userdata)
{
    conn.conn_last_event = status;

    switch (status) {

    // login success
    case XMPP_CONN_CONNECT:
        log_debug("Connection handler: XMPP_CONN_CONNECT");
        conn.conn_status = JABBER_CONNECTED;

        connection_get_jid();
        conn.domain = strdup(conn.jid->domainpart);

        connection_clear_data();
        conn.features_by_jid = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)g_hash_table_destroy);
        g_hash_table_insert(conn.features_by_jid, strdup(conn.domain), g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL));

        session_login_success(connection_is_secured());

        if (conn.queued_messages) {
            for (size_t n = 0; conn.queued_messages[n] != NULL; ++n) {
                xmpp_send_raw(conn.xmpp_conn, conn.queued_messages[n], strlen(conn.queued_messages[n]));
                free(conn.queued_messages[n]);
            }
            free(conn.queued_messages);
            conn.queued_messages = NULL;
        }

        break;

    // raw connection success
    case XMPP_CONN_RAW_CONNECT:
        log_debug("Connection handler: XMPP_CONN_RAW_CONNECT");
        conn.conn_status = JABBER_RAW_CONNECTED;

        connection_get_jid();
        conn.jid = jid_create(xmpp_conn_get_jid(conn.xmpp_conn));
        log_debug("jid: %s", conn.jid->str);
        conn.domain = strdup(conn.jid->domainpart);

        connection_clear_data();
        conn.features_by_jid = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)g_hash_table_destroy);
        g_hash_table_insert(conn.features_by_jid, strdup(conn.domain), g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL));

        xmpp_conn_open_stream_default(xmpp_conn);

        break;

    // disconnected
    case XMPP_CONN_DISCONNECT:
        log_debug("Connection handler: XMPP_CONN_DISCONNECT");

        // lost connection for unknown reason
        if (conn.conn_status == JABBER_CONNECTED || conn.conn_status == JABBER_DISCONNECTING) {
            if (prefs_get_boolean(PREF_STROPHE_SM_ENABLED)) {
                int send_queue_len = xmpp_conn_send_queue_len(conn.xmpp_conn);
                log_debug("Connection handler: Lost connection for unknown reason, %d messages in send queue", send_queue_len);
                conn.sm_state = xmpp_conn_get_sm_state(conn.xmpp_conn);
                if (send_queue_len > 0 && prefs_get_boolean(PREF_STROPHE_SM_RESEND)) {
                    conn.queued_messages = calloc(send_queue_len + 1, sizeof(*conn.queued_messages));
                    for (int n = 0; n < send_queue_len && conn.queued_messages[n]; ++n) {
                        conn.queued_messages[n] = xmpp_conn_send_queue_drop_element(conn.xmpp_conn, XMPP_QUEUE_OLDEST);
                    }
                } else if (send_queue_len > 0) {
                    log_debug("Connection handler: dropping those messages since SM RESEND is disabled");
                }
            }
            if (conn.conn_status == JABBER_CONNECTED)
                session_lost_connection();

            // login attempt failed
        } else {
            gchar* host;
            int port;
            if (stream_error && stream_error->stanza && _get_other_host(stream_error->stanza, &host, &port)) {
                session_reconnect(host, port);
                log_debug("Connection handler: Forcing a re-connect to \"%s\"", host);
                conn.conn_status = JABBER_RECONNECT;
                return;
            }
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

static int
_connection_certfail_cb(const xmpp_tlscert_t* xmpptlscert, const char* errormsg)
{
    TLSCertificate* cert = _xmppcert_to_profcert(xmpptlscert);

    int res = sv_ev_certfail(errormsg, cert);
    tlscerts_free(cert);

    return res;
}

TLSCertificate*
_xmppcert_to_profcert(const xmpp_tlscert_t* xmpptlscert)
{
    int version = (int)strtol(
        xmpp_tlscert_get_string(xmpptlscert, XMPP_CERT_VERSION), NULL, 10);
    return tlscerts_new(
        xmpp_tlscert_get_string(xmpptlscert, XMPP_CERT_FINGERPRINT_SHA1),
        version,
        xmpp_tlscert_get_string(xmpptlscert, XMPP_CERT_SERIALNUMBER),
        xmpp_tlscert_get_string(xmpptlscert, XMPP_CERT_SUBJECT),
        xmpp_tlscert_get_string(xmpptlscert, XMPP_CERT_ISSUER),
        xmpp_tlscert_get_string(xmpptlscert, XMPP_CERT_NOTBEFORE),
        xmpp_tlscert_get_string(xmpptlscert, XMPP_CERT_NOTAFTER),
        xmpp_tlscert_get_string(xmpptlscert, XMPP_CERT_KEYALG),
        xmpp_tlscert_get_string(xmpptlscert, XMPP_CERT_SIGALG),
        xmpp_tlscert_get_pem(xmpptlscert));
}

static void
_xmpp_file_logger(void* const userdata, const xmpp_log_level_t xmpp_level, const char* const area, const char* const msg)
{
    log_level_t prof_level = PROF_LEVEL_ERROR;

    switch (xmpp_level) {
    case XMPP_LEVEL_DEBUG:
        prof_level = PROF_LEVEL_DEBUG;
        break;
    case XMPP_LEVEL_INFO:
        prof_level = PROF_LEVEL_INFO;
        break;
    case XMPP_LEVEL_WARN:
        prof_level = PROF_LEVEL_WARN;
        break;
    default:
        prof_level = PROF_LEVEL_ERROR;
        break;
    }

    log_msg(prof_level, area, msg);

    if ((g_strcmp0(area, "xmpp") == 0) || (g_strcmp0(area, "conn")) == 0) {
        sv_ev_xmpp_stanza(msg);
    }
}

static void
_random_bytes_init(void)
{
    prof_keyfile_t keyfile;
    load_data_keyfile(&keyfile, FILE_PROFANITY_IDENTIFIER);

    if (g_key_file_has_group(keyfile.keyfile, "identifier")) {
        profanity_instance_id = g_key_file_get_string(keyfile.keyfile, "identifier", "random_bytes", NULL);
    } else {
        profanity_instance_id = get_random_string(10);
        g_key_file_set_string(keyfile.keyfile, "identifier", "random_bytes", profanity_instance_id);
        save_keyfile(&keyfile);
    }

    free_keyfile(&keyfile);
}

static void
_random_bytes_close(void)
{
    g_free(profanity_instance_id);
}

static void
_compute_identifier(const char* barejid)
{
    // in case of reconnect (lost connection)
    g_free(prof_identifier);

    prof_identifier = g_compute_hmac_for_string(G_CHECKSUM_SHA256,
                                                (guchar*)profanity_instance_id, strlen(profanity_instance_id),
                                                barejid, strlen(barejid));
}

const char*
connection_get_profanity_identifier(void)
{
    return prof_identifier;
}
