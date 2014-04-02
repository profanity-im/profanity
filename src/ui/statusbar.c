/*
 * statusbar.c
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
static char *bracket = "- -";
static int is_active[12];
static GHashTable *remaining_active;
static int is_new[12];
static GHashTable *remaining_new;
static int dirty;
static GDateTime *last_time;
static int current;

static void _status_bar_update_time(void);
static void _update_win_statuses(void);
static void _mark_new(int num);
static void _mark_active(int num);
static void _mark_inactive(int num);

static void
_create_status_bar(void)
{
    int rows, cols, i;
    getmaxyx(stdscr, rows, cols);

    is_active[1] = TRUE;
    is_new[1] = FALSE;
    for (i = 2; i < 12; i++) {
        is_active[i] = FALSE;
        is_new[i] = FALSE;
    }
    remaining_active = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
    remaining_new = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
    current = 1;

    status_bar = newwin(1, cols, rows-2, 0);
    wbkgd(status_bar, COLOUR_STATUS_TEXT);
    wattron(status_bar, COLOUR_STATUS_BRACKET);
    mvwprintw(status_bar, 0, cols - 34, _active);
    mvwprintw(status_bar, 0, cols - 34 + ((current - 1) * 3), bracket);
    wattroff(status_bar, COLOUR_STATUS_BRACKET);

    if (last_time != NULL)
        g_date_time_unref(last_time);
    last_time = g_date_time_new_now_local();

    dirty = TRUE;
}

static void
_status_bar_update_virtual(void)
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
        _update_win_statuses();
        wnoutrefresh(status_bar);
        inp_put_back();
        dirty = FALSE;
    }

    g_date_time_unref(now_time);
}

static void
_status_bar_resize(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    mvwin(status_bar, rows-2, 0);
    wresize(status_bar, 1, cols);
    wbkgd(status_bar, COLOUR_STATUS_TEXT);
    werase(status_bar);
    wattron(status_bar, COLOUR_STATUS_BRACKET);
    mvwprintw(status_bar, 0, cols - 34, _active);
    mvwprintw(status_bar, 0, cols - 34 + ((current - 1) * 3), bracket);
    wattroff(status_bar, COLOUR_STATUS_BRACKET);

    _update_win_statuses();

    if (message != NULL)
        mvwprintw(status_bar, 0, 10, message);

    if (last_time != NULL)
        g_date_time_unref(last_time);
    last_time = g_date_time_new_now_local();
    dirty = TRUE;
}

static void
_status_bar_set_all_inactive(void)
{
    int i = 0;
    for (i = 0; i < 12; i++) {
        is_active[i] = FALSE;
        is_new[i] = FALSE;
        _mark_inactive(i);
    }

    g_hash_table_remove_all(remaining_active);
    g_hash_table_remove_all(remaining_new);
}

static void
_status_bar_current(int i)
{
    if (i == 0) {
        current = 10;
    } else if (i > 10) {
        current = 11;
    } else {
        current = i;
    }
    int cols = getmaxx(stdscr);
    wattron(status_bar, COLOUR_STATUS_BRACKET);
    mvwprintw(status_bar, 0, cols - 34, _active);
    mvwprintw(status_bar, 0, cols - 34 + ((current - 1) * 3), bracket);
    wattroff(status_bar, COLOUR_STATUS_BRACKET);
}

static void
_status_bar_inactive(const int win)
{
    int true_win = win;
    if (true_win == 0) {
        true_win = 10;
    }

    // extra windows
    if (true_win > 10) {
        g_hash_table_remove(remaining_active, GINT_TO_POINTER(true_win));
        g_hash_table_remove(remaining_new, GINT_TO_POINTER(true_win));

        // still have new windows
        if (g_hash_table_size(remaining_new) != 0) {
            is_active[11] = TRUE;
            is_new[11] = TRUE;
            _mark_new(11);

        // still have active winsows
        } else if (g_hash_table_size(remaining_active) != 0) {
            is_active[11] = TRUE;
            is_new[11] = FALSE;
            _mark_active(11);

        // no active or new windows
        } else {
            is_active[11] = FALSE;
            is_new[11] = FALSE;
            _mark_inactive(11);
        }

    // visible window indicators
    } else {
        is_active[true_win] = FALSE;
        is_new[true_win] = FALSE;
        _mark_inactive(true_win);
    }
}

static void
_status_bar_active(const int win)
{
    int true_win = win;
    if (true_win == 0) {
        true_win = 10;
    }

    // extra windows
    if (true_win > 10) {
        g_hash_table_add(remaining_active, GINT_TO_POINTER(true_win));
        g_hash_table_remove(remaining_new, GINT_TO_POINTER(true_win));

        // still have new windows
        if (g_hash_table_size(remaining_new) != 0) {
            is_active[11] = TRUE;
            is_new[11] = TRUE;
            _mark_new(11);

        // only active windows
        } else {
            is_active[11] = TRUE;
            is_new[11] = FALSE;
            _mark_active(11);
        }

    // visible winsow indicators
    } else {
        is_active[true_win] = TRUE;
        is_new[true_win] = FALSE;
        _mark_active(true_win);
    }
}

static void
_status_bar_new(const int win)
{
    int true_win = win;
    if (true_win == 0) {
        true_win = 10;
    }

    if (true_win > 10) {
        g_hash_table_add(remaining_active, GINT_TO_POINTER(true_win));
        g_hash_table_add(remaining_new, GINT_TO_POINTER(true_win));

        is_active[11] = TRUE;
        is_new[11] = TRUE;
        _mark_new(11);

    } else {
        is_active[true_win] = TRUE;
        is_new[true_win] = TRUE;
        _mark_new(true_win);
    }
}

static void
_status_bar_get_password(void)
{
    status_bar_print_message("Enter password:");
    dirty = TRUE;
}

static void
_status_bar_print_message(const char * const msg)
{
    werase(status_bar);

    if (message != NULL) {
        free(message);
    }
    message = strdup(msg);
    mvwprintw(status_bar, 0, 10, message);

    int cols = getmaxx(stdscr);

    wattron(status_bar, COLOUR_STATUS_BRACKET);
    mvwprintw(status_bar, 0, cols - 34, _active);
    mvwprintw(status_bar, 0, cols - 34 + ((current - 1) * 3), bracket);
    wattroff(status_bar, COLOUR_STATUS_BRACKET);

    _update_win_statuses();
    dirty = TRUE;
}

static void
_status_bar_clear(void)
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
    mvwprintw(status_bar, 0, cols - 34 + ((current - 1) * 3), bracket);
    wattroff(status_bar, COLOUR_STATUS_BRACKET);

    dirty = TRUE;
}

static void
_status_bar_clear_message(void)
{
    if (message != NULL) {
        free(message);
        message = NULL;
    }

    werase(status_bar);

    int cols = getmaxx(stdscr);

    wattron(status_bar, COLOUR_STATUS_BRACKET);
    mvwprintw(status_bar, 0, cols - 34, _active);
    mvwprintw(status_bar, 0, cols - 34 + ((current - 1) * 3), bracket);
    wattroff(status_bar, COLOUR_STATUS_BRACKET);

    _update_win_statuses();
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

static void
_update_win_statuses(void)
{
    int i;
    for(i = 1; i < 12; i++) {
        if (is_new[i]) {
            _mark_new(i);
        }
        else if (is_active[i]) {
            _mark_active(i);
        }
        else {
            _mark_inactive(i);
        }
    }
}

static void
_mark_new(int num)
{
    int active_pos = 1 + ((num-1) * 3);
    int cols = getmaxx(stdscr);
    wattron(status_bar, COLOUR_STATUS_NEW);
    wattron(status_bar, A_BLINK);
    if (num == 10) {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, "0");
    } else if (num > 10) {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, ">");
    } else {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, "%d", num);
    }
    wattroff(status_bar, COLOUR_STATUS_NEW);
    wattroff(status_bar, A_BLINK);
    dirty = TRUE;
}

static void
_mark_active(int num)
{
    int active_pos = 1 + ((num-1) * 3);
    int cols = getmaxx(stdscr);
    wattron(status_bar, COLOUR_STATUS_ACTIVE);
    if (num == 10) {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, "0");
    } else if (num > 10) {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, ">");
    } else {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, "%d", num);
    }
    wattroff(status_bar, COLOUR_STATUS_ACTIVE);
    dirty = TRUE;
}

static void
_mark_inactive(int num)
{
    int active_pos = 1 + ((num-1) * 3);
    int cols = getmaxx(stdscr);
    mvwaddch(status_bar, 0, cols - 34 + active_pos, ' ');
    dirty = TRUE;
}

void
statusbar_init_module(void)
{
    create_status_bar = _create_status_bar;
    status_bar_update_virtual = _status_bar_update_virtual;
    status_bar_resize = _status_bar_resize;
    status_bar_set_all_inactive = _status_bar_set_all_inactive;
    status_bar_current = _status_bar_current;
    status_bar_inactive = _status_bar_inactive;
    status_bar_active = _status_bar_active;
    status_bar_new = _status_bar_new;
    status_bar_get_password = _status_bar_get_password;
    status_bar_print_message = _status_bar_print_message;
    status_bar_clear = _status_bar_clear;
    status_bar_clear_message = _status_bar_clear_message;
}
