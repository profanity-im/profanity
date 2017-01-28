/*
 * connection.h
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
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

#ifndef XMPP_CONNECTION_H
#define XMPP_CONNECTION_H

#include "xmpp/xmpp.h"

void connection_init(void);
void connection_shutdown(void);
void connection_check_events(void);

jabber_conn_status_t connection_connect(const char *const fulljid, const char *const passwd, const char *const altdomain, int port,
    const char *const tls_policy);
void connection_disconnect(void);
void connection_set_disconnected(void);

void connection_set_priority(const int priority);
void connection_set_priority(int priority);
void connection_set_disco_items(GSList *items);

xmpp_conn_t* connection_get_conn(void);
xmpp_ctx_t* connection_get_ctx(void);
char *connection_get_domain(void);
GHashTable* connection_get_features(const char *const jid);

void connection_clear_data(void);

void connection_add_available_resource(Resource *resource);
void connection_remove_available_resource(const char *const resource);

#endif
