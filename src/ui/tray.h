/*
 * tray.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef UI_TRAY_H
#define UI_TRAY_H

#ifdef HAVE_GTK
#include <glib.h>
void tray_init(void);
void tray_update(void);
gboolean tray_gtk_ready(void);

void tray_enable(void);
void tray_disable(void);

void tray_set_timer(int interval);
#endif

#endif
