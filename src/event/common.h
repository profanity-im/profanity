/*
 * common.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef EVENT_COMMON_H
#define EVENT_COMMON_H

void ev_disconnect_cleanup(void);
void ev_inc_connection_counter(void);
void ev_reset_connection_counter(void);
gboolean ev_was_connected_already(void);
gboolean ev_is_first_connect(void);

#endif
