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
#include "room_chat.h"
#include "stanza.h"

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
static int _groupchat_message_handler(xmpp_stanza_t * const stanza);
static int _error_handler(xmpp_stanza_t * const stanza);
static int _chat_message_handler(xmpp_stanza_t * const stanza);

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

    xmpp_stanza_t *message;
    if (prefs_get_states() && chat_session_get_recipient_supports(recipient)) {
        chat_session_set_active(recipient);
        message = stanza_create_message(jabber_conn.ctx, recipient, STANZA_TYPE_CHAT,
            msg, STANZA_NAME_ACTIVE);
    } else {
        message = stanza_create_message(jabber_conn.ctx, recipient, STANZA_TYPE_CHAT,
            msg, NULL);
    }

    xmpp_send(jabber_conn.conn, message);
    xmpp_stanza_release(message);
}

void
jabber_send_groupchat(const char * const msg, const char * const recipient)
{
    xmpp_stanza_t *message = stanza_create_message(jabber_conn.ctx, recipient,
        STANZA_TYPE_GROUPCHAT, msg, NULL);

    xmpp_send(jabber_conn.conn, message);
    xmpp_stanza_release(message);
}

void
jabber_send_composing(const char * const recipient)
{
    xmpp_stanza_t *stanza = stanza_create_chat_state(jabber_conn.ctx, recipient,
        STANZA_NAME_COMPOSING);

    xmpp_send(jabber_conn.conn, stanza);
    xmpp_stanza_release(stanza);
    chat_session_set_sent(recipient);
}

void
jabber_send_paused(const char * const recipient)
{
    xmpp_stanza_t *stanza = stanza_create_chat_state(jabber_conn.ctx, recipient,
        STANZA_NAME_PAUSED);

    xmpp_send(jabber_conn.conn, stanza);
    xmpp_stanza_release(stanza);
    chat_session_set_sent(recipient);
}

void
jabber_send_inactive(const char * const recipient)
{
    xmpp_stanza_t *stanza = stanza_create_chat_state(jabber_conn.ctx, recipient,
        STANZA_NAME_INACTIVE);

    xmpp_send(jabber_conn.conn, stanza);
    xmpp_stanza_release(stanza);
    chat_session_set_sent(recipient);
}

void
jabber_send_gone(const char * const recipient)
{
    xmpp_stanza_t *stanza = stanza_create_chat_state(jabber_conn.ctx, recipient,
        STANZA_NAME_GONE);

    xmpp_send(jabber_conn.conn, stanza);
    xmpp_stanza_release(stanza);
    chat_session_set_sent(recipient);
}

void
jabber_subscribe(const char * const recipient)
{
    xmpp_stanza_t *presence;

    presence = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(presence, STANZA_NAME_PRESENCE);
    xmpp_stanza_set_type(presence, STANZA_TYPE_SUBSCRIBE);
    xmpp_stanza_set_attribute(presence, STANZA_ATTR_TO, recipient);
    xmpp_send(jabber_conn.conn, presence);
    xmpp_stanza_release(presence);
}

void
jabber_join(const char * const room, const char * const nick)
{
    xmpp_stanza_t *presence = stanza_create_room_join_presence(jabber_conn.ctx,
        room, nick);
    xmpp_send(jabber_conn.conn, presence);
    xmpp_stanza_release(presence);

    room_join(room, nick);
}

void
jabber_leave_chat_room(const char * const room_jid)
{
    char *nick = room_get_nick_for_room(room_jid);

    xmpp_stanza_t *presence = stanza_create_room_leave_presence(jabber_conn.ctx,
        room_jid, nick);
    xmpp_send(jabber_conn.conn, presence);
    xmpp_stanza_release(presence);
}

void
jabber_update_presence(jabber_presence_t status, const char * const msg)
{
    jabber_conn.presence = status;

    char *show = NULL;
    switch(status)
    {
        case PRESENCE_AWAY:
            show = STANZA_TEXT_AWAY;
            break;
        case PRESENCE_DND:
            show = STANZA_TEXT_DND;
            break;
        case PRESENCE_CHAT:
            show = STANZA_TEXT_CHAT;
            break;
        case PRESENCE_XA:
            show = STANZA_TEXT_XA;
            break;
        default:
            show = STANZA_TEXT_ONLINE;
            break;
    }

    xmpp_stanza_t *presence = stanza_create_presence(jabber_conn.ctx, show, msg);
    xmpp_send(jabber_conn.conn, presence);
    xmpp_stanza_release(presence);
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
    xmpp_stanza_t *iq = stanza_create_roster_iq(jabber_conn.ctx);
    xmpp_send(jabber_conn.conn, iq);
    xmpp_stanza_release(iq);
}

