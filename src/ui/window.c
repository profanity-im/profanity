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
#include "roster_list.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "xmpp/xmpp.h"

#define CONS_WIN_TITLE "Profanity. Type /help for help information."
#define XML_WIN_TITLE "XML Console"

#define CEILING(X) (X-(int)(X) > 0 ? (int)(X+1) : (int)(X))

static void _win_print(ProfWin *window, const char show_char, GDateTime *time,
    int flags, theme_item_t theme_item, const char * const from, const char * const message);
static void _win_print_wrapped(WINDOW *win, const char * const message);

int
win_roster_cols(void)
{
    int roster_win_percent = prefs_get_roster_size();
    int cols = getmaxx(stdscr);
    return CEILING( (((double)cols) / 100) * roster_win_percent);
}

int
win_occpuants_cols(void)
{
    int occupants_win_percent = prefs_get_occupants_size();
    int cols = getmaxx(stdscr);
    return CEILING( (((double)cols) / 100) * occupants_win_percent);
}

static ProfLayout*
_win_create_simple_layout(void)
{
    int cols = getmaxx(stdscr);

    ProfLayoutSimple *layout = malloc(sizeof(ProfLayoutSimple));
    layout->base.type = LAYOUT_SIMPLE;
    layout->base.win = newpad(PAD_SIZE, cols);
    wbkgd(layout->base.win, theme_attrs(THEME_TEXT));
    layout->base.buffer = buffer_create();
    layout->base.y_pos = 0;
    layout->base.paged = 0;
    scrollok(layout->base.win, TRUE);

    return &layout->base;
}

static ProfLayout*
_win_create_split_layout(void)
{
    int cols = getmaxx(stdscr);

    ProfLayoutSplit *layout = malloc(sizeof(ProfLayoutSplit));
    layout->base.type = LAYOUT_SPLIT;
    layout->base.win = newpad(PAD_SIZE, cols);
    wbkgd(layout->base.win, theme_attrs(THEME_TEXT));
    layout->base.buffer = buffer_create();
    layout->base.y_pos = 0;
    layout->base.paged = 0;
    scrollok(layout->base.win, TRUE);
    layout->subwin = NULL;
    layout->sub_y_pos = 0;
    layout->memcheck = LAYOUT_SPLIT_MEMCHECK;

    return &layout->base;
}

ProfWin*
win_create_console(void)
{
    ProfConsoleWin *new_win = malloc(sizeof(ProfConsoleWin));
    new_win->window.type = WIN_CONSOLE;
    new_win->window.layout = _win_create_split_layout();

    return &new_win->window;
}

