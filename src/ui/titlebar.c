/*
 * titlebar.c
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

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "config/theme.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/windows.h"
#include "ui/window.h"
#include "roster_list.h"

#define CONSOLE_TITLE "Profanity. Type /help for help information."

static WINDOW *win;
static char *current_title = NULL;
static char *current_recipient = NULL;
static contact_presence_t current_presence;

static gboolean typing;
static GTimer *typing_elapsed;

static void _title_bar_draw(void);

static void
_create_title_bar(void)
{
    int cols = getmaxx(stdscr);

    win = newwin(1, cols, 0, 0);
    wbkgd(win, COLOUR_TITLE_TEXT);
    title_bar_console();
    title_bar_set_presence(CONTACT_OFFLINE);
    wnoutrefresh(win);
    inp_put_back();
}

static void
_title_bar_console(void)
{
    werase(win);
    current_recipient = NULL;
    typing = FALSE;
    typing_elapsed = NULL;

    free(current_title);
    current_title = strdup(CONSOLE_TITLE);

    _title_bar_draw();
}

static void
_title_bar_resize(void)
{
    int cols = getmaxx(stdscr);

    wresize(win, 1, cols);
    wbkgd(win, COLOUR_TITLE_TEXT);

    _title_bar_draw();
}

static void
_title_bar_update_virtual(void)
{
    if (current_recipient != NULL) {

        if (typing_elapsed != NULL) {
            gdouble seconds = g_timer_elapsed(typing_elapsed, NULL);

            if (seconds >= 10) {
                typing = FALSE;

                g_timer_destroy(typing_elapsed);
                typing_elapsed = NULL;

                _title_bar_draw();
            }
        }
    }
}

static void
_title_bar_set_presence(contact_presence_t presence)
{
    current_presence = presence;
    _title_bar_draw();
}

static void
_title_bar_set_recipient(const char * const recipient)
{
    if (typing_elapsed != NULL) {
        g_timer_destroy(typing_elapsed);
        typing_elapsed = NULL;
        typing = FALSE;
    }

    free(current_recipient);
    current_recipient = strdup(recipient);

    free(current_title);
    current_title = strdup(recipient);

    _title_bar_draw();
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

    typing = is_typing;


    _title_bar_draw();
}

static void
_title_bar_draw(void)
{
    werase(win);

    // show title
    wmove(win, 0, 0);
    int i;
    for (i = 0; i < 45; i++)
        waddch(win, ' ');
    mvwprintw(win, 0, 0, " %s", current_title);


#ifdef HAVE_LIBOTR
    // show privacy
    if (current_recipient != NULL) {
        char *recipient_jid = roster_barejid_from_name(current_recipient);
        ProfWin *current = wins_get_by_recipient(recipient_jid);
        if (current != NULL) {
            if (current->type == WIN_CHAT) {
                if (!current->is_otr) {
                    if (prefs_get_boolean(PREF_OTR_WARN)) {
                        wprintw(win, " ");
                        wattron(win, COLOUR_TITLE_BRACKET);
                        wprintw(win, "[");
                        wattroff(win, COLOUR_TITLE_BRACKET);
                        wattron(win, COLOUR_TITLE_UNENCRYPTED);
                        wprintw(win, "unencrypted");
                        wattroff(win, COLOUR_TITLE_UNENCRYPTED);
                        wattron(win, COLOUR_TITLE_BRACKET);
                        wprintw(win, "]");
                        wattroff(win, COLOUR_TITLE_BRACKET);
                    }
                } else {
                    wprintw(win, " ");
                    wattron(win, COLOUR_TITLE_BRACKET);
                    wprintw(win, "[");
                    wattroff(win, COLOUR_TITLE_BRACKET);
                    wattron(win, COLOUR_TITLE_ENCRYPTED);
                    wprintw(win, "OTR");
                    wattroff(win, COLOUR_TITLE_ENCRYPTED);
                    wattron(win, COLOUR_TITLE_BRACKET);
                    wprintw(win, "]");
                    wattroff(win, COLOUR_TITLE_BRACKET);
                    if (current->is_trusted) {
                        wprintw(win, " ");
                        wattron(win, COLOUR_TITLE_BRACKET);
                        wprintw(win, "[");
                        wattroff(win, COLOUR_TITLE_BRACKET);
                        wattron(win, COLOUR_TITLE_TRUSTED);
                        wprintw(win, "trusted");
                        wattroff(win, COLOUR_TITLE_TRUSTED);
                        wattron(win, COLOUR_TITLE_BRACKET);
                        wprintw(win, "]");
                        wattroff(win, COLOUR_TITLE_BRACKET);
                    } else {
                        wprintw(win, " ");
                        wattron(win, COLOUR_TITLE_BRACKET);
                        wprintw(win, "[");
                        wattroff(win, COLOUR_TITLE_BRACKET);
                        wattron(win, COLOUR_TITLE_UNTRUSTED);
                        wprintw(win, "untrusted");
                        wattroff(win, COLOUR_TITLE_UNTRUSTED);
                        wattron(win, COLOUR_TITLE_BRACKET);
                        wprintw(win, "]");
                        wattroff(win, COLOUR_TITLE_BRACKET);
                    }
                }
            }
        }
    }
#endif

    // show contact typing
    if (typing) {
        wprintw(win, " (typing...)");
    }

    // show presence
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

    wnoutrefresh(win);
    inp_put_back();
}

void
titlebar_init_module(void)
{
    create_title_bar = _create_title_bar;
    title_bar_console = _title_bar_console;
    title_bar_resize = _title_bar_resize;
    title_bar_update_virtual = _title_bar_update_virtual;
    title_bar_set_presence = _title_bar_set_presence;
    title_bar_set_recipient = _title_bar_set_recipient;
    title_bar_set_typing = _title_bar_set_typing;
}
