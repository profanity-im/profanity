/*
 * window.h
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
 *
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef UI_WINDOW_H
#define UI_WINDOW_H

#include "config.h"

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "contact.h"

#define PAD_SIZE 1000

typedef enum {
    WIN_UNUSED,
    WIN_CONSOLE,
    WIN_CHAT,
    WIN_MUC,
    WIN_PRIVATE,
    WIN_DUCK
} win_type_t;

typedef struct prof_win_t {
    char *from;
    WINDOW *win;
    win_type_t type;
    gboolean is_otr;
    gboolean is_trusted;
    int y_pos;
    int paged;
    int unread;
    int history_shown;
} ProfWin;

ProfWin* win_create(const char * const title, int cols, win_type_t type);
void win_free(ProfWin *window);
void win_vprint_line(ProfWin *self, const char show_char, int attrs,
    const char * const msg, ...);
void win_print_line(ProfWin *self, const char show_char, int attrs,
    const char * const msg);
void win_update_virtual(ProfWin *window);
void win_move_to_end(ProfWin *window);
void win_print_time(ProfWin *window, char show_char);
void win_presence_colour_on(ProfWin *window, const char * const presence);
void win_presence_colour_off(ProfWin *window, const char * const presence);
void win_show_contact(ProfWin *window, PContact contact);
void win_show_status_string(ProfWin *window, const char * const from,
    const char * const show, const char * const status,
    GDateTime *last_activity, const char * const pre,
    const char * const default_show);
void win_print_incoming_message(ProfWin *window, GTimeVal *tv_stamp,
    const char * const from, const char * const message);

#endif
