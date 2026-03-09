/*
 * clipboard.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef UI_CLIPBOARD_H
#define UI_CLIPBOARD_H

#ifdef HAVE_GTK
char* clipboard_get(void);
#endif

#endif
