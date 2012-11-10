/*
 * stanza.h
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

#ifndef STANZA_H
#define STANZA_H

#include <strophe.h>

#define STANZA_NAME_ACTIVE "active"
#define STANZA_NAME_INACTIVE "inactive"
#define STANZA_NAME_COMPOSING "composing"
#define STANZA_NAME_PAUSED "paused"
#define STANZA_NAME_GONE "gone"

#define STANZA_NAME_MESSAGE "message"
#define STANZA_NAME_BODY "body"
#define STANZA_NAME_PRESENCE "presence"
#define STANZA_NAME_X "x"
#define STANZA_NAME_SHOW "show"
#define STANZA_NAME_STATUS "status"
#define STANZA_NAME_IQ "iq"
#define STANZA_NAME_QUERY "query"
#define STANZA_NAME_DELAY "delay"
#define STANZA_NAME_ERROR "error"
#define STANZA_NAME_PING "ping"
#define STANZA_NAME_TEXT "text"
#define STANZA_NAME_SUBJECT "subject"

#define STANZA_TYPE_CHAT "chat"
#define STANZA_TYPE_GROUPCHAT "groupchat"
#define STANZA_TYPE_UNAVAILABLE "unavailable"
#define STANZA_TYPE_SUBSCRIBE "subscribe"
#define STANZA_TYPE_GET "get"
#define STANZA_TYPE_ERROR "error"

#define STANZA_ATTR_TO "to"
#define STANZA_ATTR_FROM "from"
#define STANZA_ATTR_STAMP "stamp"
#define STANZA_ATTR_TYPE "type"
#define STANZA_ATTR_JID "jid"
#define STANZA_ATTR_NAME "name"
#define STANZA_ATTR_SUBSCRIPTION "subscription"

#define STANZA_TEXT_AWAY "away"
#define STANZA_TEXT_DND "dnd"
#define STANZA_TEXT_CHAT "chat"
#define STANZA_TEXT_XA "xa"
#define STANZA_TEXT_ONLINE "online"

#define STANZA_NS_CHATSTATES "http://jabber.org/protocol/chatstates"
#define STANZA_NS_MUC "http://jabber.org/protocol/muc"
#define STANZA_NS_PING "urn:xmpp:ping"

xmpp_stanza_t* stanza_create_chat_state(xmpp_ctx_t *ctx,
    const char * const recipient, const char * const state);

xmpp_stanza_t* stanza_create_message(xmpp_ctx_t *ctx,
    const char * const recipient, const char * const type,
    const char * const message, const char * const state);

xmpp_stanza_t* stanza_create_room_join_presence(xmpp_ctx_t *ctx,
    const char * const full_room_jid);

xmpp_stanza_t* stanza_create_room_leave_presence(xmpp_ctx_t *ctx,
    const char * const room, const char * const nick);

xmpp_stanza_t* stanza_create_presence(xmpp_ctx_t *ctx, const char * const show,
    const char * const status);

xmpp_stanza_t* stanza_create_roster_iq(xmpp_ctx_t *ctx);
xmpp_stanza_t* stanza_create_ping_iq(xmpp_ctx_t *ctx);

gboolean stanza_contains_chat_state(xmpp_stanza_t *stanza);

#endif
