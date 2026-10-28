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
void plugins_message_filter_chain(void** state);
void plugins_stanza_interception_and_drop(void** state);
void plugins_disco_add_feature_advertised(void** state);
void plugins_window_lifecycle_and_input(void** state);
void plugins_settings_persistence(void** state);
void plugins_autocompletion(void** state);

#endif
