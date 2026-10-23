/*
 * dbus.c
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <gio/gio.h>
#include <glib.h>

#include "common.h"
#include "log.h"
#include "tools/dbus.h"

static GDBusConnection* session_bus = NULL;
static gboolean initialized = FALSE;

static void
_dbus_close(void)
{
    if (session_bus) {
        g_object_unref(session_bus);
        session_bus = NULL;
    }
    initialized = FALSE;
}

void
dbus_init(void)
{
    if (initialized) {
        return;
    }
    initialized = TRUE;
    prof_add_shutdown_routine(_dbus_close);

    auto_gerror GError* error = NULL;
    session_bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!session_bus) {
        log_debug("Failed to connect to D-Bus session bus: %s", PROF_GERROR_MESSAGE(error));
    }
}

GDBusConnection*
dbus_get_session_bus(void)
{
    if (!initialized) {
        dbus_init();
    }
    return session_bus;
}

gboolean
dbus_is_available(void)
{
    return dbus_get_session_bus() != NULL;
}
