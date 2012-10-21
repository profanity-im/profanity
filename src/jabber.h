/*
 * jabber.h
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

#ifndef JABBER_H
#define JABBER_H

typedef enum {
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

void jabber_init(const int disable_tls);
jabber_conn_status_t jabber_connect(const char * const user,
    const char * const passwd);
gboolean jabber_disconnect(void);
void jabber_roster_request(void);
void jabber_process_events(void);
void jabber_send(const char * const msg, const char * const recipient);
void jabber_update_presence(jabber_presence_t status, const char * const msg);
const char * jabber_get_jid(void);
jabber_conn_status_t jabber_get_connection_status(void);
void jabber_free_resources(void);

#endif
