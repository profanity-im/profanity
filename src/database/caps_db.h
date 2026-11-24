/*
 * caps_db.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef DATABASE_CAPS_DB_H
#define DATABASE_CAPS_DB_H

#include "config.h"
#include <glib.h>
#include "xmpp/capabilities.h"

gboolean caps_db_init(void);
void caps_db_close(void);
gboolean caps_db_contains(const gchar* const ver);
EntityCapabilities* caps_db_get(const gchar* const ver);
void caps_db_put(const gchar* const ver, EntityCapabilities* caps);

#endif
