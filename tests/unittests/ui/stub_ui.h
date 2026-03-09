/*
 * stub_ui.h
 *
 * Copyright (C) 2015 - 2016 James Booth <boothj5@gmail.com>
 * Copyright (C) 2015 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
void expect_cons_show(char* expected);
void expect_any_cons_show(void);
void expect_cons_show_error(char* expected);
void expect_any_cons_show_error(void);
void expect_win_println(char* message);
