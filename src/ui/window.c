/*
 * window.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2023 Michael Vetter <jubalh@iodoru.org>
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
#include "ui/window_list.h"

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
#elif HAVE_CURSES_H
#include <curses.h>
#endif

#include "log.h"
#include "config/theme.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "ui/screen.h"
#include "xmpp/xmpp.h"
#include "xmpp/roster_list.h"
#include "xmpp/connection.h"
#include "database.h"

#define CONS_WIN_TITLE "Profanity. Type /help for help information."
#define XML_WIN_TITLE  "XML Console"

#define CEILING(X) (X - (int)(X) > 0 ? (int)(X + 1) : (int)(X))

static void
_win_printf(ProfWin* window, const char* show_char, int pad_indent, GDateTime* timestamp, int flags, theme_item_t theme_item, const char* const display_from, const char* const from_jid, const char* const message_id, const char* const message, ...);
static void _win_print_internal(ProfWin* window, const char* show_char, int pad_indent, GDateTime* time,
                                int flags, theme_item_t theme_item, const char* const from, const char* const message, DeliveryReceipt* receipt);
static void _win_print_wrapped(WINDOW* win, const char* const message, size_t indent, int pad_indent);

static gboolean _reached_top_of_database = FALSE;
static gboolean _reached_bottom_of_database = FALSE;

int
win_roster_cols(void)
{
    int roster_win_percent = prefs_get_roster_size();
    int cols = getmaxx(stdscr);
    return CEILING((((double)cols) / 100) * roster_win_percent);
}

int
win_occpuants_cols(void)
{
    int occupants_win_percent = prefs_get_occupants_size();
    int cols = getmaxx(stdscr);
    return CEILING((((double)cols) / 100) * occupants_win_percent);
}

static ProfLayout*
_win_create_simple_layout(void)
{
    int cols = getmaxx(stdscr);

    ProfLayoutSimple* layout = malloc(sizeof(ProfLayoutSimple));
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

    ProfLayoutSplit* layout = malloc(sizeof(ProfLayoutSplit));
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
    ProfConsoleWin* new_win = malloc(sizeof(ProfConsoleWin));
    new_win->window.type = WIN_CONSOLE;
    new_win->window.layout = _win_create_split_layout();

    return &new_win->window;
}

ProfWin*
win_create_chat(const char* const barejid)
{
    ProfChatWin* new_win = malloc(sizeof(ProfChatWin));
    new_win->window.type = WIN_CHAT;
    new_win->window.layout = _win_create_simple_layout();

    new_win->barejid = strdup(barejid);
    new_win->resource_override = NULL;
    new_win->is_otr = FALSE;
    new_win->otr_is_trusted = FALSE;
    new_win->pgp_recv = FALSE;
    new_win->pgp_send = FALSE;
    new_win->is_omemo = FALSE;
    new_win->is_ox = FALSE;
    new_win->history_shown = FALSE;
    new_win->unread = 0;
    new_win->state = chat_state_new();
    new_win->enctext = NULL;
    new_win->incoming_char = NULL;
    new_win->outgoing_char = NULL;
    new_win->last_message = NULL;
    new_win->last_msg_id = NULL;
    new_win->has_attention = FALSE;
    new_win->memcheck = PROFCHATWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_muc(const char* const roomjid)
{
    ProfMucWin* new_win = malloc(sizeof(ProfMucWin));
    int cols = getmaxx(stdscr);

    new_win->window.type = WIN_MUC;
    ProfLayoutSplit* layout = malloc(sizeof(ProfLayoutSplit));
    layout->base.type = LAYOUT_SPLIT;

    if (prefs_get_boolean(PREF_OCCUPANTS)) {
        int subwin_cols = win_occpuants_cols();
        layout->base.win = newpad(PAD_SIZE, cols - subwin_cols);
        wbkgd(layout->base.win, theme_attrs(THEME_TEXT));
        layout->subwin = newpad(PAD_SIZE, subwin_cols);
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
    new_win->room_name = NULL;
    new_win->unread = 0;
    new_win->unread_mentions = FALSE;
    new_win->unread_triggers = FALSE;
    if (prefs_get_boolean(PREF_OCCUPANTS_JID)) {
        new_win->showjid = TRUE;
    } else {
        new_win->showjid = FALSE;
    }
    if (prefs_get_boolean(PREF_OCCUPANTS_OFFLINE)) {
        new_win->showoffline = TRUE;
    } else {
        new_win->showoffline = FALSE;
    }
    new_win->enctext = NULL;
    new_win->message_char = NULL;
    new_win->is_omemo = FALSE;
    new_win->last_message = NULL;
    new_win->last_msg_id = NULL;
    new_win->has_attention = FALSE;

    new_win->memcheck = PROFMUCWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_config(const char* const roomjid, DataForm* form, ProfConfWinCallback submit, ProfConfWinCallback cancel, const void* userdata)
{
    ProfConfWin* new_win = malloc(sizeof(ProfConfWin));
    new_win->window.type = WIN_CONFIG;
    new_win->window.layout = _win_create_simple_layout();
    new_win->roomjid = strdup(roomjid);
    new_win->form = form;
    new_win->submit = submit;
    new_win->cancel = cancel;
    new_win->userdata = userdata;

    new_win->memcheck = PROFCONFWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_private(const char* const fulljid)
{
    ProfPrivateWin* new_win = malloc(sizeof(ProfPrivateWin));
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
    ProfXMLWin* new_win = malloc(sizeof(ProfXMLWin));
    new_win->window.type = WIN_XML;
    new_win->window.layout = _win_create_simple_layout();

    new_win->memcheck = PROFXMLWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_plugin(const char* const plugin_name, const char* const tag)
{
    ProfPluginWin* new_win = malloc(sizeof(ProfPluginWin));
    new_win->window.type = WIN_PLUGIN;
    new_win->window.layout = _win_create_simple_layout();

    new_win->tag = strdup(tag);
    new_win->plugin_name = strdup(plugin_name);

    new_win->memcheck = PROFPLUGINWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_vcard(vCard* vcard)
{
    ProfVcardWin* new_win = malloc(sizeof(ProfVcardWin));
    new_win->window.type = WIN_VCARD;
    new_win->window.layout = _win_create_simple_layout();

    new_win->vcard = vcard;
    new_win->memcheck = PROFVCARDWIN_MEMCHECK;

    return &new_win->window;
}

gchar*
win_get_title(ProfWin* window)
{
    if (window == NULL) {
        return g_strdup(CONS_WIN_TITLE);
    }

    switch (window->type) {
    case WIN_CONSOLE:
    {
        return g_strdup(CONS_WIN_TITLE);
    }
    case WIN_CHAT:
    {
        const ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        PContact contact = NULL;
        if (roster_exists()) {
            contact = roster_get_contact(chatwin->barejid);
        }
        if (!contact) {
            return g_strdup(chatwin->barejid);
        }
        const char* name = p_contact_name(contact);
        if (!name) {
            return g_strdup(chatwin->barejid);
        }
        return g_strconcat(name, " <", chatwin->barejid, ">", NULL);
    }
    case WIN_MUC:
    {
        const ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        return mucwin_generate_title(mucwin->roomjid, PREF_TITLEBAR_MUC_TITLE);
    }
    case WIN_CONFIG:
    {
        const ProfConfWin* confwin = (ProfConfWin*)window;
        assert(confwin->memcheck == PROFCONFWIN_MEMCHECK);
        auto_gchar gchar* mucwin_title = mucwin_generate_title(confwin->roomjid, PREF_TITLEBAR_MUC_TITLE);
        if (confwin->form->modified) {
            return g_strconcat(mucwin_title, " config *", NULL);
        }
        return g_strconcat(mucwin_title, " config", NULL);
    }
    case WIN_PRIVATE:
    {
        const ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
        assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
        auto_jid Jid* jid = jid_create(privatewin->fulljid);
        auto_gchar gchar* mucwin_title = mucwin_generate_title(jid->barejid, PREF_TITLEBAR_MUC_TITLE);
        return g_strconcat(mucwin_title, "/", jid->resourcepart, NULL);
    }
    case WIN_XML:
    {
        return g_strdup(XML_WIN_TITLE);
    }
    case WIN_PLUGIN:
    {
        ProfPluginWin* pluginwin = (ProfPluginWin*)window;
        assert(pluginwin->memcheck == PROFPLUGINWIN_MEMCHECK);
        return g_strdup(pluginwin->tag);
    }
    case WIN_VCARD:
    {
        ProfVcardWin* vcardwin = (ProfVcardWin*)window;
        assert(vcardwin->memcheck == PROFVCARDWIN_MEMCHECK);

        GString* title = g_string_new("vCard ");
        auto_char char* jid = connection_get_barejid();

        g_string_append(title, jid);

        if (vcardwin->vcard->modified) {
            g_string_append(title, " *");
        }

        return g_string_free(title, FALSE);
    }
    }
    assert(FALSE);
}

char*
win_get_tab_identifier(ProfWin* window)
{
    assert(window != NULL);

    switch (window->type) {
    case WIN_CONSOLE:
    {
        return strdup("console");
    }
    case WIN_CHAT:
    {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        return strdup(chatwin->barejid);
    }
    case WIN_MUC:
    {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        return strdup(mucwin->roomjid);
    }
    case WIN_CONFIG:
    {
        ProfConfWin* confwin = (ProfConfWin*)window;
        return strdup(confwin->roomjid);
    }
    case WIN_PRIVATE:
    {
        ProfPrivateWin* privwin = (ProfPrivateWin*)window;
        return strdup(privwin->fulljid);
    }
    case WIN_PLUGIN:
    {
        ProfPluginWin* pluginwin = (ProfPluginWin*)window;
        return strdup(pluginwin->tag);
    }
    case WIN_XML:
    {
        return strdup("xmlconsole");
    }
    case WIN_VCARD:
    {
        return strdup("vcard");
    }
    }
    assert(FALSE);
}

char*
win_get_last_sent_message(ProfWin* window)
{
    char* last_message = NULL;
    switch (window->type) {
    case WIN_CHAT:
    {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        last_message = chatwin->last_message;
        break;
    }
    case WIN_MUC:
    {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        last_message = mucwin->last_message;
        break;
    }
    default:
        break;
    }

    if (last_message == NULL) {
        return NULL;
    }

    return last_message;
}

gchar*
win_to_string(ProfWin* window)
{
    assert(window != NULL);

    switch (window->type) {
    case WIN_CONSOLE:
    {
        ProfConsoleWin* conswin = (ProfConsoleWin*)window;
        return cons_get_string(conswin);
    }
    case WIN_CHAT:
    {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        return chatwin_get_string(chatwin);
    }
    case WIN_MUC:
    {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        return mucwin_get_string(mucwin);
    }
    case WIN_CONFIG:
    {
        ProfConfWin* confwin = (ProfConfWin*)window;
        return confwin_get_string(confwin);
    }
    case WIN_PRIVATE:
    {
        ProfPrivateWin* privwin = (ProfPrivateWin*)window;
        return privwin_get_string(privwin);
    }
    case WIN_XML:
    {
        ProfXMLWin* xmlwin = (ProfXMLWin*)window;
        return xmlwin_get_string(xmlwin);
    }
    case WIN_PLUGIN:
    {
        ProfPluginWin* pluginwin = (ProfPluginWin*)window;
        GString* gstring = g_string_new("");
        g_string_append_printf(gstring, "Plugin: %s", pluginwin->tag);
        return g_string_free(gstring, FALSE);
    }
    case WIN_VCARD:
    {
        ProfVcardWin* vcardwin = (ProfVcardWin*)window;
        return vcardwin_get_string(vcardwin);
    }
    }
    assert(FALSE);
}

void
win_hide_subwin(ProfWin* window)
{
    if (window->layout->type == LAYOUT_SPLIT) {
        ProfLayoutSplit* layout = (ProfLayoutSplit*)window->layout;
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
win_show_subwin(ProfWin* window)
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

    ProfLayoutSplit* layout = (ProfLayoutSplit*)window->layout;
    layout->subwin = newpad(PAD_SIZE, subwin_cols);
    wbkgd(layout->subwin, theme_attrs(THEME_TEXT));
    wresize(layout->base.win, PAD_SIZE, cols - subwin_cols);
    win_redraw(window);
}

void
win_free(ProfWin* window)
{
    if (window->layout->type == LAYOUT_SPLIT) {
        ProfLayoutSplit* layout = (ProfLayoutSplit*)window->layout;
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
        ProfChatWin* chatwin = (ProfChatWin*)window;
        free(chatwin->barejid);
        free(chatwin->resource_override);
        free(chatwin->enctext);
        free(chatwin->incoming_char);
        free(chatwin->outgoing_char);
        free(chatwin->last_message);
        free(chatwin->last_msg_id);
        chat_state_free(chatwin->state);
        break;
    }
    case WIN_MUC:
    {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        free(mucwin->roomjid);
        free(mucwin->room_name);
        free(mucwin->enctext);
        free(mucwin->message_char);
        free(mucwin->last_message);
        free(mucwin->last_msg_id);
        break;
    }
    case WIN_CONFIG:
    {
        ProfConfWin* conf = (ProfConfWin*)window;
        free(conf->roomjid);
        form_destroy(conf->form);
        break;
    }
    case WIN_PRIVATE:
    {
        ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
        free(privatewin->fulljid);
        break;
    }
    case WIN_PLUGIN:
    {
        ProfPluginWin* pluginwin = (ProfPluginWin*)window;
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
win_page_up(ProfWin* window, int scroll_size)
{
    _reached_bottom_of_database = FALSE;
    int rows = getmaxy(stdscr);
    int total_rows = getcury(window->layout->win);
    int page_space = rows - 4;
    int* page_start = &(window->layout->y_pos);
    int page_start_initial = *page_start;
    if (scroll_size == 0)
        scroll_size = page_space;

    *page_start -= scroll_size;

    if (*page_start == -scroll_size && window->type == WIN_CHAT) {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        ProfBuffEntry* first_entry = buffer_size(window->layout->buffer) != 0 ? buffer_get_entry(window->layout->buffer, 0) : NULL;

        // Don't do anything if still fetching mam messages
        if (first_entry && !(first_entry->theme_item == THEME_ROOMINFO && g_strcmp0(first_entry->message, LOADING_MESSAGE) == 0)) {
            if (!_reached_top_of_database) {
                _reached_top_of_database = !chatwin_db_history(chatwin, NULL, NULL, TRUE);
            }

            if (_reached_top_of_database && prefs_get_boolean(PREF_MAM)) {
                win_print_loading_history(window);
                iq_mam_request_older(chatwin);
            }
        }
    }

    // went past beginning, show first page
    if (*page_start < 0)
        *page_start = 0;

    window->layout->paged = 1;
    // update only if position has changed
    if (page_start_initial != *page_start) {
        win_update_virtual(window);
    }

    // switch off page if last line and space line visible
    if ((total_rows) - *page_start == page_space) {
        window->layout->paged = 0;
    }
}

void
win_page_down(ProfWin* window, int scroll_size)
{
    _reached_top_of_database = FALSE;
    int rows = getmaxy(stdscr);
    int* page_start = &(window->layout->y_pos);
    int total_rows = getcury(window->layout->win);
    int page_space = rows - 4;
    int page_start_initial = *page_start;
    if (scroll_size == 0)
        scroll_size = page_space;

    *page_start += scroll_size;

    // Scrolled down after reaching the bottom of the page
    if ((*page_start == total_rows || (*page_start == page_space && *page_start >= total_rows)) && window->type == WIN_CHAT) {
        int bf_size = buffer_size(window->layout->buffer);
        if (bf_size > 0) {
            auto_gchar gchar* start = g_date_time_format_iso8601(buffer_get_entry(window->layout->buffer, bf_size - 1)->time);
            GDateTime* now = g_date_time_new_now_local();
            gchar* end = g_date_time_format_iso8601(now);
            // end is free'd inside
            if (!_reached_bottom_of_database && !chatwin_db_history((ProfChatWin*)window, start, end, FALSE)) {
                _reached_bottom_of_database = TRUE;
            }

            g_date_time_unref(now);
        }
    }

    // only got half a screen, show full screen
    if ((total_rows - (*page_start)) < page_space)
        *page_start = total_rows - page_space;

    // went past end, show full screen
    else if (*page_start >= total_rows)
        *page_start = total_rows - page_space - 1;

    window->layout->paged = 1;
    // update only if position has changed
    if (page_start_initial != *page_start) {
        win_update_virtual(window);
    }

    // switch off page if last line and space line visible
    if ((total_rows) - *page_start == page_space) {
        window->layout->paged = 0;
    }
}

void
win_sub_page_down(ProfWin* window)
{
    if (window->layout->type == LAYOUT_SPLIT) {
        int rows = getmaxy(stdscr);
        int page_space = rows - 4;
        ProfLayoutSplit* split_layout = (ProfLayoutSplit*)window->layout;
        int sub_y = getcury(split_layout->subwin);
        int* sub_y_pos = &(split_layout->sub_y_pos);

        *sub_y_pos += page_space;

        // only got half a screen, show full screen
        if ((sub_y - (*sub_y_pos)) < page_space)
            *sub_y_pos = sub_y - page_space;

        // went past end, show full screen
        else if (*sub_y_pos >= sub_y)
            *sub_y_pos = sub_y - page_space - 1;

        win_update_virtual(window);
    }
}

void
win_sub_page_up(ProfWin* window)
{
    if (window->layout->type == LAYOUT_SPLIT) {
        int rows = getmaxy(stdscr);
        int page_space = rows - 4;
        ProfLayoutSplit* split_layout = (ProfLayoutSplit*)window->layout;
        int* sub_y_pos = &(split_layout->sub_y_pos);

        *sub_y_pos -= page_space;

        // went past beginning, show first page
        if (*sub_y_pos < 0)
            *sub_y_pos = 0;

        win_update_virtual(window);
    }
}

void
win_clear(ProfWin* window)
{
    if (!prefs_get_boolean(PREF_CLEAR_PERSIST_HISTORY)) {
        werase(window->layout->win);
        buffer_free(window->layout->buffer);
        window->layout->buffer = buffer_create();
        return;
    }

    int y = getcury(window->layout->win);
    int* page_start = &(window->layout->y_pos);
    *page_start = y;
    window->layout->paged = 1;
    win_update_virtual(window);
}

void
win_resize(ProfWin* window)
{
    int cols = getmaxx(stdscr);

    if (window->layout->type == LAYOUT_SPLIT) {
        ProfLayoutSplit* layout = (ProfLayoutSplit*)window->layout;
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
                ProfMucWin* mucwin = (ProfMucWin*)window;
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
win_update_virtual(ProfWin* window)
{
    int cols = getmaxx(stdscr);

    int row_start = screen_mainwin_row_start();
    int row_end = screen_mainwin_row_end();
    if (window->layout->type == LAYOUT_SPLIT) {
        ProfLayoutSplit* layout = (ProfLayoutSplit*)window->layout;
        if (layout->subwin) {
            int subwin_cols = 0;
            if (window->type == WIN_MUC) {
                subwin_cols = win_occpuants_cols();
            } else {
                subwin_cols = win_roster_cols();
            }
            pnoutrefresh(layout->base.win, layout->base.y_pos, 0, row_start, 0, row_end, (cols - subwin_cols) - 1);
            pnoutrefresh(layout->subwin, layout->sub_y_pos, 0, row_start, (cols - subwin_cols), row_end, cols - 1);
        } else {
            pnoutrefresh(layout->base.win, layout->base.y_pos, 0, row_start, 0, row_end, cols - 1);
        }
    } else {
        pnoutrefresh(window->layout->win, window->layout->y_pos, 0, row_start, 0, row_end, cols - 1);
    }
}

void
win_refresh_without_subwin(ProfWin* window)
{
    int cols = getmaxx(stdscr);

    if ((window->type == WIN_MUC) || (window->type == WIN_CONSOLE)) {
        int row_start = screen_mainwin_row_start();
        int row_end = screen_mainwin_row_end();
        pnoutrefresh(window->layout->win, window->layout->y_pos, 0, row_start, 0, row_end, cols - 1);
    }
}

void
win_refresh_with_subwin(ProfWin* window)
{
    int subwin_cols = 0;
    int cols = getmaxx(stdscr);
    int row_start = screen_mainwin_row_start();
    int row_end = screen_mainwin_row_end();
    ProfLayoutSplit* layout = (ProfLayoutSplit*)window->layout;

    if (window->type == WIN_MUC) {
        subwin_cols = win_occpuants_cols();
    } else if (window->type == WIN_CONSOLE) {
        subwin_cols = win_roster_cols();
    } else {
        // Other window types don't support subwindows, we shouldn't be here
        return;
    }

    pnoutrefresh(layout->base.win, layout->base.y_pos, 0, row_start, 0, row_end, (cols - subwin_cols) - 1);
    pnoutrefresh(layout->subwin, layout->sub_y_pos, 0, row_start, (cols - subwin_cols), row_end, cols - 1);
}

void
win_move_to_end(ProfWin* window)
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
win_show_occupant(ProfWin* window, Occupant* occupant)
{
    const char* presence_str = string_from_resource_presence(occupant->presence);

    theme_item_t presence_colour = theme_main_presence_attrs(presence_str);

    win_print(window, presence_colour, "-", "%s", occupant->nick);
    win_append(window, presence_colour, " is %s", presence_str);

    if (occupant->status) {
        win_append(window, presence_colour, ", \"%s\"", occupant->status);
    }

    win_appendln(window, presence_colour, "");
}

void
win_show_contact(ProfWin* window, PContact contact)
{
    const char* barejid = p_contact_barejid(contact);
    const char* name = p_contact_name(contact);
    const char* presence = p_contact_presence(contact);
    const char* status = p_contact_status(contact);
    GDateTime* last_activity = p_contact_last_activity(contact);

    theme_item_t presence_colour = theme_main_presence_attrs(presence);

    if (name) {
        win_print(window, presence_colour, "-", "%s", name);
    } else {
        win_print(window, presence_colour, "-", "%s", barejid);
    }

    win_append(window, presence_colour, " is %s", presence);

    if (last_activity) {
        GDateTime* now = g_date_time_new_now_local();
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
win_show_occupant_info(ProfWin* window, const char* const room, Occupant* occupant)
{
    const char* presence_str = string_from_resource_presence(occupant->presence);
    const char* occupant_affiliation = muc_occupant_affiliation_str(occupant);
    const char* occupant_role = muc_occupant_role_str(occupant);

    theme_item_t presence_colour = theme_main_presence_attrs(presence_str);

    win_print(window, presence_colour, "!", "%s", occupant->nick);
    win_append(window, presence_colour, " is %s", presence_str);

    if (occupant->status) {
        win_append(window, presence_colour, ", \"%s\"", occupant->status);
    }

    win_newline(window);

    if (occupant->jid) {
        win_println(window, THEME_DEFAULT, "!", "  Jid: %s", occupant->jid);
    }

    win_println(window, THEME_DEFAULT, "!", "  Affiliation: %s", occupant_affiliation);
    win_println(window, THEME_DEFAULT, "!", "  Role: %s", occupant_role);

    auto_jid Jid* jidp = jid_create_from_bare_and_resource(room, occupant->nick);
    EntityCapabilities* caps = caps_lookup(jidp->fulljid);

    if (caps) {
        // show identity
        if (caps->identity) {
            DiscoIdentity* identity = caps->identity;
            win_print(window, THEME_DEFAULT, "!", "  Identity: ");
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
            SoftwareVersion* software_version = caps->software_version;
            if (software_version->software) {
                win_print(window, THEME_DEFAULT, "!", "  Software: %s", software_version->software);
            }
            if (software_version->software_version) {
                win_append(window, THEME_DEFAULT, ", %s", software_version->software_version);
            }
            if (software_version->software || software_version->software_version) {
                win_newline(window);
            }
            if (software_version->os) {
                win_print(window, THEME_DEFAULT, "!", "  OS: %s", software_version->os);
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

    win_println(window, THEME_DEFAULT, "-", "");
}

void
win_show_info(ProfWin* window, PContact contact)
{
    const char* barejid = p_contact_barejid(contact);
    const char* name = p_contact_name(contact);
    const char* presence = p_contact_presence(contact);
    const char* sub = p_contact_subscription(contact);
    GDateTime* last_activity = p_contact_last_activity(contact);

    theme_item_t presence_colour = theme_main_presence_attrs(presence);

    win_println(window, THEME_DEFAULT, "-", "");
    win_print(window, presence_colour, "-", "%s", barejid);
    if (name) {
        win_append(window, presence_colour, " (%s)", name);
    }
    win_appendln(window, THEME_DEFAULT, ":");

    if (sub) {
        win_println(window, THEME_DEFAULT, "-", "Subscription: %s", sub);
    }

    if (last_activity) {
        GDateTime* now = g_date_time_new_now_local();
        GTimeSpan span = g_date_time_difference(now, last_activity);

        int hours = span / G_TIME_SPAN_HOUR;
        span = span - hours * G_TIME_SPAN_HOUR;
        int minutes = span / G_TIME_SPAN_MINUTE;
        span = span - minutes * G_TIME_SPAN_MINUTE;
        int seconds = span / G_TIME_SPAN_SECOND;

        if (hours > 0) {
            win_println(window, THEME_DEFAULT, "-", "Last activity: %dh%dm%ds", hours, minutes, seconds);
        } else {
            win_println(window, THEME_DEFAULT, "-", "Last activity: %dm%ds", minutes, seconds);
        }

        g_date_time_unref(now);
    }

    GList* resources = p_contact_get_available_resources(contact);
    GList* ordered_resources = NULL;
    if (resources) {
        win_println(window, THEME_DEFAULT, "-", "Resources:");

        // sort in order of availability
        GList* curr = resources;
        while (curr) {
            Resource* resource = curr->data;
            ordered_resources = g_list_insert_sorted(ordered_resources,
                                                     resource, (GCompareFunc)resource_compare_availability);
            curr = g_list_next(curr);
        }
    }
    g_list_free(resources);

    GList* curr = ordered_resources;
    while (curr) {
        Resource* resource = curr->data;
        const char* resource_presence = string_from_resource_presence(resource->presence);
        theme_item_t presence_colour = theme_main_presence_attrs(resource_presence);
        win_print(window, presence_colour, "-", "  %s (%d), %s", resource->name, resource->priority, resource_presence);
        if (resource->status) {
            win_append(window, presence_colour, ", \"%s\"", resource->status);
        }
        win_newline(window);

        auto_jid Jid* jidp = jid_create_from_bare_and_resource(barejid, resource->name);
        EntityCapabilities* caps = caps_lookup(jidp->fulljid);

        if (caps) {
            // show identity
            if (caps->identity) {
                DiscoIdentity* identity = caps->identity;
                win_print(window, THEME_DEFAULT, "-", "    Identity: ");
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
                SoftwareVersion* software_version = caps->software_version;
                if (software_version->software) {
                    win_print(window, THEME_DEFAULT, "-", "    Software: %s", software_version->software);
                }
                if (software_version->software_version) {
                    win_append(window, THEME_DEFAULT, ", %s", software_version->software_version);
                }
                if (software_version->software || software_version->software_version) {
                    win_newline(window);
                }
                if (software_version->os) {
                    win_print(window, THEME_DEFAULT, "-", "    OS: %s", software_version->os);
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
win_show_vcard(ProfWin* window, vCard* vcard)
{
    GList* pointer;
    int index = 0;

    if (vcard->fullname) {
        win_println(window, THEME_DEFAULT, "!", "Full name: %s", vcard->fullname);
    }
    if (vcard->name.family || vcard->name.given || vcard->name.middle || vcard->name.prefix || vcard->name.suffix) {
        win_println(window, THEME_DEFAULT, "!", "Name: ");
        if (vcard->name.family) {
            win_println(window, THEME_DEFAULT, "!", "    Family: %s", vcard->name.family);
        }
        if (vcard->name.given) {
            win_println(window, THEME_DEFAULT, "!", "    Given: %s", vcard->name.given);
        }
        if (vcard->name.middle) {
            win_println(window, THEME_DEFAULT, "!", "    Middle: %s", vcard->name.middle);
        }
        if (vcard->name.prefix) {
            win_println(window, THEME_DEFAULT, "!", "    Prefix: %s", vcard->name.prefix);
        }
        if (vcard->name.suffix) {
            win_println(window, THEME_DEFAULT, "!", "    Suffix: %s", vcard->name.suffix);
        }
    }
    index = 0;
    for (pointer = g_queue_peek_head_link(vcard->elements); pointer != NULL; pointer = pointer->next, index++) {
        assert(pointer->data != NULL);
        vcard_element_t* element = (vcard_element_t*)pointer->data;

        switch (element->type) {
        case VCARD_NICKNAME:
        {
            win_println(window, THEME_DEFAULT, "!", "[%d] Nickname: %s", index, element->nickname);
            break;
        }
        case VCARD_PHOTO:
        {
            if (element->photo.external) {
                win_println(window, THEME_DEFAULT, "!", "[%d] Photo, External value: %s", index, element->photo.extval);
            } else {
                win_println(window, THEME_DEFAULT, "!", "[%d] Photo (%s), size: %zu", index, element->photo.type, element->photo.length);
            }
            break;
        }
        case VCARD_BIRTHDAY:
        {
            auto_gchar gchar* date_format = prefs_get_string(PREF_TIME_VCARD);
            auto_gchar gchar* date = g_date_time_format(element->birthday, date_format);

            assert(date != NULL);
            win_println(window, THEME_DEFAULT, "!", "[%d] Birthday: %s", index, date);
            break;
        }
        case VCARD_ADDRESS:
        {
            // Print the header with flags
            win_println(window, THEME_DEFAULT, "!", "[%d] Address%s%s%s%s%s%s%s", index,
                        (element->address.options & VCARD_HOME) ? " [home]" : "",
                        (element->address.options & VCARD_WORK) == VCARD_WORK ? " [work]" : "",
                        (element->address.options & VCARD_POSTAL) == VCARD_POSTAL ? " [postal]" : "",
                        (element->address.options & VCARD_PARCEL) == VCARD_PARCEL ? " [parcel]" : "",
                        (element->address.options & VCARD_INTL) == VCARD_INTL ? " [international]" : "",
                        (element->address.options & VCARD_DOM) == VCARD_DOM ? " [domestic]" : "",
                        (element->address.options & VCARD_PREF) == VCARD_PREF ? " [preferred]" : "");

            if (element->address.pobox) {
                win_println(window, THEME_DEFAULT, "!", "    P.O. Box: %s", element->address.pobox);
            }
            if (element->address.extaddr) {
                win_println(window, THEME_DEFAULT, "!", "    Extended address: %s", element->address.extaddr);
            }
            if (element->address.street) {
                win_println(window, THEME_DEFAULT, "!", "    Street: %s", element->address.street);
            }
            if (element->address.locality) {
                win_println(window, THEME_DEFAULT, "!", "    Locality: %s", element->address.locality);
            }
            if (element->address.region) {
                win_println(window, THEME_DEFAULT, "!", "    Region: %s", element->address.region);
            }
            if (element->address.pcode) {
                win_println(window, THEME_DEFAULT, "!", "    Postal code: %s", element->address.pcode);
            }
            if (element->address.country) {
                win_println(window, THEME_DEFAULT, "!", "    Country: %s", element->address.country);
            }
            break;
        }
        case VCARD_TELEPHONE:
        {
            // Print the header with flags
            win_println(window, THEME_DEFAULT, "!", "[%d] Telephone%s%s%s%s%s%s%s%s%s%s%s%s%s", index,
                        (element->telephone.options & VCARD_HOME) ? " [home]" : "",
                        (element->telephone.options & VCARD_WORK) == VCARD_WORK ? " [work]" : "",
                        (element->telephone.options & VCARD_TEL_VOICE) == VCARD_TEL_VOICE ? " [voice]" : "",
                        (element->telephone.options & VCARD_TEL_FAX) == VCARD_TEL_FAX ? " [fax]" : "",
                        (element->telephone.options & VCARD_TEL_PAGER) == VCARD_TEL_PAGER ? " [pager]" : "",
                        (element->telephone.options & VCARD_TEL_MSG) == VCARD_TEL_MSG ? " [msg]" : "",
                        (element->telephone.options & VCARD_TEL_CELL) == VCARD_TEL_CELL ? " [cell]" : "",
                        (element->telephone.options & VCARD_TEL_VIDEO) == VCARD_TEL_VIDEO ? " [video]" : "",
                        (element->telephone.options & VCARD_TEL_BBS) == VCARD_TEL_BBS ? " [bbs]" : "",
                        (element->telephone.options & VCARD_TEL_MODEM) == VCARD_TEL_MODEM ? " [modem]" : "",
                        (element->telephone.options & VCARD_TEL_ISDN) == VCARD_TEL_ISDN ? " [isdn]" : "",
                        (element->telephone.options & VCARD_TEL_PCS) == VCARD_TEL_PCS ? " [pcs]" : "",
                        (element->telephone.options & VCARD_PREF) == VCARD_PREF ? " [preferred]" : "");
            if (element->telephone.number) {
                win_println(window, THEME_DEFAULT, "!", "    Number: %s", element->telephone.number);
            }
            break;
        }
        case VCARD_EMAIL:
        {
            // Print the header with flags
            win_println(window, THEME_DEFAULT, "!", "[%d] E-mail%s%s%s%s%s", index,
                        (element->email.options & VCARD_HOME) ? " [home]" : "",
                        (element->email.options & VCARD_WORK) == VCARD_WORK ? " [work]" : "",
                        (element->email.options & VCARD_EMAIL_X400) == VCARD_EMAIL_X400 ? " [x400]" : "",
                        (element->email.options & VCARD_EMAIL_INTERNET) == VCARD_EMAIL_INTERNET ? " [internet]" : "",
                        (element->email.options & VCARD_PREF) == VCARD_PREF ? " [preferred]" : "");
            if (element->email.userid) {
                win_println(window, THEME_DEFAULT, "!", "    ID: %s", element->email.userid);
            }
            break;
        }
        case VCARD_JID:
        {
            win_println(window, THEME_DEFAULT, "!", "[%d] Jabber ID: %s", index, element->jid);
            break;
        }
        case VCARD_TITLE:
        {
            win_println(window, THEME_DEFAULT, "!", "[%d] Title: %s", index, element->title);
            break;
        }
        case VCARD_ROLE:
        {
            win_println(window, THEME_DEFAULT, "!", "[%d] Role: %s", index, element->role);
            break;
        }
        case VCARD_NOTE:
        {
            win_println(window, THEME_DEFAULT, "!", "[%d] Note: %s", index, element->note);
            break;
        }
        case VCARD_URL:
            win_println(window, THEME_DEFAULT, "!", "[%d] URL: %s", index, element->url);
            break;
        }
    }
}

void
win_show_status_string(ProfWin* window, const char* const from,
                       const char* const show, const char* const status,
                       GDateTime* last_activity, const char* const pre,
                       const char* const default_show)
{
    theme_item_t presence_colour;

    if (show) {
        presence_colour = theme_main_presence_attrs(show);
    } else if (strcmp(default_show, "online") == 0) {
        presence_colour = THEME_ONLINE;
    } else {
        presence_colour = THEME_OFFLINE;
    }

    win_print(window, presence_colour, "-", "%s %s", pre, from);

    if (show)
        win_append(window, presence_colour, " is %s", show);
    else
        win_append(window, presence_colour, " is %s", default_show);

    if (last_activity) {
        auto_gchar gchar* date_fmt = NULL;
        auto_gchar gchar* time_pref = prefs_get_string(PREF_TIME_LASTACTIVITY);
        date_fmt = g_date_time_format(last_activity, time_pref);
        assert(date_fmt != NULL);

        win_append(window, presence_colour, ", last activity: %s", date_fmt);
    }

    if (status)
        win_append(window, presence_colour, ", \"%s\"", status);

    win_appendln(window, presence_colour, "");
}

/** Corrects the visual representation of a message with prior check for sender validity.
 *
 * Returns TRUE if the message was successfully corrected and should not be printed, FALSE otherwise.
 */
