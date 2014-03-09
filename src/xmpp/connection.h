/*
 * connection.h
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

#ifndef XMPP_CONNECTION_H
#define XMPP_CONNECTION_H

#include <strophe.h>

#include "resource.h"

xmpp_conn_t *connection_get_conn(void);
xmpp_ctx_t *connection_get_ctx(void);
void connection_set_priority(int priority);
void connection_set_presence_message(const char * const message);
void connection_add_available_resource(Resource *resource);
void connection_remove_available_resource(const char * const resource);

#endif
