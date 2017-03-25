/*
 * statusbar.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
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
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/statusbar.h"
#include "ui/inputwin.h"
#include "ui/screen.h"

#define TIME_CHECK 60000000

static WINDOW *status_bar;
static char *message = NULL;
//                          1  2  3  4  5  6  7  8  9  0  >
static char _active[34] = "[ ][ ][ ][ ][ ][ ][ ][ ][ ][ ][ ]";
static char *bracket = "- -";
static int is_active[12];
static GHashTable *remaining_active;
static int is_new[12];
static GHashTable *remaining_new;
static GTimeZone *tz;
static GDateTime *last_time;
static int current;

static void _update_win_statuses(void);
static void _mark_new(int num);
static void _mark_active(int num);
static void _mark_inactive(int num);
static void _status_bar_draw(void);

void
create_status_bar(void)
{
    int i;
    int cols = getmaxx(stdscr);

    is_active[1] = TRUE;
    is_new[1] = FALSE;
    for (i = 2; i < 12; i++) {
        is_active[i] = FALSE;
        is_new[i] = FALSE;
    }
    remaining_active = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
    remaining_new = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
    current = 1;

    int bracket_attrs = theme_attrs(THEME_STATUS_BRACKET);

    int row = screen_statusbar_row();
    status_bar = newwin(1, cols, row, 0);
    wbkgd(status_bar, theme_attrs(THEME_STATUS_TEXT));
    wattron(status_bar, bracket_attrs);
    mvwprintw(status_bar, 0, cols - 34, _active);
    mvwprintw(status_bar, 0, cols - 34 + ((current - 1) * 3), bracket);
    wattroff(status_bar, bracket_attrs);

    tz = g_time_zone_new_local();

    if (last_time) {
        g_date_time_unref(last_time);
    }
    last_time = g_date_time_new_now(tz);

    _status_bar_draw();
}

void
status_bar_update_virtual(void)
{
    _status_bar_draw();
}

void
status_bar_resize(void)
{
    int cols = getmaxx(stdscr);

    werase(status_bar);

    int bracket_attrs = theme_attrs(THEME_STATUS_BRACKET);

    int row = screen_statusbar_row();
    mvwin(status_bar, row, 0);
    wresize(status_bar, 1, cols);
    wbkgd(status_bar, theme_attrs(THEME_STATUS_TEXT));
    wattron(status_bar, bracket_attrs);
    mvwprintw(status_bar, 0, cols - 34, _active);
    mvwprintw(status_bar, 0, cols - 34 + ((current - 1) * 3), bracket);
    wattroff(status_bar, bracket_attrs);

    if (message) {
        char *time_pref = prefs_get_string(PREF_TIME_STATUSBAR);

        gchar *date_fmt = NULL;
        if (g_strcmp0(time_pref, "off") == 0) {
            date_fmt = g_strdup("");
        } else {
            date_fmt = g_date_time_format(last_time, time_pref);
        }
        assert(date_fmt != NULL);
        size_t len = strlen(date_fmt);
        g_free(date_fmt);
        if (g_strcmp0(time_pref, "off") != 0) {
            /* 01234567890123456
             *  [HH:MM]  message */
            mvwprintw(status_bar, 0, 5 + len, message);
        } else {
            mvwprintw(status_bar, 0, 1, message);
        }
        prefs_free_string(time_pref);
    }
    if (last_time) {
        g_date_time_unref(last_time);
    }
    last_time = g_date_time_new_now_local();

    _status_bar_draw();
}

void
status_bar_set_all_inactive(void)
{
    int i = 0;
    for (i = 0; i < 12; i++) {
        is_active[i] = FALSE;
        is_new[i] = FALSE;
        _mark_inactive(i);
    }

    g_hash_table_remove_all(remaining_active);
    g_hash_table_remove_all(remaining_new);

    _status_bar_draw();
}

void
status_bar_current(int i)
{
    if (i == 0) {
        current = 10;
    } else if (i > 10) {
        current = 11;
    } else {
        current = i;
    }
    int cols = getmaxx(stdscr);
    int bracket_attrs = theme_attrs(THEME_STATUS_BRACKET);
    wattron(status_bar, bracket_attrs);
    mvwprintw(status_bar, 0, cols - 34, _active);
    mvwprintw(status_bar, 0, cols - 34 + ((current - 1) * 3), bracket);
    wattroff(status_bar, bracket_attrs);

    _status_bar_draw();
}

void
status_bar_inactive(const int win)
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

        // still have active windows
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

    _status_bar_draw();
}

void
status_bar_active(const int win)
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

    // visible window indicators
    } else {
        is_active[true_win] = TRUE;
        is_new[true_win] = FALSE;
        _mark_active(true_win);
    }

    _status_bar_draw();
}

void
status_bar_new(const int win)
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

    _status_bar_draw();
}

