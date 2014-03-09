/*
 * message.c
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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

#include <strophe.h>

#include "chat_session.h"
#include "config/preferences.h"
#include "log.h"
#include "muc.h"
#include "profanity.h"
#include "server_events.h"
#include "xmpp/connection.h"
#include "xmpp/message.h"
#include "xmpp/roster.h"
#include "roster_list.h"
#include "xmpp/stanza.h"
#include "xmpp/xmpp.h"
#include "ui/ui.h"

#define HANDLE(ns, type, func) xmpp_handler_add(conn, func, ns, STANZA_NAME_MESSAGE, type, ctx)

static int _groupchat_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _chat_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _muc_user_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _conference_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _captcha_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);
static int _message_error_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata);

void
message_add_handlers(void)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();

    HANDLE(NULL,                 STANZA_TYPE_ERROR,      _message_error_handler);
    HANDLE(NULL,                 STANZA_TYPE_GROUPCHAT,  _groupchat_handler);
    HANDLE(NULL,                 STANZA_TYPE_CHAT,       _chat_handler);
    HANDLE(STANZA_NS_MUC_USER,   NULL,                   _muc_user_handler);
    HANDLE(STANZA_NS_CONFERENCE, NULL,                   _conference_handler);
    HANDLE(STANZA_NS_CAPTCHA,    NULL,                   _captcha_handler);
}

static void
_message_send(const char * const msg, const char * const recipient)
{
    const char * jid = NULL;

    if (roster_barejid_from_name(recipient) != NULL) {
        jid = roster_barejid_from_name(recipient);
    } else {
        jid = recipient;
    }

    if (prefs_get_boolean(PREF_STATES)) {
        if (!chat_session_exists(jid)) {
            chat_session_start(jid, TRUE);
        }
    }

    xmpp_stanza_t *message;
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    if (prefs_get_boolean(PREF_STATES) && chat_session_get_recipient_supports(jid)) {
        chat_session_set_active(jid);
        message = stanza_create_message(ctx, jid, STANZA_TYPE_CHAT,
            msg, STANZA_NAME_ACTIVE);
    } else {
        message = stanza_create_message(ctx, jid, STANZA_TYPE_CHAT,
            msg, NULL);
    }

    xmpp_send(conn, message);
    xmpp_stanza_release(message);
}

static void
_message_send_groupchat(const char * const msg, const char * const recipient)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *message = stanza_create_message(ctx, recipient,
        STANZA_TYPE_GROUPCHAT, msg, NULL);

    xmpp_send(conn, message);
    xmpp_stanza_release(message);
}

static void
_message_send_duck(const char * const query)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *message = stanza_create_message(ctx, "im@ddg.gg",
        STANZA_TYPE_CHAT, query, NULL);

    xmpp_send(conn, message);
    xmpp_stanza_release(message);
}

static void
_message_send_invite(const char * const room, const char * const contact,
    const char * const reason)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *stanza = stanza_create_invite(ctx, room, contact, reason);

    xmpp_send(conn, stanza);
    xmpp_stanza_release(stanza);
}

static void
_message_send_composing(const char * const recipient)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *stanza = stanza_create_chat_state(ctx, recipient,
        STANZA_NAME_COMPOSING);

    xmpp_send(conn, stanza);
    xmpp_stanza_release(stanza);
    chat_session_set_sent(recipient);
}

static void
_message_send_paused(const char * const recipient)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *stanza = stanza_create_chat_state(ctx, recipient,
        STANZA_NAME_PAUSED);

    xmpp_send(conn, stanza);
    xmpp_stanza_release(stanza);
    chat_session_set_sent(recipient);
}

static void
_message_send_inactive(const char * const recipient)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *stanza = stanza_create_chat_state(ctx, recipient,
        STANZA_NAME_INACTIVE);

    xmpp_send(conn, stanza);
    xmpp_stanza_release(stanza);
    chat_session_set_sent(recipient);
}

static void
_message_send_gone(const char * const recipient)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *stanza = stanza_create_chat_state(ctx, recipient,
        STANZA_NAME_GONE);

    xmpp_send(conn, stanza);
    xmpp_stanza_release(stanza);
    chat_session_set_sent(recipient);
}

static int
_message_error_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    char *id = xmpp_stanza_get_id(stanza);
    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    xmpp_stanza_t *error_stanza = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_ERROR);
    char *type = NULL;
    if (error_stanza != NULL) {
        type = xmpp_stanza_get_attribute(error_stanza, STANZA_ATTR_TYPE);
    }

    // stanza_get_error never returns NULL
    char *err_msg = stanza_get_error_message(stanza);

    GString *log_msg = g_string_new("message stanza error received");
    if (id != NULL) {
        g_string_append(log_msg, " id=");
        g_string_append(log_msg, id);
    }
    if (from != NULL) {
        g_string_append(log_msg, " from=");
        g_string_append(log_msg, from);
    }
    if (type != NULL) {
        g_string_append(log_msg, " type=");
        g_string_append(log_msg, type);
    }
    g_string_append(log_msg, " error=");
    g_string_append(log_msg, err_msg);

    log_info(log_msg->str);

    g_string_free(log_msg, TRUE);

    handle_message_error(from, type, err_msg);

    free(err_msg);

    return 1;
}

static int
_muc_user_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_stanza_t *xns_muc_user = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
    char *room = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    char *invitor = NULL;
    char *reason = NULL;

    if (room == NULL) {
        log_warning("Message received with no from attribute, ignoring");
        return 1;
    }

    // XEP-0045
    xmpp_stanza_t *invite = xmpp_stanza_get_child_by_name(xns_muc_user, STANZA_NAME_INVITE);
    if (invite != NULL) {
        char *invitor_jid = xmpp_stanza_get_attribute(invite, STANZA_ATTR_FROM);
        if (invitor_jid == NULL) {
            log_warning("Chat room invite received with no from attribute");
            return 1;
        }

        Jid *jidp = jid_create(invitor_jid);
        if (jidp == NULL) {
            return 1;
        }
        invitor = jidp->barejid;

        xmpp_stanza_t *reason_st = xmpp_stanza_get_child_by_name(invite, STANZA_NAME_REASON);
        if (reason_st != NULL) {
            reason = xmpp_stanza_get_text(reason_st);
        }

        handle_room_invite(INVITE_MEDIATED, invitor, room, reason);
        jid_destroy(jidp);
        if (reason != NULL) {
            xmpp_free(ctx, reason);
        }
    }

    return 1;
}

static int
_conference_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    xmpp_stanza_t *xns_conference = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_CONFERENCE);
    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    char *room = NULL;
    char *invitor = NULL;
    char *reason = NULL;

    if (from == NULL) {
        log_warning("Message received with no from attribute, ignoring");
        return 1;
    }

    // XEP-0429
    room = xmpp_stanza_get_attribute(xns_conference, STANZA_ATTR_JID);
    if (room == NULL) {
        return 1;
    }

    Jid *jidp = jid_create(from);
    if (jidp == NULL) {
        return 1;
    }
    invitor = jidp->barejid;

    reason = xmpp_stanza_get_attribute(xns_conference, STANZA_ATTR_REASON);

    handle_room_invite(INVITE_DIRECT, invitor, room, reason);

    jid_destroy(jidp);


    return 1;
}

static int
_captcha_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    if (from == NULL) {
        log_warning("Message received with no from attribute, ignoring");
        return 1;
    }

    // XEP-0158
    xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);
    if (body != NULL) {
        char *message = xmpp_stanza_get_text(body);
        if (message != NULL) {
            handle_room_broadcast(from, message);
            xmpp_free(ctx, message);
        }
    }

    return 1;
}

static int
_groupchat_handler(xmpp_conn_t * const conn,
    xmpp_stanza_t * const stanza, void * const userdata)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    char *message = NULL;
    char *room_jid = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    Jid *jid = jid_create(room_jid);

    // handle room broadcasts
    if (jid->resourcepart == NULL) {
        xmpp_stanza_t *subject = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_SUBJECT);

        // handle subject
        if (subject != NULL) {
            message = xmpp_stanza_get_text(subject);
            if (message != NULL) {
                handle_room_subject(jid->barejid, message);
                xmpp_free(ctx, message);
            }

            jid_destroy(jid);
            return 1;

        // handle other room broadcasts
        } else {
            xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);
            if (body != NULL) {
                message = xmpp_stanza_get_text(body);
                if (message != NULL) {
                    handle_room_broadcast(room_jid, message);
                    xmpp_free(ctx, message);
                }
            }

            jid_destroy(jid);
            return 1;
        }
    }


    if (!jid_is_valid_room_form(jid)) {
        log_error("Invalid room JID: %s", jid->str);
        jid_destroy(jid);
        return 1;
    }

    // room not active in profanity
    if (!muc_room_is_active(jid->barejid)) {
        log_error("Message recieved for inactive chat room: %s", jid->str);
        jid_destroy(jid);
        return 1;
    }

    // determine if the notifications happened whilst offline
    GTimeVal tv_stamp;
    gboolean delayed = stanza_get_delay(stanza, &tv_stamp);
    xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);

    // check for and deal with message
    if (body != NULL) {
        message = xmpp_stanza_get_text(body);
        if (message != NULL) {
            if (delayed) {
                handle_room_history(jid->barejid, jid->resourcepart, tv_stamp, message);
            } else {
                handle_room_message(jid->barejid, jid->resourcepart, message);
            }
            xmpp_free(ctx, message);
        }
    }

    jid_destroy(jid);

    return 1;
}

static int
_chat_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    gchar *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    Jid *jid = jid_create(from);

    // handle ddg searches
    if (strcmp(jid->barejid, "im@ddg.gg") == 0) {
        xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);
        if (body != NULL) {
            char *message = xmpp_stanza_get_text(body);
            if (message != NULL) {
                handle_duck_result(message);
                xmpp_free(ctx, message);
            }
        }

        jid_destroy(jid);
        return 1;

    // private message from chat room use full jid (room/nick)
    } else if (muc_room_is_active(jid->barejid)) {
        // determine if the notifications happened whilst offline
        GTimeVal tv_stamp;
        gboolean delayed = stanza_get_delay(stanza, &tv_stamp);

        // check for and deal with message
        xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);
        if (body != NULL) {
            char *message = xmpp_stanza_get_text(body);
            if (message != NULL) {
                if (delayed) {
                    handle_delayed_message(jid->str, message, tv_stamp, TRUE);
                } else {
                    handle_incoming_message(jid->str, message, TRUE);
                }
                xmpp_free(ctx, message);
            }
        }

        jid_destroy(jid);
        return 1;

    // standard chat message, use jid without resource
    } else {
        // determine chatstate support of recipient
        gboolean recipient_supports = FALSE;
        if (stanza_contains_chat_state(stanza)) {
            recipient_supports = TRUE;
        }

        // create or update chat session
        if (!chat_session_exists(jid->barejid)) {
            chat_session_start(jid->barejid, recipient_supports);
        } else {
            chat_session_set_recipient_supports(jid->barejid, recipient_supports);
        }

        // determine if the notifications happened whilst offline
        GTimeVal tv_stamp;
        gboolean delayed = stanza_get_delay(stanza, &tv_stamp);

        // deal with chat states if recipient supports them
        if (recipient_supports && (!delayed)) {
            if (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_COMPOSING) != NULL) {
                if (prefs_get_boolean(PREF_NOTIFY_TYPING) || prefs_get_boolean(PREF_INTYPE)) {
                    handle_typing(jid->barejid);
                }
            } else if (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_GONE) != NULL) {
                handle_gone(jid->barejid);
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
            if (message != NULL) {
                if (delayed) {
                    handle_delayed_message(jid->barejid, message, tv_stamp, FALSE);
                } else {
                    handle_incoming_message(jid->barejid, message, FALSE);
                }
                xmpp_free(ctx, message);
            }
        }

        jid_destroy(jid);
        return 1;
    }
}

void
message_init_module(void)
{
    message_send = _message_send;
    message_send_groupchat = _message_send_groupchat;
    message_send_duck = _message_send_duck;
    message_send_invite = _message_send_invite;
    message_send_composing = _message_send_composing;
    message_send_paused = _message_send_paused;
    message_send_inactive = _message_send_inactive;
    message_send_gone = _message_send_gone;
}

