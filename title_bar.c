/* 
 * title_bar.c
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

#include <ncurses.h>
#include "windows.h"

static WINDOW *title_bar;
static int dirty;

void create_title_bar(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    title_bar = newwin(1, cols, 0, 0);
    wbkgd(title_bar, COLOR_PAIR(3));
    title_bar_title();
    title_bar_disconnected();
    dirty = TRUE;
}

void title_bar_title(void)
{
    title_bar_show("Profanity. Type /help for help information.");
    dirty = TRUE;
}

void title_bar_connected(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    wattron(title_bar, COLOR_PAIR(4));
    mvwaddch(title_bar, 0, cols - 14, '[');
    wattroff(title_bar, COLOR_PAIR(4));

    mvwprintw(title_bar, 0, cols - 13, " ...online ");
    
    wattron(title_bar, COLOR_PAIR(4));
    mvwaddch(title_bar, 0, cols - 2, ']');
    wattroff(title_bar, COLOR_PAIR(4));
    
    dirty = TRUE;
}

void title_bar_disconnected(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
   
    wattron(title_bar, COLOR_PAIR(4));
    mvwaddch(title_bar, 0, cols - 14, '[');
    wattroff(title_bar, COLOR_PAIR(4));
    
    mvwprintw(title_bar, 0, cols - 13, " ..offline ");
    
    wattron(title_bar, COLOR_PAIR(4));
    mvwaddch(title_bar, 0, cols - 2, ']');
    wattroff(title_bar, COLOR_PAIR(4));
    
    dirty = TRUE;
}

void title_bar_refresh(void)
{
    if (dirty) {
        wrefresh(title_bar);
        inp_put_back();
        dirty = FALSE;
    }
}

void title_bar_show(const char * const title)
{
    wmove(title_bar, 0, 0);
    int i;
    for (i = 0; i < 45; i++)
        waddch(title_bar, ' ');
    mvwprintw(title_bar, 0, 0, " %s", title);
    
    dirty = TRUE;
}

