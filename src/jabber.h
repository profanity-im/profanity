/*
 * jabber.h
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

#ifndef JABBER_H
#define JABBER_H

#include "accounts.h"
#include "jid.h"

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

#define JABBER_PRIORITY_MIN -128
#define JABBER_PRIORITY_MAX 127

void jabber_init(const int disable_tls);
jabber_conn_status_t jabber_connect(const char * const jid,
    const char * const passwd, const char * const altdomain);
jabber_conn_status_t jabber_connect_with_account(ProfAccount *account,
    const char * const passwd);
void jabber_disconnect(void);
void jabber_process_events(void);
void jabber_join(Jid *jid);
void jabber_change_room_nick(const char * const room, const char * const nick);
void jabber_leave_chat_room(const char * const room_jid);
void jabber_subscription(const char * const jid, jabber_subscr_t action);
GList* jabber_get_subscription_requests(void);
void jabber_send(const char * const msg, const char * const recipient);
void jabber_send_groupchat(const char * const msg, const char * const recipient);
void jabber_send_inactive(const char * const recipient);
void jabber_send_composing(const char * const recipient);
void jabber_send_paused(const char * const recipient);
void jabber_send_gone(const char * const recipient);
void jabber_update_presence(jabber_presence_t status, const char * const msg,
    int idle);
const char * jabber_get_jid(void);
jabber_conn_status_t jabber_get_connection_status(void);
int jabber_get_priority(void);
jabber_presence_t jabber_get_presence(void);
char * jabber_get_status(void);
void jabber_free_resources(void);
void jabber_restart(void);
void jabber_set_autoping(int seconds);

#endif