static gboolean
_win_correct(ProfWin* window, const char* const message, const char* const id, const char* const replace_id, const char* const from_jid)
{
    if (!replace_id) {
        return FALSE;
    }

    ProfBuffEntry* entry = buffer_get_entry_by_id(window->layout->buffer, replace_id);
    if (!entry) {
        log_warning("Replace ID %s could not be found in buffer. Message: %s", replace_id, message);
        return FALSE;
    }

    if (g_strcmp0(entry->from_jid, from_jid) != 0) {
        log_debug("Illicit LMC attempt from %s for message from %s with: %s", from_jid, entry->from_jid, message);
        cons_show("Illicit LMC attempt from %s for message from %s", from_jid, entry->from_jid);
        return TRUE;
    }

    /*TODO: set date?
    if (entry->date) {
        if (entry->date->timestamp) {
            g_date_time_unref(entry->date->timestamp);
        }
        free(entry->date);
    }

    entry->date = buffer_date_new_now();
    */

    g_free(entry->show_char);
    entry->show_char = prefs_get_correction_char();

    if (entry->message) {
        free(entry->message);
    }
    entry->message = strdup(message);

    // LMC requires original message ID, hence ID remains the same

    win_redraw(window);
    return TRUE;
}

void
win_print_incoming(ProfWin* window, const char* const display_name_from, ProfMessage* message)
{
    int flags = NO_ME;

    if (!message->trusted) {
        flags |= UNTRUSTED;
    }

    switch (window->type) {
    case WIN_CHAT:
    {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        auto_char char* enc_char = get_enc_char(message->enc, chatwin->incoming_char);

        if (!prefs_get_boolean(PREF_CORRECTION_ALLOW) || !_win_correct(window, message->plain, message->id, message->replace_id, message->from_jid->barejid)) {
            // Prevent duplicate messages when current client is sending a message or if it's mam
            if (g_strcmp0(message->from_jid->fulljid, connection_get_fulljid()) != 0 && !message->is_mam) {
                _win_printf(window, enc_char, 0, message->timestamp, flags, THEME_TEXT_THEM, display_name_from, message->from_jid->barejid, message->id, "%s", message->plain);
            }
        }
        break;
    }
    case WIN_PRIVATE:
        _win_printf(window, "-", 0, message->timestamp, flags, THEME_TEXT_THEM, display_name_from, message->from_jid->barejid, message->id, "%s", message->plain);
        break;
    default:
        assert(FALSE);
        break;
    }
}

