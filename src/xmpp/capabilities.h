/*
 * capabilities.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_CAPABILITIES_H
#define XMPP_CAPABILITIES_H

#include "config.h"

#include <strophe.h>

#include "xmpp/xmpp.h"

void caps_init(void);

EntityCapabilities* caps_create(const char* const category, const char* const type, const char* const name,
                                const char* const software, const char* const software_version,
                                const char* const os, const char* const os_version,
                                GSList* features);
void caps_add_by_ver(const char* const ver, EntityCapabilities* caps);
void caps_add_by_jid(const char* const jid, EntityCapabilities* caps);
void caps_map_jid_to_ver(const char* const jid, const char* const ver);
gboolean caps_cache_contains(const char* const ver);
GList* caps_get_features(void);
char* caps_get_my_sha1(xmpp_ctx_t* const ctx);

#endif
