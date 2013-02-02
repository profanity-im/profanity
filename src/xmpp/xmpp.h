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

typedef enum {
    JABBER_UNDEFINED,
    JABBER_STARTED,
    JABBER_CONNECTING,
    JABBER_CONNECTED,
    JABBER_DISCONNECTING,
    JABBER_DISCONNECTED
} jabber_conn_status_t;

typedef enum {
    PRESENCE_SUBSCRIBE,
    PRESENCE_SUBSCRIBED,
    PRESENCE_UNSUBSCRIBED
} jabber_subscr_t;

typedef struct capabilities_t {
    char *client;
} Capabilities;

// connection functions
void jabber_init(const int disable_tls);
jabber_conn_status_t jabber_connect_with_details(const char * const jid,
    const char * const passwd, const char * const altdomain);
jabber_conn_status_t jabber_connect_with_account(ProfAccount *account,
    const char * const passwd);
void jabber_disconnect(void);
void jabber_process_events(void);
const char * jabber_get_jid(void);
jabber_conn_status_t jabber_get_connection_status(void);
jabber_presence_t jabber_get_presence_type(void);
char * jabber_get_presence_message(void);
void jabber_free_resources(void);
void jabber_restart(void);
void jabber_set_autoping(int seconds);
xmpp_conn_t *jabber_get_conn(void);
xmpp_ctx_t *jabber_get_ctx(void);
int error_handler(xmpp_stanza_t * const stanza);
void jabber_conn_set_presence_type(jabber_presence_t presence_type);
void jabber_conn_set_priority(int priority);
void jabber_conn_set_presence_message(const char * const message);
char* jabber_get_account_name(void);

// message functions
void message_send(const char * const msg, const char * const recipient);
void message_send_groupchat(const char * const msg, const char * const recipient);
void message_send_inactive(const char * const recipient);
void message_send_composing(const char * const recipient);
void message_send_paused(const char * const recipient);
void message_send_gone(const char * const recipient);

// presence functions
void presence_subscription(const char * const jid, const jabber_subscr_t action);
GList* presence_get_subscription_requests(void);
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

#endif