void
win_print_them(ProfWin* window, theme_item_t theme_item, const char* const show_char, int flags, const char* const them)
{
    _win_printf(window, show_char, 0, NULL, flags | NO_ME | NO_EOL, theme_item, them, NULL, NULL, "");
}

void
win_println_incoming_muc_msg(ProfWin* window, char* show_char, int flags, const ProfMessage* const message)
{
    if (!prefs_get_boolean(PREF_CORRECTION_ALLOW) || !_win_correct(window, message->plain, message->id, message->replace_id, message->from_jid->fulljid)) {
        _win_printf(window, show_char, 0, message->timestamp, flags | NO_ME, THEME_TEXT_THEM, message->from_jid->resourcepart, message->from_jid->fulljid, message->id, "%s", message->plain);
    }

    inp_nonblocking(TRUE);
}

void
win_print_outgoing_muc_msg(ProfWin* window, char* show_char, const char* const me, const char* const id, const char* const replace_id, const char* const message)
{
    GDateTime* timestamp = g_date_time_new_now_local();

    if (!prefs_get_boolean(PREF_CORRECTION_ALLOW) || !_win_correct(window, message, id, replace_id, me)) {
        _win_printf(window, show_char, 0, timestamp, 0, THEME_TEXT_ME, me, me, id, "%s", message);
    }

    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);
}

