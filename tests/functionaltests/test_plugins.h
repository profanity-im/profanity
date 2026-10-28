/*
 * test_plugins.h
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef TEST_PLUGINS_H
#define TEST_PLUGINS_H

void plugins_load_and_run_command(void** state);
void plugins_priority_ordering(void** state);
void plugins_priority_alphabetical_tie_break(void** state);

#endif
