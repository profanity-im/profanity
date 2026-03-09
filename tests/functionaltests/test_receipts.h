/*
 * test_receipts.h
 *
 * Copyright (C) 2015 - 2016 James Booth <boothj5@gmail.com>
 * Copyright (C) 2026 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
void does_not_send_receipt_request_to_barejid(void** state);
void send_receipt_request(void** state);
void send_receipt_on_request(void** state);
