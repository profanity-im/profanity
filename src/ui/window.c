/*
 * window.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"
#include "ui/window_list.h"
#ifdef HAVE_OMEMO
#include "omemo/omemo.h"
#endif

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

static const int PAD_MIN_HEIGHT = 100;
static const int PAD_THRESHOLD = 3000;
static gboolean _in_redraw = FALSE;

static void
_win_ensure_pad_capacity(ProfWin* window, WINDOW* win, int lines_needed)
{
    if (!win) {
        return;
    }

    int cur_height = getmaxy(win);
    int cur_width = getmaxx(win);
    if (lines_needed >= cur_height - 1) {
        // If we are getting too large, trigger a redraw to clean up old lines
        // but only if we are not already in a redraw process.
        if (window && cur_height >= PAD_THRESHOLD && !_in_redraw) {
            win_redraw(window);
        } else {
            // resize to required lines + some buffer for next messages
            int new_height = lines_needed + 100;
            wresize(win, new_height, cur_width);
        }
    }
}
static const char* LOADING_MESSAGE = "Loading older messages…";
static const char* END_OF_ARCHIVE_MESSAGE = "End of archive reached";
static const char* CONS_WIN_TITLE = "Profanity. Type /help for help information.";
static const char* XML_WIN_TITLE = "XML Console";

#define CEILING(X) (X - (int)(X) > 0 ? (int)(X + 1) : (int)(X))

static void
_win_printf(ProfWin* window, const gchar* show_char, int pad_indent, GDateTime* timestamp, int flags, theme_item_t theme_item, const gchar* const display_from, const gchar* const from_jid, const gchar* const message_id, const gchar* const message, ...);
static void _win_print_internal(ProfWin* window, const gchar* show_char, int pad_indent, GDateTime* time,
                                int flags, theme_item_t theme_item, const gchar* const from, const gchar* const message, DeliveryReceipt* receipt);
static void _win_print_wrapped(WINDOW* win, const gchar* const message, size_t indent, int pad_indent);

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

    ProfLayoutSimple* layout = g_new0(ProfLayoutSimple, 1);
    layout->base.type = LAYOUT_SIMPLE;
    layout->base.win = newpad(PAD_MIN_HEIGHT, cols);
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

    ProfLayoutSplit* layout = g_new0(ProfLayoutSplit, 1);
    layout->base.type = LAYOUT_SPLIT;
    layout->base.win = newpad(PAD_MIN_HEIGHT, cols);
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
    ProfConsoleWin* new_win = g_new0(ProfConsoleWin, 1);
    new_win->window.type = WIN_CONSOLE;
    new_win->window.scroll_state = WIN_SCROLL_INNER;
    new_win->window.layout = _win_create_split_layout();

    return &new_win->window;
}

ProfWin*
win_create_chat(const gchar* const barejid)
{
    ProfChatWin* new_win = g_new0(ProfChatWin, 1);
    new_win->window.type = WIN_CHAT;
    new_win->window.scroll_state = WIN_SCROLL_INNER;
    new_win->window.layout = _win_create_simple_layout();

    new_win->barejid = g_strdup(barejid);
    new_win->resource_override = NULL;
    new_win->is_otr = FALSE;
    new_win->otr_is_trusted = FALSE;
    new_win->pgp_recv = FALSE;
    new_win->pgp_send = FALSE;
    new_win->is_omemo = FALSE;
#ifdef HAVE_OMEMO
    new_win->omemo_trusted = omemo_is_jid_trusted(barejid);
#endif
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
win_create_muc(const gchar* const roomjid)
{
    ProfMucWin* new_win = g_new0(ProfMucWin, 1);
    int cols = getmaxx(stdscr);

    new_win->window.type = WIN_MUC;
    new_win->window.scroll_state = WIN_SCROLL_INNER;
    ProfLayoutSplit* layout = g_new0(ProfLayoutSplit, 1);
    layout->base.type = LAYOUT_SPLIT;

    if (prefs_get_boolean(PREF_OCCUPANTS)) {
        int subwin_cols = win_occpuants_cols();
        layout->base.win = newpad(PAD_MIN_HEIGHT, cols - subwin_cols);
        wbkgd(layout->base.win, theme_attrs(THEME_TEXT));
        layout->subwin = newpad(PAD_MIN_HEIGHT, subwin_cols);
        wbkgd(layout->subwin, theme_attrs(THEME_TEXT));
    } else {
        layout->base.win = newpad(PAD_MIN_HEIGHT, (cols));
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

    new_win->roomjid = g_strdup(roomjid);
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
#ifdef HAVE_OMEMO
    new_win->omemo_trusted = omemo_is_jid_trusted(roomjid);
#endif
    new_win->last_message = NULL;
    new_win->last_msg_id = NULL;
    new_win->has_attention = FALSE;

    new_win->memcheck = PROFMUCWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_config(const gchar* const roomjid, DataForm* form, ProfConfWinCallback submit, ProfConfWinCallback cancel, const void* userdata)
{
    ProfConfWin* new_win = g_new0(ProfConfWin, 1);
    new_win->window.type = WIN_CONFIG;
    new_win->window.scroll_state = WIN_SCROLL_INNER;
    new_win->window.layout = _win_create_simple_layout();
    new_win->roomjid = g_strdup(roomjid);
    new_win->form = form;
    new_win->submit = submit;
    new_win->cancel = cancel;
    new_win->userdata = userdata;

    new_win->memcheck = PROFCONFWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_private(const gchar* const fulljid)
{
    ProfPrivateWin* new_win = g_new0(ProfPrivateWin, 1);
    new_win->window.type = WIN_PRIVATE;
    new_win->window.scroll_state = WIN_SCROLL_INNER;
    new_win->window.layout = _win_create_simple_layout();
    new_win->fulljid = g_strdup(fulljid);
    new_win->unread = 0;
    new_win->occupant_offline = FALSE;
    new_win->room_left = FALSE;

    new_win->memcheck = PROFPRIVATEWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_xmlconsole(void)
{
    ProfXMLWin* new_win = g_new0(ProfXMLWin, 1);
    new_win->window.type = WIN_XML;
    new_win->window.scroll_state = WIN_SCROLL_INNER;
    new_win->window.layout = _win_create_simple_layout();

    new_win->memcheck = PROFXMLWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_plugin(const gchar* const plugin_name, const gchar* const tag)
{
    ProfPluginWin* new_win = g_new0(ProfPluginWin, 1);
    new_win->window.type = WIN_PLUGIN;
    new_win->window.scroll_state = WIN_SCROLL_INNER;
    new_win->window.layout = _win_create_simple_layout();

    new_win->tag = g_strdup(tag);
    new_win->plugin_name = g_strdup(plugin_name);

    new_win->memcheck = PROFPLUGINWIN_MEMCHECK;

    return &new_win->window;
}

ProfWin*
win_create_vcard(vCard* vcard)
{
    ProfVcardWin* new_win = g_new0(ProfVcardWin, 1);
    new_win->window.type = WIN_VCARD;
    new_win->window.scroll_state = WIN_SCROLL_INNER;
    new_win->window.layout = _win_create_simple_layout();

    new_win->vcard = vcard;
    new_win->memcheck = PROFVCARDWIN_MEMCHECK;

    return &new_win->window;
}

void
win_title_append(ProfWin* window, GString* target)
{
    if (target == NULL) {
        return;
    }

    if (window == NULL) {
        g_string_append(target, CONS_WIN_TITLE);
        return;
    }

    switch (window->type) {
    case WIN_CONSOLE:
    {
        g_string_append(target, CONS_WIN_TITLE);
        return;
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
            g_string_append(target, chatwin->barejid);
            return;
        }
        const char* name = p_contact_name(contact);
        if (!name) {
            g_string_append(target, chatwin->barejid);
            return;
        }
        g_string_append(target, name);
        g_string_append(target, " <");
        g_string_append(target, chatwin->barejid);
        g_string_append(target, ">");
        return;
    }
    case WIN_MUC:
    {
        const ProfMucWin* mucwin = (ProfMucWin*)window;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        mucwin_title_append(mucwin->roomjid, PREF_TITLEBAR_MUC_TITLE, target);
        return;
    }
    case WIN_CONFIG:
    {
        const ProfConfWin* confwin = (ProfConfWin*)window;
        assert(confwin->memcheck == PROFCONFWIN_MEMCHECK);
        mucwin_title_append(confwin->roomjid, PREF_TITLEBAR_MUC_TITLE, target);
        if (confwin->form->modified) {
            g_string_append(target, " config *");
        } else {
            g_string_append(target, " config");
        }
        return;
    }
    case WIN_PRIVATE:
    {
        const ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
        assert(privatewin->memcheck == PROFPRIVATEWIN_MEMCHECK);
        const gchar* slash = strchr(privatewin->fulljid, '/');
        if (slash) {
            auto_gchar gchar* bare = g_strndup(privatewin->fulljid, slash - privatewin->fulljid);
            mucwin_title_append(bare, PREF_TITLEBAR_MUC_TITLE, target);
            g_string_append(target, slash);
        } else {
            mucwin_title_append(privatewin->fulljid, PREF_TITLEBAR_MUC_TITLE, target);
        }
        return;
    }
    case WIN_XML:
    {
        g_string_append(target, XML_WIN_TITLE);
        return;
    }
    case WIN_PLUGIN:
    {
        ProfPluginWin* pluginwin = (ProfPluginWin*)window;
        assert(pluginwin->memcheck == PROFPLUGINWIN_MEMCHECK);
        g_string_append(target, pluginwin->tag);
        return;
    }
    case WIN_VCARD:
    {
        ProfVcardWin* vcardwin = (ProfVcardWin*)window;
        assert(vcardwin->memcheck == PROFVCARDWIN_MEMCHECK);

        g_string_append(target, "vCard ");
        g_string_append(target, connection_get_barejid());

        if (vcardwin->vcard->modified) {
            g_string_append(target, " *");
        }
        return;
    }
    }
    assert(FALSE);
}

gchar*
win_get_title(ProfWin* window)
{
    GString* title = g_string_new("");
    win_title_append(window, title);
    return g_string_free(title, FALSE);
}

const gchar*
win_get_tab_identifier(ProfWin* window)
{
    assert(window != NULL);

    switch (window->type) {
    case WIN_CONSOLE:
    {
        return "console";
    }
    case WIN_CHAT:
    {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        return chatwin->barejid;
    }
    case WIN_MUC:
    {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        return mucwin->roomjid;
    }
    case WIN_CONFIG:
    {
        ProfConfWin* confwin = (ProfConfWin*)window;
        return confwin->roomjid;
    }
    case WIN_PRIVATE:
    {
        ProfPrivateWin* privwin = (ProfPrivateWin*)window;
        return privwin->fulljid;
    }
    case WIN_PLUGIN:
    {
        ProfPluginWin* pluginwin = (ProfPluginWin*)window;
        return pluginwin->tag;
    }
    case WIN_XML:
    {
        return "xmlconsole";
    }
    case WIN_VCARD:
    {
        return "vcard";
    }
    }
    assert(FALSE);
    return NULL;
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
        wresize(layout->base.win, PAD_MIN_HEIGHT, cols);
        win_redraw(window);
    } else {
        int cols = getmaxx(stdscr);
        wresize(window->layout->win, PAD_MIN_HEIGHT, cols);
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
    layout->subwin = newpad(PAD_MIN_HEIGHT, subwin_cols);
    wbkgd(layout->subwin, theme_attrs(THEME_TEXT));
    wresize(layout->base.win, PAD_MIN_HEIGHT, cols - subwin_cols);
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
    g_free(window->layout);

    if (window->warned_jids) {
        g_hash_table_destroy(window->warned_jids);
    }

    switch (window->type) {
    case WIN_CHAT:
    {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        g_free(chatwin->barejid);
        g_free(chatwin->resource_override);
        g_free(chatwin->enctext);
        g_free(chatwin->incoming_char);
        g_free(chatwin->outgoing_char);
        g_free(chatwin->last_message);
        g_free(chatwin->last_msg_id);
        chat_state_free(chatwin->state);
        break;
    }
    case WIN_MUC:
    {
        ProfMucWin* mucwin = (ProfMucWin*)window;
        g_free(mucwin->roomjid);
        g_free(mucwin->room_name);
        g_free(mucwin->enctext);
        g_free(mucwin->message_char);
        g_free(mucwin->last_message);
        g_free(mucwin->last_msg_id);
        break;
    }
    case WIN_CONFIG:
    {
        ProfConfWin* conf = (ProfConfWin*)window;
        g_free(conf->roomjid);
        form_destroy(conf->form);
        break;
    }
    case WIN_PRIVATE:
    {
        ProfPrivateWin* privatewin = (ProfPrivateWin*)window;
        g_free(privatewin->fulljid);
        break;
    }
    case WIN_PLUGIN:
    {
        ProfPluginWin* pluginwin = (ProfPluginWin*)window;
        g_free(pluginwin->tag);
        g_free(pluginwin->plugin_name);
        break;
    }
    default:
        break;
    }

    g_free(window);
}

gboolean
win_warn_needed(ProfWin* window, const gchar* const type, const gchar* const jid)
{
    if (!window || !type || !jid) {
        return TRUE;
    }

    if (!window->warned_jids) {
        return TRUE;
    }

    auto_gchar gchar* key = g_strdup_printf("%s:%s", type, jid);
    return !g_hash_table_contains(window->warned_jids, key);
}

void
win_warn_sent(ProfWin* window, const gchar* const type, const gchar* const jid)
{
    if (!window || !type || !jid) {
        return;
    }

    if (!window->warned_jids) {
        window->warned_jids = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    }

    gchar* key = g_strdup_printf("%s:%s", type, jid);
    g_hash_table_insert(window->warned_jids, key, GINT_TO_POINTER(1));
}

void
win_clear_warned_jids(ProfWin* window)
{
    if (window && window->warned_jids) {
        g_hash_table_remove_all(window->warned_jids);
    }
}

void
win_page_up(ProfWin* window, int scroll_size)
{
    int* page_start = &(window->layout->y_pos);
    int page_start_initial = *page_start;
    int total_rows = getcury(window->layout->win);
    int page_space = getmaxy(stdscr) - 4;
    if (scroll_size == 0)
        scroll_size = page_space;
    win_scroll_state_t* scroll_state = &window->scroll_state;
    *scroll_state = (*scroll_state == WIN_SCROLL_REACHED_BOTTOM) ? WIN_SCROLL_INNER : *scroll_state;
    *page_start -= scroll_size;

    if (*page_start == -scroll_size && (window->type == WIN_CHAT || window->type == WIN_MUC)) {
        ProfBuffEntry* first_entry = buffer_size(window->layout->buffer) != 0 ? buffer_get_entry(window->layout->buffer, 0) : NULL;

        // Don't do anything if still fetching mam messages or if end of archive is reached
        if (first_entry && !(first_entry->theme_item == THEME_ROOMINFO && g_strcmp0(first_entry->message, LOADING_MESSAGE) == 0)
            && !(first_entry->theme_item == THEME_ROOMINFO && g_strcmp0(first_entry->message, END_OF_ARCHIVE_MESSAGE) == 0)) {
            if (*scroll_state != WIN_SCROLL_REACHED_TOP) {
                if (window->type == WIN_MUC) {
                    *scroll_state = !mucwin_db_history((ProfMucWin*)window, NULL, NULL, TRUE) ? WIN_SCROLL_REACHED_TOP : WIN_SCROLL_INNER;
                } else {
                    *scroll_state = !chatwin_db_history((ProfChatWin*)window, NULL, NULL, TRUE) ? WIN_SCROLL_REACHED_TOP : WIN_SCROLL_INNER;
                }
            }

            if (*scroll_state == WIN_SCROLL_REACHED_TOP && prefs_get_boolean(PREF_MAM)) {
                win_print_loading_history(window);
                iq_mam_request_older(window);
            }

            unsigned int buff_size = buffer_size(window->layout->buffer);
            int offset_entry_id = buff_size > 10 ? 10 : buff_size - 1;
            int offset = buffer_get_entry(window->layout->buffer, offset_entry_id)->y_end_pos;
            *page_start = offset - page_space;
        }
    }

    // went past beginning, show first page
    if (*page_start < 0) {
        *page_start = 0;
    }

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
    int total_rows = getcury(window->layout->win);
    int* page_start = &(window->layout->y_pos);
    int page_space = getmaxy(stdscr) - 4;
    int page_start_initial = *page_start;
    if (scroll_size == 0)
        scroll_size = page_space;
    win_scroll_state_t* scroll_state = &window->scroll_state;
    *scroll_state = (*scroll_state == WIN_SCROLL_REACHED_TOP) ? WIN_SCROLL_INNER : *scroll_state;

    *page_start += scroll_size;

    // Scrolled down after reaching the bottom of the page
    if ((*page_start > total_rows - page_space || (*page_start == page_space && *page_start >= total_rows)) && (window->type == WIN_CHAT || window->type == WIN_MUC)) {
        unsigned int bf_size = buffer_size(window->layout->buffer);
        if (bf_size != 0) {
            ProfBuffEntry* last_entry = buffer_get_entry(window->layout->buffer, bf_size - 1);
            auto_gchar gchar* start = g_date_time_format_iso8601(last_entry->time);
            auto_gchar gchar* end_date = prof_date_time_format_iso8601(NULL);
            if (*scroll_state != WIN_SCROLL_REACHED_BOTTOM) {
                gboolean has_items;
                if (window->type == WIN_MUC) {
                    has_items = mucwin_db_history((ProfMucWin*)window, start, end_date, FALSE);
                } else {
                    has_items = chatwin_db_history((ProfChatWin*)window, start, end_date, FALSE);
                }
                if (!has_items) {
                    *scroll_state = WIN_SCROLL_REACHED_BOTTOM;
                }
            }

            int offset = last_entry->y_end_pos - 1;
            *page_start = offset - page_space + scroll_size;
        }
    }

    total_rows = getcury(window->layout->win);

    // near the end, but will display only part of the page space, move a bit back to show full page
    if ((total_rows - *page_start) < page_space) {
        *page_start = total_rows - page_space;
        // went past end, show last page
    } else if (*page_start >= total_rows) {
        *page_start = total_rows - page_space - 1;
    }

    window->layout->paged = 1;

    // update only if position has changed
    if (page_start_initial != *page_start) {
        win_update_virtual(window);
    }

    // switch off page if last line and space line visible
    if (total_rows - *page_start == page_space) {
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
            wresize(layout->base.win, PAD_MIN_HEIGHT, cols - subwin_cols);
            wbkgd(layout->subwin, theme_attrs(THEME_TEXT));
            wresize(layout->subwin, PAD_MIN_HEIGHT, subwin_cols);
            if (window->type == WIN_CONSOLE) {
                rosterwin_roster();
            } else if (window->type == WIN_MUC) {
                ProfMucWin* mucwin = (ProfMucWin*)window;
                assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
                occupantswin_occupants(mucwin->roomjid);
            }
        } else {
            wbkgd(layout->base.win, theme_attrs(THEME_TEXT));
            wresize(layout->base.win, PAD_MIN_HEIGHT, cols);
        }
    } else {
        wbkgd(window->layout->win, theme_attrs(THEME_TEXT));
        wresize(window->layout->win, PAD_MIN_HEIGHT, cols);
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
win_show_occupant_info(ProfWin* window, const gchar* const room, Occupant* occupant)
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
    if (!vcard) {
        return;
    }

    if (vcard->fullname && *vcard->fullname) {
        win_println(window, THEME_DEFAULT, "-", "  Full name : %s", vcard->fullname);
    }
    if (vcard->name.family || vcard->name.given || vcard->name.middle || vcard->name.prefix || vcard->name.suffix) {
        if (vcard->name.given && vcard->name.family && (!vcard->fullname || strcmp(vcard->fullname, vcard->name.given) == 0)) {
            win_println(window, THEME_DEFAULT, "-", "  Name      : %s %s", vcard->name.given, vcard->name.family);
        } else {
            if (vcard->name.given && *vcard->name.given) {
                win_println(window, THEME_DEFAULT, "-", "  Given     : %s", vcard->name.given);
            }
            if (vcard->name.family && *vcard->name.family) {
                win_println(window, THEME_DEFAULT, "-", "  Family    : %s", vcard->name.family);
            }
            if (vcard->name.middle && *vcard->name.middle) {
                win_println(window, THEME_DEFAULT, "-", "  Middle    : %s", vcard->name.middle);
            }
            if (vcard->name.prefix && *vcard->name.prefix) {
                win_println(window, THEME_DEFAULT, "-", "  Prefix    : %s", vcard->name.prefix);
            }
            if (vcard->name.suffix && *vcard->name.suffix) {
                win_println(window, THEME_DEFAULT, "-", "  Suffix    : %s", vcard->name.suffix);
            }
        }
    }

    if (!vcard->elements) {
        return;
    }

    for (GList* pointer = g_queue_peek_head_link(vcard->elements); pointer != NULL; pointer = pointer->next) {
        assert(pointer->data != NULL);
        vcard_element_t* element = (vcard_element_t*)pointer->data;

        switch (element->type) {
        case VCARD_NICKNAME:
            if (element->nickname && *element->nickname) {
                win_println(window, THEME_DEFAULT, "-", "  Nickname  : %s", element->nickname);
            }
            break;

        case VCARD_EMAIL:
            if (element->email.userid && *element->email.userid) {
                GString* opts = g_string_new("");
                if (element->email.options & VCARD_PREF)
                    g_string_append(opts, "preferred, ");
                if (element->email.options & VCARD_HOME)
                    g_string_append(opts, "home, ");
                if (element->email.options & VCARD_WORK)
                    g_string_append(opts, "work, ");
                if (element->email.options & VCARD_EMAIL_INTERNET)
                    g_string_append(opts, "internet, ");
                if (opts->len > 2)
                    g_string_truncate(opts, opts->len - 2);

                if (opts->len > 0) {
                    win_println(window, THEME_DEFAULT, "-", "  Email     : %s (%s)", element->email.userid, opts->str);
                } else {
                    win_println(window, THEME_DEFAULT, "-", "  Email     : %s", element->email.userid);
                }
                g_string_free(opts, TRUE);
            }
            break;

        case VCARD_URL:
            if (element->url && *element->url) {
                win_println(window, THEME_DEFAULT, "-", "  Website   : %s", element->url);
            }
            break;

        case VCARD_TITLE:
            if (element->title && *element->title) {
                win_println(window, THEME_DEFAULT, "-", "  Title     : %s", element->title);
            }
            break;

        case VCARD_ROLE:
            if (element->role && *element->role) {
                win_println(window, THEME_DEFAULT, "-", "  Role      : %s", element->role);
            }
            break;

        case VCARD_BIRTHDAY:
            if (element->birthday) {
                auto_gchar gchar* date_format = prefs_get_string(PREF_TIME_VCARD);
                auto_gchar gchar* date = g_date_time_format(element->birthday, date_format);
                if (date) {
                    win_println(window, THEME_DEFAULT, "-", "  Birthday  : %s", date);
                }
            }
            break;

        case VCARD_NOTE:
            if (element->note && *element->note) {
                win_println(window, THEME_DEFAULT, "-", "  Note      : %s", element->note);
            }
            break;

        case VCARD_JID:
            if (element->jid && *element->jid) {
                win_println(window, THEME_DEFAULT, "-", "  JID       : %s", element->jid);
            }
            break;

        case VCARD_TELEPHONE:
            if (element->telephone.number && *element->telephone.number) {
                GString* topts = g_string_new("");
                if (element->telephone.options & VCARD_PREF)
                    g_string_append(topts, "preferred, ");
                if (element->telephone.options & VCARD_TEL_CELL)
                    g_string_append(topts, "mobile, ");
                if (element->telephone.options & VCARD_HOME)
                    g_string_append(topts, "home, ");
                if (element->telephone.options & VCARD_WORK)
                    g_string_append(topts, "work, ");
                if (element->telephone.options & VCARD_TEL_VOICE)
                    g_string_append(topts, "voice, ");
                if (element->telephone.options & VCARD_TEL_FAX)
                    g_string_append(topts, "fax, ");
                if (topts->len > 2)
                    g_string_truncate(topts, topts->len - 2);

                if (topts->len > 0) {
                    win_println(window, THEME_DEFAULT, "-", "  Phone     : %s (%s)", element->telephone.number, topts->str);
                } else {
                    win_println(window, THEME_DEFAULT, "-", "  Phone     : %s", element->telephone.number);
                }
                g_string_free(topts, TRUE);
            }
            break;

        case VCARD_PHOTO:
            if (element->photo.external) {
                win_println(window, THEME_DEFAULT, "-", "  Photo     : %s (external)", element->photo.extval);
            } else if (element->photo.length > 0) {
                win_println(window, THEME_DEFAULT, "-", "  Photo     : %s (%.1f KB)",
                            element->photo.type ? element->photo.type : "image",
                            (double)element->photo.length / 1024.0);
            }
            break;

        case VCARD_ADDRESS:
            win_println(window, THEME_DEFAULT, "-", "  Address   :");
            if (element->address.street && *element->address.street) {
                win_println(window, THEME_DEFAULT, "-", "    Street   : %s", element->address.street);
            }
            if (element->address.extaddr && *element->address.extaddr) {
                win_println(window, THEME_DEFAULT, "-", "    Extended : %s", element->address.extaddr);
            }
            if (element->address.pobox && *element->address.pobox) {
                win_println(window, THEME_DEFAULT, "-", "    P.O. Box : %s", element->address.pobox);
            }
            if (element->address.locality && *element->address.locality) {
                win_println(window, THEME_DEFAULT, "-", "    City     : %s", element->address.locality);
            }
            if (element->address.region && *element->address.region) {
                win_println(window, THEME_DEFAULT, "-", "    Region   : %s", element->address.region);
            }
            if (element->address.pcode && *element->address.pcode) {
                win_println(window, THEME_DEFAULT, "-", "    Postcode : %s", element->address.pcode);
            }
            if (element->address.country && *element->address.country) {
                win_println(window, THEME_DEFAULT, "-", "    Country  : %s", element->address.country);
            }
            break;

        default:
            break;
        }
    }
}

void
win_show_status_string(ProfWin* window, const gchar* const from,
                       const gchar* const show, const gchar* const status,
                       GDateTime* last_activity, const gchar* const pre,
                       const gchar* const default_show)
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
_win_correct(ProfWin* window, const gchar* const message, const gchar* const id, const gchar* const replace_id, const gchar* const from_jid)
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
        g_free(entry->message);
    }
    entry->message = g_strdup(message);

    // LMC requires original message ID, hence ID remains the same

    win_redraw(window);
    return TRUE;
}

void
win_print_incoming(ProfWin* window, const gchar* const display_name_from, ProfMessage* message)
{
    int flags = NO_ME;

    if (!message->trusted) {
        flags |= UNTRUSTED;
    }

    switch (window->type) {
    case WIN_CHAT:
    {
        ProfChatWin* chatwin = (ProfChatWin*)window;
        const gchar* enc_char = get_enc_char(message->enc, chatwin->incoming_char);

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
win_print_them(ProfWin* window, theme_item_t theme_item, GDateTime* timestamp, const gchar* const show_char, int flags, const gchar* const them)
{
    _win_printf(window, show_char, 0, timestamp, flags | NO_ME | NO_EOL, theme_item, them, NULL, NULL, "");
}

void
win_println_incoming_muc_msg(ProfWin* window, const gchar* show_char, int flags, const ProfMessage* const message)
{
    if (!prefs_get_boolean(PREF_CORRECTION_ALLOW) || !_win_correct(window, message->plain, message->id, message->replace_id, message->from_jid->fulljid)) {
        _win_printf(window, show_char, 0, message->timestamp, flags | NO_ME, THEME_TEXT_THEM, message->from_jid->resourcepart, message->from_jid->fulljid, message->id, "%s", message->plain);
    }

    inp_nonblocking(TRUE);
}

void
win_print_outgoing_muc_msg(ProfWin* window, const gchar* show_char, const gchar* const me, const gchar* const id, const gchar* const replace_id, const gchar* const message)
{
    GDateTime* timestamp = g_date_time_new_now_local();

    if (!prefs_get_boolean(PREF_CORRECTION_ALLOW) || !_win_correct(window, message, id, replace_id, me)) {
        _win_printf(window, show_char, 0, timestamp, 0, THEME_TEXT_ME, me, me, id, "%s", message);
    }

    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);
}

void
win_print_outgoing(ProfWin* window, const gchar* show_char, const gchar* const id, const gchar* const replace_id, const gchar* const message)
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
    const gchar* ch = get_show_char(message->enc);

    wins_add_urls_ac(window, message, FALSE);
    wins_add_quotes_ac(window, message->plain, FALSE);
    int y_start_pos = getcury(window->layout->win);
    _win_print_internal(window, ch, 0, message->timestamp, flags, THEME_TEXT_HISTORY, display_name, message->plain, NULL);
    buffer_append(window->layout->buffer, ch, 0, message->timestamp, flags, THEME_TEXT_HISTORY, display_name, message->from_jid->barejid, message->plain, NULL, message->id, y_start_pos, getcury(window->layout->win));

    inp_nonblocking(TRUE);
    g_date_time_unref(message->timestamp);
}

void
win_print_old_history(ProfWin* window, const ProfMessage* const message)
{
    g_date_time_ref(message->timestamp);

    int flags = 0;
    auto_gchar gchar* display_name = get_display_name(message, &flags);

    const gchar* ch = get_show_char(message->enc);

    int y_start_pos = getcury(window->layout->win);
    wins_add_urls_ac(window, message, TRUE);
    wins_add_quotes_ac(window, message->plain, TRUE);
    _win_print_internal(window, ch, 0, message->timestamp, flags, THEME_TEXT_HISTORY, display_name, message->plain, NULL);
    buffer_prepend(window->layout->buffer, ch, 0, message->timestamp, flags, THEME_TEXT_HISTORY, display_name, message->from_jid->barejid, message->plain, NULL, message->id, y_start_pos, getcury(window->layout->win));

    inp_nonblocking(TRUE);
    g_date_time_unref(message->timestamp);
}

static void
win_println_va_internal_with_timestamp(ProfWin* window, theme_item_t theme_item, int pad, int flags, const gchar* show_char, GDateTime* timestamp, const gchar* const message, va_list arg)
{
    gboolean created_timestamp = FALSE;
    if (!timestamp) {
        timestamp = g_date_time_new_now_local();
        created_timestamp = TRUE;
    }

    auto_gchar gchar* msg = g_strdup_vprintf(message, arg);

    int y_start_pos = getcury(window->layout->win);
    _win_print_internal(window, show_char, pad, timestamp, flags, theme_item, "", msg, NULL);
    buffer_append(window->layout->buffer, show_char, pad, timestamp, flags, theme_item, "", NULL, msg, NULL, NULL, y_start_pos, getcury(window->layout->win));

    inp_nonblocking(TRUE);
    if (created_timestamp) {
        g_date_time_unref(timestamp);
    }
}

static void
win_println_va_internal(ProfWin* window, theme_item_t theme_item, int pad, int flags, const gchar* show_char, const gchar* const message, va_list arg)
{
    win_println_va_internal_with_timestamp(window, theme_item, pad, flags, show_char, NULL, message, arg);
}

void
win_print(ProfWin* window, theme_item_t theme_item, const gchar* show_char, const gchar* const message, ...)
{
    va_list arg;
    va_start(arg, message);
    win_println_va_internal(window, theme_item, 0, NO_EOL, show_char, message, arg);
    va_end(arg);
}

void
win_println_va(ProfWin* window, theme_item_t theme_item, const gchar* show_char, const gchar* const message, va_list arg)
{
    win_println_va_internal(window, theme_item, 0, 0, show_char, message, arg);
}

void
win_println(ProfWin* window, theme_item_t theme_item, const gchar* show_char, const gchar* const message, ...)
{
    va_list arg;
    va_start(arg, message);
    win_println_va_internal(window, theme_item, 0, 0, show_char, message, arg);
    va_end(arg);
}

void
win_println_indent(ProfWin* window, int pad, const gchar* const message, ...)
{
    va_list arg;
    va_start(arg, message);
    win_println_va_internal(window, THEME_DEFAULT, pad, 0, "-", message, arg);
    va_end(arg);
}

void
win_append(ProfWin* window, theme_item_t theme_item, const gchar* const message, ...)
{
    va_list arg;
    va_start(arg, message);
    win_println_va_internal(window, theme_item, 0, NO_DATE | NO_EOL, "-", message, arg);
    va_end(arg);
}

void
win_appendln(ProfWin* window, theme_item_t theme_item, const gchar* const message, ...)
{
    va_list arg;
    va_start(arg, message);
    win_println_va_internal(window, theme_item, 0, NO_DATE, "-", message, arg);
    va_end(arg);
}

void
win_append_highlight(ProfWin* window, theme_item_t theme_item, GDateTime* timestamp, const gchar* const message, ...)
{
    va_list arg;
    va_start(arg, message);
    win_println_va_internal_with_timestamp(window, theme_item, 0, NO_DATE | NO_ME | NO_EOL, "-", timestamp, message, arg);
    va_end(arg);
}

void
win_appendln_highlight(ProfWin* window, theme_item_t theme_item, GDateTime* timestamp, const gchar* const message, ...)
{
    va_list arg;
    va_start(arg, message);
    win_println_va_internal_with_timestamp(window, theme_item, 0, NO_DATE | NO_ME, "-", timestamp, message, arg);
    va_end(arg);
}

void
win_print_http_transfer(ProfWin* window, const gchar* const message, const gchar* const id)
{
    win_print_status_with_id(window, message, id, THEME_DEFAULT, 0);
}

void
win_print_outgoing_with_receipt(ProfWin* window, const gchar* show_char, const gchar* const from, const gchar* const message, const gchar* const id, const gchar* const replace_id)
{
    GDateTime* time = g_date_time_new_now_local();

    DeliveryReceipt* receipt = g_new0(struct delivery_receipt_t, 1);
    receipt->received = FALSE;

    const char* myjid = connection_get_fulljid();
    if (_win_correct(window, message, id, replace_id, myjid)) {
        g_free(receipt); // TODO: probably we should use this in _win_correct()
    } else {
        int y_start_pos = getcury(window->layout->win);
        _win_print_internal(window, show_char, 0, time, 0, THEME_TEXT_ME, from, message, receipt);
        buffer_append(window->layout->buffer, show_char, 0, time, 0, THEME_TEXT_ME, from, myjid, message, receipt, id, y_start_pos, getcury(window->layout->win));
    }

    // TODO: cross-reference.. this should be replaced by a real event-based system
    inp_nonblocking(TRUE);
    g_date_time_unref(time);
}

void
win_mark_received(ProfWin* window, const gchar* const id)
{
    if (window->type == WIN_CONSOLE)
        return;
    gboolean received = buffer_mark_received(window->layout->buffer, id);
    if (received) {
        win_redraw(window);
    }
}

void
win_update_entry_message(ProfWin* window, const gchar* const id, const gchar* const message)
{
    if (window->type == WIN_CONSOLE)
        return;
    ProfBuffEntry* entry = buffer_get_entry_by_id(window->layout->buffer, id);
    if (entry) {
        g_free(entry->message);
        entry->message = g_strdup(message);
        win_redraw(window);
    }
}

void
win_update_entry(ProfWin* window, const gchar* const id, const gchar* const message, theme_item_t theme_item, int flags)
{
    if (window->type == WIN_CONSOLE)
        return;
    ProfBuffEntry* entry = buffer_get_entry_by_id(window->layout->buffer, id);
    if (entry) {
        if (message) {
            g_free(entry->message);
            entry->message = g_strdup(message);
        }
        entry->theme_item = theme_item;
        entry->flags |= flags;
        win_redraw(window);
    }
}

void
win_print_status_with_id(ProfWin* window, const gchar* const message, const gchar* const id, theme_item_t theme_item, int flags)
{
    GDateTime* time = g_date_time_new_now_local();
    int y_start_pos = getcury(window->layout->win);
    _win_print_internal(window, "!", 0, time, flags, theme_item, NULL, message, NULL);
    buffer_append(window->layout->buffer, "!", 0, time, flags, theme_item, NULL, NULL, message, NULL, id, y_start_pos, getcury(window->layout->win));
    g_date_time_unref(time);
}

void
win_remove_entry_message(ProfWin* window, const gchar* const id)
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
_win_printf(ProfWin* window, const gchar* show_char, int pad_indent, GDateTime* timestamp, int flags, theme_item_t theme_item, const gchar* const display_from, const gchar* const from_jid, const gchar* const message_id, const gchar* const message, ...)
{
    if (timestamp == NULL) {
        timestamp = g_date_time_new_now_local();
    } else {
        g_date_time_ref(timestamp);
    }

    va_list arg;
    va_start(arg, message);

    auto_gchar gchar* msg = g_strdup_vprintf(message, arg);

    int y_start_pos = getcury(window->layout->win);
    _win_print_internal(window, show_char, pad_indent, timestamp, flags, theme_item, display_from, msg, NULL);
    buffer_append(window->layout->buffer, show_char, pad_indent, timestamp, flags, theme_item, display_from, from_jid, msg, NULL, message_id, y_start_pos, getcury(window->layout->win));

    inp_nonblocking(TRUE);
    g_date_time_unref(timestamp);

    va_end(arg);
}

static void
_win_print_internal(ProfWin* window, const gchar* show_char, int pad_indent, GDateTime* time,
                    int flags, theme_item_t theme_item, const gchar* const from, const gchar* const message, DeliveryReceipt* receipt)
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

    _win_ensure_pad_capacity(window, window->layout->win, getcury(window->layout->win));

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
_win_print_wrapped(WINDOW* win, const gchar* const message, size_t indent, int pad_indent)
{
    int starty = getcury(win);
    int wordi = 0;
    auto_gchar gchar* word = g_malloc(strlen(message) + 1);

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
                size_t offset = 0;
                while (offset < ch_len) {
                    word[wordi++] = curr_ch[offset++];
                }
                curr_ch = g_utf8_next_char(curr_ch);
            }
            word[wordi] = '\0';
            wordlen = utf8_display_len(word);

            size_t curx = (size_t)getcurx(win);
            int cury;
            size_t maxx = getmaxx(win);

            // wrap required
            if (curx + wordlen > maxx) {
                int linelen = maxx - (indent + pad_indent);

                // word larger than line
                if (wordlen > linelen) {
                    gchar* word_ch = g_utf8_offset_to_pointer(word, 0);
                    while (*word_ch != '\0') {
                        curx = (size_t)getcurx(win);
                        cury = getcury(win);
                        gboolean firstline = cury == starty;

                        if (firstline && curx < indent) {
                            _win_indent(win, indent);
                        }
                        if (!firstline && curx < (indent + pad_indent)) {
                            _win_indent(win, indent + pad_indent);
                        }

                        gchar* next = g_utf8_next_char(word_ch);
                        gsize bytes_to_copy = next - word_ch;
                        gchar copy[bytes_to_copy + 1];
                        memcpy(copy, word_ch, bytes_to_copy);
                        copy[bytes_to_copy] = '\0';
                        waddstr(win, copy);

                        word_ch = g_utf8_next_char(word_ch);
                    }

                    // newline and print word
                } else {
                    waddch(win, '\n');
                    curx = (size_t)getcurx(win);
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
                curx = (size_t)getcurx(win);
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
    _win_ensure_pad_capacity(window, window->layout->win, getcury(window->layout->win));
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
    unsigned int size = buffer_size(window->layout->buffer);
    _in_redraw = TRUE;

    // shrink pad back to minimum size and erase it
    int cols = getmaxx(window->layout->win);
    wresize(window->layout->win, PAD_MIN_HEIGHT, cols);
    werase(window->layout->win);

    for (unsigned int i = 0; i < size; i++) {
        ProfBuffEntry* e = buffer_get_entry(window->layout->buffer, i);

        // check if we need more space before printing
        _win_ensure_pad_capacity(window, window->layout->win, getcury(window->layout->win));

        e->y_start_pos = getcury(window->layout->win);
        if (e->display_from == NULL && e->message && e->message[0] == '-') {
            // just an indicator to print the trackbar/separator not the actual message
            win_print_trackbar(window);
        } else {
            // regular thing to print
            _win_print_internal(window, e->show_char, e->pad_indent, e->time, e->flags, e->theme_item, e->display_from, e->message, e->receipt);
        }
        e->y_end_pos = getcury(window->layout->win);
    }

    _in_redraw = FALSE;
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

    int cur_y = getcury(window->layout->win);
    buffer_prepend(window->layout->buffer, "-", 0, timestamp, NO_DATE, THEME_ROOMINFO, NULL, NULL, LOADING_MESSAGE, NULL, NULL, cur_y, cur_y + 1);

    if (is_buffer_empty)
        g_date_time_unref(timestamp);

    win_redraw(window);
}

void
win_remove_loading_history(ProfWin* window)
{
    if (!window || !window->layout || !window->layout->buffer) {
        return;
    }

    gboolean changed = FALSE;
    unsigned int size = buffer_size(window->layout->buffer);
    for (int i = (int)size - 1; i >= 0; i--) {
        ProfBuffEntry* entry = buffer_get_entry(window->layout->buffer, i);
        if (entry && entry->theme_item == THEME_ROOMINFO && g_strcmp0(entry->message, LOADING_MESSAGE) == 0) {
            buffer_remove_entry(window->layout->buffer, i);
            changed = TRUE;
        }
    }

    if (changed) {
        win_redraw(window);
    }
}

void
win_print_end_of_archive(ProfWin* window)
{
    GDateTime* timestamp;
    gboolean is_buffer_empty = buffer_size(window->layout->buffer) == 0;

    // Reset scroll position and exit paged state to make the banner visible!
    window->layout->y_pos = 0;
    window->layout->paged = 0;

    if (!is_buffer_empty) {
        timestamp = buffer_get_entry(window->layout->buffer, 0)->time;
    } else {
        timestamp = g_date_time_new_now_local();
    }

    if (!is_buffer_empty) {
        ProfBuffEntry* first_entry = buffer_get_entry(window->layout->buffer, 0);
        if (first_entry->theme_item == THEME_ROOMINFO && g_strcmp0(first_entry->message, END_OF_ARCHIVE_MESSAGE) == 0) {
            if (is_buffer_empty)
                g_date_time_unref(timestamp);
            win_redraw(window);
            return;
        }
    }

    int cur_y = getcury(window->layout->win);
    buffer_prepend(window->layout->buffer, "-", 0, timestamp, NO_DATE, THEME_ROOMINFO, NULL, NULL, END_OF_ARCHIVE_MESSAGE, NULL, NULL, cur_y, cur_y + 1);

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
    _win_ensure_pad_capacity(NULL, win, getcury(win));
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
win_command_list_error(ProfWin* window, const gchar* const error)
{
    assert(window != NULL);

    win_println(window, THEME_ERROR, "!", "Error retrieving command list: %s", error);
}

void
win_command_exec_error(ProfWin* window, const gchar* const command, const gchar* const error, ...)
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
win_handle_command_exec_status(ProfWin* window, const gchar* const command, const gchar* const value)
{
    assert(window != NULL);
    win_println(window, THEME_DEFAULT, "!", "%s %s", command, value);
}

void
win_handle_command_exec_result_note(ProfWin* window, const gchar* const type, const gchar* const value)
{
    assert(window != NULL);
    win_println(window, THEME_DEFAULT, "!", value);
}

void
win_insert_last_read_position_marker(ProfWin* window, const gchar* const id)
{
    unsigned int size = buffer_size(window->layout->buffer);

    // TODO: this is somewhat costly. We should improve this later.
    // check if we already have a separator present
    for (unsigned int i = 0; i < size; i++) {
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
    int y_start_pos = getcury(window->layout->win);
    buffer_append(window->layout->buffer, " ", 0, time, 0, THEME_TEXT, NULL, NULL, "-", NULL, id, y_start_pos, getcury(window->layout->win));
    win_redraw(window);

    g_date_time_unref(time);
}

gchar*
win_quote_autocomplete(ProfWin* window, const gchar* const input, gboolean previous)
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

// Derive encryption char from encryption mode.
const gchar*
get_show_char(prof_enc_t encryption_mode)
{
    if (encryption_mode == PROF_MSG_ENC_OTR) {
        return prefs_get_otr_char();
    } else if (encryption_mode == PROF_MSG_ENC_PGP) {
        return prefs_get_pgp_char();
    } else if (encryption_mode == PROF_MSG_ENC_OMEMO) {
        return prefs_get_omemo_char();
    } else if (encryption_mode == PROF_MSG_ENC_OX) {
        return prefs_get_ox_char();
    } else {
        return "-";
    }
}

const gchar*
get_enc_char(prof_enc_t enc_mode, const gchar* alt)
{
    if (alt) {
        return alt;
    } else {
        return get_show_char(enc_mode);
    }
}