ProfWin*
win_create_chat(const char * const barejid)
{
    ProfChatWin *new_win = malloc(sizeof(ProfChatWin));
    new_win->window.type = WIN_CHAT;
    new_win->window.layout = _win_create_simple_layout();

    new_win->barejid = strdup(barejid);
    new_win->resource = NULL;
    new_win->is_otr = FALSE;
    new_win->is_trusted = FALSE;
    new_win->history_shown = FALSE;
    new_win->unread = 0;

    new_win->memcheck = PROFCHATWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_muc(const char * const roomjid)
{
    ProfMucWin *new_win = malloc(sizeof(ProfMucWin));
    int cols = getmaxx(stdscr);

    new_win->window.type = WIN_MUC;

    ProfLayoutSplit *layout = malloc(sizeof(ProfLayoutSplit));
    layout->base.type = LAYOUT_SPLIT;

    if (prefs_get_boolean(PREF_OCCUPANTS)) {
        int subwin_cols = win_occpuants_cols();
        layout->base.win = newpad(PAD_SIZE, cols - subwin_cols);
        wbkgd(layout->base.win, theme_attrs(THEME_TEXT));
        layout->subwin = newpad(PAD_SIZE, subwin_cols);;
        wbkgd(layout->subwin, theme_attrs(THEME_TEXT));
    } else {
        layout->base.win = newpad(PAD_SIZE, (cols));
        wbkgd(layout->base.win, theme_attrs(THEME_TEXT));
        layout->subwin = NULL;
    }
    layout->sub_y_pos = 0;
    layout->memcheck = LAYOUT_SPLIT_MEMCHECK;
    layout->base.buffer = buffer_create();
    layout->base.y_pos = 0;
    layout->base.paged = 0;
    scrollok(layout->base.win, TRUE);
    new_win->window.layout = (ProfLayout*)layout;

    new_win->roomjid = strdup(roomjid);
    new_win->unread = 0;

    new_win->memcheck = PROFMUCWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_muc_config(const char * const roomjid, DataForm *form)
{
    ProfMucConfWin *new_win = malloc(sizeof(ProfMucConfWin));
    new_win->window.type = WIN_MUC_CONFIG;
    new_win->window.layout = _win_create_simple_layout();

    new_win->roomjid = strdup(roomjid);
    new_win->form = form;

    new_win->memcheck = PROFCONFWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_private(const char * const fulljid)
{
    ProfPrivateWin *new_win = malloc(sizeof(ProfPrivateWin));
    new_win->window.type = WIN_PRIVATE;
    new_win->window.layout = _win_create_simple_layout();

    new_win->fulljid = strdup(fulljid);
    new_win->unread = 0;

    new_win->memcheck = PROFPRIVATEWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_xmlconsole(void)
{
    ProfXMLWin *new_win = malloc(sizeof(ProfXMLWin));
    new_win->window.type = WIN_XML;
    new_win->window.layout = _win_create_simple_layout();

    new_win->memcheck = PROFXMLWIN_MEMCHECK;

    return &new_win->window;
}

char *
win_get_title(ProfWin *window)
{
    if (window == NULL) {
        return strdup(CONS_WIN_TITLE);
    }
    if (window->type == WIN_CONSOLE) {
        return strdup(CONS_WIN_TITLE);
    }
    if (window->type == WIN_CHAT) {
        ProfChatWin *chatwin = (ProfChatWin*) window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        PContact contact = roster_get_contact(chatwin->barejid);
        if (contact) {
            const char *name = p_contact_name_or_jid(contact);
            return strdup(name);
        } else {
            return strdup(chatwin->barejid);
        }
    }
    if (window->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*) window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        return strdup(mucwin->roomjid);
    }
    if (window->type == WIN_MUC_CONFIG) {
        ProfMucConfWin *confwin = (ProfMucConfWin*) window;
        assert(confwin->memcheck == PROFCONFWIN_MEMCHECK);
        GString *title = g_string_new(confwin->roomjid);
        g_string_append(title, " config");
        if (confwin->form->modified) {
            g_string_append(title, " *");
        }
        char *title_str = title->str;
        g_string_free(title, FALSE);
        return title_str;
    }
    if (window->type == WIN_PRIVATE) {
        ProfPrivateWin *privatewin = (ProfPrivateWin*) window;
        assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
        return strdup(privatewin->fulljid);
    }
    if (window->type == WIN_XML) {
        return strdup(XML_WIN_TITLE);
    }

    return NULL;
}

void
win_hide_subwin(ProfWin *window)
{
    if (window->layout->type == LAYOUT_SPLIT) {
        ProfLayoutSplit *layout = (ProfLayoutSplit*)window->layout;
        if (layout->subwin) {
            delwin(layout->subwin);
        }
        layout->subwin = NULL;
        layout->sub_y_pos = 0;
        int cols = getmaxx(stdscr);
        wresize(layout->base.win, PAD_SIZE, cols);
        win_redraw(window);
    } else {
        int cols = getmaxx(stdscr);
        wresize(window->layout->win, PAD_SIZE, cols);
        win_redraw(window);
    }
}

void
win_show_subwin(ProfWin *window)
{
    int cols = getmaxx(stdscr);
    int subwin_cols = 0;

    if (window->layout->type != LAYOUT_SPLIT) {
        return;
    }

    if (window->type == WIN_MUC) {
        subwin_cols = win_occpuants_cols();
    } else if (window->type == WIN_CONSOLE) {
        subwin_cols = win_roster_cols();
    }

    ProfLayoutSplit *layout = (ProfLayoutSplit*)window->layout;
    layout->subwin = newpad(PAD_SIZE, subwin_cols);
    wbkgd(layout->subwin, theme_attrs(THEME_TEXT));
    wresize(layout->base.win, PAD_SIZE, cols - subwin_cols);
    win_redraw(window);
}

void
win_free(ProfWin* window)
{
    if (window->layout->type == LAYOUT_SPLIT) {
        ProfLayoutSplit *layout = (ProfLayoutSplit*)window->layout;
        if (layout->subwin) {
            delwin(layout->subwin);
        }
        buffer_free(layout->base.buffer);
        delwin(layout->base.win);
    } else {
        buffer_free(window->layout->buffer);
        delwin(window->layout->win);
    }

    if (window->type == WIN_CHAT) {
        ProfChatWin *chatwin = (ProfChatWin*)window;
        free(chatwin->barejid);
        free(chatwin->resource);
    }

    if (window->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*)window;
        free(mucwin->roomjid);
    }

    if (window->type == WIN_MUC_CONFIG) {
        ProfMucConfWin *mucconf = (ProfMucConfWin*)window;
        free(mucconf->roomjid);
        form_destroy(mucconf->form);
    }

    if (window->type == WIN_PRIVATE) {
        ProfPrivateWin *privatewin = (ProfPrivateWin*)window;
        free(privatewin->fulljid);
    }

    free(window);
}

void
win_update_virtual(ProfWin *window)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int subwin_cols = 0;

    if (window->layout->type == LAYOUT_SPLIT) {
        ProfLayoutSplit *layout = (ProfLayoutSplit*)window->layout;
        if (layout->subwin) {
            if (window->type == WIN_MUC) {
                subwin_cols = win_occpuants_cols();
            } else {
                subwin_cols = win_roster_cols();
            }
            pnoutrefresh(layout->base.win, layout->base.y_pos, 0, 1, 0, rows-3, (cols-subwin_cols)-1);
            pnoutrefresh(layout->subwin, layout->sub_y_pos, 0, 1, (cols-subwin_cols), rows-3, cols-1);
        } else {
            pnoutrefresh(layout->base.win, layout->base.y_pos, 0, 1, 0, rows-3, cols-1);
        }
    } else {
        pnoutrefresh(window->layout->win, window->layout->y_pos, 0, 1, 0, rows-3, cols-1);
    }
}

void
win_move_to_end(ProfWin *window)
{
    window->layout->paged = 0;

    int rows = getmaxy(stdscr);
    int y = getcury(window->layout->win);
    int size = rows - 3;

    window->layout->y_pos = y - (size - 1);
    if (window->layout->y_pos < 0) {
        window->layout->y_pos = 0;
    }
}

void
win_show_occupant(ProfWin *window, Occupant *occupant)
{
    const char *presence_str = string_from_resource_presence(occupant->presence);

    theme_item_t presence_colour = theme_main_presence_attrs(presence_str);

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

    theme_item_t presence_colour = theme_main_presence_attrs(presence);

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

    theme_item_t presence_colour = theme_main_presence_attrs(presence_str);

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

    theme_item_t presence_colour = theme_main_presence_attrs(presence);

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
        theme_item_t presence_colour = theme_main_presence_attrs(resource_presence);
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
    theme_item_t presence_colour;

    if (show != NULL) {
        presence_colour = theme_main_presence_attrs(show);
    } else if (strcmp(default_show, "online") == 0) {
        presence_colour = THEME_ONLINE;
    } else {
        presence_colour = THEME_OFFLINE;
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
            win_save_print(window, '-', tv_stamp, NO_ME, THEME_TEXT_THEM, from, message);
            break;
        default:
            assert(FALSE);
            break;
    }
}

void
win_save_vprint(ProfWin *window, const char show_char, GTimeVal *tstamp,
    int flags, theme_item_t theme_item, const char * const from, const char * const message, ...)
{
    va_list arg;
    va_start(arg, message);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, message, arg);
    win_save_print(window, show_char, tstamp, flags, theme_item, from, fmt_msg->str);
    g_string_free(fmt_msg, TRUE);
}

