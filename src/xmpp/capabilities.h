/*
 * capabilities.h
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

#ifndef XMPP_CAPABILITIES_H
#define XMPP_CAPABILITIES_H

#include "config.h"

#ifdef HAVE_LIBMESODE
#include <mesode.h>
#endif

#ifdef HAVE_LIBSTROPHE
#include <strophe.h>
#endif

#include "xmpp/xmpp.h"

void caps_init(void);

EntityCapabilities* caps_create(const char *const category, const char *const type, const char *const name,
    const char *const software, const char *const software_version,
    const char *const os, const char *const os_version,
    GSList *features);
void caps_add_by_ver(const char *const ver, EntityCapabilities *caps);
void caps_add_by_jid(const char *const jid, EntityCapabilities *caps);
void caps_map_jid_to_ver(const char *const jid, const char *const ver);
gboolean caps_cache_contains(const char *const ver);
GList* caps_get_features(void);
char* caps_get_my_sha1(xmpp_ctx_t *const ctx);

#endif
