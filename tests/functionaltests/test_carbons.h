/*
 * test_carbons.h
 *
 * Copyright (C) 2015 - 2016 James Booth <boothj5@gmail.com>
 * Copyright (C) 2026 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
void send_enable_carbons(void** state);
void connect_with_carbons_enabled(void** state);
void send_disable_carbons(void** state);
void receive_carbon(void** state);
void receive_self_carbon(void** state);
void receive_private_carbon(void** state);
