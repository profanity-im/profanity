/*
 * inputwin.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef UI_INPUTWIN_H
#define UI_INPUTWIN_H

#include <glib.h>

#define INP_WIN_MAX 1000

void create_input_window(void);
void inp_close(void);
void inp_win_resize(void);
void inp_put_back(void);
char* inp_get_password(void);
char* inp_get_line(void);

#endif
