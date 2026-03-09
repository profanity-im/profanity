/*
 * titlebar.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef UI_TITLEBAR_H
#define UI_TITLEBAR_H

void create_title_bar(void);
void free_title_bar(void);
void title_bar_update_virtual(void);
void title_bar_resize(void);
void title_bar_console(void);
void title_bar_set_connected(gboolean connected);
void title_bar_set_tls(gboolean secured);
void title_bar_switch(void);
void title_bar_set_typing(gboolean is_typing);

#endif
