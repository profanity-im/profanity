/*
 * statusbar.c
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

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#elif HAVE_CURSES_H
#include <curses.h>
#endif

#include "config/theme.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/statusbar.h"
#include "ui/inputwin.h"
#include "ui/screen.h"
#include "xmpp/roster_list.h"
#include "xmpp/contact.h"

typedef struct _status_bar_tab_t
{
    win_type_t window_type;
    char* identifier;
    gboolean highlight;
    char* display_name;
} StatusBarTab;

typedef struct _status_bar_t
{
    gchar* time;
    char* prompt;
    char* fulljid;
    GHashTable* tabs;
    int current_tab;
} StatusBar;

static GTimeZone* tz;
static StatusBar* statusbar;
static WINDOW* statusbar_win;

void _get_range_bounds(int* start, int* end, gboolean is_static);
static int _status_bar_draw_time(int pos);
static int _status_bar_draw_maintext(int pos);
static int _status_bar_draw_bracket(gboolean current, int pos, const char* ch);
static int _status_bar_draw_extended_tabs(int pos, gboolean prefix, int start, int end, gboolean is_static);
static int _status_bar_draw_tab(StatusBarTab* tab, int pos, int num);
static int _status_bar_draw_tabs(int pos);
static void _destroy_tab(StatusBarTab* tab);
static int _tabs_width(int start, int end);
static unsigned int _count_digits(int number);
static unsigned int _count_digits_in_range(int start, int end);
static char* _display_name(StatusBarTab* tab);
static gboolean _tabmode_is_actlist(void);

void
status_bar_init(void)
{
    tz = g_time_zone_new_local();

    statusbar = malloc(sizeof(StatusBar));
    statusbar->time = NULL;
    statusbar->prompt = NULL;
    statusbar->fulljid = NULL;
    statusbar->tabs = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)_destroy_tab);
    StatusBarTab* console = calloc(1, sizeof(StatusBarTab));
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
    wresize(statusbar_win, 1, cols);
    mvwin(statusbar_win, row, 0);

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
_create_tab(const int win, win_type_t wintype, char* identifier, gboolean highlight)
{
    int true_win = win == 0 ? 10 : win;

    StatusBarTab* tab = malloc(sizeof(StatusBarTab));
    tab->identifier = strdup(identifier);
    tab->highlight = highlight;
    tab->window_type = wintype;
    tab->display_name = NULL;

    if (tab->window_type == WIN_CHAT) {
        PContact contact = NULL;
        if (roster_exists()) {
            contact = roster_get_contact(tab->identifier);
        }
        const char* pcontact_name = contact ? p_contact_name(contact) : NULL;
        auto_gchar gchar* pref = prefs_get_string(PREF_STATUSBAR_CHAT);
        if (g_strcmp0("user", pref) == 0) {
            if (pcontact_name) {
                tab->display_name = strdup(pcontact_name);
            } else {
                auto_jid Jid* jidp = jid_create(tab->identifier);
                if (jidp) {
                    tab->display_name = jidp->localpart != NULL ? strdup(jidp->localpart) : strdup(jidp->barejid);
                } else {
                    tab->display_name = strdup(tab->identifier);
                }
            }
        } else {
            tab->display_name = strdup(tab->identifier);
        }
    }

    g_hash_table_replace(statusbar->tabs, GINT_TO_POINTER(true_win), tab);

    status_bar_draw();
}

void
status_bar_active(const int win, win_type_t wintype, char* identifier)
{
    _create_tab(win, wintype, identifier, FALSE);
}

void
status_bar_new(const int win, win_type_t wintype, char* identifier)
{
    _create_tab(win, wintype, identifier, TRUE);
}

void
status_bar_set_prompt(const char* const prompt)
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
status_bar_set_fulljid(const char* const fulljid)
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

    gint max_tabs = prefs_get_statusbartabs();
    int pos = 1;

    pos = _status_bar_draw_time(pos);
    pos = _status_bar_draw_maintext(pos);
    if (max_tabs != 0)
        pos = _status_bar_draw_tabs(pos);

    wnoutrefresh(statusbar_win);
    inp_put_back();
}

static int
_status_bar_draw_tabs(int pos)
{
    auto_gchar gchar* tabmode = prefs_get_string(PREF_STATUSBAR_TABMODE);

    if (g_strcmp0(tabmode, "actlist") != 0) {
        int start, end;
        gboolean is_static = g_strcmp0(tabmode, "dynamic") != 0;
        _get_range_bounds(&start, &end, is_static);

        pos = getmaxx(stdscr) - _tabs_width(start, end);
        if (pos < 0) {
            pos = 0;
        }

        pos = _status_bar_draw_extended_tabs(pos, TRUE, start, end, is_static);

        for (int i = start; i <= end; i++) {
            StatusBarTab* tab = g_hash_table_lookup(statusbar->tabs, GINT_TO_POINTER(i));
            if (tab) {
                pos = _status_bar_draw_tab(tab, pos, i);
            }
        }

        pos = _status_bar_draw_extended_tabs(pos, FALSE, start, end, is_static);
    } else {
        pos++;
        guint print_act = 0;
        guint tabnum = g_hash_table_size(statusbar->tabs);
        for (guint i = 1; i <= tabnum; ++i) {
            StatusBarTab* tab = g_hash_table_lookup(statusbar->tabs, GINT_TO_POINTER(i));
            if (tab && tab->highlight) {
                print_act++;
            }
        }
        if (print_act) {
            pos = _status_bar_draw_bracket(FALSE, pos, "[");
            mvwprintw(statusbar_win, 0, pos, "Act: ");
            pos += 5;
            int status_attrs = theme_attrs(THEME_STATUS_NEW);

            wattron(statusbar_win, status_attrs);
            for (guint i = 1; i <= tabnum && print_act; ++i) {
                StatusBarTab* tab = g_hash_table_lookup(statusbar->tabs, GINT_TO_POINTER(i));
                if (tab && tab->highlight) {
                    if (print_act == 1) {
                        mvwprintw(statusbar_win, 0, pos, "%d", i);
                        pos++;
                    } else {
                        mvwprintw(statusbar_win, 0, pos, "%d,", i);
                        pos += 2;
                    }
                    for (guint limit = 10; i >= limit; limit *= 10) {
                        pos++;
                    }
                    print_act--;
                }
            }
            wattroff(statusbar_win, status_attrs);
            pos = _status_bar_draw_bracket(FALSE, pos, "]");
        }
    }
    return pos;
}

// Checks if there are highlighted (unread) messages on the (left side if `left_side` is true, else right side)
// within the current displayed tabs range.
static gboolean
_has_new_msgs_beyond_range_on_side(gboolean left_side, int display_tabs_start, int display_tabs_end)
{
    gint max_tabs = prefs_get_statusbartabs();
    int tabs_count = g_hash_table_size(statusbar->tabs);
    if (tabs_count <= max_tabs) {
        return FALSE;
    }

    int start = left_side ? 1 : display_tabs_end + 1;
    int end = left_side ? display_tabs_start - 1 : tabs_count;

    for (int i = start; i <= end; i++) {
        StatusBarTab* tab = g_hash_table_lookup(statusbar->tabs, GINT_TO_POINTER(i));
        if (tab && tab->highlight) {
            return TRUE;
        }
    }

    return FALSE;
}

static int
_status_bar_draw_extended_tabs(int pos, gboolean prefix, int start, int end, gboolean is_static)
{
    gint max_tabs = prefs_get_statusbartabs();
    if (max_tabs == 0) {
        return pos;
    }

    guint opened_tabs = g_hash_table_size(statusbar->tabs);
    if (g_hash_table_size(statusbar->tabs) <= max_tabs) {
        return pos;
    }

    if (prefix && start < 2) {
        return pos;
    }
    if (!prefix && end > opened_tabs - 1) {
        return pos;
    }
    gboolean is_current = is_static && statusbar->current_tab > max_tabs;

    pos = _status_bar_draw_bracket(is_current, pos, "[");

    int status_attrs;

    if (_has_new_msgs_beyond_range_on_side(prefix, start, end)) {
        // Apply theme attributes for a tab with new messages.
        status_attrs = theme_attrs(THEME_STATUS_NEW);
    } else {
        // Apply theme attributes for a tab without new messages.
        status_attrs = theme_attrs(THEME_STATUS_ACTIVE);
    }
    wattron(statusbar_win, status_attrs);
    mvwprintw(statusbar_win, 0, pos, prefix ? "<" : ">");
    wattroff(statusbar_win, status_attrs);
    pos++;

    pos = _status_bar_draw_bracket(is_current, pos, "]");

    return pos;
}

static int
_status_bar_draw_tab(StatusBarTab* tab, int pos, int num)
{
    gboolean is_current = num == statusbar->current_tab;

    gboolean show_number = prefs_get_boolean(PREF_STATUSBAR_SHOW_NUMBER);
    gboolean show_name = prefs_get_boolean(PREF_STATUSBAR_SHOW_NAME);
    gboolean show_read = prefs_get_boolean(PREF_STATUSBAR_SHOW_READ);

    // dont show this
    if (!show_read && !is_current && !tab->highlight)
        return pos;

    pos = _status_bar_draw_bracket(is_current, pos, "[");

    int status_attrs;
    if (is_current) {
        status_attrs = theme_attrs(THEME_STATUS_CURRENT);
    } else if (tab->highlight) {
        status_attrs = theme_attrs(THEME_STATUS_NEW);
    } else {
        status_attrs = theme_attrs(THEME_STATUS_ACTIVE);
    }
    wattron(statusbar_win, status_attrs);
    if (show_number) {
        mvwprintw(statusbar_win, 0, pos, "%d", num);
        // calculate number of digits
        pos += _count_digits(num);
    }
    if (show_number && show_name) {
        mvwprintw(statusbar_win, 0, pos, ":");
        pos++;
    }
    if (show_name) {
        auto_char char* display_name = _display_name(tab);
        mvwprintw(statusbar_win, 0, pos, "%s", display_name);
        pos += utf8_display_len(display_name);
    }
    wattroff(statusbar_win, status_attrs);

    pos = _status_bar_draw_bracket(is_current, pos, "]");

    return pos;
}

static int
_status_bar_draw_bracket(gboolean current, int pos, const char* ch)
{
    int bracket_attrs = theme_attrs(THEME_STATUS_BRACKET);
    wattron(statusbar_win, bracket_attrs);
    if (current) {
        mvwprintw(statusbar_win, 0, pos, "-");
    } else {
        mvwprintw(statusbar_win, 0, pos, "%s", ch);
    }
    wattroff(statusbar_win, bracket_attrs);
    pos++;

    return pos;
}

static int
_status_bar_draw_time(int pos)
{
    auto_gchar gchar* time_pref = prefs_get_string(PREF_TIME_STATUSBAR);
    if (g_strcmp0(time_pref, "off") == 0) {
        return pos;
    }

    if (statusbar->time) {
        g_free(statusbar->time);
        statusbar->time = NULL;
    }

    GDateTime* datetime = g_date_time_new_now(tz);
    statusbar->time = g_date_time_format(datetime, time_pref);
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
    mvwprintw(statusbar_win, 0, pos, "%s", statusbar->time);
    pos += len;
    wattroff(statusbar_win, time_attrs);
    wattron(statusbar_win, bracket_attrs);
    mvwaddch(statusbar_win, 0, pos, ']');
    wattroff(statusbar_win, bracket_attrs);
    pos += 2;

    return pos;
}

static gboolean
_tabmode_is_actlist(void)
{
    auto_gchar gchar* tabmode = prefs_get_string(PREF_STATUSBAR_TABMODE);
    return g_strcmp0(tabmode, "actlist") == 0;
}

static int
_status_bar_draw_maintext(int pos)
{
    const char* maintext = NULL;
    auto_jid Jid* jidp = NULL;
    auto_gchar gchar* self = prefs_get_string(PREF_STATUSBAR_SELF);

    if (statusbar->prompt) {
        mvwprintw(statusbar_win, 0, pos, "%s", statusbar->prompt);
        return utf8_display_len(statusbar->prompt);
    }

    if (g_strcmp0(self, "off") == 0) {
        return pos;
    }

    if (statusbar->fulljid) {
        jidp = jid_create(statusbar->fulljid);
        if (g_strcmp0(self, "user") == 0) {
            maintext = jidp->localpart;
        } else if (g_strcmp0(self, "barejid") == 0) {
            maintext = jidp->barejid;
        } else {
            maintext = statusbar->fulljid;
        }
    }

    if (maintext == NULL) {
        return pos;
    }

    if (statusbar->fulljid) {
        auto_gchar gchar* pref = prefs_get_string(PREF_STATUSBAR_SELF);

        if (g_strcmp0(pref, "off") == 0) {
            return pos;
        }
        if (g_strcmp0(pref, "user") == 0) {
            auto_jid Jid* jidp = jid_create(statusbar->fulljid);
            mvwprintw(statusbar_win, 0, pos, "%s", jidp->localpart);
            return pos;
        }
        if (g_strcmp0(pref, "barejid") == 0) {
            auto_jid Jid* jidp = jid_create(statusbar->fulljid);
            mvwprintw(statusbar_win, 0, pos, "%s", jidp->barejid);
            return pos;
        }

        mvwprintw(statusbar_win, 0, pos, "%s", statusbar->fulljid);
    }

    gboolean actlist_tabmode = _tabmode_is_actlist();
    auto_gchar gchar* maintext_ = NULL;
    if (actlist_tabmode) {
        pos = _status_bar_draw_bracket(FALSE, pos, "[");
        maintext_ = g_strdup_printf("%d:%s", statusbar->current_tab, maintext);
        maintext = maintext_;
    }
    mvwprintw(statusbar_win, 0, pos, "%s", maintext);
    pos += utf8_display_len(maintext);
    if (actlist_tabmode) {
        pos = _status_bar_draw_bracket(FALSE, pos, "]");
    }
    return pos;
}

static void
_destroy_tab(StatusBarTab* tab)
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
}

static int
_tabs_width(int start, int end)
{
    gboolean show_number = prefs_get_boolean(PREF_STATUSBAR_SHOW_NUMBER);
    gboolean show_name = prefs_get_boolean(PREF_STATUSBAR_SHOW_NAME);
    gboolean show_read = prefs_get_boolean(PREF_STATUSBAR_SHOW_READ);
    gint max_tabs = prefs_get_statusbartabs();
    guint opened_tabs = g_hash_table_size(statusbar->tabs);

    int width = start < 2 ? 1 : 4;
    width += end > opened_tabs - 1 ? 0 : 3;

    if (show_name && show_number) {
        for (int i = start; i <= end; i++) {
            StatusBarTab* tab = g_hash_table_lookup(statusbar->tabs, GINT_TO_POINTER(i));
            if (tab) {
                gboolean is_current = i == statusbar->current_tab;
                // dont calculate this in because not shown
                if (!show_read && !is_current && !tab->highlight)
                    continue;

                auto_char char* display_name = _display_name(tab);
                width += utf8_display_len(display_name);
                width += 3 + _count_digits(i);
            }
        }
        return width;
    }

    if (show_name && !show_number) {
        for (int i = start; i <= end; i++) {
            StatusBarTab* tab = g_hash_table_lookup(statusbar->tabs, GINT_TO_POINTER(i));
            if (tab) {
                gboolean is_current = i == statusbar->current_tab;
                // dont calculate this in because not shown
                if (!show_read && !is_current && !tab->highlight)
                    continue;

                auto_char char* display_name = _display_name(tab);
                width += utf8_display_len(display_name);
                width += 2;
            }
        }
        return width;
    }

    if (opened_tabs > max_tabs) {
        width += _count_digits_in_range(start, end);
        width += start > end ? 0 : (end - start) * 3;
        return width;
    }
    return opened_tabs * 3 + 1;
}

static char*
_display_name(StatusBarTab* tab)
{
    char* fullname = NULL;

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
        auto_gchar gchar* mucwin_title = mucwin_generate_title(tab->identifier, PREF_STATUSBAR_ROOM_TITLE);
        fullname = strdup(mucwin_title);
    } else if (tab->window_type == WIN_CONFIG) {
        auto_gchar gchar* mucwin_title = mucwin_generate_title(tab->identifier, PREF_STATUSBAR_ROOM_TITLE);
        fullname = g_strconcat(mucwin_title, " conf", NULL);
    } else if (tab->window_type == WIN_PRIVATE) {
        auto_jid Jid* jid = jid_create(tab->identifier);
        auto_gchar gchar* mucwin_title = mucwin_generate_title(jid->barejid, PREF_STATUSBAR_ROOM_TITLE);
        fullname = g_strconcat(mucwin_title, "/", jid->resourcepart, NULL);
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

    auto_gchar gchar* trimmed = g_utf8_substring(fullname, 0, tablen);
    char* trimmedname = strdup(trimmed);
    free(fullname);

    return trimmedname;
}

void
_get_range_bounds(int* start, int* end, gboolean is_static)
{
    int current_tab = statusbar->current_tab;
    gint display_range = prefs_get_statusbartabs();
    int total_tabs = g_hash_table_size(statusbar->tabs);
    int side_range = display_range / 2;

    if (total_tabs <= display_range) {
        *start = 1;
        *end = total_tabs;
    } else if (current_tab - side_range <= 1 || is_static) {
        *start = 1;
        *end = display_range;
    } else if (current_tab + side_range >= total_tabs) {
        *start = total_tabs - display_range + 1;
        *end = total_tabs;
    } else {
        *start = current_tab - side_range;
        *end = current_tab + side_range;
    }
}

// Counts amount of digits in a number
static unsigned int
_count_digits(int number)
{
    unsigned int digits_count = 0;
    if (number < 0)
        number = -number;

    do {
        number /= 10;
        digits_count++;
    } while (number != 0);

    return digits_count;
}

// Counts the total number of digits in a range of numbers, inclusive.
// Example: _count_digits_in_range(2, 3) returns 2, _count_digits_in_range(2, 922) returns 2657
static unsigned int
_count_digits_in_range(int start, int end)
{
    int total_digits = 0;

    for (int i = start; i <= end; i++) {
        total_digits += _count_digits(i);
    }

    return total_digits;
}