void
status_bar_get_password(void)
{
    status_bar_print_message("Enter password:");

    _status_bar_draw();
}

void
status_bar_print_message(const char *const msg)
{
    werase(status_bar);

    if (message) {
        free(message);
    }
    message = strdup(msg);

    char *time_pref = prefs_get_string(PREF_TIME_STATUSBAR);
    gchar *date_fmt = NULL;
    if (g_strcmp0(time_pref, "off") == 0) {
        date_fmt = g_strdup("");
    } else {
        date_fmt = g_date_time_format(last_time, time_pref);
    }

    assert(date_fmt != NULL);
    size_t len = strlen(date_fmt);
    g_free(date_fmt);
    if (g_strcmp0(time_pref, "off") != 0) {
        mvwprintw(status_bar, 0, 5 + len, message);
    } else {
        mvwprintw(status_bar, 0, 1, message);
    }
    prefs_free_string(time_pref);

    int cols = getmaxx(stdscr);
    int bracket_attrs = theme_attrs(THEME_STATUS_BRACKET);

    wattron(status_bar, bracket_attrs);
    mvwprintw(status_bar, 0, cols - 34, _active);
    mvwprintw(status_bar, 0, cols - 34 + ((current - 1) * 3), bracket);
    wattroff(status_bar, bracket_attrs);

    _status_bar_draw();
}

void
status_bar_clear(void)
{
    if (message) {
        free(message);
        message = NULL;
    }

    werase(status_bar);

    int cols = getmaxx(stdscr);
    int bracket_attrs = theme_attrs(THEME_STATUS_BRACKET);

    wattron(status_bar, bracket_attrs);
    mvwprintw(status_bar, 0, cols - 34, _active);
    mvwprintw(status_bar, 0, cols - 34 + ((current - 1) * 3), bracket);
    wattroff(status_bar, bracket_attrs);

    _status_bar_draw();
}

void
status_bar_clear_message(void)
{
    if (message) {
        free(message);
        message = NULL;
    }

    werase(status_bar);

    int cols = getmaxx(stdscr);
    int bracket_attrs = theme_attrs(THEME_STATUS_BRACKET);

    wattron(status_bar, bracket_attrs);
    mvwprintw(status_bar, 0, cols - 34, _active);
    mvwprintw(status_bar, 0, cols - 34 + ((current - 1) * 3), bracket);
    wattroff(status_bar, bracket_attrs);

    _status_bar_draw();
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
    int status_attrs = theme_attrs(THEME_STATUS_NEW);
    wattron(status_bar, status_attrs);
    wattron(status_bar, A_BLINK);
    if (num == 10) {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, "0");
    } else if (num > 10) {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, ">");
    } else {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, "%d", num);
    }
    wattroff(status_bar, status_attrs);
    wattroff(status_bar, A_BLINK);
}

static void
_mark_active(int num)
{
    int active_pos = 1 + ((num-1) * 3);
    int cols = getmaxx(stdscr);
    int status_attrs = theme_attrs(THEME_STATUS_ACTIVE);
    wattron(status_bar, status_attrs);
    if (num == 10) {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, "0");
    } else if (num > 10) {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, ">");
    } else {
        mvwprintw(status_bar, 0, cols - 34 + active_pos, "%d", num);
    }
    wattroff(status_bar, status_attrs);
}

static void
_mark_inactive(int num)
{
    int active_pos = 1 + ((num-1) * 3);
    int cols = getmaxx(stdscr);
    mvwaddch(status_bar, 0, cols - 34 + active_pos, ' ');
}

static void
_status_bar_draw(void)
{
    if (last_time) {
        g_date_time_unref(last_time);
    }
    last_time = g_date_time_new_now(tz);

    int bracket_attrs = theme_attrs(THEME_STATUS_BRACKET);
    int time_attrs = theme_attrs(THEME_STATUS_TIME);

    char *time_pref = prefs_get_string(PREF_TIME_STATUSBAR);
    if (g_strcmp0(time_pref, "off") != 0) {
        gchar *date_fmt = g_date_time_format(last_time, time_pref);
        assert(date_fmt != NULL);
        size_t len = strlen(date_fmt);
        wattron(status_bar, bracket_attrs);
        mvwaddch(status_bar, 0, 1, '[');
        wattroff(status_bar, bracket_attrs);
        wattron(status_bar, time_attrs);
        mvwprintw(status_bar, 0, 2, date_fmt);
        wattroff(status_bar, time_attrs);
        wattron(status_bar, bracket_attrs);
        mvwaddch(status_bar, 0, 2 + len, ']');
        wattroff(status_bar, bracket_attrs);
        g_free(date_fmt);
    }
    prefs_free_string(time_pref);

    _update_win_statuses();
    wnoutrefresh(status_bar);
    inp_put_back();
}
