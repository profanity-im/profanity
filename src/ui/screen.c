/*
 * screen.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#elif HAVE_CURSES_H
#include <curses.h>
#endif

#include "config/preferences.h"

int
_screen_line_row(int win_pos, int mainwin_pos)
{
    int wrows = getmaxy(stdscr);

    if (win_pos == 1) {
        return 0;
    }

    if (win_pos == 2) {
        int row = 1;
        if (mainwin_pos == 1) {
            row = wrows - 3;
        }

        return row;
    }

    if (win_pos == 3) {
        int row = 2;
        if (mainwin_pos == 1 || mainwin_pos == 2) {
            row = wrows - 2;
        }

        return row;
    }

    return wrows - 1;
}

int
screen_titlebar_row(void)
{
    ProfWinPlacement* placement = prefs_get_win_placement();
    int row = _screen_line_row(placement->titlebar_pos, placement->mainwin_pos);
    prefs_free_win_placement(placement);

    return row;
}

int
screen_statusbar_row(void)
{
    ProfWinPlacement* placement = prefs_get_win_placement();
    int row = _screen_line_row(placement->statusbar_pos, placement->mainwin_pos);
    prefs_free_win_placement(placement);

    return row;
}

int
screen_inputwin_row(void)
{
    ProfWinPlacement* placement = prefs_get_win_placement();
    int row = _screen_line_row(placement->inputwin_pos, placement->mainwin_pos);
    prefs_free_win_placement(placement);

    return row;
}

int
screen_mainwin_row_start(void)
{
    ProfWinPlacement* placement = prefs_get_win_placement();
    int row = placement->mainwin_pos - 1;
    prefs_free_win_placement(placement);

    return row;
}

int
screen_mainwin_row_end(void)
{
    ProfWinPlacement* placement = prefs_get_win_placement();
    int wrows = getmaxy(stdscr);
    int row = wrows - (5 - placement->mainwin_pos);
    prefs_free_win_placement(placement);

    return row;
}