static int
_message_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    gchar *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);

    if (type == NULL) {
        log_error("Message stanza received with no type attribute");
        return 1;
    } else if (strcmp(type, STANZA_TYPE_ERROR) == 0) {
        return _error_handler(stanza);
    } else if (strcmp(type, STANZA_TYPE_GROUPCHAT) == 0) {
        return _groupchat_message_handler(stanza);
    } else if (strcmp(type, STANZA_TYPE_CHAT) == 0) {
        return _chat_message_handler(stanza);
    } else {
        log_error("Message stanza received with unknown type: %s", type);
        return 1;
    }
}

static int
_groupchat_message_handler(xmpp_stanza_t * const stanza)
{
    char *room = NULL;
    char *nick = NULL;
    char *message = NULL;
    xmpp_stanza_t *subject = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_SUBJECT);
    gchar *room_jid = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    // handle subject
    if (subject != NULL) {
        message = xmpp_stanza_get_text(subject);
        if (message != NULL) {
            room = room_get_room_for_full_jid(room_jid);
            prof_handle_room_subject(room, message);
        }

        return 1;
    }

    // room jid not of form room/nick
    if (!room_parse_room_jid(room_jid, &room, &nick)) {
        log_error("Could not parse room jid: %s", room_jid);
        g_free(room);
        g_free(nick);

        return 1;
    }

    // room not active in profanity
    if (!room_is_active(room_jid)) {
        log_error("Message recieved for inactive groupchat: %s", room_jid);
        g_free(room);
        g_free(nick);

        return 1;
    }

    xmpp_stanza_t *delay = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_DELAY);
    xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);

    if (body != NULL) {
        message = xmpp_stanza_get_text(body);
    }

    // handle chat room history
    if (delay != NULL) {
        char *utc_stamp = xmpp_stanza_get_attribute(delay, STANZA_ATTR_STAMP);
        GTimeVal tv_stamp;

        if (g_time_val_from_iso8601(utc_stamp, &tv_stamp)) {
            if (message != NULL) {
                prof_handle_room_history(room, nick, tv_stamp, message);
            }
        } else {
            log_error("Couldn't parse datetime string receiving room history: %s", utc_stamp);
        }

    // handle regular chat room message
    } else {
        if (message != NULL) {
            prof_handle_room_message(room, nick, message);
        }
    }

    return 1;
}

static int
_error_handler(xmpp_stanza_t * const stanza)
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

