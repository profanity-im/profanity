/*
 * bookmark_ignore.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef BOOKMARK_IGNORE_H
#define BOOKMARK_IGNORE_H

typedef struct bookmark_t Bookmark;

void bookmark_ignore_on_connect(const gchar* const barejid);
void bookmark_ignore_on_disconnect(void);
gboolean bookmark_ignored(Bookmark* bookmark);
gchar** bookmark_ignore_list(gsize* len);
void bookmark_ignore_add(const gchar* const barejid);
void bookmark_ignore_remove(const gchar* const barejid);

#endif