void
win_print_outgoing(ProfWin* window, const char* show_char, const char* const id, const char* const replace_id, const char* const message)
{
    GDateTime* timestamp = g_date_time_new_now_local();

    const char* myjid = connection_get_fulljid();
    if (!_win_correct(window, message, id, replace_id, myjid)) {
        auto_gchar gchar* outgoing_str = prefs_get_string(PREF_OUTGOING_STAMP);
        _win_printf(window, show_char, 0, timestamp, 0, THEME_TEXT_ME, outgoing_str, myjid, id, "%s", message);
    }

    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);
}

void
win_print_history(ProfWin* window, const ProfMessage* const message)
{
    g_date_time_ref(message->timestamp);

    int flags = 0;
    auto_gchar gchar* display_name = get_display_name(message, &flags);
    auto_char char* ch = get_show_char(message->enc);

    buffer_append(window->layout->buffer, ch, 0, message->timestamp, flags, THEME_TEXT_HISTORY, display_name, NULL, message->plain, NULL, NULL);
    wins_add_urls_ac(window, message, FALSE);
    wins_add_quotes_ac(window, message->plain, FALSE);
    _win_print_internal(window, ch, 0, message->timestamp, flags, THEME_TEXT_HISTORY, display_name, message->plain, NULL);

    inp_nonblocking(TRUE);
    g_date_time_unref(message->timestamp);
}

