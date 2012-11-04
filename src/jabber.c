/*
 * jabber.c
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
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

#include <strophe.h>

#include "chat_session.h"
#include "common.h"
#include "contact_list.h"
#include "jabber.h"
#include "log.h"
#include "preferences.h"
#include "profanity.h"

#define PING_INTERVAL 120000 // 2 minutes

static struct _jabber_conn_t {
    xmpp_log_t *log;
    xmpp_ctx_t *ctx;
    xmpp_conn_t *conn;
    jabber_conn_status_t conn_status;
    jabber_presence_t presence;
    int tls_disabled;
} jabber_conn;

static log_level_t _get_log_level(xmpp_log_level_t xmpp_level);
static xmpp_log_level_t _get_xmpp_log_level();
static void _xmpp_file_logger(void * const userdata,
    const xmpp_log_level_t level, const char * const area,
    const char * const msg);
static xmpp_log_t * _xmpp_get_file_logger();

static void _jabber_roster_request(void);

// XMPP event handlers
static void _connection_handler(xmpp_conn_t * const conn,
    const xmpp_conn_event_t status, const int error,
    xmpp_stream_error_t * const stream_error, void * const userdata);
static int _message_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _roster_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _presence_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _ping_timed_handler(xmpp_conn_t * const conn, void * const userdata);

void
jabber_init(const int disable_tls)
{
    log_info("Initialising XMPP");
    jabber_conn.conn_status = JABBER_STARTED;
    jabber_conn.presence = PRESENCE_OFFLINE;
    jabber_conn.tls_disabled = disable_tls;
}

void
jabber_restart(void)
{
    jabber_conn.conn_status = JABBER_STARTED;
    jabber_conn.presence = PRESENCE_OFFLINE;
}

jabber_conn_status_t
jabber_connect(const char * const user,
    const char * const passwd)
{
    log_info("Connecting as %s", user);
    xmpp_initialize();

    jabber_conn.log = _xmpp_get_file_logger();
    jabber_conn.ctx = xmpp_ctx_new(NULL, jabber_conn.log);
    jabber_conn.conn = xmpp_conn_new(jabber_conn.ctx);

    xmpp_conn_set_jid(jabber_conn.conn, user);
    xmpp_conn_set_pass(jabber_conn.conn, passwd);

    if (jabber_conn.tls_disabled)
        xmpp_conn_disable_tls(jabber_conn.conn);

    int connect_status = xmpp_connect_client(jabber_conn.conn, NULL, 0,
        _connection_handler, jabber_conn.ctx);

    if (connect_status == 0)
        jabber_conn.conn_status = JABBER_CONNECTING;
    else
        jabber_conn.conn_status = JABBER_DISCONNECTED;

    return jabber_conn.conn_status;
}

void
jabber_disconnect(void)
{
    // if connected, send end stream and wait for response
    if (jabber_conn.conn_status == JABBER_CONNECTED) {
        log_info("Closing connection");
        xmpp_disconnect(jabber_conn.conn);
        jabber_conn.conn_status = JABBER_DISCONNECTING;

        while (jabber_get_connection_status() == JABBER_DISCONNECTING) {
            jabber_process_events();
        }
        jabber_free_resources();
    }
}

void
jabber_process_events(void)
{
    if (jabber_conn.conn_status == JABBER_CONNECTED
            || jabber_conn.conn_status == JABBER_CONNECTING
            || jabber_conn.conn_status == JABBER_DISCONNECTING)
        xmpp_run_once(jabber_conn.ctx, 10);
}

void
jabber_send(const char * const msg, const char * const recipient)
{
    if (prefs_get_states()) {
        if (!chat_session_exists(recipient)) {
            chat_session_start(recipient, TRUE);
        }
    }

    char *coded_msg = str_replace(msg, "&", "&amp;");
    char *coded_msg2 = str_replace(coded_msg, "<", "&lt;");
    char *coded_msg3 = str_replace(coded_msg2, ">", "&gt;");

    xmpp_stanza_t *reply, *body, *text, *active;

    reply = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(reply, "message");
    xmpp_stanza_set_type(reply, "chat");
    xmpp_stanza_set_attribute(reply, "to", recipient);

    body = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(body, "body");

    text = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_text(text, coded_msg3);

    if (prefs_get_states()) {

        // always send <active/> with messages when recipient supports chat states
        if (chat_session_get_recipient_supports(recipient)) {
            chat_session_set_active(recipient);
            active = xmpp_stanza_new(jabber_conn.ctx);
            xmpp_stanza_set_name(active, "active");
            xmpp_stanza_set_ns(active, "http://jabber.org/protocol/chatstates");
            xmpp_stanza_add_child(reply, active);
        }
    }

    xmpp_stanza_add_child(body, text);
    xmpp_stanza_add_child(reply, body);

    xmpp_send(jabber_conn.conn, reply);
    xmpp_stanza_release(reply);
    free(coded_msg);
    free(coded_msg2);
    free(coded_msg3);
}

void
jabber_send_inactive(const char * const recipient)
{
    xmpp_stanza_t *message, *inactive;

    message = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(message, "message");
    xmpp_stanza_set_type(message, "chat");
    xmpp_stanza_set_attribute(message, "to", recipient);

    inactive = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(inactive, "inactive");
    xmpp_stanza_set_ns(inactive, "http://jabber.org/protocol/chatstates");
    xmpp_stanza_add_child(message, inactive);

    xmpp_send(jabber_conn.conn, message);
    xmpp_stanza_release(message);

    chat_session_set_sent(recipient);
}

void
jabber_send_composing(const char * const recipient)
{
    xmpp_stanza_t *message, *composing;

    message = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(message, "message");
    xmpp_stanza_set_type(message, "chat");
    xmpp_stanza_set_attribute(message, "to", recipient);

    composing = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(composing, "composing");
    xmpp_stanza_set_ns(composing, "http://jabber.org/protocol/chatstates");
    xmpp_stanza_add_child(message, composing);

    xmpp_send(jabber_conn.conn, message);
    xmpp_stanza_release(message);

    chat_session_set_sent(recipient);
}

void
jabber_send_paused(const char * const recipient)
{
    xmpp_stanza_t *message, *paused;

    message = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(message, "message");
    xmpp_stanza_set_type(message, "chat");
    xmpp_stanza_set_attribute(message, "to", recipient);

    paused = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(paused, "paused");
    xmpp_stanza_set_ns(paused, "http://jabber.org/protocol/chatstates");
    xmpp_stanza_add_child(message, paused);

    xmpp_send(jabber_conn.conn, message);
    xmpp_stanza_release(message);

    chat_session_set_sent(recipient);
}

void
jabber_send_gone(const char * const recipient)
{
    xmpp_stanza_t *message, *gone;

    message = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(message, "message");
    xmpp_stanza_set_type(message, "chat");
    xmpp_stanza_set_attribute(message, "to", recipient);

    gone = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(gone, "gone");
    xmpp_stanza_set_ns(gone, "http://jabber.org/protocol/chatstates");
    xmpp_stanza_add_child(message, gone);

    xmpp_send(jabber_conn.conn, message);
    xmpp_stanza_release(message);

    chat_session_set_sent(recipient);
}

void
jabber_subscribe(const char * const recipient)
{
    xmpp_stanza_t *presence;

    presence = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(presence, "presence");
    xmpp_stanza_set_type(presence, "subscribe");
    xmpp_stanza_set_attribute(presence, "to", recipient);
    xmpp_send(jabber_conn.conn, presence);
    xmpp_stanza_release(presence);
}

void
jabber_join(const char * const room_jid, const char * const nick)
{
    xmpp_stanza_t *presence = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(presence, "presence");

    GString *to = g_string_new(room_jid);
    g_string_append(to, "/");
    g_string_append(to, nick);

    xmpp_stanza_set_attribute(presence, "to", to->str);
    xmpp_send(jabber_conn.conn, presence);
    xmpp_stanza_release(presence);
}

void
jabber_update_presence(jabber_presence_t status, const char * const msg)
{
    jabber_conn.presence = status;

    xmpp_stanza_t *pres, *show;

    pres = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(pres, "presence");

    if (status != PRESENCE_ONLINE) {
        show = xmpp_stanza_new(jabber_conn.ctx);
        xmpp_stanza_set_name(show, "show");
        xmpp_stanza_t *text = xmpp_stanza_new(jabber_conn.ctx);

        if (status == PRESENCE_AWAY)
            xmpp_stanza_set_text(text, "away");
        else if (status == PRESENCE_DND)
            xmpp_stanza_set_text(text, "dnd");
        else if (status == PRESENCE_CHAT)
            xmpp_stanza_set_text(text, "chat");
        else if (status == PRESENCE_XA)
            xmpp_stanza_set_text(text, "xa");
        else
            xmpp_stanza_set_text(text, "online");

        xmpp_stanza_add_child(show, text);
        xmpp_stanza_add_child(pres, show);
        xmpp_stanza_release(text);
        xmpp_stanza_release(show);
    }

    if (msg != NULL) {
        xmpp_stanza_t *status = xmpp_stanza_new(jabber_conn.ctx);
        xmpp_stanza_set_name(status, "status");
        xmpp_stanza_t *text = xmpp_stanza_new(jabber_conn.ctx);

        xmpp_stanza_set_text(text, msg);

        xmpp_stanza_add_child(status, text);
        xmpp_stanza_add_child(pres, status);
        xmpp_stanza_release(text);
        xmpp_stanza_release(status);
    }

    xmpp_send(jabber_conn.conn, pres);
    xmpp_stanza_release(pres);
}

jabber_conn_status_t
jabber_get_connection_status(void)
{
    return (jabber_conn.conn_status);
}

const char *
jabber_get_jid(void)
{
    return xmpp_conn_get_jid(jabber_conn.conn);
}

void
jabber_free_resources(void)
{
    chat_sessions_clear();
    xmpp_conn_release(jabber_conn.conn);
    xmpp_ctx_free(jabber_conn.ctx);
    xmpp_shutdown();
}

static void
_jabber_roster_request(void)
{
    xmpp_stanza_t *iq, *query;

    iq = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(iq, "iq");
    xmpp_stanza_set_type(iq, "get");
    xmpp_stanza_set_id(iq, "roster");

    query = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(query, "query");
    xmpp_stanza_set_ns(query, XMPP_NS_ROSTER);

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);
    xmpp_send(jabber_conn.conn, iq);
    xmpp_stanza_release(iq);
}

static int
_message_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    char *type = NULL;
    char *from = NULL;

    type = xmpp_stanza_get_attribute(stanza, "type");
    from = xmpp_stanza_get_attribute(stanza, "from");

    if (type != NULL) {
        if (strcmp(type, "error") == 0) {
            char *err_msg = NULL;
            xmpp_stanza_t *error = xmpp_stanza_get_child_by_name(stanza, "error");
            if (error == NULL) {
                log_debug("error message without <error/> received");
                return 1;
            } else {
                xmpp_stanza_t *err_cond = xmpp_stanza_get_children(error);
                if (err_cond == NULL) {
                    log_debug("error message without <defined-condition/> received");
                    return 1;
                } else {
                    err_msg = xmpp_stanza_get_name(err_cond);
                }
                // TODO: process 'type' attribute from <error/> [RFC6120, 8.3.2]
            }
            prof_handle_error_message(from, err_msg);
            return 1;
        }
    }


    char from_cpy[strlen(from) + 1];
    strcpy(from_cpy, from);
    char *short_from = strtok(from_cpy, "/");

    //determine chatstate support of recipient
    gboolean recipient_supports = FALSE;

    if ((xmpp_stanza_get_child_by_name(stanza, "active") != NULL) ||
            (xmpp_stanza_get_child_by_name(stanza, "composing") != NULL) ||
            (xmpp_stanza_get_child_by_name(stanza, "paused") != NULL) ||
            (xmpp_stanza_get_child_by_name(stanza, "gone") != NULL) ||
            (xmpp_stanza_get_child_by_name(stanza, "inactive") != NULL)) {
        recipient_supports = TRUE;
    }

    // create of update session
    if (!chat_session_exists(short_from)) {
        chat_session_start(short_from, recipient_supports);
    } else {
        chat_session_set_recipient_supports(short_from, recipient_supports);
    }

    // deal with chat states
    if (recipient_supports) {

        // handle <composing/>
        if (xmpp_stanza_get_child_by_name(stanza, "composing") != NULL) {
            if (prefs_get_notify_typing() || prefs_get_intype()) {
                prof_handle_typing(short_from);
            }

        // handle <paused/>
        } else if (xmpp_stanza_get_child_by_name(stanza, "paused") != NULL) {
            // do something

        // handle <inactive/>
        } else if (xmpp_stanza_get_child_by_name(stanza, "inactive") != NULL) {
            // do something

        // handle <gone/>
        } else if (xmpp_stanza_get_child_by_name(stanza, "gone") != NULL) {
            prof_handle_gone(short_from);

        // handle <active/>
        } else {
            // do something
        }
    }

    // check for and deal with message
    xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, "body");
    if (body != NULL) {
        char *message = xmpp_stanza_get_text(body);
        prof_handle_incoming_message(short_from, message);
    }

    return 1;
}

static void
_connection_handler(xmpp_conn_t * const conn,
    const xmpp_conn_event_t status, const int error,
    xmpp_stream_error_t * const stream_error, void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

    if (status == XMPP_CONN_CONNECT) {
        const char *jid = xmpp_conn_get_jid(conn);
        prof_handle_login_success(jid);
        chat_sessions_init();

        xmpp_handler_add(conn, _message_handler, NULL, "message", NULL, ctx);
        xmpp_handler_add(conn, _presence_handler, NULL, "presence", NULL, ctx);
        xmpp_id_handler_add(conn, _roster_handler, "roster", ctx);
        xmpp_timed_handler_add(conn, _ping_timed_handler, PING_INTERVAL, ctx);

        _jabber_roster_request();
        jabber_conn.conn_status = JABBER_CONNECTED;
        jabber_conn.presence = PRESENCE_ONLINE;
    } else {

        // received close stream response from server after disconnect
        if (jabber_conn.conn_status == JABBER_DISCONNECTING) {
            jabber_conn.conn_status = JABBER_DISCONNECTED;
            jabber_conn.presence = PRESENCE_OFFLINE;

        // lost connection for unkown reason
        } else if (jabber_conn.conn_status == JABBER_CONNECTED) {
            prof_handle_lost_connection();
            xmpp_stop(ctx);
            jabber_conn.conn_status = JABBER_DISCONNECTED;
            jabber_conn.presence = PRESENCE_OFFLINE;

        // login attempt failed
        } else {
            prof_handle_failed_login();
            xmpp_stop(ctx);
            jabber_conn.conn_status = JABBER_DISCONNECTED;
            jabber_conn.presence = PRESENCE_OFFLINE;
        }
    }
}

static int
_roster_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    xmpp_stanza_t *query, *item;
    char *type = xmpp_stanza_get_type(stanza);

    if (strcmp(type, "error") == 0)
        log_error("Roster query failed");
    else {
        query = xmpp_stanza_get_child_by_name(stanza, "query");
        item = xmpp_stanza_get_children(query);

        while (item != NULL) {
            const char *jid = xmpp_stanza_get_attribute(item, "jid");
            const char *name = xmpp_stanza_get_attribute(item, "name");
            const char *sub = xmpp_stanza_get_attribute(item, "subscription");
            gboolean added = contact_list_add(jid, name, "offline", NULL, sub);

            if (!added) {
                log_warning("Attempt to add contact twice: %s", jid);
            }

            item = xmpp_stanza_get_next(item);
        }
        xmpp_stanza_t* pres;
        pres = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(pres, "presence");
        xmpp_send(conn, pres);
        xmpp_stanza_release(pres);
    }

    return 1;
}

static int
_ping_timed_handler(xmpp_conn_t * const conn, void * const userdata)
{
    if (jabber_conn.conn_status == JABBER_CONNECTED) {
        xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

        xmpp_stanza_t *iq, *ping;

        iq = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(iq, "iq");
        xmpp_stanza_set_type(iq, "get");
        xmpp_stanza_set_id(iq, "c2s1");

        ping = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(ping, "ping");

        xmpp_stanza_set_ns(ping, "urn:xmpp:ping");

        xmpp_stanza_add_child(iq, ping);
        xmpp_stanza_release(ping);
        xmpp_send(conn, iq);
        xmpp_stanza_release(iq);
    }

    return 1;
}

static int
_presence_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    const char *jid = xmpp_conn_get_jid(jabber_conn.conn);
    char jid_cpy[strlen(jid) + 1];
    strcpy(jid_cpy, jid);
    char *short_jid = strtok(jid_cpy, "/");

    char *from = xmpp_stanza_get_attribute(stanza, "from");
    char *short_from = strtok(from, "/");
    char *type = xmpp_stanza_get_attribute(stanza, "type");
    char *show_str, *status_str;

    xmpp_stanza_t *status = xmpp_stanza_get_child_by_name(stanza, "status");
    if (status != NULL)
        status_str = xmpp_stanza_get_text(status);
    else
        status_str = NULL;

    if ((type != NULL) && (strcmp(type, "unavailable") == 0)) {
        if (strcmp(short_jid, short_from) !=0) {
            prof_handle_contact_offline(short_from, "offline", status_str);
        }
    } else {

        xmpp_stanza_t *show = xmpp_stanza_get_child_by_name(stanza, "show");
        if (show != NULL)
            show_str = xmpp_stanza_get_text(show);
        else
            show_str = "online";

        if (strcmp(short_jid, short_from) !=0) {
            prof_handle_contact_online(short_from, show_str, status_str);
        }
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