static int
_chat_message_handler(xmpp_stanza_t * const stanza)
{
    gchar *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    char from_cpy[strlen(from) + 1];
    strcpy(from_cpy, from);
    char *short_from = strtok(from_cpy, "/");

    // determine chatstate support of recipient
    gboolean recipient_supports = FALSE;
    if (stanza_contains_chat_state(stanza)) {
        recipient_supports = TRUE;
    }

    // create or update chat session
    if (!chat_session_exists(short_from)) {
        chat_session_start(short_from, recipient_supports);
    } else {
        chat_session_set_recipient_supports(short_from, recipient_supports);
    }

    // determine if the notifications happened whilst offline
    xmpp_stanza_t *delay = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_DELAY);

    // deal with chat states if recipient supports them
    if (recipient_supports && (delay == NULL)) {
        if (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_COMPOSING) != NULL) {
            if (prefs_get_notify_typing() || prefs_get_intype()) {
                prof_handle_typing(short_from);
            }
        } else if (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_GONE) != NULL) {
            prof_handle_gone(short_from);
        } else if (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_PAUSED) != NULL) {
            // do something
        } else if (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_INACTIVE) != NULL) {
            // do something
        } else { // handle <active/>
            // do something
        }
    }

    // check for and deal with message
    xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);
    if (body != NULL) {
        char *message = xmpp_stanza_get_text(body);
        if (delay != NULL) {
            char *utc_stamp = xmpp_stanza_get_attribute(delay, STANZA_ATTR_STAMP);
            GTimeVal tv_stamp;

            if (g_time_val_from_iso8601(utc_stamp, &tv_stamp)) {
                if (message != NULL) {
                    prof_handle_delayed_message(short_from, message, tv_stamp);
                }
            } else {
                log_error("Couldn't parse datetime string of historic message: %s", utc_stamp);
            }
        } else {
            prof_handle_incoming_message(short_from, message);
        }
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

        xmpp_handler_add(conn, _message_handler, NULL, STANZA_NAME_MESSAGE, NULL, ctx);
        xmpp_handler_add(conn, _presence_handler, NULL, STANZA_NAME_PRESENCE, NULL, ctx);
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

    if (strcmp(type, STANZA_TYPE_ERROR) == 0)
        log_error("Roster query failed");
    else {
        query = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_QUERY);
        item = xmpp_stanza_get_children(query);

        while (item != NULL) {
            const char *jid = xmpp_stanza_get_attribute(item, STANZA_ATTR_JID);
            const char *name = xmpp_stanza_get_attribute(item, STANZA_ATTR_NAME);
            const char *sub = xmpp_stanza_get_attribute(item, STANZA_ATTR_SUBSCRIPTION);
            gboolean added = contact_list_add(jid, name, "offline", NULL, sub);

            if (!added) {
                log_warning("Attempt to add contact twice: %s", jid);
            }

            item = xmpp_stanza_get_next(item);
        }
        xmpp_stanza_t* pres;
        pres = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(pres, STANZA_NAME_PRESENCE);
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

        xmpp_stanza_t *iq = stanza_create_ping_iq(ctx);
        xmpp_send(conn, iq);
        xmpp_stanza_release(iq);
    }

    return 1;
}

static int
_room_presence_handler(const char * const jid, xmpp_stanza_t * const stanza)
{
    char *room = NULL;
    char *nick = NULL;

    if (!room_parse_room_jid(jid, &room, &nick)) {
        log_error("Could not parse room jid: %s", room);
        g_free(room);
        g_free(nick);

        return 1;
    }

    // handle self presence
    if (strcmp(room_get_nick_for_room(room), nick) == 0) {
        char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);

        // left room
        if (type != NULL) {
            if (strcmp(type, STANZA_TYPE_UNAVAILABLE) == 0) {
                prof_handle_leave_room(room);
            }

        // roster received
        } else {
            prof_handle_room_roster_complete(room);
        }

    // handle presence from room members
    } else {
        // roster not yet complete, just add to roster
        if (!room_get_roster_received(room)) {
            room_add_to_roster(room, nick);

        // deal with presence information
        } else {
            char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);
            char *show_str, *status_str;

            xmpp_stanza_t *status = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_STATUS);
            if (status != NULL) {
                status_str = xmpp_stanza_get_text(status);
            } else {
                status_str = NULL;
            }

            if ((type != NULL) && (strcmp(type, STANZA_TYPE_UNAVAILABLE) == 0)) {
                prof_handle_room_member_offline(room, nick, "offline", status_str);
            } else {
                xmpp_stanza_t *show = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_SHOW);
                if (show != NULL) {
                    show_str = xmpp_stanza_get_text(show);
                } else {
                    show_str = "online";
                    prof_handle_room_member_online(room, nick, show_str, status_str);
                }
            }
        }
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

    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);

    if ((type != NULL) && (strcmp(type, STANZA_TYPE_ERROR) == 0)) {
        return _error_handler(stanza);
    }

    // handle chat room presence
    if (room_is_active(from)) {
        return _room_presence_handler(from, stanza);

    // handle regular presence
    } else {
        char *short_from = strtok(from, "/");
        char *type = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_TYPE);
        char *show_str, *status_str;

        xmpp_stanza_t *status = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_STATUS);
        if (status != NULL)
            status_str = xmpp_stanza_get_text(status);
        else
            status_str = NULL;

        if ((type != NULL) && (strcmp(type, STANZA_TYPE_UNAVAILABLE) == 0)) {
            if (strcmp(short_jid, short_from) !=0) {
                prof_handle_contact_offline(short_from, "offline", status_str);
            }
        } else {

            xmpp_stanza_t *show = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_SHOW);
            if (show != NULL)
                show_str = xmpp_stanza_get_text(show);
            else
                show_str = "online";

            if (strcmp(short_jid, short_from) !=0) {
                prof_handle_contact_online(short_from, show_str, status_str);
            }
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

