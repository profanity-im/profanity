/*
 * test_software.h
 *
 * Copyright (C) 2015 James Booth <boothj5@gmail.com>
 * Copyright (C) 2026 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
void send_software_version_request(void** state);
void display_software_version_result(void** state);
void shows_message_when_software_version_error(void** state);
void display_software_version_result_when_from_domainpart(void** state);
void show_message_in_chat_window_when_no_resource(void** state);
void display_software_version_result_in_chat(void** state);
