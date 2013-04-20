/*
 * window.c
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "config/theme.h"
#include "ui/window.h"

#define CONS_WIN_TITLE "_cons"

ProfWin*
window_create(const char * const title, int cols, win_type_t type)
{
    ProfWin *new_win = malloc(sizeof(struct prof_win_t));
    new_win->from = strdup(title);
    new_win->win = newpad(PAD_SIZE, cols);
    wbkgd(new_win->win, COLOUR_TEXT);
    new_win->y_pos = 0;
    new_win->paged = 0;
    new_win->unread = 0;
    new_win->history_shown = 0;
    new_win->type = type;
    scrollok(new_win->win, TRUE);

    return new_win;
}

void
window_free(ProfWin* window)
{
    delwin(window->win);
    free(window->from);
    window->from = NULL;
    window->win = NULL;
    free(window);
    window = NULL;
}

void
window_show_time(ProfWin* window, char show_char)
{
    GDateTime *time = g_date_time_new_now_local();
    gchar *date_fmt = g_date_time_format(time, "%H:%M:%S");
    wattron(window->win, COLOUR_TIME);
    wprintw(window->win, "%s %c ", date_fmt, show_char);
    wattroff(window->win, COLOUR_TIME);
    g_date_time_unref(time);
    g_free(date_fmt);
}

void
window_presence_colour_on(ProfWin *window, const char * const presence)
{
    if (g_strcmp0(presence, "online") == 0) {
        wattron(window->win, COLOUR_ONLINE);
    } else if (g_strcmp0(presence, "away") == 0) {
        wattron(window->win, COLOUR_AWAY);
    } else if (g_strcmp0(presence, "chat") == 0) {
        wattron(window->win, COLOUR_CHAT);
    } else if (g_strcmp0(presence, "dnd") == 0) {
        wattron(window->win, COLOUR_DND);
    } else if (g_strcmp0(presence, "xa") == 0) {
        wattron(window->win, COLOUR_XA);
    } else {
        wattron(window->win, COLOUR_OFFLINE);
    }
}

void
window_presence_colour_off(ProfWin *window, const char * const presence)
{
    if (g_strcmp0(presence, "online") == 0) {
        wattroff(window->win, COLOUR_ONLINE);
    } else if (g_strcmp0(presence, "away") == 0) {
        wattroff(window->win, COLOUR_AWAY);
    } else if (g_strcmp0(presence, "chat") == 0) {
        wattroff(window->win, COLOUR_CHAT);
    } else if (g_strcmp0(presence, "dnd") == 0) {
        wattroff(window->win, COLOUR_DND);
    } else if (g_strcmp0(presence, "xa") == 0) {
        wattroff(window->win, COLOUR_XA);
    } else {
        wattroff(window->win, COLOUR_OFFLINE);
    }
}

