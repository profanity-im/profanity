/*
 * test_omemo_db.h
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TEST_OMEMO_DB_H
#define TEST_OMEMO_DB_H

void omemo_db_test__identity(void** state);
void omemo_db_test__pre_keys(void** state);
void omemo_db_test__signed_pre_keys(void** state);
void omemo_db_test__sessions(void** state);
void omemo_db_test__trust_and_verified(void** state);
void omemo_db_test__devices(void** state);
void omemo_db_test__legacy_migration(void** state);

#endif
