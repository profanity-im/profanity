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

void
message_send_chat(const char * const barejid, const char * const resource, const char * const msg, gboolean send_state)
{
    xmpp_stanza_t *message;
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();

    GString *jid = g_string_new(barejid);
    if (resource) {
        g_string_append(jid, "/");
        g_string_append(jid, resource);
    }

    if (send_state) {
        message = stanza_create_message(ctx, jid->str, STANZA_TYPE_CHAT, msg, STANZA_NAME_ACTIVE);
    } else {
        message = stanza_create_message(ctx, jid->str, STANZA_TYPE_CHAT, msg, NULL);
    }

    xmpp_send(conn, message);
    xmpp_stanza_release(message);
    g_string_free(jid, TRUE);
}

void
message_send_private(const char * const fulljid, const char * const msg)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *message = stanza_create_message(ctx, fulljid, STANZA_TYPE_CHAT, msg, NULL);

    xmpp_send(conn, message);
    xmpp_stanza_release(message);
}

void
message_send_groupchat(const char * const roomjid, const char * const msg)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *message = stanza_create_message(ctx, roomjid, STANZA_TYPE_GROUPCHAT, msg, NULL);

    xmpp_send(conn, message);
    xmpp_stanza_release(message);
}

void
message_send_groupchat_subject(const char * const roomjid, const char * const subject)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *message = stanza_create_room_subject_message(ctx, roomjid, subject);

    xmpp_send(conn, message);
    xmpp_stanza_release(message);
}

void
message_send_invite(const char * const roomjid, const char * const contact,
    const char * const reason)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *stanza = stanza_create_invite(ctx, roomjid, contact, reason);

    xmpp_send(conn, stanza);
    xmpp_stanza_release(stanza);
}

void
message_send_composing(const char * const barejid)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *stanza = stanza_create_chat_state(ctx, barejid,
        STANZA_NAME_COMPOSING);

    xmpp_send(conn, stanza);
    xmpp_stanza_release(stanza);
}

void
message_send_paused(const char * const barejid)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *stanza = stanza_create_chat_state(ctx, barejid,
        STANZA_NAME_PAUSED);

    xmpp_send(conn, stanza);
    xmpp_stanza_release(stanza);
}

void
message_send_inactive(const char * const barejid)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *stanza = stanza_create_chat_state(ctx, barejid,
        STANZA_NAME_INACTIVE);

    xmpp_send(conn, stanza);
    xmpp_stanza_release(stanza);
}

void
message_send_gone(const char * const barejid)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *stanza = stanza_create_chat_state(ctx, barejid,
        STANZA_NAME_GONE);

    xmpp_send(conn, stanza);
    xmpp_stanza_release(stanza);
}

static int
_message_error_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    char *id = xmpp_stanza_get_id(stanza);
    char *jid = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
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
    if (jid != NULL) {
        g_string_append(log_msg, " from=");
        g_string_append(log_msg, jid);
    }
    if (type != NULL) {
        g_string_append(log_msg, " type=");
        g_string_append(log_msg, type);
    }
    g_string_append(log_msg, " error=");
    g_string_append(log_msg, err_msg);

    log_info(log_msg->str);

    g_string_free(log_msg, TRUE);

    handle_message_error(jid, type, err_msg);

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
        char *invitor = jidp->barejid;

        char *reason = NULL;
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

    // handle room subject
    xmpp_stanza_t *subject = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_SUBJECT);
    if (subject != NULL) {
        message = xmpp_stanza_get_text(subject);
        handle_room_subject(jid->barejid, jid->resourcepart, message);
        xmpp_free(ctx, message);

        jid_destroy(jid);
        return 1;
    }

    // handle room broadcasts
    if (jid->resourcepart == NULL) {
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

    if (!jid_is_valid_room_form(jid)) {
        log_error("Invalid room JID: %s", jid->str);
        jid_destroy(jid);
        return 1;
    }

    // room not active in profanity
    if (!muc_active(jid->barejid)) {
        log_error("Message received for inactive chat room: %s", jid->str);
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

    // private message from chat room use full jid (room/nick)
    if (muc_active(jid->barejid)) {
        // determine if the notifications happened whilst offline
        GTimeVal tv_stamp;
        gboolean delayed = stanza_get_delay(stanza, &tv_stamp);

        // check for and deal with message
        xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);
        if (body != NULL) {
            char *message = xmpp_stanza_get_text(body);
            if (message != NULL) {
                if (delayed) {
                    handle_delayed_private_message(jid->str, message, tv_stamp);
                } else {
                    handle_incoming_private_message(jid->str, message);
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
        chat_session_on_incoming_message(jid->barejid, recipient_supports);

        // determine if the notifications happened whilst offline
        GTimeVal tv_stamp;
        gboolean delayed = stanza_get_delay(stanza, &tv_stamp);

        // deal with chat states if recipient supports them
        if (recipient_supports && (!delayed)) {
            if (xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_COMPOSING) != NULL) {
                handle_typing(jid->barejid);
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
                    handle_delayed_message(jid->barejid, message, tv_stamp);
                } else {
                    handle_incoming_message(jid->barejid, message);
                }
                xmpp_free(ctx, message);
            }
        }

        jid_destroy(jid);
        return 1;
    }
}