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
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "xmpp/xmpp.h"

static void _win_print(ProfWin *window, const char show_char, const char * const date_fmt,
    int flags, int attrs, const char * const from, const char * const message);


ProfWin*
win_create(const char * const title, int cols, win_type_t type)
{
    ProfWin *new_win = malloc(sizeof(struct prof_win_t));
    new_win->from = strdup(title);

    if (type == WIN_MUC && prefs_get_boolean(PREF_OCCUPANTS)) {
        new_win->win = newpad(PAD_SIZE, (cols/OCCUPANT_WIN_RATIO) * (OCCUPANT_WIN_RATIO-1));
        wbkgd(new_win->win, COLOUR_TEXT);

        new_win->subwin = newpad(PAD_SIZE, OCCUPANT_WIN_WIDTH);
        wbkgd(new_win->subwin, COLOUR_TEXT);
    } else {
        new_win->win = newpad(PAD_SIZE, (cols));
        wbkgd(new_win->win, COLOUR_TEXT);

        new_win->subwin = NULL;
    }

    new_win->buffer = buffer_create();
    new_win->y_pos = 0;
    new_win->sub_y_pos = 0;
    new_win->paged = 0;
    new_win->unread = 0;
    new_win->history_shown = 0;
    new_win->type = type;
    new_win->is_otr = FALSE;
    new_win->is_trusted = FALSE;
    new_win->form = NULL;
    scrollok(new_win->win, TRUE);

    return new_win;
}

void
win_hide_subwin(ProfWin *window)
{
    if (window->subwin) {
        delwin(window->subwin);
    }
    window->subwin = NULL;
    window->sub_y_pos = 0;

    int cols = getmaxx(stdscr);
    wresize(window->win, PAD_SIZE, cols);
    win_redraw(window);
}

void
win_show_subwin(ProfWin *window)
{
    if (!window->subwin) {
        window->subwin = newpad(PAD_SIZE, OCCUPANT_WIN_WIDTH);
        wbkgd(window->subwin, COLOUR_TEXT);

        int cols = getmaxx(stdscr);
        wresize(window->win, PAD_SIZE, (cols/OCCUPANT_WIN_RATIO) * (OCCUPANT_WIN_RATIO-1));
        win_redraw(window);
    }
}

void
win_free(ProfWin* window)
{
    buffer_free(window->buffer);
    delwin(window->win);
    if (window->subwin) {
        delwin(window->subwin);
    }
    free(window->from);
    form_destroy(window->form);
    free(window);
}

void
win_update_virtual(ProfWin *window)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if ((window->type == WIN_MUC) && (window->subwin)) {
        pnoutrefresh(window->win, window->y_pos, 0, 1, 0, rows-3, ((cols/OCCUPANT_WIN_RATIO) * (OCCUPANT_WIN_RATIO-1)) -1);
        pnoutrefresh(window->subwin, window->sub_y_pos, 0, 1, (cols/OCCUPANT_WIN_RATIO) * (OCCUPANT_WIN_RATIO-1), rows-3, cols-1);
    } else {
        pnoutrefresh(window->win, window->y_pos, 0, 1, 0, rows-3, cols-1);
    }
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

int
win_presence_colour(const char * const presence)
{
    if (g_strcmp0(presence, "online") == 0) {
        return COLOUR_ONLINE;
    } else if (g_strcmp0(presence, "away") == 0) {
        return COLOUR_AWAY;
    } else if (g_strcmp0(presence, "chat") == 0) {
        return COLOUR_CHAT;
    } else if (g_strcmp0(presence, "dnd") == 0) {
        return COLOUR_DND;
    } else if (g_strcmp0(presence, "xa") == 0) {
        return COLOUR_XA;
    } else {
        return COLOUR_OFFLINE;
    }
}

void
win_show_occupant(ProfWin *window, Occupant *occupant)
{
    const char *presence_str = string_from_resource_presence(occupant->presence);

    int presence_colour = win_presence_colour(presence_str);

    win_save_print(window, '-', NULL, NO_EOL, presence_colour, "", occupant->nick);
    win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", " is %s", presence_str);

    if (occupant->status) {
        win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", ", \"%s\"", occupant->status);
    }

    win_save_print(window, '-', NULL, NO_DATE, presence_colour, "", "");
}

