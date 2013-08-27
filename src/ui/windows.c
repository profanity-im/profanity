/*
 * windows.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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

#include <string.h>

#include <glib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "common.h"
#include "config/theme.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "ui/windows.h"

#define NUM_WINS 10
#define CONS_WIN_TITLE "_cons"

static ProfWin* windows[NUM_WINS];
static int current;
static int max_cols;

//static GHashTable *windows;

void
wins_init(void)
{
    /*
    windows = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL,
        window_free);

    int cols = getmaxx(stdscr);
    ProfWin *console = win_create(CONS_WIN_TITLE, cols, WIN_CONSOLE);
    g_hash_table_insert(GINT_TO_POINTER(1), console);

    current = console;
*/
    int i;
    for (i = 0; i < NUM_WINS; i++) {
        windows[i] = NULL;
    }

    max_cols = getmaxx(stdscr);
    int cols = getmaxx(stdscr);
    windows[0] = win_create(CONS_WIN_TITLE, cols, WIN_CONSOLE);
    current = 0;
}

ProfWin *
wins_get_console(void)
{
    return windows[0];
//    return g_hash_table_lookup(windows, GINT_TO_POINTER(1));
}

ProfWin *
wins_get_current(void)
{
    return windows[current];
}

void
wins_set_current_by_num(int i)
{
    if (i < NUM_WINS) {
        current = i;
    }
}

ProfWin *
wins_get_by_num(int i)
{
    if (i >= NUM_WINS) {
        return NULL;
    } else {
        return windows[i];
    }
}

ProfWin *
wins_get_by_recipient(const char * const recipient)
{
    int i = 0;
    for (i = 1; i < NUM_WINS; i++) {
        if ((windows[i] != NULL) && g_strcmp0(windows[i]->from, recipient) == 0) {
            return windows[i];
        }
    }

    return NULL;
}

int
wins_get_num(ProfWin *window)
{
    int i = 0;
    for (i = 0; i < NUM_WINS; i++) {
        if ((windows[i] != NULL) && g_strcmp0(windows[i]->from, window->from) == 0) {
            return i;
        }
    }

    return 0;
}

int
wins_get_current_num(void)
{
    return current;
}

void
wins_close_current(void)
{
    wins_close_by_num(current);
}

void
wins_close_by_num(int i)
{
    if (i > 0 && i < NUM_WINS) {
        win_free(windows[i]);
        windows[i] = NULL;
        if (i == current) {
            current = 0;
            wins_refresh_current();
        }
    }
}

void
wins_refresh_current(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    prefresh(windows[current]->win, windows[current]->y_pos, 0, 1, 0, rows-3, cols-1);
}

void
wins_clear_current(void)
{
    werase(windows[current]->win);
    wins_refresh_current();
}

