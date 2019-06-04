/*
 * statusbar.c
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
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

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "config/theme.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/statusbar.h"
#include "ui/inputwin.h"
#include "ui/screen.h"
#include "xmpp/roster_list.h"
#include "xmpp/contact.h"

typedef struct _status_bar_tab_t {
    win_type_t window_type;
    char *identifier;
    gboolean highlight;
    char *display_name;
} StatusBarTab;

typedef struct _status_bar_t {
    gchar *time;
    char *prompt;
    char *fulljid;
    GHashTable *tabs;
    int current_tab;
} StatusBar;

static GTimeZone *tz;
static StatusBar *statusbar;
static WINDOW *statusbar_win;

static int _status_bar_draw_time(int pos);
static void _status_bar_draw_maintext(int pos);
static int _status_bar_draw_bracket(gboolean current, int pos, char* ch);
static int _status_bar_draw_extended_tabs(int pos);
static int _status_bar_draw_tab(StatusBarTab *tab, int pos, int num);
static void _destroy_tab(StatusBarTab *tab);
static int _tabs_width(void);
static char* _display_name(StatusBarTab *tab);
static gboolean _extended_new(void);

void
status_bar_init(void)
{
    tz = g_time_zone_new_local();

    statusbar = malloc(sizeof(StatusBar));
    statusbar->time = NULL;
    statusbar->prompt = NULL;
    statusbar->fulljid = NULL;
    statusbar->tabs = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)_destroy_tab);
    StatusBarTab *console = malloc(sizeof(StatusBarTab));
    console->window_type = WIN_CONSOLE;
    console->identifier = strdup("console");
    console->display_name = NULL;
    g_hash_table_insert(statusbar->tabs, GINT_TO_POINTER(1), console);
    statusbar->current_tab = 1;

    int row = screen_statusbar_row();
    int cols = getmaxx(stdscr);
    statusbar_win = newwin(1, cols, row, 0);

    status_bar_draw();
}

void
status_bar_close(void)
{
    if (tz) {
        g_time_zone_unref(tz);
    }
    if (statusbar) {
        if (statusbar->time) {
            g_free(statusbar->time);
        }
        if (statusbar->prompt) {
            free(statusbar->prompt);
        }
        if (statusbar->fulljid) {
            free(statusbar->fulljid);
        }
        if (statusbar->tabs) {
            g_hash_table_destroy(statusbar->tabs);
        }
        free(statusbar);
    }
}

void
status_bar_resize(void)
{
    int cols = getmaxx(stdscr);
    werase(statusbar_win);
    int row = screen_statusbar_row();
    mvwin(statusbar_win, row, 0);
    wresize(statusbar_win, 1, cols);

    status_bar_draw();
}

void
status_bar_set_all_inactive(void)
{
    g_hash_table_remove_all(statusbar->tabs);
}

void
status_bar_current(int i)
{
    if (i == 0) {
        statusbar->current_tab = 10;
    } else {
        statusbar->current_tab = i;
    }

    status_bar_draw();
}

void
status_bar_inactive(const int win)
{
    int true_win = win;
    if (true_win == 0) {
        true_win = 10;
    }

    g_hash_table_remove(statusbar->tabs, GINT_TO_POINTER(true_win));

    status_bar_draw();
}

void
status_bar_active(const int win, win_type_t wintype, char *identifier)
{
    int true_win = win;
    if (true_win == 0) {
        true_win = 10;
    }

    StatusBarTab *tab = malloc(sizeof(StatusBarTab));
    tab->identifier = strdup(identifier);
    tab->highlight = FALSE;
    tab->window_type = wintype;
    tab->display_name = NULL;

    if (tab->window_type == WIN_CHAT) {
        PContact contact = roster_get_contact(tab->identifier);
        if (contact && p_contact_name(contact)) {
            tab->display_name = strdup(p_contact_name(contact));
        } else {
            char *pref = prefs_get_string(PREF_STATUSBAR_CHAT);
            if (g_strcmp0("user", pref) == 0) {
                Jid *jidp = jid_create(tab->identifier);
                tab->display_name = strdup(jidp->localpart);
                jid_destroy(jidp);
            } else {
                tab->display_name = strdup(tab->identifier);
            }
        }
    }

    g_hash_table_replace(statusbar->tabs, GINT_TO_POINTER(true_win), tab);

    status_bar_draw();
}

void
status_bar_new(const int win, win_type_t wintype, char* identifier)
{
    int true_win = win;
    if (true_win == 0) {
        true_win = 10;
    }

    StatusBarTab *tab = malloc(sizeof(StatusBarTab));
    tab->identifier = strdup(identifier);
    tab->highlight = TRUE;
    tab->window_type = wintype;
    tab->display_name = NULL;

    if (tab->window_type == WIN_CHAT) {
        PContact contact = roster_get_contact(tab->identifier);
        if (contact && p_contact_name(contact)) {
            tab->display_name = strdup(p_contact_name(contact));
        } else {
            char *pref = prefs_get_string(PREF_STATUSBAR_CHAT);
            if (g_strcmp0("user", pref) == 0) {
                Jid *jidp = jid_create(tab->identifier);
                tab->display_name = strdup(jidp->localpart);
                jid_destroy(jidp);
            } else {
                tab->display_name = strdup(tab->identifier);
            }
        }
    }

    g_hash_table_replace(statusbar->tabs, GINT_TO_POINTER(true_win), tab);

    status_bar_draw();
}

void
status_bar_set_prompt(const char *const prompt)
{
    if (statusbar->prompt) {
        free(statusbar->prompt);
        statusbar->prompt = NULL;
    }
    statusbar->prompt = strdup(prompt);

    status_bar_draw();
}

void
status_bar_clear_prompt(void)
{
    if (statusbar->prompt) {
        free(statusbar->prompt);
        statusbar->prompt = NULL;
    }

    status_bar_draw();
}

void
status_bar_set_fulljid(const char *const fulljid)
{
    if (statusbar->fulljid) {
        free(statusbar->fulljid);
        statusbar->fulljid = NULL;
    }
    statusbar->fulljid = strdup(fulljid);

    status_bar_draw();
}

void
status_bar_clear_fulljid(void)
{
    if (statusbar->fulljid) {
        free(statusbar->fulljid);
        statusbar->fulljid = NULL;
    }

    status_bar_draw();
}

void
status_bar_draw(void)
{
    werase(statusbar_win);
    wbkgd(statusbar_win, theme_attrs(THEME_STATUS_TEXT));

    int pos = 1;

    pos = _status_bar_draw_time(pos);

    _status_bar_draw_maintext(pos);

    pos = getmaxx(stdscr) - _tabs_width();
    if (pos < 0) {
        pos = 0;
    }
    gint max_tabs = prefs_get_statusbartabs();
    int i = 1;
    for (i = 1; i <= max_tabs; i++) {
        StatusBarTab *tab = g_hash_table_lookup(statusbar->tabs, GINT_TO_POINTER(i));
        if (tab) {
            pos = _status_bar_draw_tab(tab, pos, i);
        }
    }

    pos = _status_bar_draw_extended_tabs(pos);

    wnoutrefresh(statusbar_win);
    inp_put_back();
}

static gboolean
_extended_new(void)
{
    gint max_tabs = prefs_get_statusbartabs();
    int tabs_count = g_hash_table_size(statusbar->tabs);
    if (tabs_count <= max_tabs) {
        return FALSE;
    }

    int i = 0;
    for (i = max_tabs + 1; i <= tabs_count; i++) {
        StatusBarTab *tab = g_hash_table_lookup(statusbar->tabs, GINT_TO_POINTER(i));
        if (tab && tab->highlight) {
            return TRUE;
        }
    }

    return FALSE;
}

static int
_status_bar_draw_extended_tabs(int pos)
{
    gint max_tabs = prefs_get_statusbartabs();
    if (max_tabs == 0) {
        return pos;
    }

    if (g_hash_table_size(statusbar->tabs) > max_tabs) {
        gboolean is_current = statusbar->current_tab > max_tabs;

        pos = _status_bar_draw_bracket(is_current, pos, "[");

        int status_attrs = theme_attrs(THEME_STATUS_ACTIVE);
        if (_extended_new()) {
            status_attrs = theme_attrs(THEME_STATUS_NEW);
        }
        wattron(statusbar_win, status_attrs);
        mvwprintw(statusbar_win, 0, pos, ">");
        wattroff(statusbar_win, status_attrs);
        pos++;

        pos = _status_bar_draw_bracket(is_current, pos, "]");
    }

    return pos;
}

static int
_status_bar_draw_tab(StatusBarTab *tab, int pos, int num)
{
    int display_num = num == 10 ? 0 : num;
    gboolean is_current = num == statusbar->current_tab;

    gboolean show_number = prefs_get_boolean(PREF_STATUSBAR_SHOW_NUMBER);
    gboolean show_name = prefs_get_boolean(PREF_STATUSBAR_SHOW_NAME);

    pos = _status_bar_draw_bracket(is_current, pos, "[");

    int status_attrs = 0;
    if (tab->highlight) {
        status_attrs = theme_attrs(THEME_STATUS_NEW);
    } else {
        status_attrs = theme_attrs(THEME_STATUS_ACTIVE);
    }
    wattron(statusbar_win, status_attrs);
    if (show_number) {
        mvwprintw(statusbar_win, 0, pos, "%d", display_num);
        pos++;
    }
    if (show_number && show_name) {
        mvwprintw(statusbar_win, 0, pos, ":");
        pos++;
    }
    if (show_name) {
        char *display_name = _display_name(tab);
        mvwprintw(statusbar_win, 0, pos, display_name);
        pos += utf8_display_len(display_name);
        free(display_name);
    }
    wattroff(statusbar_win, status_attrs);

    pos = _status_bar_draw_bracket(is_current, pos, "]");

    return pos;
}

static int
_status_bar_draw_bracket(gboolean current, int pos, char* ch)
{
    int bracket_attrs = theme_attrs(THEME_STATUS_BRACKET);
    wattron(statusbar_win, bracket_attrs);
    if (current) {
        mvwprintw(statusbar_win, 0, pos, "-");
    } else {
        mvwprintw(statusbar_win, 0, pos, ch);
    }
    wattroff(statusbar_win, bracket_attrs);
    pos++;

    return pos;
}

static int
_status_bar_draw_time(int pos)
{
    char *time_pref = prefs_get_string(PREF_TIME_STATUSBAR);
    if (g_strcmp0(time_pref, "off") == 0) {
        prefs_free_string(time_pref);
        return pos;
    }

    if (statusbar->time) {
        g_free(statusbar->time);
        statusbar->time = NULL;
    }

    GDateTime *datetime = g_date_time_new_now(tz);
    statusbar->time  = g_date_time_format(datetime, time_pref);
    assert(statusbar->time != NULL);
    g_date_time_unref(datetime);

    int bracket_attrs = theme_attrs(THEME_STATUS_BRACKET);
    int time_attrs = theme_attrs(THEME_STATUS_TIME);

    size_t len = strlen(statusbar->time);
    wattron(statusbar_win, bracket_attrs);
    mvwaddch(statusbar_win, 0, pos, '[');
    pos++;
    wattroff(statusbar_win, bracket_attrs);
    wattron(statusbar_win, time_attrs);
    mvwprintw(statusbar_win, 0, pos, statusbar->time);
    pos += len;
    wattroff(statusbar_win, time_attrs);
    wattron(statusbar_win, bracket_attrs);
    mvwaddch(statusbar_win, 0, pos, ']');
    wattroff(statusbar_win, bracket_attrs);
    pos += 2;

    prefs_free_string(time_pref);

    return pos;
}

static void
_status_bar_draw_maintext(int pos)
{
    if (statusbar->prompt) {
        mvwprintw(statusbar_win, 0, pos, statusbar->prompt);
        return;
    }

    if (statusbar->fulljid) {
        char *pref = prefs_get_string(PREF_STATUSBAR_SELF);
        if (g_strcmp0(pref, "off") == 0) {
            return;
        }
        if (g_strcmp0(pref, "user") == 0) {
            Jid *jidp = jid_create(statusbar->fulljid);
            mvwprintw(statusbar_win, 0, pos, jidp->localpart);
            jid_destroy(jidp);
            return;
        }
        if (g_strcmp0(pref, "barejid") == 0) {
            Jid *jidp = jid_create(statusbar->fulljid);
            mvwprintw(statusbar_win, 0, pos, jidp->barejid);
            jid_destroy(jidp);
            return;
        }
        mvwprintw(statusbar_win, 0, pos, statusbar->fulljid);
    }
}

static void
_destroy_tab(StatusBarTab *tab)
{
    if (tab) {
        if (tab->identifier) {
            free(tab->identifier);
        }
        if (tab->display_name) {
            free(tab->display_name);
        }
        free(tab);
    }
    tab = NULL;
}

static int
_tabs_width(void)
{
    gboolean show_number = prefs_get_boolean(PREF_STATUSBAR_SHOW_NUMBER);
    gboolean show_name = prefs_get_boolean(PREF_STATUSBAR_SHOW_NAME);
    gint max_tabs = prefs_get_statusbartabs();

    if (show_name && show_number) {
        int width = g_hash_table_size(statusbar->tabs) > max_tabs ? 4 : 1;
        int i = 0;
        for (i = 1; i <= max_tabs; i++) {
            StatusBarTab *tab = g_hash_table_lookup(statusbar->tabs, GINT_TO_POINTER(i));
            if (tab) {
                char *display_name = _display_name(tab);
                width += utf8_display_len(display_name);
                width += 4;
                free(display_name);
            }
        }
        return width;
    }

    if (show_name && !show_number) {
        int width = g_hash_table_size(statusbar->tabs) > max_tabs ? 4 : 1;
        int i = 0;
        for (i = 1; i <= max_tabs; i++) {
            StatusBarTab *tab = g_hash_table_lookup(statusbar->tabs, GINT_TO_POINTER(i));
            if (tab) {
                char *display_name = _display_name(tab);
                width += utf8_display_len(display_name);
                width += 2;
                free(display_name);
            }
        }
        return width;
    }

    if (g_hash_table_size(statusbar->tabs) > max_tabs) {
        return max_tabs * 3 + (g_hash_table_size(statusbar->tabs) > max_tabs ? 4 : 1);
    }
    return g_hash_table_size(statusbar->tabs) * 3 + (g_hash_table_size(statusbar->tabs) > max_tabs ? 4 : 1);
}

static char*
_display_name(StatusBarTab *tab)
{
    char *fullname = NULL;

    if (tab->window_type == WIN_CONSOLE) {
        fullname = strdup("console");
    } else if (tab->window_type == WIN_XML) {
        fullname = strdup("xmlconsole");
    } else if (tab->window_type == WIN_PLUGIN) {
        fullname = strdup(tab->identifier);
    } else if (tab->window_type == WIN_CHAT) {
        if (tab && tab->display_name) {
            fullname = strdup(tab->display_name);
        }
    } else if (tab->window_type == WIN_MUC) {
        char *pref = prefs_get_string(PREF_STATUSBAR_ROOM);
        if (g_strcmp0("room", pref) == 0) {
            Jid *jidp = jid_create(tab->identifier);
            char *room = strdup(jidp->localpart);
            jid_destroy(jidp);
            fullname = room;
        } else {
            fullname = strdup(tab->identifier);
        }
    } else if (tab->window_type == WIN_CONFIG) {
        char *pref = prefs_get_string(PREF_STATUSBAR_ROOM);
        GString *display_str = g_string_new("");
        if (g_strcmp0("room", pref) == 0) {
            Jid *jidp = jid_create(tab->identifier);
            g_string_append(display_str, jidp->localpart);
            jid_destroy(jidp);
        } else {
            g_string_append(display_str, tab->identifier);
        }
        g_string_append(display_str, " conf");
        char *result = strdup(display_str->str);
        g_string_free(display_str, TRUE);
        fullname = result;
    } else if (tab->window_type == WIN_PRIVATE) {
        char *pref = prefs_get_string(PREF_STATUSBAR_ROOM);
        if (g_strcmp0("room", pref) == 0) {
            GString *display_str = g_string_new("");
            Jid *jidp = jid_create(tab->identifier);
            g_string_append(display_str, jidp->localpart);
            g_string_append(display_str, "/");
            g_string_append(display_str, jidp->resourcepart);
            jid_destroy(jidp);
            char *result = strdup(display_str->str);
            g_string_free(display_str, TRUE);
            fullname = result;
        } else {
            fullname = strdup(tab->identifier);
        }
    } else {
        fullname = strdup("window");
    }

    gint tablen = prefs_get_statusbartablen();
    if (tablen == 0) {
        return fullname;
    }

    int namelen = utf8_display_len(fullname);
    if (namelen < tablen) {
        return fullname;
    }

    gchar *trimmed = g_utf8_substring(fullname, 0, tablen);
    free(fullname);
    char *trimmedname = strdup(trimmed);
    g_free(trimmed);

    return trimmedname;

}
