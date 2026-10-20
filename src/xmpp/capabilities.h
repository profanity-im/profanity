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

EntityCapabilities* caps_create(const gchar* const category, const gchar* const type, const gchar* const name,
                                const gchar* const software, const gchar* const software_version,
                                const gchar* const os, const gchar* const os_version,
                                GSList* features);
void caps_add_by_ver(const gchar* const ver, EntityCapabilities* caps);
void caps_add_by_jid(const gchar* const jid, EntityCapabilities* caps);
void caps_map_jid_to_ver(const gchar* const jid, const gchar* const ver);
gboolean caps_cache_contains(const gchar* const ver);
GList* caps_get_features(void);
gchar* caps_get_my_sha1(xmpp_ctx_t* const ctx);

#endif