void
win_show_contact(ProfWin *window, PContact contact)
{
    const char *barejid = p_contact_barejid(contact);
    const char *name = p_contact_name(contact);
    const char *presence = p_contact_presence(contact);
    const char *status = p_contact_status(contact);
    GDateTime *last_activity = p_contact_last_activity(contact);

    int presence_colour = win_presence_colour(presence);

    if (name != NULL) {
        win_save_print(window, '-', NULL, NO_EOL, presence_colour, "", name);
    } else {
        win_save_print(window, '-', NULL, NO_EOL, presence_colour, "", barejid);
    }

    win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", " is %s", presence);

    if (last_activity != NULL) {
        GDateTime *now = g_date_time_new_now_local();
        GTimeSpan span = g_date_time_difference(now, last_activity);

        int hours = span / G_TIME_SPAN_HOUR;
        span = span - hours * G_TIME_SPAN_HOUR;
        int minutes = span / G_TIME_SPAN_MINUTE;
        span = span - minutes * G_TIME_SPAN_MINUTE;
        int seconds = span / G_TIME_SPAN_SECOND;

        if (hours > 0) {
          win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", ", idle %dh%dm%ds", hours, minutes, seconds);
        }
        else {
          win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", ", idle %dm%ds", minutes, seconds);
        }
    }

    if (status != NULL) {
        win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", ", \"%s\"", p_contact_status(contact));
    }

    win_save_print(window, '-', NULL, NO_DATE, presence_colour, "", "");
}

void
win_show_occupant_info(ProfWin *window, const char * const room, Occupant *occupant)
{
    const char *presence_str = string_from_resource_presence(occupant->presence);
    const char *occupant_affiliation = muc_occupant_affiliation_str(occupant);
    const char *occupant_role = muc_occupant_role_str(occupant);

    int presence_colour = win_presence_colour(presence_str);

    win_save_print(window, '!', NULL, NO_EOL, presence_colour, "", occupant->nick);
    win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, presence_colour, "", " is %s", presence_str);

    if (occupant->status) {
        win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, presence_colour, "", ", \"%s\"", occupant->status);
    }

    win_save_newline(window);

    if (occupant->jid) {
        win_save_vprint(window, '!', NULL, 0, 0, "", "  Jid: %s", occupant->jid);
    }

    win_save_vprint(window, '!', NULL, 0, 0, "", "  Affiliation: %s", occupant_affiliation);
    win_save_vprint(window, '!', NULL, 0, 0, "", "  Role: %s", occupant_role);

    Jid *jidp = jid_create_from_bare_and_resource(room, occupant->nick);
    Capabilities *caps = caps_lookup(jidp->fulljid);
    jid_destroy(jidp);

    if (caps) {
        // show identity
        if ((caps->category != NULL) || (caps->type != NULL) || (caps->name != NULL)) {
            win_save_print(window, '!', NULL, NO_EOL, 0, "", "  Identity: ");
            if (caps->name != NULL) {
                win_save_print(window, '!', NULL, NO_DATE | NO_EOL, 0, "", caps->name);
                if ((caps->category != NULL) || (caps->type != NULL)) {
                    win_save_print(window, '-', NULL, NO_DATE | NO_EOL, 0, "", " ");
                }
            }
            if (caps->type != NULL) {
                win_save_print(window, '!', NULL, NO_DATE | NO_EOL, 0, "", caps->type);
                if (caps->category != NULL) {
                    win_save_print(window, '!', NULL, NO_DATE | NO_EOL, 0, "", " ");
                }
            }
            if (caps->category != NULL) {
                win_save_print(window, '!', NULL, NO_DATE | NO_EOL, 0, "", caps->category);
            }
            win_save_newline(window);
        }
        if (caps->software != NULL) {
            win_save_vprint(window, '!', NULL, NO_EOL, 0, "", "  Software: %s", caps->software);
        }
        if (caps->software_version != NULL) {
            win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, 0, "", ", %s", caps->software_version);
        }
        if ((caps->software != NULL) || (caps->software_version != NULL)) {
            win_save_newline(window);
        }
        if (caps->os != NULL) {
            win_save_vprint(window, '!', NULL, NO_EOL, 0, "", "  OS: %s", caps->os);
        }
        if (caps->os_version != NULL) {
            win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, 0, "", ", %s", caps->os_version);
        }
        if ((caps->os != NULL) || (caps->os_version != NULL)) {
            win_save_newline(window);
        }
        caps_destroy(caps);
    }

    win_save_print(window, '-', NULL, 0, 0, "", "");
}

