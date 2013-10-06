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

#include "prof_config.h"

#ifdef PROF_HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif PROF_HAVE_NCURSES_H
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
    WIN_DUCK,
    WIN_PLUGIN
} win_type_t;

typedef struct prof_win_t {
    char *from;
    WINDOW *win;
    win_type_t type;
    int y_pos;
    int paged;
    int unread;
    int history_shown;
    void (*print_time)(struct prof_win_t *self, char show_char);
    void (*print_line)(struct prof_win_t *self, const char * const msg, ...);
    void (*refresh_win)(struct prof_win_t *self);
    void (*presence_colour_on)(struct prof_win_t *self, const char * const presence);
    void (*presence_colour_off)(struct prof_win_t *self, const char * const presence);
    void (*show_contact)(struct prof_win_t *self, PContact contact);
    gboolean (*handle_error_message)(struct prof_win_t *self,
        const char * const from, const char * const err_msg);
} ProfWin;

ProfWin* win_create(const char * const title, int cols, win_type_t type);
void win_free(ProfWin *window);

#endif
