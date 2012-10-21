/*
 * status_bar.c
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
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

#include <string.h>
#include <stdlib.h>

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#endif
#ifdef HAVE_NCURSES_NCURSES_H
#include <ncurses/ncurses.h>
#endif

#include "ui.h"

static WINDOW *status_bar;
static char *message = NULL;
static char _active[29] = "[ ][ ][ ][ ][ ][ ][ ][ ][  ]";
static int is_active[9];
static int is_new[9];
static int dirty;
static GDateTime *last_time;

static void _status_bar_update_time(void);

void
create_status_bar(void)
{
    int rows, cols, i;
    getmaxyx(stdscr, rows, cols);

    for (i = 0; i < 9; i++) {
        is_active[i] = FALSE;
        is_new[i] = FALSE;
    }

    status_bar = newwin(1, cols, rows-2, 0);
    wbkgd(status_bar, COLOUR_BAR_DEF);
    wattron(status_bar, COLOUR_BAR_DRAW);
    mvwprintw(status_bar, 0, cols - 29, _active);
    wattroff(status_bar, COLOUR_BAR_DRAW);

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
        last_time = g_date_time_new_now_local();
    }

    if (dirty) {
        _status_bar_update_time();
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
    wbkgd(status_bar, COLOUR_BAR_DEF);
    wclear(status_bar);
    wattron(status_bar, COLOUR_BAR_DRAW);
    mvwprintw(status_bar, 0, cols - 29, _active);
    wattroff(status_bar, COLOUR_BAR_DRAW);

    for(i = 0; i < 9; i++) {
        if (is_new[i])
            status_bar_new(i+1);
        else if (is_active[i])
            status_bar_active(i+1);
    }

    if (message != NULL)
        mvwprintw(status_bar, 0, 9, message);

    last_time = g_date_time_new_now_local();
    dirty = TRUE;
}

void
status_bar_inactive(const int win)
{
    is_active[win-1] = FALSE;
    is_new[win-1] = FALSE;

    int active_pos = 1 + ((win -1) * 3);

    int cols = getmaxx(stdscr);

    mvwaddch(status_bar, 0, cols - 29 + active_pos, ' ');
    if (win == 9)
        mvwaddch(status_bar, 0, cols - 29 + active_pos + 1, ' ');

    dirty = TRUE;
}

void
status_bar_active(const int win)
{
    is_active[win-1] = TRUE;
    is_new[win-1] = FALSE;

    int active_pos = 1 + ((win -1) * 3);

    int cols = getmaxx(stdscr);

    wattron(status_bar, COLOUR_BAR_DRAW);
    if (win < 9)
        mvwprintw(status_bar, 0, cols - 29 + active_pos, "%d", win+1);
    else
        mvwprintw(status_bar, 0, cols - 29 + active_pos, "10");
    wattroff(status_bar, COLOUR_BAR_DRAW);

    dirty = TRUE;
}

void
status_bar_new(const int win)
{
    is_active[win-1] = TRUE;
    is_new[win-1] = TRUE;

    int active_pos = 1 + ((win -1) * 3);

    int cols = getmaxx(stdscr);

    wattron(status_bar, COLOUR_BAR_TEXT);
    wattron(status_bar, A_BLINK);
    if (win < 9)
        mvwprintw(status_bar, 0, cols - 29 + active_pos, "%d", win+1);
    else
        mvwprintw(status_bar, 0, cols - 29 + active_pos, "10");
    wattroff(status_bar, COLOUR_BAR_TEXT);
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
    if (message != NULL)
        free(message);

    message = (char *) malloc((strlen(msg) + 1) * sizeof(char));
    strcpy(message, msg);
    mvwprintw(status_bar, 0, 9, message);

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
    for (i = 0; i < 9; i++) {
        is_active[i] = FALSE;
        is_new[i] = FALSE;
    }

    wclear(status_bar);

    int cols = getmaxx(stdscr);

    wattron(status_bar, COLOUR_BAR_DRAW);
    mvwprintw(status_bar, 0, cols - 29, _active);
    wattroff(status_bar, COLOUR_BAR_DRAW);

    dirty = TRUE;
}

static void
_status_bar_update_time(void)
{
    gchar *date_fmt = g_date_time_format(last_time, "%H:%M");

    wattron(status_bar, COLOUR_BAR_DRAW);
    mvwaddch(status_bar, 0, 1, '[');
    wattroff(status_bar, COLOUR_BAR_DRAW);
    mvwprintw(status_bar, 0, 2, date_fmt);
    wattron(status_bar, COLOUR_BAR_DRAW);
    mvwaddch(status_bar, 0, 7, ']');
    wattroff(status_bar, COLOUR_BAR_DRAW);

    free(date_fmt);

    dirty = TRUE;
}
