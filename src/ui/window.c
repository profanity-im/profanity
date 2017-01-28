/*
 * window.c
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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <wchar.h>

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
#include "ui/screen.h"
#include "xmpp/xmpp.h"
#include "xmpp/roster_list.h"

#define CONS_WIN_TITLE "Profanity. Type /help for help information."
#define XML_WIN_TITLE "XML Console"

#define CEILING(X) (X-(int)(X) > 0 ? (int)(X+1) : (int)(X))

static void _win_printf(ProfWin *window, const char show_char, int pad_indent, GDateTime *timestamp,
    int flags, theme_item_t theme_item, const char *const from, const char *const message, ...);
static void _win_print(ProfWin *window, const char show_char, int pad_indent, GDateTime *time,
    int flags, theme_item_t theme_item, const char *const from, const char *const message, DeliveryReceipt *receipt);
static void _win_print_wrapped(WINDOW *win, const char *const message, size_t indent, int pad_indent);

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
win_create_chat(const char *const barejid)
{
    ProfChatWin *new_win = malloc(sizeof(ProfChatWin));
    new_win->window.type = WIN_CHAT;
    new_win->window.layout = _win_create_simple_layout();

    new_win->barejid = strdup(barejid);
    new_win->resource_override = NULL;
    new_win->is_otr = FALSE;
    new_win->otr_is_trusted = FALSE;
    new_win->pgp_recv = FALSE;
    new_win->pgp_send = FALSE;
    new_win->history_shown = FALSE;
    new_win->unread = 0;
    new_win->state = chat_state_new();
    new_win->enctext = NULL;
    new_win->incoming_char = NULL;
    new_win->outgoing_char = NULL;

    new_win->memcheck = PROFCHATWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_muc(const char *const roomjid)
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
    new_win->unread_mentions = FALSE;
    new_win->unread_triggers = FALSE;
    if (prefs_get_boolean(PREF_OCCUPANTS_JID)) {
        new_win->showjid = TRUE;
    } else {
        new_win->showjid = FALSE;
    }
    new_win->enctext = NULL;
    new_win->message_char = NULL;

    new_win->memcheck = PROFMUCWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_muc_config(const char *const roomjid, DataForm *form)
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
win_create_private(const char *const fulljid)
{
    ProfPrivateWin *new_win = malloc(sizeof(ProfPrivateWin));
    new_win->window.type = WIN_PRIVATE;
    new_win->window.layout = _win_create_simple_layout();

    new_win->fulljid = strdup(fulljid);
    new_win->unread = 0;
    new_win->occupant_offline = FALSE;
    new_win->room_left = FALSE;

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

ProfWin*
win_create_plugin(const char *const plugin_name, const char *const tag)
{
    ProfPluginWin *new_win = malloc(sizeof(ProfPluginWin));
    new_win->super.type = WIN_PLUGIN;
    new_win->super.layout = _win_create_simple_layout();

    new_win->tag = strdup(tag);
    new_win->plugin_name = strdup(plugin_name);

    new_win->memcheck = PROFPLUGINWIN_MEMCHECK;

    return &new_win->super;
}

char*
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
        jabber_conn_status_t conn_status = connection_get_status();
        if (conn_status == JABBER_CONNECTED) {
            PContact contact = roster_get_contact(chatwin->barejid);
            if (contact) {
                const char *name = p_contact_name_or_jid(contact);
                return strdup(name);
            } else {
                return strdup(chatwin->barejid);
            }
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
    if (window->type == WIN_PLUGIN) {
        ProfPluginWin *pluginwin = (ProfPluginWin*) window;
        assert(pluginwin->memcheck == PROFPLUGINWIN_MEMCHECK);
        return strdup(pluginwin->tag);
    }

    return NULL;
}

char*
win_to_string(ProfWin *window)
{
    assert(window != NULL);

    switch (window->type) {
        case WIN_CONSOLE:
        {
            ProfConsoleWin *conswin = (ProfConsoleWin*)window;
            return cons_get_string(conswin);
        }
        case WIN_CHAT:
        {
            ProfChatWin *chatwin = (ProfChatWin*)window;
            return chatwin_get_string(chatwin);
        }
        case WIN_MUC:
        {
            ProfMucWin *mucwin = (ProfMucWin*)window;
            return mucwin_get_string(mucwin);
        }
        case WIN_MUC_CONFIG:
        {
            ProfMucConfWin *mucconfwin = (ProfMucConfWin*)window;
            return mucconfwin_get_string(mucconfwin);
        }
        case WIN_PRIVATE:
        {
            ProfPrivateWin *privwin = (ProfPrivateWin*)window;
            return privwin_get_string(privwin);
        }
        case WIN_XML:
        {
            ProfXMLWin *xmlwin = (ProfXMLWin*)window;
            return xmlwin_get_string(xmlwin);
        }
        case WIN_PLUGIN:
        {
            ProfPluginWin *pluginwin = (ProfPluginWin*)window;
            GString *gstring = g_string_new("");
            g_string_append_printf(gstring, "Plugin: %s", pluginwin->tag);
            char *res = gstring->str;
            g_string_free(gstring, FALSE);
            return res;
        }
        default:
            return NULL;
    }
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
    free(window->layout);

    switch (window->type) {
    case WIN_CHAT:
    {
        ProfChatWin *chatwin = (ProfChatWin*)window;
        free(chatwin->barejid);
        free(chatwin->resource_override);
        free(chatwin->enctext);
        free(chatwin->incoming_char);
        free(chatwin->outgoing_char);
        chat_state_free(chatwin->state);
        break;
    }
    case WIN_MUC:
    {
        ProfMucWin *mucwin = (ProfMucWin*)window;
        free(mucwin->roomjid);
        free(mucwin->enctext);
        free(mucwin->message_char);
        break;
    }
    case WIN_MUC_CONFIG:
    {
        ProfMucConfWin *mucconf = (ProfMucConfWin*)window;
        free(mucconf->roomjid);
        form_destroy(mucconf->form);
        break;
    }
    case WIN_PRIVATE:
    {
        ProfPrivateWin *privatewin = (ProfPrivateWin*)window;
        free(privatewin->fulljid);
        break;
    }
    case WIN_PLUGIN:
    {
        ProfPluginWin *pluginwin = (ProfPluginWin*)window;
        free(pluginwin->tag);
        free(pluginwin->plugin_name);
        break;
    }
    default:
        break;
    }

    free(window);
}

void
win_page_up(ProfWin *window)
{
    int rows = getmaxy(stdscr);
    int y = getcury(window->layout->win);
    int page_space = rows - 4;
    int *page_start = &(window->layout->y_pos);

    *page_start -= page_space;

    // went past beginning, show first page
    if (*page_start < 0)
        *page_start = 0;

    window->layout->paged = 1;
    win_update_virtual(window);

    // switch off page if last line and space line visible
    if ((y) - *page_start == page_space) {
        window->layout->paged = 0;
    }
}

void
win_page_down(ProfWin *window)
{
    int rows = getmaxy(stdscr);
    int y = getcury(window->layout->win);
    int page_space = rows - 4;
    int *page_start = &(window->layout->y_pos);

    *page_start += page_space;

    // only got half a screen, show full screen
    if ((y - (*page_start)) < page_space)
        *page_start = y - page_space;

    // went past end, show full screen
    else if (*page_start >= y)
        *page_start = y - page_space - 1;

    window->layout->paged = 1;
    win_update_virtual(window);

    // switch off page if last line and space line visible
    if ((y) - *page_start == page_space) {
        window->layout->paged = 0;
    }
}

void
win_sub_page_down(ProfWin *window)
{
    if (window->layout->type == LAYOUT_SPLIT) {
        int rows = getmaxy(stdscr);
        int page_space = rows - 4;
        ProfLayoutSplit *split_layout = (ProfLayoutSplit*)window->layout;
        int sub_y = getcury(split_layout->subwin);
        int *sub_y_pos = &(split_layout->sub_y_pos);

        *sub_y_pos += page_space;

        // only got half a screen, show full screen
        if ((sub_y- (*sub_y_pos)) < page_space)
            *sub_y_pos = sub_y - page_space;

        // went past end, show full screen
        else if (*sub_y_pos >= sub_y)
            *sub_y_pos = sub_y - page_space - 1;

        win_update_virtual(window);
    }
}

void
win_sub_page_up(ProfWin *window)
{
    if (window->layout->type == LAYOUT_SPLIT) {
        int rows = getmaxy(stdscr);
        int page_space = rows - 4;
        ProfLayoutSplit *split_layout = (ProfLayoutSplit*)window->layout;
        int *sub_y_pos = &(split_layout->sub_y_pos);

        *sub_y_pos -= page_space;

        // went past beginning, show first page
        if (*sub_y_pos < 0)
            *sub_y_pos = 0;

        win_update_virtual(window);
    }
}

void
win_clear(ProfWin *window)
{
    werase(window->layout->win);
    win_update_virtual(window);
}

void
win_resize(ProfWin *window)
{
    int cols = getmaxx(stdscr);

    if (window->layout->type == LAYOUT_SPLIT) {
        ProfLayoutSplit *layout = (ProfLayoutSplit*)window->layout;
        if (layout->subwin) {
            int subwin_cols = 0;
            if (window->type == WIN_CONSOLE) {
                subwin_cols = win_roster_cols();
            } else if (window->type == WIN_MUC) {
                subwin_cols = win_occpuants_cols();
            }
            wbkgd(layout->base.win, theme_attrs(THEME_TEXT));
            wresize(layout->base.win, PAD_SIZE, cols - subwin_cols);
            wbkgd(layout->subwin, theme_attrs(THEME_TEXT));
            wresize(layout->subwin, PAD_SIZE, subwin_cols);
            if (window->type == WIN_CONSOLE) {
                rosterwin_roster();
            } else if (window->type == WIN_MUC) {
                ProfMucWin *mucwin = (ProfMucWin *)window;
                assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
                occupantswin_occupants(mucwin->roomjid);
            }
        } else {
            wbkgd(layout->base.win, theme_attrs(THEME_TEXT));
            wresize(layout->base.win, PAD_SIZE, cols);
        }
    } else {
        wbkgd(window->layout->win, theme_attrs(THEME_TEXT));
        wresize(window->layout->win, PAD_SIZE, cols);
    }

    win_redraw(window);
}

void
win_update_virtual(ProfWin *window)
{
    int cols = getmaxx(stdscr);

    int row_start = screen_mainwin_row_start();
    int row_end = screen_mainwin_row_end();
    if (window->layout->type == LAYOUT_SPLIT) {
        ProfLayoutSplit *layout = (ProfLayoutSplit*)window->layout;
        if (layout->subwin) {
            int subwin_cols = 0;
            if (window->type == WIN_MUC) {
                subwin_cols = win_occpuants_cols();
            } else {
                subwin_cols = win_roster_cols();
            }
            pnoutrefresh(layout->base.win, layout->base.y_pos, 0, row_start, 0, row_end, (cols-subwin_cols)-1);
            pnoutrefresh(layout->subwin, layout->sub_y_pos, 0, row_start, (cols-subwin_cols), row_end, cols-1);
        } else {
            pnoutrefresh(layout->base.win, layout->base.y_pos, 0, row_start, 0, row_end, cols-1);
        }
    } else {
        pnoutrefresh(window->layout->win, window->layout->y_pos, 0, row_start, 0, row_end, cols-1);
    }
}

void
win_refresh_without_subwin(ProfWin *window)
{
    int cols = getmaxx(stdscr);

    if ((window->type == WIN_MUC) || (window->type == WIN_CONSOLE)) {
        int row_start = screen_mainwin_row_start();
        int row_end = screen_mainwin_row_end();
        pnoutrefresh(window->layout->win, window->layout->y_pos, 0, row_start, 0, row_end, cols-1);
    }
}

void
win_refresh_with_subwin(ProfWin *window)
{
    int cols = getmaxx(stdscr);
    int subwin_cols = 0;

    int row_start = screen_mainwin_row_start();
    int row_end = screen_mainwin_row_end();
    if (window->type == WIN_MUC) {
        ProfLayoutSplit *layout = (ProfLayoutSplit*)window->layout;
        subwin_cols = win_occpuants_cols();
        pnoutrefresh(layout->base.win, layout->base.y_pos, 0, row_start, 0, row_end, (cols-subwin_cols)-1);
        pnoutrefresh(layout->subwin, layout->sub_y_pos, 0, row_start, (cols-subwin_cols), row_end, cols-1);
    } else if (window->type == WIN_CONSOLE) {
        ProfLayoutSplit *layout = (ProfLayoutSplit*)window->layout;
        subwin_cols = win_roster_cols();
        pnoutrefresh(layout->base.win, layout->base.y_pos, 0, row_start, 0, row_end, (cols-subwin_cols)-1);
        pnoutrefresh(layout->subwin, layout->sub_y_pos, 0, row_start, (cols-subwin_cols), row_end, cols-1);
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

    win_print(window, presence_colour, '-', "%s", occupant->nick);
    win_append(window, presence_colour, " is %s", presence_str);

    if (occupant->status) {
        win_append(window, presence_colour, ", \"%s\"", occupant->status);
    }

    win_appendln(window, presence_colour, "");
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

    if (name) {
        win_print(window, presence_colour, '-', "%s", name);
    } else {
        win_print(window, presence_colour, '-', "%s", barejid);
    }

    win_append(window, presence_colour, " is %s", presence);

    if (last_activity) {
        GDateTime *now = g_date_time_new_now_local();
        GTimeSpan span = g_date_time_difference(now, last_activity);
        g_date_time_unref(now);

        int hours = span / G_TIME_SPAN_HOUR;
        span = span - hours * G_TIME_SPAN_HOUR;
        int minutes = span / G_TIME_SPAN_MINUTE;
        span = span - minutes * G_TIME_SPAN_MINUTE;
        int seconds = span / G_TIME_SPAN_SECOND;

        if (hours > 0) {
            win_append(window, presence_colour, ", idle %dh%dm%ds", hours, minutes, seconds);
        } else {
            win_append(window, presence_colour, ", idle %dm%ds", minutes, seconds);
        }
    }

    if (status) {
        win_append(window, presence_colour, ", \"%s\"", p_contact_status(contact));
    }

    win_appendln(window, presence_colour, "");
}

void
win_show_occupant_info(ProfWin *window, const char *const room, Occupant *occupant)
{
    const char *presence_str = string_from_resource_presence(occupant->presence);
    const char *occupant_affiliation = muc_occupant_affiliation_str(occupant);
    const char *occupant_role = muc_occupant_role_str(occupant);

    theme_item_t presence_colour = theme_main_presence_attrs(presence_str);

    win_print(window, presence_colour, '!', "%s", occupant->nick);
    win_append(window, presence_colour, " is %s", presence_str);

    if (occupant->status) {
        win_append(window, presence_colour, ", \"%s\"", occupant->status);
    }

    win_newline(window);

    if (occupant->jid) {
        win_println(window, THEME_DEFAULT, '!', "  Jid: %s", occupant->jid);
    }

    win_println(window, THEME_DEFAULT, '!', "  Affiliation: %s", occupant_affiliation);
    win_println(window, THEME_DEFAULT, '!', "  Role: %s", occupant_role);

    Jid *jidp = jid_create_from_bare_and_resource(room, occupant->nick);
    EntityCapabilities *caps = caps_lookup(jidp->fulljid);
    jid_destroy(jidp);

    if (caps) {
        // show identity
        if (caps->identity) {
            DiscoIdentity *identity = caps->identity;
            win_print(window, THEME_DEFAULT, '!', "  Identity: ");
            if (identity->name) {
                win_append(window, THEME_DEFAULT, "%s", identity->name);
                if (identity->category || identity->type) {
                    win_append(window, THEME_DEFAULT, " ");
                }
            }
            if (identity->type) {
                win_append(window, THEME_DEFAULT, "%s", identity->type);
                if (identity->category) {
                    win_append(window, THEME_DEFAULT, " ");
                }
            }
            if (identity->category) {
                win_append(window, THEME_DEFAULT, "%s", identity->category);
            }
            win_newline(window);
        }

        if (caps->software_version) {
            SoftwareVersion *software_version = caps->software_version;
            if (software_version->software) {
                win_print(window, THEME_DEFAULT, '!', "  Software: %s", software_version->software);
            }
            if (software_version->software_version) {
                win_append(window, THEME_DEFAULT, ", %s", software_version->software_version);
            }
            if (software_version->software || software_version->software_version) {
                win_newline(window);
            }
            if (software_version->os) {
                win_print(window, THEME_DEFAULT, '!', "  OS: %s", software_version->os);
            }
            if (software_version->os_version) {
                win_append(window, THEME_DEFAULT, ", %s", software_version->os_version);
            }
            if (software_version->os || software_version->os_version) {
                win_newline(window);
            }
        }

        caps_destroy(caps);
    }

    win_println(window, THEME_DEFAULT, '-', "");
}

void
win_show_info(ProfWin *window, PContact contact)
{
    const char *barejid = p_contact_barejid(contact);
    const char *name = p_contact_name(contact);
    const char *presence = p_contact_presence(contact);
    const char *sub = p_contact_subscription(contact);
    GDateTime *last_activity = p_contact_last_activity(contact);

    theme_item_t presence_colour = theme_main_presence_attrs(presence);

    win_println(window, THEME_DEFAULT, '-', "");
    win_print(window, presence_colour, '-', "%s", barejid);
    if (name) {
        win_append(window, presence_colour, " (%s)", name);
    }
    win_appendln(window, THEME_DEFAULT, ":");

    if (sub) {
        win_println(window, THEME_DEFAULT, '-', "Subscription: %s", sub);
    }

    if (last_activity) {
        GDateTime *now = g_date_time_new_now_local();
        GTimeSpan span = g_date_time_difference(now, last_activity);

        int hours = span / G_TIME_SPAN_HOUR;
        span = span - hours * G_TIME_SPAN_HOUR;
        int minutes = span / G_TIME_SPAN_MINUTE;
        span = span - minutes * G_TIME_SPAN_MINUTE;
        int seconds = span / G_TIME_SPAN_SECOND;

        if (hours > 0) {
          win_println(window, THEME_DEFAULT, '-', "Last activity: %dh%dm%ds", hours, minutes, seconds);
        }
        else {
          win_println(window, THEME_DEFAULT, '-', "Last activity: %dm%ds", minutes, seconds);
        }

        g_date_time_unref(now);
    }

    GList *resources = p_contact_get_available_resources(contact);
    GList *ordered_resources = NULL;
    if (resources) {
        win_println(window, THEME_DEFAULT, '-', "Resources:");

        // sort in order of availability
        GList *curr = resources;
        while (curr) {
            Resource *resource = curr->data;
            ordered_resources = g_list_insert_sorted(ordered_resources,
                resource, (GCompareFunc)resource_compare_availability);
            curr = g_list_next(curr);
        }
    }
    g_list_free(resources);

    GList *curr = ordered_resources;
    while (curr) {
        Resource *resource = curr->data;
        const char *resource_presence = string_from_resource_presence(resource->presence);
        theme_item_t presence_colour = theme_main_presence_attrs(resource_presence);
        win_print(window, presence_colour, '-', "  %s (%d), %s", resource->name, resource->priority, resource_presence);
        if (resource->status) {
            win_append(window, presence_colour, ", \"%s\"", resource->status);
        }
        win_newline(window);

        Jid *jidp = jid_create_from_bare_and_resource(barejid, resource->name);
        EntityCapabilities *caps = caps_lookup(jidp->fulljid);
        jid_destroy(jidp);

        if (caps) {
            // show identity
            if (caps->identity) {
                DiscoIdentity *identity = caps->identity;
                win_print(window, THEME_DEFAULT, '-', "    Identity: ");
                if (identity->name) {
                    win_append(window, THEME_DEFAULT, "%s", identity->name);
                    if (identity->category || identity->type) {
                        win_append(window, THEME_DEFAULT, " ");
                    }
                }
                if (identity->type) {
                    win_append(window, THEME_DEFAULT, "%s", identity->type);
                    if (identity->category) {
                        win_append(window, THEME_DEFAULT, " ");
                    }
                }
                if (identity->category) {
                    win_append(window, THEME_DEFAULT, "%s", identity->category);
                }
                win_newline(window);
            }

            if (caps->software_version) {
                SoftwareVersion *software_version = caps->software_version;
                if (software_version->software) {
                    win_print(window, THEME_DEFAULT, '-', "    Software: %s", software_version->software);
                }
                if (software_version->software_version) {
                    win_append(window, THEME_DEFAULT, ", %s", software_version->software_version);
                }
                if (software_version->software || software_version->software_version) {
                    win_newline(window);
                }
                if (software_version->os) {
                    win_print(window, THEME_DEFAULT, '-', "    OS: %s", software_version->os);
                }
                if (software_version->os_version) {
                    win_append(window, THEME_DEFAULT, ", %s", software_version->os_version);
                }
                if (software_version->os || software_version->os_version) {
                    win_newline(window);
                }
            }

            caps_destroy(caps);
        }

        curr = g_list_next(curr);
    }
    g_list_free(ordered_resources);
}

void
win_show_status_string(ProfWin *window, const char *const from,
    const char *const show, const char *const status,
    GDateTime *last_activity, const char *const pre,
    const char *const default_show)
{
    theme_item_t presence_colour;

    if (show) {
        presence_colour = theme_main_presence_attrs(show);
    } else if (strcmp(default_show, "online") == 0) {
        presence_colour = THEME_ONLINE;
    } else {
        presence_colour = THEME_OFFLINE;
    }

    win_print(window, presence_colour, '-', "%s %s", pre, from);

    if (show)
        win_append(window, presence_colour, " is %s", show);
    else
        win_append(window, presence_colour, " is %s", default_show);

    if (last_activity) {
        gchar *date_fmt = NULL;
        char *time_pref = prefs_get_string(PREF_TIME_LASTACTIVITY);
        date_fmt = g_date_time_format(last_activity, time_pref);
        prefs_free_string(time_pref);
        assert(date_fmt != NULL);

        win_append(window, presence_colour, ", last activity: %s", date_fmt);

        g_free(date_fmt);
    }

    if (status)
        win_append(window, presence_colour, ", \"%s\"", status);

    win_appendln(window, presence_colour, "");
}

void
win_print_incoming(ProfWin *window, GDateTime *timestamp,
    const char *const from, const char *const message, prof_enc_t enc_mode)
{
    char enc_char = '-';

    switch (window->type)
    {
        case WIN_CHAT:
        {
            ProfChatWin *chatwin = (ProfChatWin*)window;
            if (chatwin->incoming_char) {
                enc_char = chatwin->incoming_char[0];
            } else if (enc_mode == PROF_MSG_OTR) {
                enc_char = prefs_get_otr_char();
            } else if (enc_mode == PROF_MSG_PGP) {
                enc_char = prefs_get_pgp_char();
            }
            _win_printf(window, enc_char, 0, timestamp, NO_ME, THEME_TEXT_THEM, from, "%s", message);
            break;
        }
        case WIN_PRIVATE:
            _win_printf(window, '-', 0, timestamp, NO_ME, THEME_TEXT_THEM, from, "%s", message);
            break;
        default:
            assert(FALSE);
            break;
    }
}

void
win_print_them(ProfWin *window, theme_item_t theme_item, char ch, const char *const them)
{
    _win_printf(window, ch, 0, NULL, NO_ME | NO_EOL, theme_item, them, "");
}

void
win_println_them_message(ProfWin *window, char ch, const char *const them, const char *const message, ...)
{
    GDateTime *timestamp = g_date_time_new_now_local();

    va_list arg;
    va_start(arg, message);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, message, arg);

    buffer_append(window->layout->buffer, ch, 0, timestamp, NO_ME, THEME_TEXT_THEM, them, fmt_msg->str, NULL);

    _win_print(window, ch, 0, timestamp, NO_ME, THEME_TEXT_THEM, them, fmt_msg->str, NULL);
    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);

    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
win_println_me_message(ProfWin *window, char ch, const char *const me, const char *const message, ...)
{
    GDateTime *timestamp = g_date_time_new_now_local();

    va_list arg;
    va_start(arg, message);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, message, arg);

    buffer_append(window->layout->buffer, ch, 0, timestamp, 0, THEME_TEXT_ME, me, fmt_msg->str, NULL);

    _win_print(window, ch, 0, timestamp, 0, THEME_TEXT_ME, me, fmt_msg->str, NULL);
    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);

    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
win_print_outgoing(ProfWin *window, const char ch, const char *const message, ...)
{
    GDateTime *timestamp = g_date_time_new_now_local();

    va_list arg;
    va_start(arg, message);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, message, arg);

    buffer_append(window->layout->buffer, ch, 0, timestamp, 0, THEME_TEXT_ME, "me", fmt_msg->str, NULL);

    _win_print(window, ch, 0, timestamp, 0, THEME_TEXT_ME, "me", fmt_msg->str, NULL);
    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);

    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
win_print_history(ProfWin *window, GDateTime *timestamp, const char *const message, ...)
{
    g_date_time_ref(timestamp);

    va_list arg;
    va_start(arg, message);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, message, arg);

    buffer_append(window->layout->buffer, '-', 0, timestamp, NO_COLOUR_DATE, THEME_DEFAULT, "", fmt_msg->str, NULL);

    _win_print(window, '-', 0, timestamp, NO_COLOUR_DATE, THEME_DEFAULT, "", fmt_msg->str, NULL);
    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);

    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
win_print(ProfWin *window, theme_item_t theme_item, const char ch, const char *const message, ...)
{
    GDateTime *timestamp = g_date_time_new_now_local();

    va_list arg;
    va_start(arg, message);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, message, arg);

    buffer_append(window->layout->buffer, ch, 0, timestamp, NO_EOL, theme_item, "", fmt_msg->str, NULL);

    _win_print(window, ch, 0, timestamp, NO_EOL, theme_item, "", fmt_msg->str, NULL);
    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);

    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
win_println(ProfWin *window, theme_item_t theme_item, const char ch, const char *const message, ...)
{
    GDateTime *timestamp = g_date_time_new_now_local();

    va_list arg;
    va_start(arg, message);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, message, arg);

    buffer_append(window->layout->buffer, ch, 0, timestamp, 0, theme_item, "", fmt_msg->str, NULL);

    _win_print(window, ch, 0, timestamp, 0, theme_item, "", fmt_msg->str, NULL);
    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);

    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
win_println_indent(ProfWin *window, int pad, const char *const message, ...)
{
    GDateTime *timestamp = g_date_time_new_now_local();

    va_list arg;
    va_start(arg, message);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, message, arg);

    buffer_append(window->layout->buffer, '-', pad, timestamp, 0, THEME_DEFAULT, "", fmt_msg->str, NULL);

    _win_print(window, '-', pad, timestamp, 0, THEME_DEFAULT, "", fmt_msg->str, NULL);
    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);

    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
win_append(ProfWin *window, theme_item_t theme_item, const char *const message, ...)
{
    GDateTime *timestamp = g_date_time_new_now_local();

    va_list arg;
    va_start(arg, message);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, message, arg);

    buffer_append(window->layout->buffer, '-', 0, timestamp, NO_DATE | NO_EOL, theme_item, "", fmt_msg->str, NULL);

    _win_print(window, '-', 0, timestamp, NO_DATE | NO_EOL, theme_item, "", fmt_msg->str, NULL);
    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);

    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
win_appendln(ProfWin *window, theme_item_t theme_item, const char *const message, ...)
{
    GDateTime *timestamp = g_date_time_new_now_local();

    va_list arg;
    va_start(arg, message);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, message, arg);

    buffer_append(window->layout->buffer, '-', 0, timestamp, NO_DATE, theme_item, "", fmt_msg->str, NULL);

    _win_print(window, '-', 0, timestamp, NO_DATE, theme_item, "", fmt_msg->str, NULL);
    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);

    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
win_append_highlight(ProfWin *window, theme_item_t theme_item, const char *const message, ...)
{
    GDateTime *timestamp = g_date_time_new_now_local();

    va_list arg;
    va_start(arg, message);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, message, arg);

    buffer_append(window->layout->buffer, '-', 0, timestamp, NO_DATE | NO_ME | NO_EOL, theme_item, "", fmt_msg->str, NULL);

    _win_print(window, '-', 0, timestamp, NO_DATE | NO_ME | NO_EOL, theme_item, "", fmt_msg->str, NULL);
    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);

    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
win_appendln_highlight(ProfWin *window, theme_item_t theme_item, const char *const message, ...)
{
    GDateTime *timestamp = g_date_time_new_now_local();

    va_list arg;
    va_start(arg, message);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, message, arg);

    buffer_append(window->layout->buffer, '-', 0, timestamp, NO_DATE | NO_ME, theme_item, "", fmt_msg->str, NULL);

    _win_print(window, '-', 0, timestamp, NO_DATE | NO_ME, theme_item, "", fmt_msg->str, NULL);
    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);

    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

void
win_print_http_upload(ProfWin *window, const char *const message, char *url)
{
    win_print_with_receipt(window, '!', NULL, message, url);
}

void
win_print_with_receipt(ProfWin *window, const char show_char, const char *const from, const char *const message,
    char *id)
{
    GDateTime *time = g_date_time_new_now_local();

    DeliveryReceipt *receipt = malloc(sizeof(struct delivery_receipt_t));
    receipt->id = strdup(id);
    receipt->received = FALSE;

    buffer_append(window->layout->buffer, show_char, 0, time, 0, THEME_TEXT_ME, from, message, receipt);
    _win_print(window, show_char, 0, time, 0, THEME_TEXT_ME, from, message, receipt);
    // TODO: cross-reference.. this should be replaced by a real event-based system
    inp_nonblocking(TRUE);
    g_date_time_unref(time);
}

void
win_mark_received(ProfWin *window, const char *const id)
{
    gboolean received = buffer_mark_received(window->layout->buffer, id);
    if (received) {
        win_redraw(window);
    }
}

void
win_update_entry_message(ProfWin *window, const char *const id, const char *const message)
{
    ProfBuffEntry *entry = buffer_get_entry_by_id(window->layout->buffer, id);
    if (entry) {
        free(entry->message);
        entry->message = strdup(message);
        win_redraw(window);
    }
}

void
win_newline(ProfWin *window)
{
    win_appendln(window, THEME_DEFAULT, "");
}

static void
_win_printf(ProfWin *window, const char show_char, int pad_indent, GDateTime *timestamp,
    int flags, theme_item_t theme_item, const char *const from, const char *const message, ...)
{
    if (timestamp == NULL) {
        timestamp = g_date_time_new_now_local();
    } else {
        g_date_time_ref(timestamp);
    }

    va_list arg;
    va_start(arg, message);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, message, arg);

    buffer_append(window->layout->buffer, show_char, pad_indent, timestamp, flags, theme_item, from, fmt_msg->str, NULL);

    _win_print(window, show_char, pad_indent, timestamp, flags, theme_item, from, fmt_msg->str, NULL);
    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);

    g_string_free(fmt_msg, TRUE);
    va_end(arg);
}

static void
_win_print(ProfWin *window, const char show_char, int pad_indent, GDateTime *time,
    int flags, theme_item_t theme_item, const char *const from, const char *const message, DeliveryReceipt *receipt)
{
    // flags : 1st bit =  0/1 - me/not me
    //         2nd bit =  0/1 - date/no date
    //         3rd bit =  0/1 - eol/no eol
    //         4th bit =  0/1 - color from/no color from
    //         5th bit =  0/1 - color date/no date
    gboolean me_message = FALSE;
    int offset = 0;
    int colour = theme_attrs(THEME_ME);
    size_t indent = 0;

    char *time_pref = NULL;
    switch (window->type) {
        case WIN_CHAT:
            time_pref = prefs_get_string(PREF_TIME_CHAT);
            break;
        case WIN_MUC:
            time_pref = prefs_get_string(PREF_TIME_MUC);
            break;
        case WIN_MUC_CONFIG:
            time_pref = prefs_get_string(PREF_TIME_MUCCONFIG);
            break;
        case WIN_PRIVATE:
            time_pref = prefs_get_string(PREF_TIME_PRIVATE);
            break;
        case WIN_XML:
            time_pref = prefs_get_string(PREF_TIME_XMLCONSOLE);
            break;
        default:
            time_pref = prefs_get_string(PREF_TIME_CONSOLE);
            break;
    }

    gchar *date_fmt = NULL;
    if (g_strcmp0(time_pref, "off") == 0) {
        date_fmt = g_strdup("");
    } else {
        date_fmt = g_date_time_format(time, time_pref);
    }
    prefs_free_string(time_pref);
    assert(date_fmt != NULL);

    if(strlen(date_fmt) != 0){
        indent = 3 + strlen(date_fmt);
    }

    if ((flags & NO_DATE) == 0) {
        if (date_fmt && strlen(date_fmt)) {
            if ((flags & NO_COLOUR_DATE) == 0) {
                wbkgdset(window->layout->win, theme_attrs(THEME_TIME));
                wattron(window->layout->win, theme_attrs(THEME_TIME));
            }
            wprintw(window->layout->win, "%s %c ", date_fmt, show_char);
            if ((flags & NO_COLOUR_DATE) == 0) {
                wattroff(window->layout->win, theme_attrs(THEME_TIME));
            }
        }
    }

    if (from && strlen(from) > 0) {
        if (flags & NO_ME) {
            colour = theme_attrs(THEME_THEM);
        }

        if (flags & NO_COLOUR_FROM) {
            colour = 0;
        }

        if (receipt && !receipt->received) {
            colour = theme_attrs(THEME_RECEIPT_SENT);
        }

        wbkgdset(window->layout->win, colour);
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
        if (receipt && !receipt->received) {
            wbkgdset(window->layout->win, theme_attrs(THEME_RECEIPT_SENT));
            wattron(window->layout->win, theme_attrs(THEME_RECEIPT_SENT));
        } else {
            wbkgdset(window->layout->win, theme_attrs(theme_item));
            wattron(window->layout->win, theme_attrs(theme_item));
        }
    }

    if (prefs_get_boolean(PREF_WRAP)) {
        _win_print_wrapped(window->layout->win, message+offset, indent, pad_indent);
    } else {
        wprintw(window->layout->win, "%s", message+offset);
    }

    if ((flags & NO_EOL) == 0) {
        int curx = getcurx(window->layout->win);
        if (curx != 0) {
            wprintw(window->layout->win, "\n");
        }
    }

    if (me_message) {
        wattroff(window->layout->win, colour);
    } else {
        if (receipt && !receipt->received) {
            wattroff(window->layout->win, theme_attrs(THEME_RECEIPT_SENT));
        } else {
            wattroff(window->layout->win, theme_attrs(theme_item));
        }
    }

    g_free(date_fmt);
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
_win_print_wrapped(WINDOW *win, const char *const message, size_t indent, int pad_indent)
{
    int starty = getcury(win);
    int wordi = 0;
    char *word = malloc(strlen(message) + 1);

    gchar *curr_ch = g_utf8_offset_to_pointer(message, 0);

    while (*curr_ch != '\0') {

        // handle space
        if (*curr_ch == ' ') {
            waddch(win, ' ');
            curr_ch = g_utf8_next_char(curr_ch);

        // handle newline
        } else if (*curr_ch == '\n') {
            waddch(win, '\n');
            _win_indent(win, indent + pad_indent);
            curr_ch = g_utf8_next_char(curr_ch);

        // handle word
        } else {
            wordi = 0;
            int wordlen = 0;
            while (*curr_ch != ' ' && *curr_ch != '\n' && *curr_ch != '\0') {
                size_t ch_len = mbrlen(curr_ch, MB_CUR_MAX, NULL);
                if ((ch_len == (size_t)-2) || (ch_len == (size_t)-1)) {
                    curr_ch++;
                    continue;
                }
                int offset = 0;
                while (offset < ch_len) {
                    word[wordi++] = curr_ch[offset++];
                }
                curr_ch = g_utf8_next_char(curr_ch);
            }
            word[wordi] = '\0';
            wordlen = utf8_display_len(word);

            int curx = getcurx(win);
            int cury = getcury(win);
            int maxx = getmaxx(win);

            // wrap required
            if (curx + wordlen > maxx) {
                int linelen = maxx - (indent + pad_indent);

                // word larger than line
                if (wordlen > linelen) {
                    gchar *word_ch = g_utf8_offset_to_pointer(word, 0);
                    while(*word_ch != '\0') {
                        curx = getcurx(win);
                        cury = getcury(win);
                        gboolean firstline = cury == starty;

                        if (firstline && curx < indent) {
                            _win_indent(win, indent);
                        }
                        if (!firstline && curx < (indent + pad_indent)) {
                            _win_indent(win, indent + pad_indent);
                        }

                        gchar copy[wordi+1];
                        g_utf8_strncpy(copy, word_ch, 1);
                        waddstr(win, copy);

                        word_ch = g_utf8_next_char(word_ch);
                    }

                // newline and print word
                } else {
                    waddch(win, '\n');
                    curx = getcurx(win);
                    cury = getcury(win);
                    gboolean firstline = cury == starty;

                    if (firstline && curx < indent) {
                        _win_indent(win, indent);
                    }
                    if (!firstline && curx < (indent + pad_indent)) {
                        _win_indent(win, indent + pad_indent);
                    }
                    waddstr(win, word);
                }

            // no wrap required
            } else {
                curx = getcurx(win);
                cury = getcury(win);
                gboolean firstline = cury == starty;

                if (firstline && curx < indent) {
                    _win_indent(win, indent);
                }
                if (!firstline && curx < (indent + pad_indent)) {
                    _win_indent(win, indent + pad_indent);
                }
                waddstr(win, word);
            }
        }

        // consume first space of next line
        int curx = getcurx(win);
        int cury = getcury(win);
        gboolean firstline = (cury == starty);

        if (!firstline && curx == 0 && *curr_ch == ' ') {
            curr_ch = g_utf8_next_char(curr_ch);
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
        ProfBuffEntry *e = buffer_get_entry(window->layout->buffer, i);
        _win_print(window, e->show_char, e->pad_indent, e->time, e->flags, e->theme_item, e->from, e->message, e->receipt);
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

gboolean
win_notify_remind(ProfWin *window)
{
    switch (window->type) {
    case WIN_CHAT:
    {
        ProfChatWin *chatwin = (ProfChatWin*) window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);

        if (prefs_get_boolean(PREF_NOTIFY_CHAT) && chatwin->unread > 0) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
    case WIN_MUC:
    {
        ProfMucWin *mucwin = (ProfMucWin*) window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

        return prefs_do_room_notify_mention(mucwin->roomjid, mucwin->unread, mucwin->unread_mentions, mucwin->unread_triggers);
    }
    case WIN_PRIVATE:
    {
        ProfPrivateWin *privatewin = (ProfPrivateWin*) window;
        assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);

        if (prefs_get_boolean(PREF_NOTIFY_CHAT) && privatewin->unread > 0) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
    default:
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
win_sub_print(WINDOW *win, char *msg, gboolean newline, gboolean wrap, int indent)
{
    int maxx = getmaxx(win);
    int curx = getcurx(win);
    int cury = getcury(win);

    if (wrap) {
        _win_print_wrapped(win, msg, 1, indent);
    } else {
        waddnstr(win, msg, maxx - curx);
    }

    if (newline) {
        wmove(win, cury+1, 0);
    }
}

void
win_sub_newline_lazy(WINDOW *win)
{
    int curx = getcurx(win);
    if (curx > 0) {
        int cury = getcury(win);
        wmove(win, cury+1, 0);
    }
}