void
win_save_print(ProfWin *window, const char show_char, GTimeVal *tstamp,
    int flags, theme_item_t theme_item, const char * const from, const char * const message)
{
    GDateTime *time;

    if (tstamp == NULL) {
        time = g_date_time_new_now_local();
    } else {
        time = g_date_time_new_from_timeval_utc(tstamp);
    }

    buffer_push(window->layout->buffer, show_char, time, flags, theme_item, from, message);
    _win_print(window, show_char, time, flags, theme_item, from, message);
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
_win_print(ProfWin *window, const char show_char, GDateTime *time,
    int flags, theme_item_t theme_item, const char * const from, const char * const message)
{
    // flags : 1st bit =  0/1 - me/not me
    //         2nd bit =  0/1 - date/no date
    //         3rd bit =  0/1 - eol/no eol
    //         4th bit =  0/1 - color from/no color from
    //         5th bit =  0/1 - color date/no date
    gboolean me_message = FALSE;
    int offset = 0;
    int colour = theme_attrs(THEME_ME);

    if ((flags & NO_DATE) == 0) {
        gchar *date_fmt = NULL;
        char *time_pref = prefs_get_string(PREF_TIME);
        if (g_strcmp0(time_pref, "minutes") == 0) {
            date_fmt = g_date_time_format(time, "%H:%M");
        } else if (g_strcmp0(time_pref, "seconds") == 0) {
            date_fmt = g_date_time_format(time, "%H:%M:%S");
        }
        free(time_pref);

        if (date_fmt) {
            if ((flags & NO_COLOUR_DATE) == 0) {
                wattron(window->layout->win, theme_attrs(THEME_TIME));
            }
            wprintw(window->layout->win, "%s %c ", date_fmt, show_char);
            if ((flags & NO_COLOUR_DATE) == 0) {
                wattroff(window->layout->win, theme_attrs(THEME_TIME));
            }
        }
        g_free(date_fmt);
    }

    if (strlen(from) > 0) {
        if (flags & NO_ME) {
            colour = theme_attrs(THEME_THEM);
        }

        if (flags & NO_COLOUR_FROM) {
            colour = 0;
        }

        wattron(window->layout->win, colour);
        if (strncmp(message, "/me ", 4) == 0) {
            wprintw(window->layout->win, "*%s ", from);
            offset = 4;
            me_message = TRUE;
        } else {
            wprintw(window->layout->win, "%s: ", from);
            wattroff(window->layout->win, colour);
        }
    }

    if (!me_message) {
        wattron(window->layout->win, theme_attrs(theme_item));
    }

    if (prefs_get_boolean(PREF_WRAP)) {
        _win_print_wrapped(window->layout->win, message+offset);
    } else {
        wprintw(window->layout->win, "%s", message+offset);
    }

    if ((flags & NO_EOL) == 0) {
        wprintw(window->layout->win, "\n");
    }

    if (me_message) {
        wattroff(window->layout->win, colour);
    } else {
        wattroff(window->layout->win, theme_attrs(theme_item));
    }
}

static void
_win_indent(WINDOW *win, int size)
{
    int i = 0;
    for (i = 0; i < size; i++) {
        waddch(win, ' ');
    }
}

static void
_win_print_wrapped(WINDOW *win, const char * const message)
{
    int linei = 0;
    int wordi = 0;
    char *word = malloc(strlen(message) + 1);

    char *time_pref = prefs_get_string(PREF_TIME);
    int indent = 0;
    if (g_strcmp0(time_pref, "minutes") == 0) {
        indent = 8;
    } else if (g_strcmp0(time_pref, "seconds") == 0) {
        indent = 11;
    }
    free(time_pref);

    while (message[linei] != '\0') {
        if (message[linei] == ' ') {
            waddch(win, ' ');
            linei++;
        } else if (message[linei] == '\n') {
            waddch(win, '\n');
            _win_indent(win, indent);
            linei++;
        } else {
            wordi = 0;
            while (message[linei] != ' ' && message[linei] != '\n' && message[linei] != '\0') {
                word[wordi++] = message[linei++];
            }
            word[wordi] = '\0';

            int curx = getcurx(win);
            int maxx = getmaxx(win);

            // word larger than line
            if (strlen(word) > (maxx - indent)) {
                int i;
                for (i = 0; i < wordi; i++) {
                    curx = getcurx(win);
                    if (curx < indent) {
                        _win_indent(win, indent);
                    }
                    waddch(win, word[i]);
                }
            } else {
                if (curx + strlen(word) > maxx) {
                    waddch(win, '\n');
                    _win_indent(win, indent);
                }
                if (curx < indent) {
                    _win_indent(win, indent);
                }
                wprintw(win, "%s", word);
            }
        }
    }

    free(word);
}

void
win_redraw(ProfWin *window)
{
    int i, size;
    werase(window->layout->win);
    size = buffer_size(window->layout->buffer);

    for (i = 0; i < size; i++) {
        ProfBuffEntry *e = buffer_yield_entry(window->layout->buffer, i);
        _win_print(window, e->show_char, e->time, e->flags, e->theme_item, e->from, e->message);
    }
}

gboolean
win_has_active_subwin(ProfWin *window)
{
    if (window->layout->type == LAYOUT_SPLIT) {
        ProfLayoutSplit *layout = (ProfLayoutSplit*)window->layout;
        return (layout->subwin != NULL);
    } else {
        return FALSE;
    }
}

int
win_unread(ProfWin *window)
{
    if (window->type == WIN_CHAT) {
        ProfChatWin *chatwin = (ProfChatWin*) window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        return chatwin->unread;
    } else if (window->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*) window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        return mucwin->unread;
    } else if (window->type == WIN_PRIVATE) {
        ProfPrivateWin *privatewin = (ProfPrivateWin*) window;
        assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
        return privatewin->unread;
    } else {
        return 0;
    }
}

void
win_printline_nowrap(WINDOW *win, char *msg)
{
    int maxx = getmaxx(win);
    int cury = getcury(win);

    waddnstr(win, msg, maxx);

    wmove(win, cury+1, 0);
}