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

#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#include "common.h"
#include "ui.h"

static WINDOW *title_bar;
static char *current_title = NULL;
static int dirty;
static jabber_presence_t current_status;

static void _title_bar_draw_title(void);
static void _title_bar_draw_status(void);

void create_title_bar(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    title_bar = newwin(1, cols, 0, 0);
    wbkgd(title_bar, COLOR_PAIR(3));
    title_bar_title();
    title_bar_set_status(PRESENCE_OFFLINE);
    dirty = TRUE;
}

void title_bar_title(void)
{
    title_bar_show("Profanity. Type /help for help information.");
    dirty = TRUE;
}

void title_bar_resize(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    wresize(title_bar, 1, cols);
    wbkgd(title_bar, COLOR_PAIR(3));
    wclear(title_bar);
    _title_bar_draw_title();
    _title_bar_draw_status();
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
    if (current_title != NULL)
        free(current_title);

    current_title = (char *) malloc((strlen(title) + 1) * sizeof(char));
    strcpy(current_title, title);
    _title_bar_draw_title();
}

void title_bar_set_status(jabber_presence_t status)
{
    current_status = status;
    _title_bar_draw_status();
}

static void _title_bar_draw_status()
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    wattron(title_bar, COLOR_PAIR(4));
    mvwaddch(title_bar, 0, cols - 14, '[');
    wattroff(title_bar, COLOR_PAIR(4));

    if (current_status == PRESENCE_ONLINE) {
        mvwprintw(title_bar, 0, cols - 13, " ...online ");
    } else if (current_status == PRESENCE_AWAY) {
        mvwprintw(title_bar, 0, cols - 13, " .....away ");
    } else {
        mvwprintw(title_bar, 0, cols - 13, " ..offline ");
    }
    
    wattron(title_bar, COLOR_PAIR(4));
    mvwaddch(title_bar, 0, cols - 2, ']');
    wattroff(title_bar, COLOR_PAIR(4));
    
    dirty = TRUE;
}

static void _title_bar_draw_title(void)
{
    wmove(title_bar, 0, 0);
    int i;
    for (i = 0; i < 45; i++)
        waddch(title_bar, ' ');
    mvwprintw(title_bar, 0, 0, " %s", current_title);
    
    dirty = TRUE;
}

