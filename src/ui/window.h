/*
 * window.h
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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

#ifndef WINDOW_H
#define WINDOW_H

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
    int y_pos;
    int paged;
    int unread;
    int history_shown;
    gboolean (*handle_error_message)(struct prof_win_t *self,
        const char * const from, const char * const err_msg);
    void (*print_incoming_message)(struct prof_win_t *self, GTimeVal *tv_stamp,
        const char * const from, const char * const message);
} ProfWin;

ProfWin* win_create(const char * const title, int cols, win_type_t type);
void win_free(ProfWin *window);
void win_print_line(ProfWin *self, const char show_char, int attrs,
    const char * const msg, ...);
void win_refresh(ProfWin *window);
void win_print_time(ProfWin *window, char show_char);
void win_presence_colour_on(ProfWin *window, const char * const presence);
void win_presence_colour_off(ProfWin *window, const char * const presence);
void win_show_contact(ProfWin *window, PContact contact);

#endif
