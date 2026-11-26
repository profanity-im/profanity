/*
 * test_db.h
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TEST_DB_H
#define TEST_DB_H

void test_db__open_and_close(void** state);
void test_db__exec_and_prepare(void** state);
void test_db__transactions(void** state);
void test_db__version(void** state);

#endif
