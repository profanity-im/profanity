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
    JABBER_DISCONNECTED
} jabber_status_t;

void init_jabber(int disable_tls);
jabber_status_t jabber_connection_status(void);
jabber_status_t jabber_connect(char *user, char *passwd);
void jabber_disconnect(void);
void jabber_roster_request(void);
void jabber_process_events(void);
void jabber_send(char *msg, char *recipient);

#endif
