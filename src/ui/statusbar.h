/*
 * statusbar.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef UI_STATUSBAR_H
#define UI_STATUSBAR_H

void status_bar_init(void);
void status_bar_draw(void);
void status_bar_close(void);
void status_bar_resize(void);
void status_bar_set_prompt(const char* const prompt);
void status_bar_clear_prompt(void);
void status_bar_set_fulljid(const char* const fulljid);
void status_bar_clear_fulljid(void);
void status_bar_current(guint i);

#endif
