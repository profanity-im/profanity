/*
 * dbus.h
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef TOOLS_DBUS_H
#define TOOLS_DBUS_H

#include <gio/gio.h>
#include <glib.h>

void dbus_init(void);
GDBusConnection* dbus_get_session_bus(void);
gboolean dbus_is_available(void);

#endif
