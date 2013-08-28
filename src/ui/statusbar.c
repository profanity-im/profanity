/*
 * statusbar.c
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

#include "config.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "config/theme.h"
#include "ui/ui.h"

static WINDOW *status_bar;
static char *message = NULL;
//                          1  2  3  4  5  6  7  8  9  0  >
static char _active[34] = "[ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]";
static int is_active[12];
static int is_new[12];
static int dirty;
static GDateTime *last_time;

static void _status_bar_update_time(void);

void
create_status_bar(void)
{
    int rows, cols, i;
    getmaxyx(stdscr, rows, cols);

    is_active[1] = TRUE;
    is_new[1] = FALSE;
    for (i = 2; i < 12; i++) {
        is_active[i] = FALSE;
        is_new[i] = FALSE;
    }

    status_bar = newwin(1, cols, rows-2, 0);
    wbkgd(status_bar, COLOUR_STATUS_TEXT);
    wattron(status_bar, COLOUR_STATUS_BRACKET);
    mvwprintw(status_bar, 0, cols - 34, _active);
    wattroff(status_bar, COLOUR_STATUS_BRACKET);

    if (last_time != NULL)
        g_date_time_unref(last_time);
    last_time = g_date_time_new_now_local();

    dirty = TRUE;
}

void
status_bar_refresh(void)
{
    GDateTime *now_time = g_date_time_new_now_local();
    GTimeSpan elapsed = g_date_time_difference(now_time, last_time);

    if (elapsed >= 60000000) {
        dirty = TRUE;
        if (last_time != NULL)
            g_date_time_unref(last_time);
        last_time = g_date_time_new_now_local();
    }

    if (dirty) {
        _status_bar_update_time();
        int i;
        for(i = 1; i < 12; i++) {
            if (is_new[i])
                status_bar_new(i);
            else if (is_active[i])
                status_bar_active(i);
        }
        wrefresh(status_bar);
        inp_put_back();
        dirty = FALSE;
    }

    g_date_time_unref(now_time);
}

void
status_bar_resize(void)
{
    int rows, cols, i;
    getmaxyx(stdscr, rows, cols);

    mvwin(status_bar, rows-2, 0);
    wresize(status_bar, 1, cols);
    wbkgd(status_bar, COLOUR_STATUS_TEXT);
    werase(status_bar);
    wattron(status_bar, COLOUR_STATUS_BRACKET);
    mvwprintw(status_bar, 0, cols - 34, _active);
    wattroff(status_bar, COLOUR_STATUS_BRACKET);

    for(i = 1; i < 12; i++) {
        if (is_new[i])
            status_bar_new(i);
        else if (is_active[i])
            status_bar_active(i);
    }

    if (message != NULL)
        mvwprintw(status_bar, 0, 10, message);

    if (last_time != NULL)
        g_date_time_unref(last_time);
    last_time = g_date_time_new_now_local();
    dirty = TRUE;
}

void
status_bar_inactive(const int win)
{
    int true_win = win;
    if (true_win == 0) {
        true_win = 10;
    }
    is_active[true_win] = FALSE;
    is_new[true_win] = FALSE;

    int active_pos = 1 + ((true_win-1) * 3);

    int cols = getmaxx(stdscr);

    mvwaddch(status_bar, 0, cols - 34 + active_pos, ' ');

    dirty = TRUE;
}

void
status_bar_active(const int win)
{
    int true_win = win;
    if (true_win == 0) {
        true_win = 10;
    }
    is_active[true_win] = TRUE;
    is_new[true_win] = FALSE;

    int active_pos = 1 + ((true_win-1) * 3);

    int cols = getmaxx(stdscr);

    wattron(status_bar, COLOUR_STATUS_ACTIVE);

    if (true_win == 10) {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, "0");
    } else if (true_win > 10) {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, ">");
    } else {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, "%d", true_win);
    }

    wattroff(status_bar, COLOUR_STATUS_ACTIVE);

    dirty = TRUE;
}

void
status_bar_new(const int win)
{
    int true_win = win;
    if (true_win == 0) {
        true_win = 10;
    }
    is_active[true_win] = TRUE;
    is_new[true_win] = TRUE;

    int active_pos = 1 + ((true_win-1) * 3);

    int cols = getmaxx(stdscr);

    wattron(status_bar, COLOUR_STATUS_NEW);
    wattron(status_bar, A_BLINK);

    if (true_win == 10) {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, "0");
    } else if (true_win > 10) {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, ">");
    } else {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, "%d", true_win);
    }

    wattroff(status_bar, COLOUR_STATUS_NEW);
    wattroff(status_bar, A_BLINK);

    dirty = TRUE;
}

void
status_bar_get_password(void)
{
    status_bar_print_message("Enter password:");
    dirty = TRUE;
}

void
status_bar_print_message(const char * const msg)
{
    werase(status_bar);

    if (message != NULL) {
        free(message);
    }
    message = (char *) malloc(strlen(msg) + 1);
    strcpy(message, msg);
    mvwprintw(status_bar, 0, 10, message);

    int cols = getmaxx(stdscr);

    wattron(status_bar, COLOUR_STATUS_BRACKET);
    mvwprintw(status_bar, 0, cols - 34, _active);
    wattroff(status_bar, COLOUR_STATUS_BRACKET);

    int i;
    for(i = 1; i < 12; i++) {
        if (is_new[i])
            status_bar_new(i);
        else if (is_active[i])
            status_bar_active(i);
    }

    dirty = TRUE;
}

void
status_bar_clear(void)
{
    if (message != NULL) {
        free(message);
        message = NULL;
    }

    int i;
    is_active[1] = TRUE;
    is_new[1] = FALSE;
    for (i = 2; i < 12; i++) {
        is_active[i] = FALSE;
        is_new[i] = FALSE;
    }

    werase(status_bar);

    int cols = getmaxx(stdscr);

    wattron(status_bar, COLOUR_STATUS_BRACKET);
    mvwprintw(status_bar, 0, cols - 34, _active);
    wattroff(status_bar, COLOUR_STATUS_BRACKET);

    dirty = TRUE;
}

void
status_bar_clear_message(void)
{
    if (message != NULL) {
        free(message);
        message = NULL;
    }

    werase(status_bar);

    int cols = getmaxx(stdscr);

    wattron(status_bar, COLOUR_STATUS_BRACKET);
    mvwprintw(status_bar, 0, cols - 34, _active);
    wattroff(status_bar, COLOUR_STATUS_BRACKET);

    int i;
    for(i = 1; i < 12; i++) {
        if (is_new[i])
            status_bar_new(i);
        else if (is_active[i])
            status_bar_active(i);
    }

    dirty = TRUE;
}

static void
_status_bar_update_time(void)
{
    gchar *date_fmt = g_date_time_format(last_time, "%H:%M");
    assert(date_fmt != NULL);

    wattron(status_bar, COLOUR_STATUS_BRACKET);
    mvwaddch(status_bar, 0, 1, '[');
    wattroff(status_bar, COLOUR_STATUS_BRACKET);
    mvwprintw(status_bar, 0, 2, date_fmt);
    wattron(status_bar, COLOUR_STATUS_BRACKET);
    mvwaddch(status_bar, 0, 7, ']');
    wattroff(status_bar, COLOUR_STATUS_BRACKET);

    g_free(date_fmt);

    dirty = TRUE;
}