void
win_show_info(ProfWin *window, PContact contact)
{
    const char *barejid = p_contact_barejid(contact);
    const char *name = p_contact_name(contact);
    const char *presence = p_contact_presence(contact);
    const char *sub = p_contact_subscription(contact);
    GList *resources = p_contact_get_available_resources(contact);
    GList *ordered_resources = NULL;
    GDateTime *last_activity = p_contact_last_activity(contact);

    int presence_colour = win_presence_colour(presence);

    win_save_print(window, '-', NULL, 0, 0, "", "");
    win_save_print(window, '-', NULL, NO_EOL, presence_colour, "", barejid);
    if (name != NULL) {
        win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", " (%s)", name);
    }
    win_save_print(window, '-', NULL, NO_DATE, 0, "", ":");

    if (sub != NULL) {
        win_save_vprint(window, '-', NULL, 0, 0, "", "Subscription: %s", sub);
    }

    if (last_activity != NULL) {
        GDateTime *now = g_date_time_new_now_local();
        GTimeSpan span = g_date_time_difference(now, last_activity);

        int hours = span / G_TIME_SPAN_HOUR;
        span = span - hours * G_TIME_SPAN_HOUR;
        int minutes = span / G_TIME_SPAN_MINUTE;
        span = span - minutes * G_TIME_SPAN_MINUTE;
        int seconds = span / G_TIME_SPAN_SECOND;

        if (hours > 0) {
          win_save_vprint(window, '-', NULL, 0, 0, "", "Last activity: %dh%dm%ds", hours, minutes, seconds);
        }
        else {
          win_save_vprint(window, '-', NULL, 0, 0, "", "Last activity: %dm%ds", minutes, seconds);
        }

        g_date_time_unref(now);
    }

    if (resources != NULL) {
        win_save_print(window, '-', NULL, 0, 0, "", "Resources:");

        // sort in order of availabiltiy
        while (resources != NULL) {
            Resource *resource = resources->data;
            ordered_resources = g_list_insert_sorted(ordered_resources,
                resource, (GCompareFunc)resource_compare_availability);
            resources = g_list_next(resources);
        }
    }

    while (ordered_resources != NULL) {
        Resource *resource = ordered_resources->data;
        const char *resource_presence = string_from_resource_presence(resource->presence);
        int presence_colour = win_presence_colour(resource_presence);
        win_save_vprint(window, '-', NULL, NO_EOL, presence_colour, "", "  %s (%d), %s", resource->name, resource->priority, resource_presence);
        if (resource->status != NULL) {
            win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", ", \"%s\"", resource->status);
        }
        win_save_newline(window);

        Jid *jidp = jid_create_from_bare_and_resource(barejid, resource->name);
        Capabilities *caps = caps_lookup(jidp->fulljid);
        jid_destroy(jidp);

        if (caps) {
            // show identity
            if ((caps->category != NULL) || (caps->type != NULL) || (caps->name != NULL)) {
                win_save_print(window, '-', NULL, NO_EOL, 0, "", "    Identity: ");
                if (caps->name != NULL) {
                    win_save_print(window, '-', NULL, NO_DATE | NO_EOL, 0, "", caps->name);
                    if ((caps->category != NULL) || (caps->type != NULL)) {
                        win_save_print(window, '-', NULL, NO_DATE | NO_EOL, 0, "", " ");
                    }
                }
                if (caps->type != NULL) {
                    win_save_print(window, '-', NULL, NO_DATE | NO_EOL, 0, "", caps->type);
                    if (caps->category != NULL) {
                        win_save_print(window, '-', NULL, NO_DATE | NO_EOL, 0, "", " ");
                    }
                }
                if (caps->category != NULL) {
                    win_save_print(window, '-', NULL, NO_DATE | NO_EOL, 0, "", caps->category);
                }
                win_save_newline(window);
            }
            if (caps->software != NULL) {
                win_save_vprint(window, '-', NULL, NO_EOL, 0, "", "    Software: %s", caps->software);
            }
            if (caps->software_version != NULL) {
                win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, 0, "", ", %s", caps->software_version);
            }
            if ((caps->software != NULL) || (caps->software_version != NULL)) {
                win_save_newline(window);
            }
            if (caps->os != NULL) {
                win_save_vprint(window, '-', NULL, NO_EOL, 0, "", "    OS: %s", caps->os);
            }
            if (caps->os_version != NULL) {
                win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, 0, "", ", %s", caps->os_version);
            }
            if ((caps->os != NULL) || (caps->os_version != NULL)) {
                win_save_newline(window);
            }
            caps_destroy(caps);
        }

        ordered_resources = g_list_next(ordered_resources);
    }
}

