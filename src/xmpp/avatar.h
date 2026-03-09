/*
 * avatar.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_AVATAR_H
#define XMPP_AVATAR_H

#include <glib.h>

void avatar_pep_subscribe(void);
gboolean avatar_get_by_nick(const char* nick, gboolean open);
#ifdef HAVE_PIXBUF
gboolean avatar_set(const char* path);
#endif

#endif
gboolean avatar_publishing_disable();
