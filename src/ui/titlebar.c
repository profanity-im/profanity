/*
 * titlebar.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "config.h"

#include "common.h"
#include "config/theme.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/titlebar.h"
#include "ui/inputwin.h"
#include "ui/window_list.h"
#include "ui/window.h"
#include "ui/screen.h"
#ifdef HAVE_OMEMO
#include "omemo/omemo.h"
#endif
#include "xmpp/roster_list.h"
#include "xmpp/chat_session.h"

static WINDOW* win;
static contact_presence_t current_presence;
static gboolean tls_secured;
static gboolean is_connected;

static gboolean typing;
static GTimer* typing_elapsed;

static void _title_bar_draw(void);
static void _show_self_presence(void);
static int _calc_self_presence(void);
static void _show_contact_presence(ProfChatWin* chatwin, int pos, int maxpos);
static void _show_privacy(ProfChatWin* chatwin);
static void _show_muc_privacy(ProfMucWin* mucwin);
static void _show_scrolled(ProfWin* current);
static void _show_attention(ProfWin* current, gboolean attention);

void
create_title_bar(void)
{
    int cols = getmaxx(stdscr);

    int row = screen_titlebar_row();
    win = newwin(1, cols, row, 0);
    wbkgd(win, theme_attrs(THEME_TITLE_TEXT));
    title_bar_console();
    title_bar_set_presence(CONTACT_OFFLINE);
    title_bar_set_tls(FALSE);
    title_bar_set_connected(FALSE);
    wnoutrefresh(win);
    inp_put_back();
}

void
free_title_bar(void)
{
    delwin(win);
    win = NULL;
}

void
title_bar_update_virtual(void)
{
    ProfWin* window = wins_get_current();
    if (window->type != WIN_CONSOLE) {
        if (typing_elapsed) {
            gdouble seconds = g_timer_elapsed(typing_elapsed, NULL);

            if (seconds >= 10) {
                typing = FALSE;

                g_timer_destroy(typing_elapsed);
                typing_elapsed = NULL;
            }
        }
    }
    _title_bar_draw();
}

void
title_bar_resize(void)
{
    int cols = getmaxx(stdscr);

    werase(win);

    int row = screen_titlebar_row();

    wresize(win, 1, cols);
    mvwin(win, row, 0);

    wbkgd(win, theme_attrs(THEME_TITLE_TEXT));

    _title_bar_draw();
}

void
title_bar_console(void)
{
    werase(win);
    if (typing_elapsed) {
        g_timer_destroy(typing_elapsed);
    }
    typing_elapsed = NULL;
    typing = FALSE;

    _title_bar_draw();
}

void
title_bar_set_presence(contact_presence_t presence)
{
    current_presence = presence;
    _title_bar_draw();
}

void
title_bar_set_connected(gboolean connected)
{
    is_connected = connected;
    _title_bar_draw();
}

void
title_bar_set_tls(gboolean secured)
{
    tls_secured = secured;
    _title_bar_draw();
}

void
title_bar_switch(void)
{
    if (typing_elapsed) {
        g_timer_destroy(typing_elapsed);
        typing_elapsed = NULL;
        typing = FALSE;
    }

    _title_bar_draw();
}

void
title_bar_set_typing(gboolean is_typing)
{
    if (is_typing) {
        if (typing_elapsed) {
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
    int pos;
    int maxrightpos;
    ProfWin* current = wins_get_current();

    werase(win);
    wmove(win, 0, 0);
    for (int i = 0; i < 45; i++) {
        waddch(win, ' ');
    }

    auto_gchar gchar* title = win_get_title(current);

    mvwprintw(win, 0, 0, " %s", title);
    pos = strlen(title) + 1;

    // presence is written from the right side
    // calculate it first so we have a maxposition
    maxrightpos = _calc_self_presence();

    if (current && current->type == WIN_CHAT) {
        ProfChatWin* chatwin = (ProfChatWin*)current;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        _show_contact_presence(chatwin, pos, maxrightpos);
        _show_privacy(chatwin);
        _show_attention(current, chatwin->has_attention);
        _show_scrolled(current);

        if (typing) {
            wprintw(win, " (typing...)");
        }
    } else if (current && current->type == WIN_MUC) {
        ProfMucWin* mucwin = (ProfMucWin*)current;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        _show_muc_privacy(mucwin);
        _show_attention(current, mucwin->has_attention);
        _show_scrolled(current);
    }

    _show_self_presence();

    wnoutrefresh(win);
    inp_put_back();
}

static void
_show_attention(ProfWin* current, gboolean attention)
{
    int bracket_attrs = theme_attrs(THEME_TITLE_BRACKET);
    int text_attrs = theme_attrs(THEME_TITLE_TEXT);

    if (attention) {
        wprintw(win, " ");
        wattron(win, bracket_attrs);
        wprintw(win, "[");
        wattroff(win, bracket_attrs);
        wattron(win, text_attrs);
        wprintw(win, "ATTENTION");
        wattroff(win, text_attrs);
        wattron(win, bracket_attrs);
        wprintw(win, "]");
        wattroff(win, bracket_attrs);
    }
}

static void
_show_scrolled(ProfWin* current)
{
    if (current && current->layout->paged == 1) {
        int bracket_attrs = theme_attrs(THEME_TITLE_BRACKET);
        int scrolled_attrs = theme_attrs(THEME_TITLE_SCROLLED);

        wattron(win, bracket_attrs);
        wprintw(win, "[");
        wattroff(win, bracket_attrs);

        wattron(win, scrolled_attrs);
        wprintw(win, "SCROLLED");
        wattroff(win, scrolled_attrs);

        wattron(win, bracket_attrs);
        wprintw(win, "]");
        wattroff(win, bracket_attrs);
    }
}

static int
_calc_self_presence(void)
{
    int tls_start = 0;

    switch (current_presence) {
    case CONTACT_ONLINE:
        tls_start = 15;
        break;
    case CONTACT_AWAY:
        tls_start = 13;
        break;
    case CONTACT_DND:
        tls_start = 12;
        break;
    case CONTACT_CHAT:
        tls_start = 13;
        break;
    case CONTACT_XA:
        tls_start = 11;
        break;
    case CONTACT_OFFLINE:
        tls_start = 16;
        break;
    }

    return tls_start - 1;
}

static void
_show_self_presence(void)
{
    int presence_attrs = 0;
    int bracket_attrs = theme_attrs(THEME_TITLE_BRACKET);
    int encrypted_attrs = theme_attrs(THEME_TITLE_ENCRYPTED);
    int unencrypted_attrs = theme_attrs(THEME_TITLE_UNENCRYPTED);
    int cols = getmaxx(stdscr);

    int tls_start = 0;

    switch (current_presence) {
    case CONTACT_ONLINE:
        presence_attrs = theme_attrs(THEME_TITLE_ONLINE);
        wattron(win, bracket_attrs);
        mvwaddch(win, 0, cols - 9, '[');
        wattroff(win, bracket_attrs);
        wattron(win, presence_attrs);
        mvwprintw(win, 0, cols - 8, "online");
        wattroff(win, presence_attrs);
        tls_start = 15;
        break;
    case CONTACT_AWAY:
        presence_attrs = theme_attrs(THEME_TITLE_AWAY);
        wattron(win, bracket_attrs);
        mvwaddch(win, 0, cols - 7, '[');
        wattroff(win, bracket_attrs);
        wattron(win, presence_attrs);
        mvwprintw(win, 0, cols - 6, "away");
        wattroff(win, presence_attrs);
        tls_start = 13;
        break;
    case CONTACT_DND:
        presence_attrs = theme_attrs(THEME_TITLE_DND);
        wattron(win, bracket_attrs);
        mvwaddch(win, 0, cols - 6, '[');
        wattroff(win, bracket_attrs);
        wattron(win, presence_attrs);
        mvwprintw(win, 0, cols - 5, "dnd");
        wattroff(win, presence_attrs);
        tls_start = 12;
        break;
    case CONTACT_CHAT:
        presence_attrs = theme_attrs(THEME_TITLE_CHAT);
        wattron(win, bracket_attrs);
        mvwaddch(win, 0, cols - 7, '[');
        wattroff(win, bracket_attrs);
        wattron(win, presence_attrs);
        mvwprintw(win, 0, cols - 6, "chat");
        wattroff(win, presence_attrs);
        tls_start = 13;
        break;
    case CONTACT_XA:
        presence_attrs = theme_attrs(THEME_TITLE_XA);
        wattron(win, bracket_attrs);
        mvwaddch(win, 0, cols - 5, '[');
        wattroff(win, bracket_attrs);
        wattron(win, presence_attrs);
        mvwprintw(win, 0, cols - 4, "xa");
        wattroff(win, presence_attrs);
        tls_start = 11;
        break;
    case CONTACT_OFFLINE:
        presence_attrs = theme_attrs(THEME_TITLE_OFFLINE);
        wattron(win, bracket_attrs);
        mvwaddch(win, 0, cols - 10, '[');
        wattroff(win, bracket_attrs);
        wattron(win, presence_attrs);
        mvwprintw(win, 0, cols - 9, "offline");
        wattroff(win, presence_attrs);
        tls_start = 16;
        break;
    }

    wattron(win, bracket_attrs);
    mvwaddch(win, 0, cols - 2, ']');
    wattroff(win, bracket_attrs);

    if (is_connected && prefs_get_boolean(PREF_TLS_SHOW)) {
        wattron(win, bracket_attrs);
        mvwaddch(win, 0, cols - tls_start, '[');
        wattroff(win, bracket_attrs);

        if (tls_secured) {
            wattron(win, encrypted_attrs);
            mvwprintw(win, 0, cols - (tls_start - 1), "TLS");
            wattroff(win, encrypted_attrs);
        } else {
            wattron(win, unencrypted_attrs);
            mvwprintw(win, 0, cols - (tls_start - 1), "TLS");
            wattroff(win, unencrypted_attrs);
        }

        wattron(win, bracket_attrs);
        mvwaddch(win, 0, cols - (tls_start - 4), ']');
        wattroff(win, bracket_attrs);
    }
}

static void
_show_muc_privacy(ProfMucWin* mucwin)
{
    int bracket_attrs = theme_attrs(THEME_TITLE_BRACKET);
    int encrypted_attrs = theme_attrs(THEME_TITLE_ENCRYPTED);
#ifdef HAVE_OMEMO
    int trusted_attrs = theme_attrs(THEME_TITLE_TRUSTED);
    int untrusted_attrs = theme_attrs(THEME_TITLE_UNTRUSTED);
#endif

    if (mucwin->is_omemo) {
        wprintw(win, " ");
        wattron(win, bracket_attrs);
        wprintw(win, "[");
        wattroff(win, bracket_attrs);
        wattron(win, encrypted_attrs);
        wprintw(win, "OMEMO");
        wattroff(win, encrypted_attrs);
        wattron(win, bracket_attrs);
        wprintw(win, "]");
        wattroff(win, bracket_attrs);

#ifdef HAVE_OMEMO
        if (mucwin->omemo_trusted) {
            wprintw(win, " ");
            wattron(win, bracket_attrs);
            wprintw(win, "[");
            wattroff(win, bracket_attrs);
            wattron(win, trusted_attrs);
            wprintw(win, "trusted");
            wattroff(win, trusted_attrs);
            wattron(win, bracket_attrs);
            wprintw(win, "]");
            wattroff(win, bracket_attrs);
        } else {
            wprintw(win, " ");
            wattron(win, bracket_attrs);
            wprintw(win, "[");
            wattroff(win, bracket_attrs);
            wattron(win, untrusted_attrs);
            wprintw(win, "untrusted");
            wattroff(win, untrusted_attrs);
            wattron(win, bracket_attrs);
            wprintw(win, "]");
            wattroff(win, bracket_attrs);
        }
#endif

        return;
    }

    if (mucwin->enctext) {
        wprintw(win, " ");
        wattron(win, bracket_attrs);
        wprintw(win, "[");
        wattroff(win, bracket_attrs);
        wattron(win, encrypted_attrs);
        wprintw(win, "%s", mucwin->enctext);
        wattroff(win, encrypted_attrs);
        wattron(win, bracket_attrs);
        wprintw(win, "]");

        return;
    }
}

static void
_show_privacy(ProfChatWin* chatwin)
{
    int bracket_attrs = theme_attrs(THEME_TITLE_BRACKET);
    int encrypted_attrs = theme_attrs(THEME_TITLE_ENCRYPTED);
    int unencrypted_attrs = theme_attrs(THEME_TITLE_UNENCRYPTED);
    int trusted_attrs = theme_attrs(THEME_TITLE_TRUSTED);
    int untrusted_attrs = theme_attrs(THEME_TITLE_UNTRUSTED);

    if (chatwin->enctext) {
        wprintw(win, " ");
        wattron(win, bracket_attrs);
        wprintw(win, "[");
        wattroff(win, bracket_attrs);
        wattron(win, encrypted_attrs);
        wprintw(win, "%s", chatwin->enctext);
        wattroff(win, encrypted_attrs);
        wattron(win, bracket_attrs);
        wprintw(win, "]");

        return;
    }

    // XEP-0373: OpenPGP for XMPP
    if (chatwin->is_ox) {
        wprintw(win, " ");
        wattron(win, bracket_attrs);
        wprintw(win, "[");
        wattroff(win, bracket_attrs);
        wattron(win, encrypted_attrs);
        wprintw(win, "OX");
        wattroff(win, encrypted_attrs);
        wattron(win, bracket_attrs);
        wprintw(win, "]");
        wattroff(win, bracket_attrs);
    }

    if (chatwin->is_otr) {
        wprintw(win, " ");
        wattron(win, bracket_attrs);
        wprintw(win, "[");
        wattroff(win, bracket_attrs);
        wattron(win, encrypted_attrs);
        wprintw(win, "OTR");
        wattroff(win, encrypted_attrs);
        wattron(win, bracket_attrs);
        wprintw(win, "]");
        wattroff(win, bracket_attrs);
        if (chatwin->otr_is_trusted) {
            wprintw(win, " ");
            wattron(win, bracket_attrs);
            wprintw(win, "[");
            wattroff(win, bracket_attrs);
            wattron(win, trusted_attrs);
            wprintw(win, "trusted");
            wattroff(win, trusted_attrs);
            wattron(win, bracket_attrs);
            wprintw(win, "]");
            wattroff(win, bracket_attrs);
        } else {
            wprintw(win, " ");
            wattron(win, bracket_attrs);
            wprintw(win, "[");
            wattroff(win, bracket_attrs);
            wattron(win, untrusted_attrs);
            wprintw(win, "untrusted");
            wattroff(win, untrusted_attrs);
            wattron(win, bracket_attrs);
            wprintw(win, "]");
            wattroff(win, bracket_attrs);
        }

        return;
    }

    if (chatwin->pgp_send || chatwin->pgp_recv) {
        GString* pgpmsg = g_string_new("PGP ");
        if (chatwin->pgp_send && !chatwin->pgp_recv) {
            g_string_append(pgpmsg, "send");
        } else if (!chatwin->pgp_send && chatwin->pgp_recv) {
            g_string_append(pgpmsg, "recv");
        } else {
            g_string_append(pgpmsg, "send/recv");
        }
        wprintw(win, " ");
        wattron(win, bracket_attrs);
        wprintw(win, "[");
        wattroff(win, bracket_attrs);
        wattron(win, encrypted_attrs);
        wprintw(win, "%s", pgpmsg->str);
        wattroff(win, encrypted_attrs);
        wattron(win, bracket_attrs);
        wprintw(win, "]");
        wattroff(win, bracket_attrs);
        g_string_free(pgpmsg, TRUE);

        return;
    }

    if (chatwin->is_omemo) {
        wprintw(win, " ");
        wattron(win, bracket_attrs);
        wprintw(win, "[");
        wattroff(win, bracket_attrs);
        wattron(win, encrypted_attrs);
        wprintw(win, "OMEMO");
        wattroff(win, encrypted_attrs);
        wattron(win, bracket_attrs);
        wprintw(win, "]");
        wattroff(win, bracket_attrs);

#ifdef HAVE_OMEMO
        if (chatwin->omemo_trusted) {
            wprintw(win, " ");
            wattron(win, bracket_attrs);
            wprintw(win, "[");
            wattroff(win, bracket_attrs);
            wattron(win, trusted_attrs);
            wprintw(win, "trusted");
            wattroff(win, trusted_attrs);
            wattron(win, bracket_attrs);
            wprintw(win, "]");
            wattroff(win, bracket_attrs);
        } else {
            wprintw(win, " ");
            wattron(win, bracket_attrs);
            wprintw(win, "[");
            wattroff(win, bracket_attrs);
            wattron(win, untrusted_attrs);
            wprintw(win, "untrusted");
            wattroff(win, untrusted_attrs);
            wattron(win, bracket_attrs);
            wprintw(win, "]");
            wattroff(win, bracket_attrs);
        }
#endif

        return;
    }

    if (prefs_get_boolean(PREF_ENC_WARN)) {
        wprintw(win, " ");
        wattron(win, bracket_attrs);
        wprintw(win, "[");
        wattroff(win, bracket_attrs);
        wattron(win, unencrypted_attrs);
        wprintw(win, "unencrypted");
        wattroff(win, unencrypted_attrs);
        wattron(win, bracket_attrs);
        wprintw(win, "]");
        wattroff(win, bracket_attrs);
    }
}

static void
_show_contact_presence(ProfChatWin* chatwin, int pos, int maxpos)
{
    int bracket_attrs = theme_attrs(THEME_TITLE_BRACKET);
    char* resource = NULL;

    ChatSession* session = chat_session_get(chatwin->barejid);
    if (chatwin->resource_override) {
        resource = chatwin->resource_override;
    } else if (session && session->resource) {
        resource = session->resource;
    }

    if (resource && prefs_get_boolean(PREF_RESOURCE_TITLE)) {
        int needed = strlen(resource) + 1;
        if (pos + needed < maxpos) {
            wprintw(win, "/");
            wprintw(win, "%s", resource);
        }
    }

    if (prefs_get_boolean(PREF_PRESENCE)) {
        theme_item_t presence_colour = THEME_TITLE_OFFLINE;
        const char* presence = "offline";

        jabber_conn_status_t conn_status = connection_get_status();
        if (conn_status == JABBER_CONNECTED) {
            PContact contact = roster_get_contact(chatwin->barejid);
            if (contact) {
                if (resource) {
                    Resource* resourcep = p_contact_get_resource(contact, resource);
                    if (resourcep) {
                        presence = string_from_resource_presence(resourcep->presence);
                    }
                } else {
                    presence = p_contact_presence(contact);
                }
            }
        }

        presence_colour = THEME_TITLE_ONLINE;
        if (g_strcmp0(presence, "offline") == 0) {
            presence_colour = THEME_TITLE_OFFLINE;
        } else if (g_strcmp0(presence, "away") == 0) {
            presence_colour = THEME_TITLE_AWAY;
        } else if (g_strcmp0(presence, "xa") == 0) {
            presence_colour = THEME_TITLE_XA;
        } else if (g_strcmp0(presence, "chat") == 0) {
            presence_colour = THEME_TITLE_CHAT;
        } else if (g_strcmp0(presence, "dnd") == 0) {
            presence_colour = THEME_TITLE_DND;
        }

        int presence_attrs = theme_attrs(presence_colour);
        wprintw(win, " ");
        wattron(win, bracket_attrs);
        wprintw(win, "[");
        wattroff(win, bracket_attrs);
        wattron(win, presence_attrs);
        wprintw(win, "%s", presence);
        wattroff(win, presence_attrs);
        wattron(win, bracket_attrs);
        wprintw(win, "]");
        wattroff(win, bracket_attrs);
    }
}
