/*
 * test_dbus.c
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "prof_cmocka.h"
#include <gio/gio.h>
#include <glib.h>

#include "tools/dbus.h"

void
dbus_is_available__returns_boolean(void** state)
{
    gboolean available = dbus_is_available();
    GDBusConnection* conn = dbus_get_session_bus();

    if (available) {
        assert_non_null(conn);
        assert_true(G_IS_DBUS_CONNECTION(conn));
    } else {
        assert_null(conn);
    }
}

void
dbus_get_session_bus__idempotent(void** state)
{
    GDBusConnection* conn1 = dbus_get_session_bus();
    GDBusConnection* conn2 = dbus_get_session_bus();

    assert_ptr_equal(conn1, conn2);
}
