/*
 * windows.c
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

#include <string.h>

#include <glib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "common.h"
#include "roster_list.h"
#include "config/theme.h"
#include "ui/ui.h"
#include "ui/statusbar.h"
#include "ui/window.h"
#include "ui/windows.h"

static GHashTable *windows;
static int current;
static int max_cols;

void
wins_init(void)
{
    windows = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL,
        (GDestroyNotify)win_free);

    max_cols = getmaxx(stdscr);
    ProfWin *console = win_create_console();
    g_hash_table_insert(windows, GINT_TO_POINTER(1), console);

    current = 1;
}

ProfWin *
wins_get_console(void)
{
    return g_hash_table_lookup(windows, GINT_TO_POINTER(1));
}

ProfWin *
wins_get_current(void)
{
    if (windows != NULL) {
        return g_hash_table_lookup(windows, GINT_TO_POINTER(current));
    } else {
        return NULL;
    }
}

GList *
wins_get_nums(void)
{
    return g_hash_table_get_keys(windows);
}

void
wins_set_current_by_num(int i)
{
    if (g_hash_table_lookup(windows, GINT_TO_POINTER(i)) != NULL) {
        current = i;
    }
}

ProfWin *
wins_get_by_num(int i)
{
    return g_hash_table_lookup(windows, GINT_TO_POINTER(i));
}

ProfWin *
wins_get_next(void)
{
    // get and sort win nums
    GList *keys = g_hash_table_get_keys(windows);
    keys = g_list_sort(keys, cmp_win_num);
    GList *curr = keys;

    // find our place in the list
    while (curr != NULL) {
        if (current == GPOINTER_TO_INT(curr->data)) {
            break;
        }
        curr = g_list_next(curr);
    }

    // if there is a next window return it
    curr = g_list_next(curr);
    if (curr != NULL) {
        int next = GPOINTER_TO_INT(curr->data);
        g_list_free(keys);
        return wins_get_by_num(next);
    // otherwise return the first window (console)
    } else {
        g_list_free(keys);
        return wins_get_console();
    }
}

ProfWin *
wins_get_previous(void)
{
    // get and sort win nums
    GList *keys = g_hash_table_get_keys(windows);
    keys = g_list_sort(keys, cmp_win_num);
    GList *curr = keys;

    // find our place in the list
    while (curr != NULL) {
        if (current == GPOINTER_TO_INT(curr->data)) {
            break;
        }
        curr = g_list_next(curr);
    }

    // if there is a previous window return it
    curr = g_list_previous(curr);
    if (curr != NULL) {
        int previous = GPOINTER_TO_INT(curr->data);
        g_list_free(keys);
        return wins_get_by_num(previous);
    // otherwise return the last window
    } else {
        int new_num = GPOINTER_TO_INT(g_list_last(keys)->data);
        g_list_free(keys);
        return wins_get_by_num(new_num);
    }
}

ProfWin *
wins_get_by_recipient(const char * const recipient)
{
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr != NULL) {
        ProfWin *window = curr->data;
        if (g_strcmp0(window->from, recipient) == 0) {
            g_list_free(values);
            return window;
        }
        curr = g_list_next(curr);
    }

    g_list_free(values);
    return NULL;
}

ProfMucWin *
wins_get_muc_win(const char * const roomjid)
{
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr != NULL) {
        ProfWin *window = curr->data;
        if ((g_strcmp0(window->from, roomjid) == 0) && window->type == WIN_MUC) {
            g_list_free(values);
            return (ProfMucWin*)window;
        }
        curr = g_list_next(curr);
    }

    g_list_free(values);
    return NULL;
}

int
wins_get_num(ProfWin *window)
{
    GList *keys = g_hash_table_get_keys(windows);
    GList *curr = keys;

    while (curr != NULL) {
        gconstpointer num_p = curr->data;
        ProfWin *curr_win = g_hash_table_lookup(windows, num_p);
        if (g_strcmp0(curr_win->from, window->from) == 0) {
            g_list_free(keys);
            return GPOINTER_TO_INT(num_p);
        }
        curr = g_list_next(curr);
    }

    g_list_free(keys);
    return -1;
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
    // console cannot be closed
    if (i != 1) {

        // go to console if closing current window
        if (i == current) {
            current = 1;
            ProfWin *window = wins_get_current();
            win_update_virtual(window);
        }

        g_hash_table_remove(windows, GINT_TO_POINTER(i));
        status_bar_inactive(i);
    }
}

void
wins_clear_current(void)
{
    ProfWin *window = wins_get_current();
    werase(window->layout->win);
    win_update_virtual(window);
}

gboolean
wins_is_current(ProfWin *window)
{
    ProfWin *current_window = wins_get_current();

    if (g_strcmp0(current_window->from, window->from) == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

ProfWin *
wins_new_xmlconsole(void)
{
    GList *keys = g_hash_table_get_keys(windows);
    int result = get_next_available_win_num(keys);
    ProfWin *new = win_create_xmlconsole();
    g_hash_table_insert(windows, GINT_TO_POINTER(result), new);
    g_list_free(keys);
    return new;
}

ProfWin *
wins_new_chat(const char * const barejid)
{
    GList *keys = g_hash_table_get_keys(windows);
    int result = get_next_available_win_num(keys);
    ProfWin *new = win_create_chat(barejid);
    g_hash_table_insert(windows, GINT_TO_POINTER(result), new);
    g_list_free(keys);
    return new;
}

ProfWin *
wins_new_muc(const char * const roomjid)
{
    GList *keys = g_hash_table_get_keys(windows);
    int result = get_next_available_win_num(keys);
    ProfWin *new = win_create_muc(roomjid);
    g_hash_table_insert(windows, GINT_TO_POINTER(result), new);
    g_list_free(keys);
    return new;
}

ProfWin *
wins_new_muc_config(const char * const title, DataForm *form)
{
    GList *keys = g_hash_table_get_keys(windows);
    int result = get_next_available_win_num(keys);
    ProfWin *new = win_create_muc_config(title, form);
    g_hash_table_insert(windows, GINT_TO_POINTER(result), new);
    g_list_free(keys);
    return new;
}

ProfWin *
wins_new_private(const char * const fulljid)
{
    GList *keys = g_hash_table_get_keys(windows);
    int result = get_next_available_win_num(keys);
    ProfWin *new = win_create_private(fulljid);
    g_hash_table_insert(windows, GINT_TO_POINTER(result), new);
    g_list_free(keys);
    return new;
}

int
wins_get_total_unread(void)
{
    int result = 0;
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr != NULL) {
        ProfWin *window = curr->data;
        result += window->unread;
        curr = g_list_next(curr);
    }
    g_list_free(values);
    return result;
}

void
wins_resize_all(void)
{
    int cols = getmaxx(stdscr);

    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;
    while (curr != NULL) {
        ProfWin *window = curr->data;
        int subwin_cols = 0;

        if (window->layout->type == LAYOUT_SPLIT) {
            ProfLayoutSplit *layout = (ProfLayoutSplit*)window->layout;
            if (layout->subwin) {
                if (window->type == WIN_CONSOLE) {
                    subwin_cols = win_roster_cols();
                } else if (window->type == WIN_MUC) {
                    subwin_cols = win_occpuants_cols();
                }
                wresize(layout->super.win, PAD_SIZE, cols - subwin_cols);
                wresize(layout->subwin, PAD_SIZE, subwin_cols);
                rosterwin_roster();
            } else {
                wresize(layout->super.win, PAD_SIZE, cols);
            }
        } else {
            wresize(window->layout->win, PAD_SIZE, cols);
        }

        win_redraw(window);
        curr = g_list_next(curr);
    }
    g_list_free(values);

    ProfWin *current_win = wins_get_current();
    win_update_virtual(current_win);
}

void
wins_hide_subwin(ProfWin *window)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    win_hide_subwin(window);

    ProfWin *current_win = wins_get_current();
    if ((current_win->type == WIN_MUC) || (current_win->type == WIN_CONSOLE)) {
        pnoutrefresh(current_win->layout->win, current_win->layout->y_pos, 0, 1, 0, rows-3, cols-1);
    }
}

void
wins_show_subwin(ProfWin *window)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int subwin_cols = 0;

    win_show_subwin(window);

    ProfWin *current_win = wins_get_current();
    if (current_win->type == WIN_MUC) {
        ProfLayoutSplit *layout = (ProfLayoutSplit*)current_win->layout;
        subwin_cols = win_occpuants_cols();
        pnoutrefresh(layout->super.win, layout->super.y_pos, 0, 1, 0, rows-3, (cols-subwin_cols)-1);
        pnoutrefresh(layout->subwin, layout->sub_y_pos, 0, 1, (cols-subwin_cols), rows-3, cols-1);
    } else if (current_win->type == WIN_CONSOLE) {
        ProfLayoutSplit *layout = (ProfLayoutSplit*)current_win->layout;
        subwin_cols = win_roster_cols();
        pnoutrefresh(layout->super.win, layout->super.y_pos, 0, 1, 0, rows-3, (cols-subwin_cols)-1);
        pnoutrefresh(layout->subwin, layout->sub_y_pos, 0, 1, (cols-subwin_cols), rows-3, cols-1);
    }
}

gboolean
wins_xmlconsole_exists(void)
{
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr != NULL) {
        ProfWin *window = curr->data;
        if (window->type == WIN_XML) {
            g_list_free(values);
            return TRUE;
        }
        curr = g_list_next(curr);
    }

    g_list_free(values);
    return FALSE;
}

ProfWin *
wins_get_xmlconsole(void)
{
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr != NULL) {
        ProfWin *window = curr->data;
        if (window->type == WIN_XML) {
            g_list_free(values);
            return window;
        }
        curr = g_list_next(curr);
    }

    g_list_free(values);
    return NULL;
}

GSList *
wins_get_chat_recipients(void)
{
    GSList *result = NULL;
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr != NULL) {
        ProfWin *window = curr->data;
        if (window->type == WIN_CHAT) {
            result = g_slist_append(result, window->from);
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);
    return result;
}

GSList *
wins_get_prune_recipients(void)
{
    GSList *result = NULL;
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr != NULL) {
        ProfWin *window = curr->data;
        if (window->unread == 0 &&
                window->type != WIN_MUC &&
                window->type != WIN_MUC_CONFIG &&
                window->type != WIN_XML &&
                window->type != WIN_CONSOLE) {
            result = g_slist_append(result, window->from);
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);
    return result;
}

void
wins_lost_connection(void)
{
    GList *values = g_hash_table_get_values(windows);
    GList *curr = values;

    while (curr != NULL) {
        ProfWin *window = curr->data;
        if (window->type != WIN_CONSOLE) {
            win_save_print(window, '-', NULL, 0, THEME_ERROR, "", "Lost connection.");

            // if current win, set current_win_dirty
            if (wins_is_current(window)) {
                win_update_virtual(window);
            }
        }
        curr = g_list_next(curr);
    }
    g_list_free(values);
}

gboolean
wins_swap(int source_win, int target_win)
{
    ProfWin *source = g_hash_table_lookup(windows, GINT_TO_POINTER(source_win));

    if (source != NULL) {
        ProfWin *target = g_hash_table_lookup(windows, GINT_TO_POINTER(target_win));

        // target window empty
        if (target == NULL) {
            g_hash_table_steal(windows, GINT_TO_POINTER(source_win));
            status_bar_inactive(source_win);
            g_hash_table_insert(windows, GINT_TO_POINTER(target_win), source);
            if (source->unread > 0) {
                status_bar_new(target_win);
            } else {
                status_bar_active(target_win);
            }
            if ((wins_get_current_num() == source_win) || (wins_get_current_num() == target_win)) {
                ui_switch_win(1);
            }
            return TRUE;

        // target window occupied
        } else {
            g_hash_table_steal(windows, GINT_TO_POINTER(source_win));
            g_hash_table_steal(windows, GINT_TO_POINTER(target_win));
            g_hash_table_insert(windows, GINT_TO_POINTER(source_win), target);
            g_hash_table_insert(windows, GINT_TO_POINTER(target_win), source);
            if (source->unread > 0) {
                status_bar_new(target_win);
            } else {
                status_bar_active(target_win);
            }
            if (target->unread > 0) {
                status_bar_new(source_win);
            } else {
                status_bar_active(source_win);
            }
            if ((wins_get_current_num() == source_win) || (wins_get_current_num() == target_win)) {
                ui_switch_win(1);
            }
            return TRUE;
        }
    } else {
        return FALSE;
    }
}

gboolean
wins_tidy(void)
{
    gboolean tidy_required = FALSE;
    // check for gaps
    GList *keys = g_hash_table_get_keys(windows);
    keys = g_list_sort(keys, cmp_win_num);

    // get last used
    GList *last = g_list_last(keys);
    int last_num = GPOINTER_TO_INT(last->data);

    // find first free num TODO - Will sort again
    int next_available = get_next_available_win_num(keys);

    // found gap (next available before last window)
    if (cmp_win_num(GINT_TO_POINTER(next_available), GINT_TO_POINTER(last_num)) < 0) {
        tidy_required = TRUE;
    }

    if (tidy_required) {
        status_bar_set_all_inactive();
        GHashTable *new_windows = g_hash_table_new_full(g_direct_hash,
            g_direct_equal, NULL, (GDestroyNotify)win_free);

        int num = 1;
        GList *curr = keys;
        while (curr != NULL) {
            ProfWin *window = g_hash_table_lookup(windows, curr->data);
            if (num == 10) {
                g_hash_table_insert(new_windows, GINT_TO_POINTER(0), window);
                if (window->unread > 0) {
                    status_bar_new(0);
                } else {
                    status_bar_active(0);
                }
            } else {
                g_hash_table_insert(new_windows, GINT_TO_POINTER(num), window);
                if (window->unread > 0) {
                    status_bar_new(num);
                } else {
                    status_bar_active(num);
                }
            }
            num++;
            curr = g_list_next(curr);
        }

        windows = new_windows;
        current = 1;
        ui_switch_win(1);
        g_list_free(keys);
        return TRUE;
    } else {
        g_list_free(keys);
        return FALSE;
    }
}

GSList *
wins_create_summary(void)
{
    GSList *result = NULL;

    GList *keys = g_hash_table_get_keys(windows);
    keys = g_list_sort(keys, cmp_win_num);
    GList *curr = keys;

    while (curr != NULL) {
        ProfWin *window = g_hash_table_lookup(windows, curr->data);
        int ui_index = GPOINTER_TO_INT(curr->data);

        GString *chat_string;
        GString *priv_string;
        GString *muc_string;
        GString *muc_config_string;
        GString *xml_string;

        switch (window->type)
        {
            case WIN_CONSOLE:
                result = g_slist_append(result, strdup("1: Console"));
                break;
            case WIN_CHAT:
                chat_string = g_string_new("");

                PContact contact = roster_get_contact(window->from);
                if (contact == NULL) {
                    g_string_printf(chat_string, "%d: Chat %s", ui_index, window->from);
                } else {
                    const char *display_name = p_contact_name_or_jid(contact);
                    g_string_printf(chat_string, "%d: Chat %s", ui_index, display_name);
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

            case WIN_MUC_CONFIG:
                muc_config_string = g_string_new("");
                g_string_printf(muc_config_string, "%d: %s", ui_index, window->from);
                if (win_has_modified_form(window)) {
                    g_string_append(muc_config_string, " *");
                }
                result = g_slist_append(result, strdup(muc_config_string->str));
                g_string_free(muc_config_string, TRUE);

                break;

            case WIN_XML:
                xml_string = g_string_new("");
                g_string_printf(xml_string, "%d: XML console", ui_index);
                result = g_slist_append(result, strdup(xml_string->str));
                g_string_free(xml_string, TRUE);

                break;

            default:
                break;
        }
        curr = g_list_next(curr);
    }

    g_list_free(keys);
    return result;
}

void
wins_destroy(void)
{
    g_hash_table_destroy(windows);
}
