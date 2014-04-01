/*
 * window.c
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <glib.h>
#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "config/theme.h"
#include "ui/window.h"

static void _win_chat_print_incoming_message(ProfWin *window, GTimeVal *tv_stamp,
    const char * const from, const char * const message);

ProfWin*
win_create(const char * const title, int cols, win_type_t type)
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
    new_win->is_otr = FALSE;
    new_win->is_trusted = FALSE;
    scrollok(new_win->win, TRUE);

    return new_win;
}

void
win_free(ProfWin* window)
{
    delwin(window->win);
    free(window->from);
    free(window);
    window = NULL;
}

void
win_print_time(ProfWin* window, char show_char)
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
win_print_line(ProfWin *window, const char show_char, int attrs,
    const char * const msg)
{
    win_print_time(window, show_char);
    wattron(window->win, attrs);
    wprintw(window->win, "%s\n", msg);
    wattroff(window->win, attrs);
}

void
win_vprint_line(ProfWin *window, const char show_char, int attrs,
    const char * const msg, ...)
{
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    win_print_time(window, show_char);
    wattron(window->win, attrs);
    wprintw(window->win, "%s\n", fmt_msg->str);
    wattroff(window->win, attrs);
    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
win_update_virtual(ProfWin *window)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    pnoutrefresh(window->win, window->y_pos, 0, 1, 0, rows-3, cols-1);
}

void
win_move_to_end(ProfWin *window)
{
    window->paged = 0;

    int rows = getmaxy(stdscr);
    int y = getcury(window->win);
    int size = rows - 3;

    window->y_pos = y - (size - 1);
    if (window->y_pos < 0) {
        window->y_pos = 0;
    }
}

void
win_presence_colour_on(ProfWin *window, const char * const presence)
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
win_presence_colour_off(ProfWin *window, const char * const presence)
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

void
win_show_contact(ProfWin *window, PContact contact)
{
    const char *barejid = p_contact_barejid(contact);
    const char *name = p_contact_name(contact);
    const char *presence = p_contact_presence(contact);
    const char *status = p_contact_status(contact);
    GDateTime *last_activity = p_contact_last_activity(contact);

    win_print_time(window, '-');
    win_presence_colour_on(window, presence);

    if (name != NULL) {
        wprintw(window->win, "%s", name);
    } else {
        wprintw(window->win, "%s", barejid);
    }

    wprintw(window->win, " is %s", presence);

    if (last_activity != NULL) {
        GDateTime *now = g_date_time_new_now_local();
        GTimeSpan span = g_date_time_difference(now, last_activity);

        wprintw(window->win, ", idle ");

        int hours = span / G_TIME_SPAN_HOUR;
        span = span - hours * G_TIME_SPAN_HOUR;
        if (hours > 0) {
            wprintw(window->win, "%dh", hours);
        }

        int minutes = span / G_TIME_SPAN_MINUTE;
        span = span - minutes * G_TIME_SPAN_MINUTE;
        wprintw(window->win, "%dm", minutes);

        int seconds = span / G_TIME_SPAN_SECOND;
        wprintw(window->win, "%ds", seconds);
    }

    if (status != NULL) {
        wprintw(window->win, ", \"%s\"", p_contact_status(contact));
    }

    wprintw(window->win, "\n");
    win_presence_colour_off(window, presence);
}

void
win_show_status_string(ProfWin *window, const char * const from,
    const char * const show, const char * const status,
    GDateTime *last_activity, const char * const pre,
    const char * const default_show)
{
    WINDOW *win = window->win;

    win_print_time(window, '-');

    if (show != NULL) {
        if (strcmp(show, "away") == 0) {
            wattron(win, COLOUR_AWAY);
        } else if (strcmp(show, "chat") == 0) {
            wattron(win, COLOUR_CHAT);
        } else if (strcmp(show, "dnd") == 0) {
            wattron(win, COLOUR_DND);
        } else if (strcmp(show, "xa") == 0) {
            wattron(win, COLOUR_XA);
        } else if (strcmp(show, "online") == 0) {
            wattron(win, COLOUR_ONLINE);
        } else {
            wattron(win, COLOUR_OFFLINE);
        }
    } else if (strcmp(default_show, "online") == 0) {
        wattron(win, COLOUR_ONLINE);
    } else {
        wattron(win, COLOUR_OFFLINE);
    }

    wprintw(win, "%s %s", pre, from);

    if (show != NULL)
        wprintw(win, " is %s", show);
    else
        wprintw(win, " is %s", default_show);

    if (last_activity != NULL) {
        GDateTime *now = g_date_time_new_now_local();
        GTimeSpan span = g_date_time_difference(now, last_activity);

        wprintw(win, ", idle ");

        int hours = span / G_TIME_SPAN_HOUR;
        span = span - hours * G_TIME_SPAN_HOUR;
        if (hours > 0) {
            wprintw(win, "%dh", hours);
        }

        int minutes = span / G_TIME_SPAN_MINUTE;
        span = span - minutes * G_TIME_SPAN_MINUTE;
        wprintw(win, "%dm", minutes);

        int seconds = span / G_TIME_SPAN_SECOND;
        wprintw(win, "%ds", seconds);
    }

    if (status != NULL)
        wprintw(win, ", \"%s\"", status);

    wprintw(win, "\n");

    if (show != NULL) {
        if (strcmp(show, "away") == 0) {
            wattroff(win, COLOUR_AWAY);
        } else if (strcmp(show, "chat") == 0) {
            wattroff(win, COLOUR_CHAT);
        } else if (strcmp(show, "dnd") == 0) {
            wattroff(win, COLOUR_DND);
        } else if (strcmp(show, "xa") == 0) {
            wattroff(win, COLOUR_XA);
        } else if (strcmp(show, "online") == 0) {
            wattroff(win, COLOUR_ONLINE);
        } else {
            wattroff(win, COLOUR_OFFLINE);
        }
    } else if (strcmp(default_show, "online") == 0) {
        wattroff(win, COLOUR_ONLINE);
    } else {
        wattroff(win, COLOUR_OFFLINE);
    }
}

void
win_print_incoming_message(ProfWin *window, GTimeVal *tv_stamp,
    const char * const from, const char * const message)
{
    switch (window->type)
    {
        case WIN_CHAT:
        case WIN_PRIVATE:
            _win_chat_print_incoming_message(window, tv_stamp, from, message);
            break;
        default:
            assert(FALSE);
            break;
    }
}

static void
_win_chat_print_incoming_message(ProfWin *window, GTimeVal *tv_stamp,
    const char * const from, const char * const message)
{
    if (tv_stamp == NULL) {
        win_print_time(window, '-');
    } else {
        GDateTime *time = g_date_time_new_from_timeval_utc(tv_stamp);
        gchar *date_fmt = g_date_time_format(time, "%H:%M:%S");
        wattron(window->win, COLOUR_TIME);
        wprintw(window->win, "%s - ", date_fmt);
        wattroff(window->win, COLOUR_TIME);
        g_date_time_unref(time);
        g_free(date_fmt);
    }

    if (strncmp(message, "/me ", 4) == 0) {
        wattron(window->win, COLOUR_THEM);
        wprintw(window->win, "*%s ", from);
        waddstr(window->win, message + 4);
        wprintw(window->win, "\n");
        wattroff(window->win, COLOUR_THEM);
    } else {
        wattron(window->win, COLOUR_THEM);
        wprintw(window->win, "%s: ", from);
        wattroff(window->win, COLOUR_THEM);
        waddstr(window->win, message);
        wprintw(window->win, "\n");
    }
}