gboolean
wins_is_current(ProfWin *window)
{
    if (g_strcmp0(windows[current]->from, window->from) == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

ProfWin *
wins_new(const char * const from, win_type_t type)
{
    int i;
    for (i = 1; i < NUM_WINS; i++) {
        if (windows[i] == NULL) {
            break;
        }
    }

    if (i != NUM_WINS) {
        int cols = getmaxx(stdscr);
        windows[i] = win_create(from, cols, type);
        return windows[i];
    } else {
        return NULL;
    }
}

int
wins_get_total_unread(void)
{
    int i;
    int result = 0;
    for (i = 0; i < NUM_WINS; i++) {
        if (windows[i] != NULL) {
            result += windows[i]->unread;
        }
    }
    return result;
}

void
wins_resize_all(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // only make the pads bigger, to avoid data loss on cropping
    if (cols > max_cols) {
        max_cols = cols;

        int i;
        for (i = 0; i < NUM_WINS; i++) {
            if (windows[i] != NULL) {
                wresize(windows[i]->win, PAD_SIZE, cols);
            }
        }
    }

    prefresh(windows[current]->win, windows[current]->y_pos, 0, 1, 0, rows-3, cols-1);
}

void
wins_refresh_console(void)
{
    if (current == 0) {
        wins_refresh_current();
    }
}

gboolean
wins_full(void)
{
    int i;
    for (i = 1; i < NUM_WINS; i++) {
        if (windows[i] == NULL) {
            return FALSE;
        }
    }

    return TRUE;
}

gboolean
wins_duck_exists(void)
{
    int i;
    for (i = 1; i < NUM_WINS; i++) {
        if (windows[i] != NULL) {
            if (windows[i]->type == WIN_DUCK)
                return TRUE;
        }
    }

    return FALSE;
}

GSList *
wins_get_chat_recipients(void)
{
    GSList *result = NULL;
    int i;
    for (i = 1; i < NUM_WINS; i++) {
        if (windows[i] != NULL && windows[i]->type == WIN_CHAT) {
            result = g_slist_append(result, windows[i]->from);
        }
    }
    return result;
}

GSList *
wins_get_prune_recipients(void)
{
    GSList *result = NULL;
    int i;
    for (i = 1; i < NUM_WINS; i++) {
        if (windows[i] != NULL && windows[i]->unread == 0 && windows[i]->type != WIN_MUC) {
            result = g_slist_append(result, windows[i]->from);
        }
    }
    return result;

}

void
wins_lost_connection(void)
{
    int i;
    // show message in all active chats
    for (i = 1; i < NUM_WINS; i++) {
        if (windows[i] != NULL) {
            ProfWin *window = windows[i];
            win_print_time(window, '-');
            wattron(window->win, COLOUR_ERROR);
            wprintw(window->win, "%s\n", "Lost connection.");
            wattroff(window->win, COLOUR_ERROR);

            // if current win, set current_win_dirty
            if (wins_is_current(window)) {
                wins_refresh_current();
            }
        }
    }
}

gboolean
wins_tidy(void)
{
    int gap = 1;
    int filler = 1;
    gboolean tidied = FALSE;

    for (gap = 1; gap < NUM_WINS; gap++) {
        // if a gap
        if (windows[gap] == NULL) {

            // find next used window and move into gap
            for (filler = gap + 1; filler < NUM_WINS; filler++) {
                if (windows[filler] != NULL) {
                    windows[gap] = windows[filler];
                    if (windows[gap]->unread > 0) {
                        status_bar_new(gap);
                    } else {
                        status_bar_active(gap);
                    }
                    windows[filler] = NULL;
                    status_bar_inactive(filler);
                    tidied = TRUE;
                    break;
                }
            }
        }
    }

    return tidied;
}

GSList *
wins_create_summary(void)
{
    GSList *result = NULL;
    result = g_slist_append(result, strdup("1: Console"));
    int count = 0;
    int ui_index = 0;

    int i;
    for (i = 1; i < NUM_WINS; i++) {
        if (windows[i] != NULL) {
            count++;
        }
    }

    if (count != 0) {
        for (i = 1; i < NUM_WINS; i++) {
            if (windows[i] != NULL) {
                ProfWin *window = windows[i];
                ui_index = i + 1;
                if (ui_index == 10) {
                    ui_index = 0;
                }

                GString *chat_string;
                GString *priv_string;
                GString *muc_string;
                GString *duck_string;

                switch (window->type)
                {
                    case WIN_CHAT:
                        chat_string = g_string_new("");
                        g_string_printf(chat_string, "%d: Chat %s", ui_index, window->from);
                        PContact contact = roster_get_contact(window->from);

                        if (contact != NULL) {
                            if (p_contact_name(contact) != NULL) {
                                GString *chat_name = g_string_new("");
                                g_string_printf(chat_name, " (%s)", p_contact_name(contact));
                                g_string_append(chat_string, chat_name->str);
                                g_string_free(chat_name, TRUE);
                            }
                            GString *chat_presence = g_string_new("");
                            g_string_printf(chat_presence, " - %s", p_contact_presence(contact));
                            g_string_append(chat_string, chat_presence->str);
                            g_string_free(chat_presence, TRUE);
                        }

                        if (window->unread > 0) {
                            GString *chat_unread = g_string_new("");
                            g_string_printf(chat_unread, ", %d unread", window->unread);
                            g_string_append(chat_string, chat_unread->str);
                            g_string_free(chat_unread, TRUE);
                        }

                        result = g_slist_append(result, strdup(chat_string->str));
                        g_string_free(chat_string, TRUE);

                        break;

                    case WIN_PRIVATE:
                        priv_string = g_string_new("");
                        g_string_printf(priv_string, "%d: Private %s", ui_index, window->from);

                        if (window->unread > 0) {
                            GString *priv_unread = g_string_new("");
                            g_string_printf(priv_unread, ", %d unread", window->unread);
                            g_string_append(priv_string, priv_unread->str);
                            g_string_free(priv_unread, TRUE);
                        }

                        result = g_slist_append(result, strdup(priv_string->str));
                        g_string_free(priv_string, TRUE);

                        break;

                    case WIN_MUC:
                        muc_string = g_string_new("");
                        g_string_printf(muc_string, "%d: Room %s", ui_index, window->from);

                        if (window->unread > 0) {
                            GString *muc_unread = g_string_new("");
                            g_string_printf(muc_unread, ", %d unread", window->unread);
                            g_string_append(muc_string, muc_unread->str);
                            g_string_free(muc_unread, TRUE);
                        }

                        result = g_slist_append(result, strdup(muc_string->str));
                        g_string_free(muc_string, TRUE);

                        break;

                    case WIN_DUCK:
                        duck_string = g_string_new("");
                        g_string_printf(duck_string, "%d: DuckDuckGo search", ui_index);
                        result = g_slist_append(result, strdup(duck_string->str));
                        g_string_free(duck_string, TRUE);

                        break;

                    default:
                        break;
                }
            }
        }
    }

    return result;
}

void
wins_destroy(void)
{
    int i;
    for (i = 0; i < NUM_WINS; i++) {
        if (windows[i] != NULL) {
            win_free(windows[i]);
        }
    }
}
