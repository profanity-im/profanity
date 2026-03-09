/*
 * test_ping.h
 *
 * Copyright (C) 2015 - 2017 James Booth <boothj5@gmail.com>
 * Copyright (C) 2026 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
void ping_server(void** state);
void ping_server_not_supported(void** state);
void ping_responds_to_server_request(void** state);
void ping_jid(void** state);
void ping_jid_not_supported(void** state);
