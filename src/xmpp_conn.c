/*
 * xmpp_conn.c
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

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <strophe.h>

#include "chat_session.h"
#include "common.h"
#include "contact_list.h"
#include "jid.h"
#include "log.h"
#include "preferences.h"
#include "profanity.h"
#include "muc.h"
#include "xmpp.h"

static struct _jabber_conn_t {
    xmpp_log_t *log;
    xmpp_ctx_t *ctx;
    xmpp_conn_t *conn;
    jabber_conn_status_t conn_status;
    jabber_presence_t presence;
    char *status;
    int tls_disabled;
    int priority;
} jabber_conn;

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
} saved_details;

static GTimer *reconnect_timer;

static log_level_t _get_log_level(xmpp_log_level_t xmpp_level);
static xmpp_log_level_t _get_xmpp_log_level();
static void _xmpp_file_logger(void * const userdata,
    const xmpp_log_level_t level, const char * const area,
    const char * const msg);
static xmpp_log_t * _xmpp_get_file_logger();

static jabber_conn_status_t _jabber_connect(const char * const fulljid,
    const char * const passwd, const char * const altdomain);
static void _jabber_reconnect(void);

static void _jabber_roster_request(void);

static void _connection_handler(xmpp_conn_t * const conn,
    const xmpp_conn_event_t status, const int error,
    xmpp_stream_error_t * const stream_error, void * const userdata);
static int _ping_timed_handler(xmpp_conn_t * const conn, void * const userdata);

void
jabber_init(const int disable_tls)
{
    log_info("Initialising XMPP");
    jabber_conn.conn_status = JABBER_STARTED;
    jabber_conn.presence = PRESENCE_OFFLINE;
    jabber_conn.status = NULL;
    jabber_conn.tls_disabled = disable_tls;
    presence_init();
}

void
jabber_restart(void)
{
    jabber_conn.conn_status = JABBER_STARTED;
    jabber_conn.presence = PRESENCE_OFFLINE;
    FREE_SET_NULL(jabber_conn.status);
}

jabber_conn_status_t
jabber_connect_with_account(ProfAccount *account, const char * const passwd)
{
    saved_account.name = strdup(account->name);
    saved_account.passwd = strdup(passwd);

    log_info("Connecting using account: %s", account->name);
    char *fulljid = create_fulljid(account->jid, account->resource);
    jabber_conn_status_t result = _jabber_connect(fulljid, passwd, account->server);

    free(fulljid);

    return result;
}

jabber_conn_status_t
jabber_connect_with_details(const char * const jid,
    const char * const passwd, const char * const altdomain)
{
    saved_details.name = strdup(jid);
    saved_details.passwd = strdup(passwd);
    if (altdomain != NULL) {
        saved_details.altdomain = strdup(altdomain);
    } else {
        saved_details.altdomain = NULL;
    }

    Jid *jidp = jid_create(jid);
    if (jidp->resourcepart == NULL) {
        jid_destroy(jidp);
        jidp = jid_create_from_bare_and_resource(jid, "profanity");
        saved_details.jid = strdup(jidp->fulljid);
    } else {
        saved_details.jid = strdup(jid);
    }
    jid_destroy(jidp);

    log_info("Connecting without account, JID: %s", saved_details.jid);
    return _jabber_connect(saved_details.jid, passwd, saved_details.altdomain);
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
        _jabber_connect(fulljid, saved_account.passwd, account->server);
        free(fulljid);
        g_timer_start(reconnect_timer);
    }
}

void
jabber_disconnect(void)
{
    // if connected, send end stream and wait for response
    if (jabber_conn.conn_status == JABBER_CONNECTED) {
        log_info("Closing connection");
        jabber_conn.conn_status = JABBER_DISCONNECTING;
        xmpp_disconnect(jabber_conn.conn);

        while (jabber_get_connection_status() == JABBER_DISCONNECTING) {
            jabber_process_events();
        }
        jabber_free_resources();
    }
}

void
jabber_process_events(void)
{
    // run xmpp event loop if connected, connecting or disconnecting
    if (jabber_conn.conn_status == JABBER_CONNECTED
            || jabber_conn.conn_status == JABBER_CONNECTING
            || jabber_conn.conn_status == JABBER_DISCONNECTING) {
        xmpp_run_once(jabber_conn.ctx, 10);

    // check timer and reconnect if disconnected and timer set
    } else if (prefs_get_reconnect() != 0) {
        if ((jabber_conn.conn_status == JABBER_DISCONNECTED) &&
            (reconnect_timer != NULL)) {
            if (g_timer_elapsed(reconnect_timer, NULL) > prefs_get_reconnect()) {
                _jabber_reconnect();
            }
        }
    }

}

void
jabber_set_autoping(int seconds)
{
    if (jabber_conn.conn_status == JABBER_CONNECTED) {
        xmpp_timed_handler_delete(jabber_conn.conn, _ping_timed_handler);

        if (seconds != 0) {
            int millis = seconds * 1000;
            xmpp_timed_handler_add(jabber_conn.conn, _ping_timed_handler, millis,
                jabber_conn.ctx);
        }
    }
}

jabber_conn_status_t
jabber_get_connection_status(void)
{
    return (jabber_conn.conn_status);
}

xmpp_conn_t *
jabber_get_conn(void)
{
    return jabber_conn.conn;
}

xmpp_ctx_t *
jabber_get_ctx(void)
{
    return jabber_conn.ctx;
}

const char *
jabber_get_jid(void)
{
    return xmpp_conn_get_jid(jabber_conn.conn);
}

int
jabber_get_priority(void)
{
    return jabber_conn.priority;
}

jabber_presence_t
jabber_get_presence(void)
{
    return jabber_conn.presence;
}

char *
jabber_get_status(void)
{
    return jabber_conn.status;
}

char *
jabber_get_account_name(void)
{
    return saved_account.name;
}

void
jabber_conn_set_presence(jabber_presence_t presence)
{
    jabber_conn.presence = presence;
}

void
jabber_conn_set_status(const char * const message)
{
    FREE_SET_NULL(jabber_conn.status);
    if (message != NULL) {
        jabber_conn.status = strdup(message);
    }
}

void
jabber_conn_set_priority(int priority)
{
    jabber_conn.priority = priority;
}

void
jabber_free_resources(void)
{
    FREE_SET_NULL(saved_details.name);
    FREE_SET_NULL(saved_details.jid);
    FREE_SET_NULL(saved_details.passwd);
    FREE_SET_NULL(saved_details.altdomain);
    FREE_SET_NULL(saved_account.name);
    FREE_SET_NULL(saved_account.passwd);
    chat_sessions_clear();
    presence_free_sub_requests();
    xmpp_conn_release(jabber_conn.conn);
    xmpp_ctx_free(jabber_conn.ctx);
    xmpp_shutdown();
}

int
error_handler(xmpp_stanza_t * const stanza)
{
    gchar *err_msg = NULL;
    gchar *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    xmpp_stanza_t *error_stanza = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_ERROR);
        xmpp_stanza_t *text_stanza =
            xmpp_stanza_get_child_by_name(error_stanza, STANZA_NAME_TEXT);

    if (error_stanza == NULL) {
        log_debug("error message without <error/> received");
    } else {

        // check for text
        if (text_stanza != NULL) {
            err_msg = xmpp_stanza_get_text(text_stanza);
            prof_handle_error_message(from, err_msg);

            // TODO : process 'type' attribute from <error/> [RFC6120, 8.3.2]

        // otherwise show defined-condition
        } else {
            xmpp_stanza_t *err_cond = xmpp_stanza_get_children(error_stanza);

            if (err_cond == NULL) {
                log_debug("error message without <defined-condition/> or <text/> received");

            } else {
                err_msg = xmpp_stanza_get_name(err_cond);
                prof_handle_error_message(from, err_msg);

                // TODO : process 'type' attribute from <error/> [RFC6120, 8.3.2]
            }
        }
    }

    return 1;
}

static jabber_conn_status_t
_jabber_connect(const char * const fulljid, const char * const passwd,
    const char * const altdomain)
{
    Jid *jid = jid_create(fulljid);

    if (jid == NULL) {
        log_error("Malformed JID not able to connect: %s", fulljid);
        jabber_conn.conn_status = JABBER_DISCONNECTED;
        return jabber_conn.conn_status;
    } else if (jid->fulljid == NULL) {
        log_error("Full JID required to connect, received: %s", fulljid);
        jabber_conn.conn_status = JABBER_DISCONNECTED;
        return jabber_conn.conn_status;
    }

    jid_destroy(jid);

    log_info("Connecting as %s", fulljid);
    xmpp_initialize();
    jabber_conn.log = _xmpp_get_file_logger();
    jabber_conn.ctx = xmpp_ctx_new(NULL, jabber_conn.log);
    jabber_conn.conn = xmpp_conn_new(jabber_conn.ctx);
    xmpp_conn_set_jid(jabber_conn.conn, fulljid);
    xmpp_conn_set_pass(jabber_conn.conn, passwd);

    if (jabber_conn.tls_disabled)
        xmpp_conn_disable_tls(jabber_conn.conn);

    int connect_status = xmpp_connect_client(jabber_conn.conn, altdomain, 0,
        _connection_handler, jabber_conn.ctx);

    if (connect_status == 0)
        jabber_conn.conn_status = JABBER_CONNECTING;
    else
        jabber_conn.conn_status = JABBER_DISCONNECTED;

    return jabber_conn.conn_status;
}

static void
_connection_handler(xmpp_conn_t * const conn,
    const xmpp_conn_event_t status, const int error,
    xmpp_stream_error_t * const stream_error, void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

    // login success
    if (status == XMPP_CONN_CONNECT) {

        // logged in with account
        if (saved_account.name != NULL) {
            prof_handle_login_account_success(saved_account.name);

        // logged in without account, use details to create new account
        } else {
            accounts_add(saved_details.name, saved_details.altdomain);
            accounts_set_jid(saved_details.name, saved_details.jid);

            prof_handle_login_account_success(saved_details.name);
            saved_account.name = strdup(saved_details.name);
            saved_account.passwd = strdup(saved_details.passwd);

            FREE_SET_NULL(saved_details.name);
            FREE_SET_NULL(saved_details.jid);
            FREE_SET_NULL(saved_details.passwd);
            FREE_SET_NULL(saved_details.altdomain);
        }

        chat_sessions_init();

        xmpp_handler_add(conn, message_handler, NULL, STANZA_NAME_MESSAGE, NULL, ctx);

        presence_add_handlers();
        iq_add_handlers();

        if (prefs_get_autoping() != 0) {
            int millis = prefs_get_autoping() * 1000;
            xmpp_timed_handler_add(conn, _ping_timed_handler, millis, ctx);
        }

        _jabber_roster_request();
        jabber_conn.conn_status = JABBER_CONNECTED;
        jabber_conn.presence = PRESENCE_ONLINE;

        if (prefs_get_reconnect() != 0) {
            if (reconnect_timer != NULL) {
                g_timer_destroy(reconnect_timer);
                reconnect_timer = NULL;
            }
        }

    } else if (status == XMPP_CONN_DISCONNECT) {

        // lost connection for unkown reason
        if (jabber_conn.conn_status == JABBER_CONNECTED) {
            prof_handle_lost_connection();
            if (prefs_get_reconnect() != 0) {
                assert(reconnect_timer == NULL);
                reconnect_timer = g_timer_new();
                // TODO: free resources but leave saved_user untouched
            } else {
                jabber_free_resources();
            }

        // login attempt failed
        } else if (jabber_conn.conn_status != JABBER_DISCONNECTING) {
            if (reconnect_timer == NULL) {
                prof_handle_failed_login();
                jabber_free_resources();
            } else {
                if (prefs_get_reconnect() != 0) {
                    g_timer_start(reconnect_timer);
                }
                // TODO: free resources but leave saved_user untouched
            }
        }

        // close stream response from server after disconnect is handled too
        jabber_conn.conn_status = JABBER_DISCONNECTED;
        jabber_conn.presence = PRESENCE_OFFLINE;
    }
}

static void
_jabber_roster_request(void)
{
    xmpp_stanza_t *iq = stanza_create_roster_iq(jabber_conn.ctx);
    xmpp_send(jabber_conn.conn, iq);
    xmpp_stanza_release(iq);
}

static int
_ping_timed_handler(xmpp_conn_t * const conn, void * const userdata)
{
    if (jabber_conn.conn_status == JABBER_CONNECTED) {
        xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

        xmpp_stanza_t *iq = stanza_create_ping_iq(ctx);
        xmpp_send(conn, iq);
        xmpp_stanza_release(iq);
    }

    return 1;
}

static log_level_t
_get_log_level(xmpp_log_level_t xmpp_level)
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

static xmpp_log_level_t
_get_xmpp_log_level()
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
_xmpp_file_logger(void * const userdata, const xmpp_log_level_t level,
    const char * const area, const char * const msg)
{
    log_level_t prof_level = _get_log_level(level);
    log_msg(prof_level, area, msg);
}

static xmpp_log_t *
_xmpp_get_file_logger()
{
    xmpp_log_level_t level = _get_xmpp_log_level();
    xmpp_log_t *file_log = malloc(sizeof(xmpp_log_t));

    file_log->handler = _xmpp_file_logger;
    file_log->userdata = &level;

    return file_log;
}

