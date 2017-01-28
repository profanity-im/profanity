/*
 * screen.c
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

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "config/preferences.h"

int
_screen_line_row(int win_pos, int mainwin_pos) {
    int wrows = getmaxy(stdscr);

    if (win_pos == 1) {
        return 0;
    }

    if (win_pos == 2) {
        int row = 1;
        if (mainwin_pos == 1) {
            row = wrows-3;
        }

        return row;
    }

    if (win_pos == 3) {
        int row = 2;
        if (mainwin_pos == 1 || mainwin_pos == 2) {
            row = wrows-2;
        }

        return row;
    }

    return wrows-1;
}

int
screen_titlebar_row(void)
{
    ProfWinPlacement *placement = prefs_get_win_placement();
    int row = _screen_line_row(placement->titlebar_pos, placement->mainwin_pos);
    prefs_free_win_placement(placement);

    return row;
}

int
screen_statusbar_row(void)
{
    ProfWinPlacement *placement = prefs_get_win_placement();
    int row = _screen_line_row(placement->statusbar_pos, placement->mainwin_pos);
    prefs_free_win_placement(placement);

    return row;
}

int
screen_inputwin_row(void)
{
    ProfWinPlacement *placement = prefs_get_win_placement();
    int row = _screen_line_row(placement->inputwin_pos, placement->mainwin_pos);
    prefs_free_win_placement(placement);

    return row;
}

int
screen_mainwin_row_start(void)
{
    ProfWinPlacement *placement = prefs_get_win_placement();
    int row = placement->mainwin_pos-1;
    prefs_free_win_placement(placement);

    return row;
}

int
screen_mainwin_row_end(void)
{
    ProfWinPlacement *placement = prefs_get_win_placement();
    int wrows = getmaxy(stdscr);
    int row = wrows - (5 - placement->mainwin_pos);
    prefs_free_win_placement(placement);

    return row;
}