void
win_print_old_history(ProfWin* window, const ProfMessage* const message)
{
    g_date_time_ref(message->timestamp);

    int flags = 0;
    auto_gchar gchar* display_name = get_display_name(message, &flags);

    auto_char char* ch = get_show_char(message->enc);

    buffer_prepend(window->layout->buffer, ch, 0, message->timestamp, flags, THEME_TEXT_HISTORY, display_name, NULL, message->plain, NULL, NULL);
    wins_add_urls_ac(window, message, TRUE);
    wins_add_quotes_ac(window, message->plain, TRUE);
    _win_print_internal(window, ch, 0, message->timestamp, flags, THEME_TEXT_HISTORY, display_name, message->plain, NULL);

    inp_nonblocking(TRUE);
    g_date_time_unref(message->timestamp);
}

static void
win_println_va_internal(ProfWin* window, theme_item_t theme_item, int pad, int flags, const char* show_char, const char* const message, va_list arg)
{
    GDateTime* timestamp = g_date_time_new_now_local();

    auto_gchar gchar* msg = g_strdup_vprintf(message, arg);

    buffer_append(window->layout->buffer, show_char, pad, timestamp, flags, theme_item, "", NULL, msg, NULL, NULL);
    _win_print_internal(window, show_char, pad, timestamp, flags, theme_item, "", msg, NULL);

    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);
}

