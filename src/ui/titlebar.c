/*
 * titlebar.c
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
#include "xmpp/roster_list.h"
#include "xmpp/chat_session.h"

static WINDOW *win;
static contact_presence_t current_presence;
static gboolean tls_secured;
static gboolean is_connected;

static gboolean typing;
static GTimer *typing_elapsed;

static void _title_bar_draw(void);
static void _show_self_presence(void);
static void _show_contact_presence(ProfChatWin *chatwin);
static void _show_privacy(ProfChatWin *chatwin);
static void _show_muc_privacy(ProfMucWin *mucwin);

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
title_bar_update_virtual(void)
{
    ProfWin *window = wins_get_current();
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
    mvwin(win, row, 0);

    wresize(win, 1, cols);
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
    ProfWin *current = wins_get_current();

    werase(win);
    wmove(win, 0, 0);
    int i;
    for (i = 0; i < 45; i++) {
        waddch(win, ' ');
    }

    char *title = win_get_title(current);
    mvwprintw(win, 0, 0, " %s", title);
    free(title);

    if (current && current->type == WIN_CHAT) {
        ProfChatWin *chatwin = (ProfChatWin*) current;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        _show_contact_presence(chatwin);
        _show_privacy(chatwin);

        if (typing) {
            wprintw(win, " (typing...)");
        }
    } else if (current && current->type == WIN_MUC) {
        ProfMucWin *mucwin = (ProfMucWin*) current;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        _show_muc_privacy(mucwin);
    }

    _show_self_presence();

    wnoutrefresh(win);
    inp_put_back();
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

    switch (current_presence)
    {
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
_show_muc_privacy(ProfMucWin *mucwin)
{
    int bracket_attrs = theme_attrs(THEME_TITLE_BRACKET);
    int encrypted_attrs = theme_attrs(THEME_TITLE_ENCRYPTED);

    if (mucwin->enctext) {
        wprintw(win, " ");
        wattron(win, bracket_attrs);
        wprintw(win, "[");
        wattroff(win, bracket_attrs);
        wattron(win, encrypted_attrs);
        wprintw(win, mucwin->enctext);
        wattroff(win, encrypted_attrs);
        wattron(win, bracket_attrs);
        wprintw(win, "]");

        return;
    }
}

static void
_show_privacy(ProfChatWin *chatwin)
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
        wprintw(win, chatwin->enctext);
        wattroff(win, encrypted_attrs);
        wattron(win, bracket_attrs);
        wprintw(win, "]");

        return;
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
        GString *pgpmsg = g_string_new("PGP ");
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
        wprintw(win, pgpmsg->str);
        wattroff(win, encrypted_attrs);
        wattron(win, bracket_attrs);
        wprintw(win, "]");
        wattroff(win, bracket_attrs);
        g_string_free(pgpmsg, TRUE);

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
_show_contact_presence(ProfChatWin *chatwin)
{
    int bracket_attrs = theme_attrs(THEME_TITLE_BRACKET);
    char *resource = NULL;

    ChatSession *session = chat_session_get(chatwin->barejid);
    if (chatwin->resource_override) {
        resource = chatwin->resource_override;
    } else if (session && session->resource) {
        resource = session->resource;
    }
    if (resource && prefs_get_boolean(PREF_RESOURCE_TITLE)) {
        wprintw(win, "/");
        wprintw(win, resource);
    }

    if (prefs_get_boolean(PREF_PRESENCE)) {
        theme_item_t presence_colour = THEME_TITLE_OFFLINE;
        const char *presence = "offline";

        jabber_conn_status_t conn_status = connection_get_status();
        if (conn_status == JABBER_CONNECTED) {
            PContact contact = roster_get_contact(chatwin->barejid);
            if (contact) {
                if (resource) {
                    Resource *resourcep = p_contact_get_resource(contact, resource);
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
        wprintw(win, presence);
        wattroff(win, presence_attrs);
        wattron(win, bracket_attrs);
        wprintw(win, "]");
        wattroff(win, bracket_attrs);
    }
}
