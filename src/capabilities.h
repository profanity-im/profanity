/*
 * capabilities.h
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

#ifndef CAPABILITIES_H
#define CAPABILITIES_H

#include <glib.h>
#include <strophe.h>

typedef struct capabilities_t {
    char *client;
} Capabilities;

void caps_init(void);
void caps_add(const char * const caps_str, const char * const client);
gboolean caps_contains(const char * const caps_str);
Capabilities* caps_get(const char * const caps_str);
char* caps_get_sha1_str(xmpp_stanza_t * const query);
xmpp_stanza_t* caps_get_query_response_stanza(xmpp_ctx_t *ctx);
void caps_close(void);

#endif