void
win_print(ProfWin* window, theme_item_t theme_item, const char* show_char, const char* const message, ...)
{
    va_list arg;
    va_start(arg, message);
    win_println_va_internal(window, theme_item, 0, NO_EOL, show_char, message, arg);
    va_end(arg);
}

void
win_println_va(ProfWin* window, theme_item_t theme_item, const char* show_char, const char* const message, va_list arg)
{
    win_println_va_internal(window, theme_item, 0, 0, show_char, message, arg);
}

void
win_println(ProfWin* window, theme_item_t theme_item, const char* show_char, const char* const message, ...)
{
    va_list arg;
    va_start(arg, message);
    win_println_va_internal(window, theme_item, 0, 0, show_char, message, arg);
    va_end(arg);
}

void
win_println_indent(ProfWin* window, int pad, const char* const message, ...)
{
    va_list arg;
    va_start(arg, message);
    win_println_va_internal(window, THEME_DEFAULT, pad, 0, "-", message, arg);
    va_end(arg);
}

void
win_append(ProfWin* window, theme_item_t theme_item, const char* const message, ...)
{
    va_list arg;
    va_start(arg, message);
    win_println_va_internal(window, theme_item, 0, NO_DATE | NO_EOL, "-", message, arg);
    va_end(arg);
}

void
win_appendln(ProfWin* window, theme_item_t theme_item, const char* const message, ...)
{
    va_list arg;
    va_start(arg, message);
    win_println_va_internal(window, theme_item, 0, NO_DATE, "-", message, arg);
    va_end(arg);
}

void
win_append_highlight(ProfWin* window, theme_item_t theme_item, const char* const message, ...)
{
    va_list arg;
    va_start(arg, message);
    win_println_va_internal(window, theme_item, 0, NO_DATE | NO_ME | NO_EOL, "-", message, arg);
    va_end(arg);
}

void
win_appendln_highlight(ProfWin* window, theme_item_t theme_item, const char* const message, ...)
{
    va_list arg;
    va_start(arg, message);
    win_println_va_internal(window, theme_item, 0, NO_DATE | NO_ME, "-", message, arg);
    va_end(arg);
}

