/*
 * test_connect.h
 *
 * Copyright (C) 2015 James Booth <boothj5@gmail.com>
 * Copyright (C) 2026 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
void connect_jid_requests_roster(void** state);
void connect_jid_sends_presence_after_receiving_roster(void** state);
void connect_jid_requests_bookmarks(void** state);
void connect_bad_password(void** state);
void connect_shows_presence_updates(void** state);
