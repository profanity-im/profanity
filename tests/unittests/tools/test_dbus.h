/*
 * test_dbus.h
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TESTS_TEST_DBUS_H
#define TESTS_TEST_DBUS_H

void dbus_is_available__returns_boolean(void** state);
void dbus_get_session_bus__idempotent(void** state);

#endif
