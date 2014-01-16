/*
 * titlebar.c
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

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "config/theme.h"
#include "ui/ui.h"

static WINDOW *win;
static char *current_title = NULL;
static char *recipient = NULL;

static GTimer *typing_elapsed;

static contact_presence_t current_presence;

static void _title_bar_draw_title(void);
static void _title_bar_draw_presence(void);

static void
_create_title_bar(void)
{
    int cols = getmaxx(stdscr);

    win = newwin(1, cols, 0, 0);
    wbkgd(win, COLOUR_TITLE_TEXT);
    title_bar_console();
    title_bar_set_presence(CONTACT_OFFLINE);
    wrefresh(win);
    inp_put_back();
}

static void
_title_bar_console(void)
{
    werase(win);
    recipient = NULL;
    typing_elapsed = NULL;

    if (current_title != NULL)
        free(current_title);
    current_title = strdup("Profanity. Type /help for help information.");

    _title_bar_draw_title();
    _title_bar_draw_presence();
    wrefresh(win);
    inp_put_back();
}

static void
_title_bar_resize(void)
{
    int cols = getmaxx(stdscr);

    wresize(win, 1, cols);
    wbkgd(win, COLOUR_TITLE_TEXT);
    werase(win);
    _title_bar_draw_title();
    _title_bar_draw_presence();
    wrefresh(win);
    inp_put_back();
}

static void
_title_bar_refresh(void)
{
    if (recipient != NULL) {

        if (typing_elapsed != NULL) {
            gdouble seconds = g_timer_elapsed(typing_elapsed, NULL);

            if (seconds >= 10) {

                if (current_title != NULL) {
                    free(current_title);
                }

                current_title = (char *) malloc(strlen(recipient) + 1);
                strcpy(current_title, recipient);

                title_bar_draw();

                g_timer_destroy(typing_elapsed);
                typing_elapsed = NULL;

                wrefresh(win);
                inp_put_back();
            }
        }
    }
}

static void
_title_bar_set_presence(contact_presence_t presence)
{
    current_presence = presence;
    _title_bar_draw_title();
    _title_bar_draw_presence();
}

static void
_title_bar_set_recipient(const char * const from)
{
    if (typing_elapsed != NULL) {
        g_timer_destroy(typing_elapsed);
        typing_elapsed = NULL;
    }
    free(recipient);
    recipient = strdup(from);

    if (current_title != NULL) {
        free(current_title);
    }

    current_title = (char *) malloc(strlen(from) + 1);
    strcpy(current_title, from);

    wrefresh(win);
    inp_put_back();
}

static void
_title_bar_set_typing(gboolean is_typing)
{
    if (is_typing) {
        if (typing_elapsed != NULL) {
            g_timer_start(typing_elapsed);
        } else {
            typing_elapsed = g_timer_new();
        }
    }

    if (current_title != NULL) {
        free(current_title);
    }

    if (is_typing) {
        current_title = (char *) malloc(strlen(recipient) + 13);
        sprintf(current_title, "%s (typing...)", recipient);
    } else {
        current_title = (char *) malloc(strlen(recipient) + 1);
        strcpy(current_title, recipient);
    }

    wrefresh(win);
    inp_put_back();
}

static void
_title_bar_draw(void)
{
    werase(win);
    _title_bar_draw_title();
    _title_bar_draw_presence();
}

static void
_title_bar_draw_presence(void)
{
    int cols = getmaxx(stdscr);

    wattron(win, COLOUR_TITLE_BRACKET);
    mvwaddch(win, 0, cols - 14, '[');
    wattroff(win, COLOUR_TITLE_BRACKET);

    switch (current_presence)
    {
        case CONTACT_ONLINE:
            mvwprintw(win, 0, cols - 13, " ...online ");
            break;
        case CONTACT_AWAY:
            mvwprintw(win, 0, cols - 13, " .....away ");
            break;
        case CONTACT_DND:
            mvwprintw(win, 0, cols - 13, " ......dnd ");
            break;
        case CONTACT_CHAT:
            mvwprintw(win, 0, cols - 13, " .....chat ");
            break;
        case CONTACT_XA:
            mvwprintw(win, 0, cols - 13, " .......xa ");
            break;
        case CONTACT_OFFLINE:
            mvwprintw(win, 0, cols - 13, " ..offline ");
            break;
    }

    wattron(win, COLOUR_TITLE_BRACKET);
    mvwaddch(win, 0, cols - 2, ']');
    wattroff(win, COLOUR_TITLE_BRACKET);

    wrefresh(win);
    inp_put_back();
}

static void
_title_bar_draw_title(void)
{
    wmove(win, 0, 0);
    int i;
    for (i = 0; i < 45; i++)
        waddch(win, ' ');
    mvwprintw(win, 0, 0, " %s", current_title);

    wrefresh(win);
    inp_put_back();
}

void
titlebar_init_module(void)
{
    create_title_bar = _create_title_bar;
    title_bar_console = _title_bar_console;
    title_bar_resize = _title_bar_resize;
    title_bar_refresh = _title_bar_refresh;
    title_bar_set_presence = _title_bar_set_presence;
    title_bar_set_recipient = _title_bar_set_recipient;
    title_bar_set_typing = _title_bar_set_typing;
    title_bar_draw = _title_bar_draw;
}
