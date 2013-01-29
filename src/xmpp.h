/*
 * xmpp.h
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

#ifndef XMPP_H
#define XMPP_H

#include <strophe.h>

#include "accounts.h"
#include "jid.h"

#define JABBER_PRIORITY_MIN -128
#define JABBER_PRIORITY_MAX 127

#define STANZA_NAME_ACTIVE "active"
#define STANZA_NAME_INACTIVE "inactive"
#define STANZA_NAME_COMPOSING "composing"
#define STANZA_NAME_PAUSED "paused"
#define STANZA_NAME_GONE "gone"

#define STANZA_NAME_MESSAGE "message"
#define STANZA_NAME_BODY "body"
#define STANZA_NAME_PRESENCE "presence"
#define STANZA_NAME_PRIORITY "priority"
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
#define STANZA_NAME_ITEM "item"
#define STANZA_NAME_C "c"
#define STANZA_NAME_IDENTITY "identity"
#define STANZA_NAME_FEATURE "feature"

#define STANZA_TYPE_CHAT "chat"
#define STANZA_TYPE_GROUPCHAT "groupchat"
#define STANZA_TYPE_UNAVAILABLE "unavailable"
#define STANZA_TYPE_SUBSCRIBE "subscribe"
#define STANZA_TYPE_SUBSCRIBED "subscribed"
#define STANZA_TYPE_UNSUBSCRIBED "unsubscribed"
#define STANZA_TYPE_GET "get"
#define STANZA_TYPE_SET "set"
#define STANZA_TYPE_ERROR "error"
#define STANZA_TYPE_RESULT "result"

#define STANZA_ATTR_TO "to"
#define STANZA_ATTR_FROM "from"
#define STANZA_ATTR_STAMP "stamp"
#define STANZA_ATTR_TYPE "type"
#define STANZA_ATTR_CODE "code"
#define STANZA_ATTR_JID "jid"
#define STANZA_ATTR_NAME "name"
#define STANZA_ATTR_SUBSCRIPTION "subscription"
#define STANZA_ATTR_XMLNS "xmlns"
#define STANZA_ATTR_NICK "nick"
#define STANZA_ATTR_ASK "ask"
#define STANZA_ATTR_ID "id"
#define STANZA_ATTR_SECONDS "seconds"
#define STANZA_ATTR_NODE "node"
#define STANZA_ATTR_VER "ver"
#define STANZA_ATTR_VAR "var"
#define STANZA_ATTR_HASH "hash"

#define STANZA_TEXT_AWAY "away"
#define STANZA_TEXT_DND "dnd"
#define STANZA_TEXT_CHAT "chat"
#define STANZA_TEXT_XA "xa"
#define STANZA_TEXT_ONLINE "online"

#define STANZA_NS_CHATSTATES "http://jabber.org/protocol/chatstates"
#define STANZA_NS_MUC "http://jabber.org/protocol/muc"
#define STANZA_NS_MUC_USER "http://jabber.org/protocol/muc#user"
#define STANZA_NS_CAPS "http://jabber.org/protocol/caps"
#define STANZA_NS_PING "urn:xmpp:ping"
#define STANZA_NS_LASTACTIVITY "jabber:iq:last"
#define STANZA_NS_DATA "jabber:x:data"
#define STANZA_NS_VERSION "jabber:iq:version"

typedef enum {
    JABBER_UNDEFINED,
    JABBER_STARTED,
    JABBER_CONNECTING,
    JABBER_CONNECTED,
    JABBER_DISCONNECTING,
    JABBER_DISCONNECTED
} jabber_conn_status_t;

typedef enum {
    PRESENCE_OFFLINE,
    PRESENCE_ONLINE,
    PRESENCE_AWAY,
    PRESENCE_DND,
    PRESENCE_CHAT,
    PRESENCE_XA
} jabber_presence_t;

typedef enum {
    PRESENCE_SUBSCRIBE,
    PRESENCE_SUBSCRIBED,
    PRESENCE_UNSUBSCRIBED
} jabber_subscr_t;

typedef struct capabilities_t {
    char *client;
} Capabilities;

typedef struct form_field_t {
    char *var;
    GSList *values;
} FormField;

typedef struct data_form_t {
    char *form_type;
    GSList *fields;
} DataForm;

// connection functions
void jabber_init(const int disable_tls);
jabber_conn_status_t jabber_connect_with_details(const char * const jid,
    const char * const passwd, const char * const altdomain);
jabber_conn_status_t jabber_connect_with_account(ProfAccount *account,
    const char * const passwd);
void jabber_disconnect(void);
void jabber_process_events(void);
void jabber_send(const char * const msg, const char * const recipient);
void jabber_send_groupchat(const char * const msg, const char * const recipient);
void jabber_send_inactive(const char * const recipient);
void jabber_send_composing(const char * const recipient);
void jabber_send_paused(const char * const recipient);
void jabber_send_gone(const char * const recipient);
const char * jabber_get_jid(void);
jabber_conn_status_t jabber_get_connection_status(void);
int jabber_get_priority(void);
jabber_presence_t jabber_get_presence(void);
char * jabber_get_status(void);
void jabber_free_resources(void);
void jabber_restart(void);
void jabber_set_autoping(int seconds);
xmpp_conn_t *jabber_get_conn(void);
xmpp_ctx_t *jabber_get_ctx(void);
int error_handler(xmpp_stanza_t * const stanza);
void jabber_conn_set_presence(jabber_presence_t presence);
void jabber_conn_set_priority(int priority);
void jabber_conn_set_status(const char * const message);
char* jabber_get_account_name(void);

// iq functions
void iq_add_handlers(void);

// presence functions
void presence_add_handlers(void);
void presence_init(void);
void presence_subscription(const char * const jid, const jabber_subscr_t action);
GList* presence_get_subscription_requests(void);
void presence_free_sub_requests(void);
void presence_join_room(Jid *jid);
void presence_change_room_nick(const char * const room, const char * const nick);
void presence_leave_chat_room(const char * const room_jid);
void presence_update(jabber_presence_t status, const char * const msg,
    int idle);

// caps functions
void caps_init(void);
void caps_add(const char * const caps_str, const char * const client);
gboolean caps_contains(const char * const caps_str);
Capabilities* caps_get(const char * const caps_str);
char* caps_create_sha1_str(xmpp_stanza_t * const query);
xmpp_stanza_t* caps_create_query_response_stanza(xmpp_ctx_t * const ctx);
void caps_close(void);

// stanza related functions
xmpp_stanza_t* stanza_create_chat_state(xmpp_ctx_t *ctx,
    const char * const recipient, const char * const state);

xmpp_stanza_t* stanza_create_message(xmpp_ctx_t *ctx,
    const char * const recipient, const char * const type,
    const char * const message, const char * const state);

xmpp_stanza_t* stanza_create_room_join_presence(xmpp_ctx_t *ctx,
    const char * const full_room_jid);

xmpp_stanza_t* stanza_create_room_newnick_presence(xmpp_ctx_t *ctx,
    const char * const full_room_jid);

xmpp_stanza_t* stanza_create_room_leave_presence(xmpp_ctx_t *ctx,
    const char * const room, const char * const nick);

xmpp_stanza_t* stanza_create_presence(xmpp_ctx_t *ctx, const char * const show,
    const char * const status);

xmpp_stanza_t* stanza_create_roster_iq(xmpp_ctx_t *ctx);
xmpp_stanza_t* stanza_create_ping_iq(xmpp_ctx_t *ctx);
xmpp_stanza_t* stanza_create_disco_iq(xmpp_ctx_t *ctx, const char * const id,
    const char * const to, const char * const node);

gboolean stanza_contains_chat_state(xmpp_stanza_t *stanza);

gboolean stanza_get_delay(xmpp_stanza_t * const stanza, GTimeVal *tv_stamp);

gboolean stanza_is_muc_self_presence(xmpp_stanza_t * const stanza,
    const char * const self_jid);
gboolean stanza_is_room_nick_change(xmpp_stanza_t * const stanza);

char * stanza_get_new_nick(xmpp_stanza_t * const stanza);

int stanza_get_idle_time(xmpp_stanza_t * const stanza);
char * stanza_get_caps_str(xmpp_stanza_t * const stanza);
gboolean stanza_contains_caps(xmpp_stanza_t * const stanza);
char * stanza_caps_get_hash(xmpp_stanza_t * const stanza);
gboolean stanza_is_caps_request(xmpp_stanza_t * const stanza);

gboolean stanza_is_version_request(xmpp_stanza_t * const stanza);

DataForm * stanza_create_form(xmpp_stanza_t * const stanza);
void stanza_destroy_form(DataForm *form);

#endif