void
win_show_status_string(ProfWin *window, const char * const from,
    const char * const show, const char * const status,
    GDateTime *last_activity, const char * const pre,
    const char * const default_show)
{
    int presence_colour;

    if (show != NULL) {
        presence_colour = win_presence_colour(show);
    } else if (strcmp(default_show, "online") == 0) {
        presence_colour = COLOUR_ONLINE;
    } else {
        presence_colour = COLOUR_OFFLINE;
    }


    win_save_vprint(window, '-', NULL, NO_EOL, presence_colour, "", "%s %s", pre, from);

    if (show != NULL)
        win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", " is %s", show);
    else
        win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", " is %s", default_show);

    if (last_activity != NULL) {
        GDateTime *now = g_date_time_new_now_local();
        GTimeSpan span = g_date_time_difference(now, last_activity);
        g_date_time_unref(now);

        int hours = span / G_TIME_SPAN_HOUR;
        span = span - hours * G_TIME_SPAN_HOUR;
        int minutes = span / G_TIME_SPAN_MINUTE;
        span = span - minutes * G_TIME_SPAN_MINUTE;
        int seconds = span / G_TIME_SPAN_SECOND;

        if (hours > 0) {
          win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", ", idle %dh%dm%ds", hours, minutes, seconds);
        }
        else {
          win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", ", idle %dm%ds", minutes, seconds);
        }
    }

    if (status != NULL)
        win_save_vprint(window, '-', NULL, NO_DATE | NO_EOL, presence_colour, "", ", \"%s\"", status);

    win_save_print(window, '-', NULL, NO_DATE, presence_colour, "", "");

}

void
win_print_incoming_message(ProfWin *window, GTimeVal *tv_stamp,
    const char * const from, const char * const message)
{
    switch (window->type)
    {
        case WIN_CHAT:
        case WIN_PRIVATE:
            win_save_print(window, '-', tv_stamp, NO_ME, 0, from, message);
            break;
        default:
            assert(FALSE);
            break;
    }
}

void
win_save_vprint(ProfWin *window, const char show_char, GTimeVal *tstamp,
    int flags, int attrs, const char * const from, const char * const message, ...)
{
    va_list arg;
    va_start(arg, message);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, message, arg);
    win_save_print(window, show_char, tstamp, flags, attrs, from, fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
}

void
win_save_print(ProfWin *window, const char show_char, GTimeVal *tstamp,
    int flags, int attrs, const char * const from, const char * const message)
{
    gchar *date_fmt;
    GDateTime *time;

    if (tstamp == NULL) {
        time = g_date_time_new_now_local();
        date_fmt = g_date_time_format(time, "%H:%M:%S");
    } else {
        time = g_date_time_new_from_timeval_utc(tstamp);
        date_fmt = g_date_time_format(time, "%H:%M:%S");
    }

    g_date_time_unref(time);
    buffer_push(window->buffer, show_char, date_fmt, flags, attrs, from, message);
    _win_print(window, show_char, date_fmt, flags, attrs, from, message);
    g_free(date_fmt);
}

void
win_save_println(ProfWin *window, const char * const message)
{
    win_save_print(window, '-', NULL, 0, 0, "", message);
}

void
win_save_newline(ProfWin *window)
{
    win_save_print(window, '-', NULL, NO_DATE, 0, "", "");
}

static void
_win_print(ProfWin *window, const char show_char, const char * const date_fmt,
    int flags, int attrs, const char * const from, const char * const message)
{
    // flags : 1st bit =  0/1 - me/not me
    //         2nd bit =  0/1 - date/no date
    //         3rd bit =  0/1 - eol/no eol
    //         4th bit =  0/1 - color from/no color from
    int unattr_me = 0;
    int offset = 0;
    int colour = COLOUR_ME;

    if ((flags & NO_DATE) == 0) {
        wattron(window->win, COLOUR_TIME);
        wprintw(window->win, "%s %c ", date_fmt, show_char);
        wattroff(window->win, COLOUR_TIME);
    }

    if (strlen(from) > 0) {
        if (flags & NO_ME) {
            colour = COLOUR_THEM;
        }

        if (flags & NO_COLOUR_FROM) {
            colour = 0;
        }

        wattron(window->win, colour);
        if (strncmp(message, "/me ", 4) == 0) {
            wprintw(window->win, "*%s ", from);
            offset = 4;
            unattr_me = 1;
        } else {
            wprintw(window->win, "%s: ", from);
            wattroff(window->win, colour);
        }
    }

    wattron(window->win, attrs);

    if (flags & NO_EOL) {
        wprintw(window->win, "%s", message+offset);
    } else {
        wprintw(window->win, "%s\n", message+offset);
    }

    wattroff(window->win, attrs);

    if (unattr_me) {
        wattroff(window->win, colour);
    }
}

void
win_redraw(ProfWin *window)
{
    int i, size;
    werase(window->win);
    size = buffer_size(window->buffer);

    for (i = 0; i < size; i++) {
        ProfBuffEntry *e = buffer_yield_entry(window->buffer, i);
        _win_print(window, e->show_char, e->date_fmt, e->flags, e->attrs, e->from, e->message);
    }
}