void
win_print_http_transfer(ProfWin* window, const char* const message, char* id)
{
    win_print_outgoing_with_receipt(window, "!", NULL, message, id, NULL);
}

void
win_print_outgoing_with_receipt(ProfWin* window, const char* show_char, const char* const from, const char* const message, char* id, const char* const replace_id)
{
    GDateTime* time = g_date_time_new_now_local();

    DeliveryReceipt* receipt = malloc(sizeof(struct delivery_receipt_t));
    receipt->received = FALSE;

    const char* myjid = connection_get_fulljid();
    if (_win_correct(window, message, id, replace_id, myjid)) {
        free(receipt); // TODO: probably we should use this in _win_correct()
    } else {
        buffer_append(window->layout->buffer, show_char, 0, time, 0, THEME_TEXT_ME, from, myjid, message, receipt, id);
        _win_print_internal(window, show_char, 0, time, 0, THEME_TEXT_ME, from, message, receipt);
    }

    // TODO: cross-reference.. this should be replaced by a real event-based system
    inp_nonblocking(TRUE);
    g_date_time_unref(time);
}

void
win_mark_received(ProfWin* window, const char* const id)
{
    if (window->type == WIN_CONSOLE)
        return;
    gboolean received = buffer_mark_received(window->layout->buffer, id);
    if (received) {
        win_redraw(window);
    }
}

void
win_update_entry_message(ProfWin* window, const char* const id, const char* const message)
{
    if (window->type == WIN_CONSOLE)
        return;
    ProfBuffEntry* entry = buffer_get_entry_by_id(window->layout->buffer, id);
    if (entry) {
        free(entry->message);
        entry->message = strdup(message);
        win_redraw(window);
    }
}

void
win_remove_entry_message(ProfWin* window, const char* const id)
{
    buffer_remove_entry_by_id(window->layout->buffer, id);
    win_redraw(window);
}

void
win_newline(ProfWin* window)
{
    win_appendln(window, THEME_DEFAULT, "");
}

static void
_win_printf(ProfWin* window, const char* show_char, int pad_indent, GDateTime* timestamp, int flags, theme_item_t theme_item, const char* const display_from, const char* const from_jid, const char* const message_id, const char* const message, ...)
{
    if (timestamp == NULL) {
        timestamp = g_date_time_new_now_local();
    } else {
        g_date_time_ref(timestamp);
    }

    va_list arg;
    va_start(arg, message);

    auto_gchar gchar* msg = g_strdup_vprintf(message, arg);

    buffer_append(window->layout->buffer, show_char, pad_indent, timestamp, flags, theme_item, display_from, from_jid, msg, NULL, message_id);
    _win_print_internal(window, show_char, pad_indent, timestamp, flags, theme_item, display_from, msg, NULL);

    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);

    va_end(arg);
}

