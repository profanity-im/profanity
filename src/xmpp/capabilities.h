/*
 * capabilities.h
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

#ifndef XMPP_CAPABILITIES_H
#define XMPP_CAPABILITIES_H

#include <strophe.h>

#include "xmpp/xmpp.h"

void caps_init(void);
void caps_add(const char * const caps_str, const char * const category,
    const char * const type, const char * const name,
    const char * const software, const char * const software_version,
    const char * const os, const char * const os_version, GSList *features);
gboolean caps_contains(const char * const caps_str);
char* caps_create_sha1_str(xmpp_stanza_t * const query);
xmpp_stanza_t* caps_create_query_response_stanza(xmpp_ctx_t * const ctx);

#endif
