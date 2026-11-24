/*
 * test_capabilities.h
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TEST_CAPABILITIES_H
#define TEST_CAPABILITIES_H

void caps_test__add_and_lookup_by_ver(void** state);
void caps_test__jid_has_feature(void** state);
void caps_test__legacy_migration(void** state);
void caps_test__empty_legacy_file(void** state);

#endif