static void
_win_print_internal(ProfWin* window, const char* show_char, int pad_indent, GDateTime* time,
                    int flags, theme_item_t theme_item, const char* const from, const char* const message, DeliveryReceipt* receipt)
{
    // flags : 1st bit =  0/1 - me/not me. define: NO_ME
    //         2nd bit =  0/1 - date/no date. define: NO_DATE
    //         3rd bit =  0/1 - eol/no eol. define: NO_EOL
    //         4th bit =  0/1 - color from/no color from. define: NO_COLOUR_FROM
    //         5th bit =  0/1 - color date/no date. define: NO_COLOUR_DATE
    //         6th bit =  0/1 - trusted/untrusted. define: UNTRUSTED
    gboolean me_message = FALSE;
    int offset = 0;
    int colour = theme_attrs(THEME_ME);
    size_t indent = 0;

    auto_gchar gchar* time_pref = NULL;
    switch (window->type) {
    case WIN_CHAT:
        time_pref = prefs_get_string(PREF_TIME_CHAT);
        break;
    case WIN_MUC:
        time_pref = prefs_get_string(PREF_TIME_MUC);
        break;
    case WIN_CONFIG:
        time_pref = prefs_get_string(PREF_TIME_CONFIG);
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

    auto_gchar gchar* date_fmt = NULL;
    if (g_strcmp0(time_pref, "off") == 0 || time == NULL) {
        date_fmt = g_strdup("");
    } else {
        date_fmt = g_date_time_format(time, time_pref);
    }
    assert(date_fmt != NULL);

    if (strlen(date_fmt) != 0) {
        indent = 3 + strlen(date_fmt);
    }

    if ((flags & NO_DATE) == 0) {
        if (date_fmt && strlen(date_fmt)) {
            if ((flags & NO_COLOUR_DATE) == 0) {
                wbkgdset(window->layout->win, theme_attrs(THEME_TIME));
                wattron(window->layout->win, theme_attrs(THEME_TIME));
            }
            wprintw(window->layout->win, "%s %s ", date_fmt, show_char);
            if ((flags & NO_COLOUR_DATE) == 0) {
                wattroff(window->layout->win, theme_attrs(THEME_TIME));
            }
        }
    }

    if (from && strlen(from) > 0) {
        if (flags & NO_ME) {
            colour = theme_attrs(THEME_THEM);
        }

        auto_gchar gchar* color_pref = prefs_get_string(PREF_COLOR_NICK);
        if (color_pref != NULL && (strcmp(color_pref, "false") != 0)) {
            if ((flags & NO_ME) || (!(flags & NO_ME) && prefs_get_boolean(PREF_COLOR_NICK_OWN))) {
                colour = theme_hash_attrs(from);
            }
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
        } else if (flags & UNTRUSTED) {
            wbkgdset(window->layout->win, theme_attrs(THEME_UNTRUSTED));
            wattron(window->layout->win, theme_attrs(THEME_UNTRUSTED));
        } else {
            wbkgdset(window->layout->win, theme_attrs(theme_item));
            wattron(window->layout->win, theme_attrs(theme_item));
        }
    }

    if (prefs_get_boolean(PREF_WRAP)) {
        _win_print_wrapped(window->layout->win, message + offset, indent, pad_indent);
    } else {
        wprintw(window->layout->win, "%s", message + offset);
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
}

static void
_win_indent(WINDOW* win, int size)
{
    for (int i = 0; i < size; i++) {
        waddch(win, ' ');
    }
}

static void
_win_print_wrapped(WINDOW* win, const char* const message, size_t indent, int pad_indent)
{
    int starty = getcury(win);
    int wordi = 0;
    auto_char char* word = malloc(strlen(message) + 1);

    gchar* curr_ch = g_utf8_offset_to_pointer(message, 0);

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
            int cury;
            int maxx = getmaxx(win);

            // wrap required
            if (curx + wordlen > maxx) {
                int linelen = maxx - (indent + pad_indent);

                // word larger than line
                if (wordlen > linelen) {
                    gchar* word_ch = g_utf8_offset_to_pointer(word, 0);
                    while (*word_ch != '\0') {
                        curx = getcurx(win);
                        cury = getcury(win);
                        gboolean firstline = cury == starty;

                        if (firstline && curx < indent) {
                            _win_indent(win, indent);
                        }
                        if (!firstline && curx < (indent + pad_indent)) {
                            _win_indent(win, indent + pad_indent);
                        }

                        gchar copy[wordi + 1];
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
}

void
win_print_trackbar(ProfWin* window)
{
    int cols = getmaxx(window->layout->win);

    wbkgdset(window->layout->win, theme_attrs(THEME_TRACKBAR));
    wattron(window->layout->win, theme_attrs(THEME_TRACKBAR));

    for (int i = 1; i <= cols; i++) {
        wprintw(window->layout->win, "-");
    }

    wattroff(window->layout->win, theme_attrs(THEME_TRACKBAR));
}

void
win_redraw(ProfWin* window)
{
    int size;
    werase(window->layout->win);
    size = buffer_size(window->layout->buffer);

    for (int i = 0; i < size; i++) {
        ProfBuffEntry* e = buffer_get_entry(window->layout->buffer, i);

        if (e->display_from == NULL && e->message && e->message[0] == '-') {
            // just an indicator to print the trackbar/separator not the actual message
            win_print_trackbar(window);
        } else {
            // regular thing to print
            _win_print_internal(window, e->show_char, e->pad_indent, e->time, e->flags, e->theme_item, e->display_from, e->message, e->receipt);
        }
    }
}

void
win_print_loading_history(ProfWin* window)
{
    GDateTime* timestamp;
    gboolean is_buffer_empty = buffer_size(window->layout->buffer) == 0;

    if (!is_buffer_empty) {
        timestamp = buffer_get_entry(window->layout->buffer, 0)->time;
    } else {
        timestamp = g_date_time_new_now_local();
    }

    buffer_prepend(window->layout->buffer, "-", 0, timestamp, NO_DATE, THEME_ROOMINFO, NULL, NULL, LOADING_MESSAGE, NULL, NULL);

    if (is_buffer_empty)
        g_date_time_unref(timestamp);

    win_redraw(window);
}

gboolean
win_has_active_subwin(ProfWin* window)
{
    if (window->layout->type == LAYOUT_SPLIT) {
        ProfLayoutSplit* layout = (ProfLayoutSplit*)window->layout;
        return (layout->subwin != NULL);
    } else {
        return FALSE;
    }
}

gboolean
win_notify_remind(ProfWin* window)
{
    switch (window->type) {
    case WIN_CHAT:
    {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);

        if (prefs_get_boolean(PREF_NOTIFY_CHAT) && chatwin->unread > 0) {
            return TRUE;
        } else {
            return FALSE;
        }
    }
    case WIN_MUC:
    {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);

        return prefs_do_room_notify_mention(mucwin->roomjid, mucwin->unread, mucwin->unread_mentions, mucwin->unread_triggers);
    }
    case WIN_PRIVATE:
    {
        ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
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
win_unread(ProfWin* window)
{
    if (window->type == WIN_CHAT) {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        return chatwin->unread;
    } else if (window->type == WIN_MUC) {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        return mucwin->unread;
    } else if (window->type == WIN_PRIVATE) {
        ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
        assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
        return privatewin->unread;
    } else {
        return 0;
    }
}

gboolean
win_has_attention(ProfWin* window)
{
    if (window->type == WIN_CHAT) {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        return chatwin->has_attention;
    } else if (window->type == WIN_MUC) {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        return mucwin->has_attention;
    }
    return FALSE;
}

gboolean
win_toggle_attention(ProfWin* window)
{
    if (window->type == WIN_CHAT) {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        chatwin->has_attention = !chatwin->has_attention;
        return chatwin->has_attention;
    } else if (window->type == WIN_MUC) {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        mucwin->has_attention = !mucwin->has_attention;
        return mucwin->has_attention;
    }
    return FALSE;
}

void
win_sub_print(WINDOW* win, char* msg, gboolean newline, gboolean wrap, int indent)
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
        wmove(win, cury + 1, 0);
    }
}

void
win_sub_newline_lazy(WINDOW* win)
{
    int curx;

    if (win == NULL) {
        return;
    }
    curx = getcurx(win);
    if (curx > 0) {
        int cury = getcury(win);
        wmove(win, cury + 1, 0);
    }
}

void
win_command_list_error(ProfWin* window, const char* const error)
{
    assert(window != NULL);

    win_println(window, THEME_ERROR, "!", "Error retrieving command list: %s", error);
}

void
win_command_exec_error(ProfWin* window, const char* const command, const char* const error, ...)
{
    assert(window != NULL);
    va_list arg;
    va_start(arg, error);
    auto_gchar gchar* msg = g_strdup_vprintf(error, arg);

    win_println(window, THEME_ERROR, "!", "Error executing command %s: %s", command, msg);

    va_end(arg);
}

void
win_handle_command_list(ProfWin* window, GSList* cmds)
{
    assert(window != NULL);

    if (cmds) {
        win_println(window, THEME_DEFAULT, "!", "Ad hoc commands:");
        GSList* curr_cmd = cmds;
        while (curr_cmd) {
            const char* cmd = curr_cmd->data;
            win_println(window, THEME_DEFAULT, "!", "  %s", cmd);
            curr_cmd = g_slist_next(curr_cmd);
        }
        win_println(window, THEME_DEFAULT, "!", "");
    } else {
        win_println(window, THEME_DEFAULT, "!", "No commands found");
        win_println(window, THEME_DEFAULT, "!", "");
    }
}

void
win_handle_command_exec_status(ProfWin* window, const char* const command, const char* const value)
{
    assert(window != NULL);
    win_println(window, THEME_DEFAULT, "!", "%s %s", command, value);
}

void
win_handle_command_exec_result_note(ProfWin* window, const char* const type, const char* const value)
{
    assert(window != NULL);
    win_println(window, THEME_DEFAULT, "!", value);
}

void
win_insert_last_read_position_marker(ProfWin* window, char* id)
{
    int size = buffer_size(window->layout->buffer);

    // TODO: this is somewhat costly. We should improve this later.
    // check if we already have a separator present
    for (int i = 0; i < size; i++) {
        ProfBuffEntry* e = buffer_get_entry(window->layout->buffer, i);

        // if yes, don't print a new one
        if (e->id && (g_strcmp0(e->id, id) == 0)) {
            return;
        }
    }

    GDateTime* time = g_date_time_new_now_local();

    // the trackbar/separator will actually be print in win_redraw().
    // this only puts it in the buffer and win_redraw() will interpret it.
    // so that we have the correct length even when resizing.
    buffer_append(window->layout->buffer, " ", 0, time, 0, THEME_TEXT, NULL, NULL, "-", NULL, id);
    win_redraw(window);

    g_date_time_unref(time);
}

char*
win_quote_autocomplete(ProfWin* window, const char* const input, gboolean previous)
{
    if (window->type != WIN_CHAT && window->type != WIN_MUC && window->type != WIN_PRIVATE) {
        return NULL;
    }

    auto_gchar gchar* result = autocomplete_complete(window->quotes_ac, input + 1, FALSE, previous);
    if (result == NULL) {
        return NULL;
    }

    auto_gcharv gchar** parts = g_strsplit(result, "\n", -1);
    auto_gchar gchar* quoted_result = g_strjoinv("\n> ", parts);

    return g_strdup_printf("> %s\n", quoted_result);
}

// Derive encryption char from encryption mode. Output needs to be freed by caller.
char*
get_show_char(prof_enc_t encryption_mode)
{
    char* enc_char;

    if (encryption_mode == PROF_MSG_ENC_OTR) {
        enc_char = prefs_get_otr_char();
    } else if (encryption_mode == PROF_MSG_ENC_PGP) {
        enc_char = prefs_get_pgp_char();
    } else if (encryption_mode == PROF_MSG_ENC_OMEMO) {
        enc_char = prefs_get_omemo_char();
    } else if (encryption_mode == PROF_MSG_ENC_OX) {
        enc_char = prefs_get_ox_char();
    } else {
        enc_char = strdup("-");
    }

    return enc_char;
}

char*
get_enc_char(prof_enc_t enc_mode, const char* alt)
{
    if (alt)
        return strdup(alt);
    else
        return get_show_char(enc_mode);
}
