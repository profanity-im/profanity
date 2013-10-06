/*
 * core.c
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

#ifdef HAVE_GIT_VERSION
#include "gitversion.c"
#endif

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_LIBXSS
#include <X11/extensions/scrnsaver.h>
#endif
#include <glib.h>
#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "chat_session.h"
#include "command/command.h"
#include "common.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "contact.h"
#include "jid.h"
#include "log.h"
#include "muc.h"
#include "ui/notifier.h"
#include "ui/ui.h"
#include "ui/window.h"
#include "ui/windows.h"
#include "xmpp/xmpp.h"

static char *win_title;

#ifdef HAVE_LIBXSS
static Display *display;
#endif

static GTimer *ui_idle_time;

static void _win_show_user(WINDOW *win, const char * const user, const int colour);
static void _win_show_message(WINDOW *win, const char * const message);
static void _win_show_error_msg(WINDOW *win, const char * const message);
static void _win_handle_switch(const wint_t * const ch);
static void _win_handle_page(const wint_t * const ch);
static void _win_show_history(WINDOW *win, int win_index,
    const char * const contact);
static void _ui_draw_win_title(void);

void
ui_init(void)
{
    log_info("Initialising UI");
    initscr();
    raw();
    keypad(stdscr, TRUE);
    if (prefs_get_boolean(PREF_MOUSE)) {
        mousemask(ALL_MOUSE_EVENTS, NULL);
        mouseinterval(5);
    }
    ui_load_colours();
    refresh();
    create_title_bar();
    create_status_bar();
    status_bar_active(1);
    create_input_window();
    wins_init();
    cons_about();
    notifier_init();
#ifdef HAVE_LIBXSS
    display = XOpenDisplay(0);
#endif
    ui_idle_time = g_timer_new();
    wins_refresh_current();
}

void
ui_refresh(void)
{
    _ui_draw_win_title();
    title_bar_refresh();
    status_bar_refresh();
    inp_put_back();
}

unsigned long
ui_get_idle_time(void)
{
// if compiled with libxss, get the x sessions idle time
#ifdef HAVE_LIBXSS
    XScreenSaverInfo *info = XScreenSaverAllocInfo();
    if (info != NULL && display != NULL) {
        XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);
        unsigned long result = info->idle;
        XFree(info);
        return result;
    }
// if no libxss or xss idle time failed, use profanity idle time
#endif
    gdouble seconds_elapsed = g_timer_elapsed(ui_idle_time, NULL);
    unsigned long ms_elapsed = seconds_elapsed * 1000.0;
    return ms_elapsed;
}

void
ui_reset_idle_time(void)
{
    g_timer_start(ui_idle_time);
}

void
ui_close(void)
{
    notifier_uninit();
    wins_destroy();
    endwin();
}

void
ui_resize(const int ch, const char * const input, const int size)
{
    log_info("Resizing UI");
    title_bar_resize();
    status_bar_resize();
    wins_resize_all();
    inp_win_resize(input, size);
    wins_refresh_current();
}

void
ui_load_colours(void)
{
    if (has_colors()) {
        use_default_colors();
        start_color();
        theme_init_colours();
    }
}

gboolean
ui_win_exists(int index)
{
    ProfWin *window = wins_get_by_num(index);
    return (window != NULL);
}

gboolean
ui_duck_exists(void)
{
    return wins_duck_exists();
}

void
ui_contact_typing(const char * const barejid)
{
    ProfWin *window = wins_get_by_recipient(barejid);

    if (prefs_get_boolean(PREF_INTYPE)) {
        // no chat window for user
        if (window == NULL) {
            cons_show_typing(barejid);

        // have chat window but not currently in it
        } else if (!wins_is_current(window)) {
            cons_show_typing(barejid);
            wins_refresh_current();

        // in chat window with user
        } else {
            title_bar_set_typing(TRUE);
            title_bar_draw();

            int num = wins_get_num(window);
            status_bar_active(num);
            wins_refresh_current();
       }
    }

    if (prefs_get_boolean(PREF_NOTIFY_TYPING)) {
        PContact contact = roster_get_contact(barejid);
        char const *display_usr = NULL;
        if (p_contact_name(contact) != NULL) {
            display_usr = p_contact_name(contact);
        } else {
            display_usr = barejid;
        }
        notify_typing(display_usr);
    }
}

void
ui_idle(void)
{
    GSList *recipients = wins_get_chat_recipients();
    GSList *curr = recipients;
    while (curr != NULL) {
        char *recipient = curr->data;
        chat_session_no_activity(recipient);

        if (chat_session_is_gone(recipient) &&
                !chat_session_get_sent(recipient)) {
            message_send_gone(recipient);
        } else if (chat_session_is_inactive(recipient) &&
                !chat_session_get_sent(recipient)) {
            message_send_inactive(recipient);
        } else if (prefs_get_boolean(PREF_OUTTYPE) &&
                chat_session_is_paused(recipient) &&
                !chat_session_get_sent(recipient)) {
            message_send_paused(recipient);
        }

        curr = g_slist_next(curr);
    }

    if (recipients != NULL) {
        g_slist_free(recipients);
    }
}

void
ui_incoming_msg(const char * const from, const char * const message,
    GTimeVal *tv_stamp, gboolean priv)
{
    gboolean win_created = FALSE;
    char *display_from = NULL;
    win_type_t win_type;

    if (priv) {
        win_type = WIN_PRIVATE;
        display_from = get_nick_from_full_jid(from);
    } else {
        win_type = WIN_CHAT;
        PContact contact = roster_get_contact(from);
        if (contact != NULL) {
            if (p_contact_name(contact) != NULL) {
                display_from = strdup(p_contact_name(contact));
            } else {
                display_from = strdup(from);
            }
        } else {
            display_from = strdup(from);
        }
    }

    ProfWin *window = wins_get_by_recipient(from);
    if (window == NULL) {
        window = wins_new(from, win_type);
        win_created = TRUE;
    }

    int num = wins_get_num(window);

    // currently viewing chat window with sender
    if (wins_is_current(window)) {
        window->print_incoming_message(window, tv_stamp, display_from, message);
        title_bar_set_typing(FALSE);
        title_bar_draw();
        status_bar_active(num);
        wins_refresh_current();

    // not currently viewing chat window with sender
    } else {
        status_bar_new(num);
        cons_show_incoming_message(display_from, num);
        if (prefs_get_boolean(PREF_FLASH))
            flash();

        window->unread++;
        if (prefs_get_boolean(PREF_CHLOG) && prefs_get_boolean(PREF_HISTORY)) {
            _win_show_history(window->win, num, from);
        }

        // show users status first, when receiving message via delayed delivery
        if ((tv_stamp != NULL) && (win_created)) {
            PContact pcontact = roster_get_contact(from);
            if (pcontact != NULL) {
                win_show_contact(window, pcontact);
            }
        }

        window->print_incoming_message(window, tv_stamp, display_from, message);
    }

    int ui_index = num;
    if (ui_index == 10) {
        ui_index = 0;
    }

    if (prefs_get_boolean(PREF_BEEP))
        beep();
    if (prefs_get_boolean(PREF_NOTIFY_MESSAGE))
        notify_message(display_from, ui_index);

    free(display_from);
}

void
ui_roster_add(const char * const barejid, const char * const name)
{
    if (name != NULL) {
        cons_show("Roster item added: %s (%s)", barejid, name);
    } else {
        cons_show("Roster item added: %s", barejid);
    }
}

void
ui_roster_remove(const char * const barejid)
{
    cons_show("Roster item removed: %s", barejid);
}

void
ui_contact_already_in_group(const char * const contact, const char * const group)
{
    cons_show("%s already in group %s", contact, group);
}

void
ui_contact_not_in_group(const char * const contact, const char * const group)
{
    cons_show("%s is not currently in group %s", contact, group);
}

void
ui_group_added(const char * const contact, const char * const group)
{
    cons_show("%s added to group %s", contact, group);
}

void
ui_group_removed(const char * const contact, const char * const group)
{
    cons_show("%s removed from group %s", contact, group);
}

void
ui_handle_error_message(const char * const from, const char * const err_msg)
{
    if (err_msg == NULL) {
        cons_show_error("Unknown error received from service.");
    } else {
        ProfWin *current = wins_get_current();
        gboolean handled = current->handle_error_message(current, from, err_msg);
        if (handled != TRUE) {
            cons_show_error("Error received from server: %s", err_msg);
        }
    }

    ui_print_error_from_recipient(from, err_msg);
}

void
ui_contact_online(const char * const barejid, const char * const resource,
    const char * const show, const char * const status, GDateTime *last_activity)
{
    PContact contact = roster_get_contact(barejid);
    char *display_str = p_contact_create_display_string(contact, resource);

    ProfWin *console = wins_get_console();
    win_show_status_string(console, display_str, show, status, last_activity,
        "++", "online");

    ProfWin *window = wins_get_by_recipient(barejid);
    if (window != NULL) {
        win_show_status_string(window, display_str, show, status,
            last_activity, "++", "online");
    }

    free(display_str);

    if (wins_is_current(console)) {
        wins_refresh_current();
    } else if ((window != NULL) && (wins_is_current(window))) {
        wins_refresh_current();
    }
}

void
ui_contact_offline(const char * const from, const char * const show,
    const char * const status)
{
    Jid *jidp = jid_create(from);
    PContact contact = roster_get_contact(jidp->barejid);
    char *display_str = p_contact_create_display_string(contact, jidp->resourcepart);

    ProfWin *console = wins_get_console();
    win_show_status_string(console, display_str, show, status, NULL, "--",
        "offline");

    ProfWin *window = wins_get_by_recipient(jidp->barejid);
    if (window != NULL) {
        win_show_status_string(window, display_str, show, status, NULL, "--",
            "offline");
    }

    jid_destroy(jidp);
    free(display_str);

    if (wins_is_current(console)) {
        wins_refresh_current();
    } else if ((window != NULL) && (wins_is_current(window))) {
        wins_refresh_current();
    }
}

void
ui_disconnected(void)
{
    wins_lost_connection();
    title_bar_set_status(CONTACT_OFFLINE);
    status_bar_clear_message();
    status_bar_refresh();
}

void
ui_handle_special_keys(const wint_t * const ch, const char * const inp,
    const int size)
{
    _win_handle_switch(ch);
    _win_handle_page(ch);
    if (*ch == KEY_RESIZE) {
        ui_resize(*ch, inp, size);
    }

}

void
ui_close_connected_win(int index)
{
    win_type_t win_type = ui_win_type(index);
    if (win_type == WIN_MUC) {
        char *room_jid = ui_recipient(index);
        presence_leave_chat_room(room_jid);
    } else if ((win_type == WIN_CHAT) || (win_type == WIN_PRIVATE)) {

        if (prefs_get_boolean(PREF_STATES)) {
            char *recipient = ui_recipient(index);

            // send <gone/> chat state before closing
            if (chat_session_get_recipient_supports(recipient)) {
                chat_session_set_gone(recipient);
                message_send_gone(recipient);
                chat_session_end(recipient);
            }
        }
    }
}

int
ui_close_all_wins(void)
{
    int count = 0;
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    GList *win_nums = wins_get_nums();
    GList *curr = win_nums;

    while (curr != NULL) {
        int num = GPOINTER_TO_INT(curr->data);
        if (num != 1) {
            if (conn_status == JABBER_CONNECTED) {
                ui_close_connected_win(num);
            }
            ui_close_win(num);
            count++;
        }
        curr = g_list_next(curr);
    }

    g_list_free(curr);
    g_list_free(win_nums);

    return count;
}

int
ui_close_read_wins(void)
{
    int count = 0;
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    GList *win_nums = wins_get_nums();
    GList *curr = win_nums;

    while (curr != NULL) {
        int num = GPOINTER_TO_INT(curr->data);
        if ((num != 1) && (ui_win_unread(num) == 0)) {
            if (conn_status == JABBER_CONNECTED) {
                ui_close_connected_win(num);
            }
            ui_close_win(num);
            count++;
        }
        curr = g_list_next(curr);
    }

    g_list_free(curr);
    g_list_free(win_nums);

    return count;
}

void
ui_switch_win(const int i)
{
    ui_current_page_off();
    ProfWin *new_current = wins_get_by_num(i);
    if (new_current != NULL) {
        wins_set_current_by_num(i);
        ui_current_page_off();

        new_current->unread = 0;

        if (i == 1) {
            title_bar_title();
            status_bar_current(1);
            status_bar_active(1);
        } else {
            PContact contact = roster_get_contact(new_current->from);
            if (contact != NULL) {
                if (p_contact_name(contact) != NULL) {
                    title_bar_set_recipient(p_contact_name(contact));
                } else {
                    title_bar_set_recipient(new_current->from);
                }
            } else {
                title_bar_set_recipient(new_current->from);
            }
            title_bar_draw();
            status_bar_current(i);
            status_bar_active(i);
        }
        wins_refresh_current();
    }
}

void
ui_next_win(void)
{
    ui_current_page_off();
    ProfWin *new_current = wins_get_next();
    int i = wins_get_num(new_current);
    wins_set_current_by_num(i);
    ui_current_page_off();

    new_current->unread = 0;

    if (i == 1) {
        title_bar_title();
        status_bar_current(1);
        status_bar_active(1);
    } else {
        PContact contact = roster_get_contact(new_current->from);
        if (contact != NULL) {
            if (p_contact_name(contact) != NULL) {
                title_bar_set_recipient(p_contact_name(contact));
            } else {
                title_bar_set_recipient(new_current->from);
            }
        } else {
            title_bar_set_recipient(new_current->from);
        }
        title_bar_draw();
        status_bar_current(i);
        status_bar_active(i);
    }
    wins_refresh_current();
}

void
ui_previous_win(void)
{
    ui_current_page_off();
    ProfWin *new_current = wins_get_previous();
    int i = wins_get_num(new_current);
    wins_set_current_by_num(i);
    ui_current_page_off();

    new_current->unread = 0;

    if (i == 1) {
        title_bar_title();
        status_bar_current(1);
        status_bar_active(1);
    } else {
        PContact contact = roster_get_contact(new_current->from);
        if (contact != NULL) {
            if (p_contact_name(contact) != NULL) {
                title_bar_set_recipient(p_contact_name(contact));
            } else {
                title_bar_set_recipient(new_current->from);
            }
        } else {
            title_bar_set_recipient(new_current->from);
        }
        title_bar_draw();
        status_bar_current(i);
        status_bar_active(i);
    }
    wins_refresh_current();
}

void
ui_clear_current(void)
{
    wins_clear_current();
}

void
ui_close_current(void)
{
    int current_index = wins_get_current_num();
    status_bar_inactive(current_index);
    wins_close_current();
    status_bar_current(1);
    status_bar_active(1);
    title_bar_title();
}

void
ui_close_win(int index)
{
    wins_close_by_num(index);
    status_bar_current(1);
    status_bar_active(1);
    title_bar_title();

    wins_refresh_current();
}

void
ui_tidy_wins(void)
{
    gboolean tidied = wins_tidy();

    if (tidied) {
        cons_show("Windows tidied.");
    } else {
        cons_show("No tidy needed.");
    }
}

void
ui_prune_wins(void)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();
    gboolean pruned = FALSE;

    GSList *recipients = wins_get_prune_recipients();
    if (recipients != NULL) {
        pruned = TRUE;
    }
    GSList *curr = recipients;
    while (curr != NULL) {
        char *recipient = curr->data;

        if (conn_status == JABBER_CONNECTED) {
            if (prefs_get_boolean(PREF_STATES)) {

                // send <gone/> chat state before closing
                if (chat_session_get_recipient_supports(recipient)) {
                    chat_session_set_gone(recipient);
                    message_send_gone(recipient);
                    chat_session_end(recipient);
                }
            }
        }

        ProfWin *window = wins_get_by_recipient(recipient);
        int num = wins_get_num(window);
        ui_close_win(num);

        curr = g_slist_next(curr);
    }

    if (recipients != NULL) {
        g_slist_free(recipients);
    }

    wins_tidy();
    if (pruned) {
        cons_show("Windows pruned.");
    } else {
        cons_show("No prune needed.");
    }
}

win_type_t
ui_current_win_type(void)
{
    ProfWin *current = wins_get_current();
    return current->type;
}

int
ui_current_win_index(void)
{
    return wins_get_current_num();
}

win_type_t
ui_win_type(int index)
{
    ProfWin *window = wins_get_by_num(index);
    return window->type;
}

char *
ui_recipient(int index)
{
    ProfWin *window = wins_get_by_num(index);
    return window->from;
}

char *
ui_current_recipient(void)
{
    ProfWin *current = wins_get_current();
    return current->from;
}

void
ui_current_print_line(const char * const msg, ...)
{
    ProfWin *current = wins_get_current();
    va_list arg;
    va_start(arg, msg);
    win_print_line(current, '-', 0, msg, arg);
    va_end(arg);
    win_refresh(current);
}

void
ui_current_error_line(const char * const msg)
{
    ProfWin *current = wins_get_current();
    win_print_line(current, '-', COLOUR_ERROR, msg);
    win_refresh(current);
}

void
ui_current_page_off(void)
{
    ProfWin *current = wins_get_current();
    current->paged = 0;

    int rows = getmaxy(stdscr);
    int y = getcury(current->win);
    int size = rows - 3;

    current->y_pos = y - (size - 1);
    if (current->y_pos < 0) {
        current->y_pos = 0;
    }

    wins_refresh_current();
}

void
ui_print_error_from_recipient(const char * const from, const char *err_msg)
{
    if (from == NULL || err_msg == NULL)
        return;

    ProfWin *window = wins_get_by_recipient(from);
    if (window != NULL) {
        win_print_time(window, '-');
        _win_show_error_msg(window->win, err_msg);
        if (wins_is_current(window)) {
            wins_refresh_current();
        }
    }
}

void
ui_print_system_msg_from_recipient(const char * const from, const char *message)
{
    int num = 0;
    char from_cpy[strlen(from) + 1];
    char *bare_jid;

    if (from == NULL || message == NULL)
        return;

    strcpy(from_cpy, from);
    bare_jid = strtok(from_cpy, "/");

    ProfWin *window = wins_get_by_recipient(bare_jid);
    if (window == NULL) {
        window = wins_new(bare_jid, WIN_CHAT);
        if (window != NULL) {
            num = wins_get_num(window);
            status_bar_active(num);
        } else {
            num = 0;
            window = wins_get_console();
            status_bar_active(1);
        }
    }

    win_print_time(window, '-');
    wprintw(window->win, "*%s %s\n", bare_jid, message);

    // this is the current window
    if (wins_is_current(window)) {
        wins_refresh_current();
    }
}

void
ui_recipient_gone(const char * const barejid)
{
    if (barejid == NULL)
        return;

    PContact contact = roster_get_contact(barejid);
    const char * display_usr = NULL;
    if (p_contact_name(contact) != NULL) {
        display_usr = p_contact_name(contact);
    } else {
        display_usr = barejid;
    }

    ProfWin *window = wins_get_by_recipient(barejid);
    if (window != NULL) {
        win_print_time(window, '!');
        wattron(window->win, COLOUR_GONE);
        wprintw(window->win, "<- %s ", display_usr);
        wprintw(window->win, "has left the conversation.");
        wprintw(window->win, "\n");
        wattroff(window->win, COLOUR_GONE);
        if (wins_is_current(window)) {
            wins_refresh_current();
        }
    }
}

void
ui_new_chat_win(const char * const to)
{
    // if the contact is offline, show a message
    PContact contact = roster_get_contact(to);
    ProfWin *window = wins_get_by_recipient(to);
    int num = 0;

    // create new window
    if (window == NULL) {
        Jid *jid = jid_create(to);

        if (muc_room_is_active(jid)) {
            window = wins_new(to, WIN_PRIVATE);
        } else {
            window = wins_new(to, WIN_CHAT);
        }

        jid_destroy(jid);
        num = wins_get_num(window);

        if (prefs_get_boolean(PREF_CHLOG) && prefs_get_boolean(PREF_HISTORY)) {
            _win_show_history(window->win, num, to);
        }

        if (contact != NULL) {
            if (strcmp(p_contact_presence(contact), "offline") == 0) {
                const char const *show = p_contact_presence(contact);
                const char const *status = p_contact_status(contact);
                win_show_status_string(window, to, show, status, NULL, "--", "offline");
            }
        }
    } else {
        num = wins_get_num(window);
    }

    ui_switch_win(num);
}

void
ui_create_duck_win(void)
{
    ProfWin *window = wins_new("DuckDuckGo search", WIN_DUCK);
    int num = wins_get_num(window);
    ui_switch_win(num);
    win_print_time(window, '-');
    wprintw(window->win, "Type ':help' to find out more.\n");
}

void
ui_open_duck_win(void)
{
    ProfWin *window = wins_get_by_recipient("DuckDuckGo search");
    if (window != NULL) {
        int num = wins_get_num(window);
        ui_switch_win(num);
    }
}

void
ui_duck(const char * const query)
{
    ProfWin *window = wins_get_by_recipient("DuckDuckGo search");
    if (window != NULL) {
        win_print_time(window, '-');
        wprintw(window->win, "\n");
        win_print_time(window, '-');
        wattron(window->win, COLOUR_ME);
        wprintw(window->win, "Query  : ");
        wattroff(window->win, COLOUR_ME);
        wprintw(window->win, query);
        wprintw(window->win, "\n");
    }
}

void
ui_duck_result(const char * const result)
{
    ProfWin *window = wins_get_by_recipient("DuckDuckGo search");

    if (window != NULL) {
        win_print_time(window, '-');
        wattron(window->win, COLOUR_THEM);
        wprintw(window->win, "Result : ");
        wattroff(window->win, COLOUR_THEM);

        glong offset = 0;
        while (offset < g_utf8_strlen(result, -1)) {
            gchar *ptr = g_utf8_offset_to_pointer(result, offset);
            gunichar unichar = g_utf8_get_char(ptr);
            if (unichar == '\n') {
                wprintw(window->win, "\n");
                win_print_time(window, '-');
            } else {
                gchar *string = g_ucs4_to_utf8(&unichar, 1, NULL, NULL, NULL);
                if (string != NULL) {
                    wprintw(window->win, string);
                    g_free(string);
                }
            }

            offset++;
        }

        wprintw(window->win, "\n");
    }
}

void
ui_outgoing_msg(const char * const from, const char * const to,
    const char * const message)
{
    PContact contact = roster_get_contact(to);
    ProfWin *window = wins_get_by_recipient(to);
    int num = 0;

    // create new window
    if (window == NULL) {
        Jid *jid = jid_create(to);

        if (muc_room_is_active(jid)) {
            window = wins_new(to, WIN_PRIVATE);
        } else {
            window = wins_new(to, WIN_CHAT);
        }

        jid_destroy(jid);
        num = wins_get_num(window);

        if (prefs_get_boolean(PREF_CHLOG) && prefs_get_boolean(PREF_HISTORY)) {
            _win_show_history(window->win, num, to);
        }

        if (contact != NULL) {
            if (strcmp(p_contact_presence(contact), "offline") == 0) {
                const char const *show = p_contact_presence(contact);
                const char const *status = p_contact_status(contact);
                win_show_status_string(window, to, show, status, NULL, "--", "offline");
            }
        }

    // use existing window
    } else {
        num = wins_get_num(window);
    }

    win_print_time(window, '-');
    if (strncmp(message, "/me ", 4) == 0) {
        wattron(window->win, COLOUR_ME);
        wprintw(window->win, "*%s ", from);
        wprintw(window->win, "%s", message + 4);
        wprintw(window->win, "\n");
        wattroff(window->win, COLOUR_ME);
    } else {
        _win_show_user(window->win, from, 0);
        _win_show_message(window->win, message);
    }
    ui_switch_win(num);
}

void
ui_room_join(Jid *jid)
{
    ProfWin *window = wins_get_by_recipient(jid->barejid);
    int num = 0;

    // create new window
    if (window == NULL) {
        window = wins_new(jid->barejid, WIN_MUC);
    }

    num = wins_get_num(window);
    ui_switch_win(num);
}

void
ui_room_roster(const char * const room, GList *roster, const char * const presence)
{
    ProfWin *window = wins_get_by_recipient(room);

    win_print_time(window, '!');
    if ((roster == NULL) || (g_list_length(roster) == 0)) {
        wattron(window->win, COLOUR_ROOMINFO);
        if (presence == NULL) {
            wprintw(window->win, "Room is empty.\n");
        } else {
            wprintw(window->win, "No participants %s.\n", presence);
        }
        wattroff(window->win, COLOUR_ROOMINFO);
    } else {
        int length = g_list_length(roster);
        wattron(window->win, COLOUR_ROOMINFO);
        if (presence == NULL) {
            length++;
            wprintw(window->win, "%d participants: ", length);
            wattroff(window->win, COLOUR_ROOMINFO);
            wattron(window->win, COLOUR_ONLINE);
            wprintw(window->win, "%s", muc_get_room_nick(room));
            wprintw(window->win, ", ");
        } else {
            wprintw(window->win, "%d %s: ", length, presence);
            wattroff(window->win, COLOUR_ROOMINFO);
            wattron(window->win, COLOUR_ONLINE);
        }

        while (roster != NULL) {
            PContact member = roster->data;
            const char const *nick = p_contact_barejid(member);
            const char const *show = p_contact_presence(member);

            win_presence_colour_on(window, show);
            wprintw(window->win, "%s", nick);
            win_presence_colour_off(window, show);

            if (roster->next != NULL) {
                wprintw(window->win, ", ");
            }

            roster = g_list_next(roster);
        }

        wprintw(window->win, "\n");
        wattroff(window->win, COLOUR_ONLINE);
    }

    if (wins_is_current(window)) {
        wins_refresh_current();
    }
}

void
ui_room_member_offline(const char * const room, const char * const nick)
{
    ProfWin *window = wins_get_by_recipient(room);

    win_print_time(window, '!');
    wattron(window->win, COLOUR_OFFLINE);
    wprintw(window->win, "<- %s has left the room.\n", nick);
    wattroff(window->win, COLOUR_OFFLINE);

    if (wins_is_current(window)) {
        wins_refresh_current();
    }
}

void
ui_room_member_online(const char * const room, const char * const nick,
    const char * const show, const char * const status)
{
    ProfWin *window = wins_get_by_recipient(room);

    win_print_time(window, '!');
    wattron(window->win, COLOUR_ONLINE);
    wprintw(window->win, "-> %s has joined the room.\n", nick);
    wattroff(window->win, COLOUR_ONLINE);

    if (wins_is_current(window)) {
        wins_refresh_current();
    }
}

void
ui_room_member_presence(const char * const room, const char * const nick,
    const char * const show, const char * const status)
{
    ProfWin *window = wins_get_by_recipient(room);

    if (window != NULL) {
        win_show_status_string(window, nick, show, status, NULL, "++", "online");
    }

    if (wins_is_current(window)) {
        wins_refresh_current();
    }
}

void
ui_room_member_nick_change(const char * const room,
    const char * const old_nick, const char * const nick)
{
    ProfWin *window = wins_get_by_recipient(room);

    win_print_time(window, '!');
    wattron(window->win, COLOUR_THEM);
    wprintw(window->win, "** %s is now known as %s\n", old_nick, nick);
    wattroff(window->win, COLOUR_THEM);

    if (wins_is_current(window)) {
        wins_refresh_current();
    }
}

void
ui_room_nick_change(const char * const room, const char * const nick)
{
    ProfWin *window = wins_get_by_recipient(room);

    win_print_time(window, '!');
    wattron(window->win, COLOUR_ME);
    wprintw(window->win, "** You are now known as %s\n", nick);
    wattroff(window->win, COLOUR_ME);

    if (wins_is_current(window)) {
        wins_refresh_current();
    }
}

void
ui_room_history(const char * const room_jid, const char * const nick,
    GTimeVal tv_stamp, const char * const message)
{
    ProfWin *window = wins_get_by_recipient(room_jid);

    GDateTime *time = g_date_time_new_from_timeval_utc(&tv_stamp);
    gchar *date_fmt = g_date_time_format(time, "%H:%M:%S");
    wprintw(window->win, "%s - ", date_fmt);
    g_date_time_unref(time);
    g_free(date_fmt);

    if (strncmp(message, "/me ", 4) == 0) {
        wprintw(window->win, "*%s ", nick);
        waddstr(window->win, message + 4);
        wprintw(window->win, "\n");
    } else {
        wprintw(window->win, "%s: ", nick);
        _win_show_message(window->win, message);
    }

    if (wins_is_current(window)) {
        wins_refresh_current();
    }
}

void
ui_room_message(const char * const room_jid, const char * const nick,
    const char * const message)
{
    ProfWin *window = wins_get_by_recipient(room_jid);
    int num = wins_get_num(window);

    win_print_time(window, '-');
    if (strcmp(nick, muc_get_room_nick(room_jid)) != 0) {
        if (strncmp(message, "/me ", 4) == 0) {
            wattron(window->win, COLOUR_THEM);
            wprintw(window->win, "*%s ", nick);
            waddstr(window->win, message + 4);
            wprintw(window->win, "\n");
            wattroff(window->win, COLOUR_THEM);
        } else {
            _win_show_user(window->win, nick, 1);
            _win_show_message(window->win, message);
        }

    } else {
        if (strncmp(message, "/me ", 4) == 0) {
            wattron(window->win, COLOUR_ME);
            wprintw(window->win, "*%s ", nick);
            waddstr(window->win, message + 4);
            wprintw(window->win, "\n");
            wattroff(window->win, COLOUR_ME);
        } else {
            _win_show_user(window->win, nick, 0);
            _win_show_message(window->win, message);
        }
    }

    // currently in groupchat window
    if (wins_is_current(window)) {
        status_bar_active(num);
        wins_refresh_current();

    // not currenlty on groupchat window
    } else {
        status_bar_new(num);
        cons_show_incoming_message(nick, num);
        if (wins_get_current_num() == 0) {
            wins_refresh_current();
        }

        if (strcmp(nick, muc_get_room_nick(room_jid)) != 0) {
            if (prefs_get_boolean(PREF_FLASH)) {
                flash();
            }
        }

        window->unread++;
    }

    int ui_index = num;
    if (ui_index == 10) {
        ui_index = 0;
    }

    if (strcmp(nick, muc_get_room_nick(room_jid)) != 0) {
        if (prefs_get_boolean(PREF_BEEP)) {
            beep();
        }
        if (prefs_get_boolean(PREF_NOTIFY_MESSAGE)) {
            Jid *jidp = jid_create(room_jid);
            notify_room_message(nick, jidp->localpart, ui_index);
            jid_destroy(jidp);
        }
    }
}

void
ui_room_subject(const char * const room_jid, const char * const subject)
{
    ProfWin *window = wins_get_by_recipient(room_jid);
    int num = wins_get_num(window);

    win_print_time(window, '!');
    wattron(window->win, COLOUR_ROOMINFO);
    wprintw(window->win, "Room subject: ");
    wattroff(window->win, COLOUR_ROOMINFO);
    wprintw(window->win, "%s\n", subject);

    // currently in groupchat window
    if (wins_is_current(window)) {
        status_bar_active(num);
        wins_refresh_current();

    // not currenlty on groupchat window
    } else {
        status_bar_new(num);
    }
}

void
ui_room_broadcast(const char * const room_jid, const char * const message)
{
    ProfWin *window = wins_get_by_recipient(room_jid);
    int num = wins_get_num(window);

    win_print_time(window, '!');
    wattron(window->win, COLOUR_ROOMINFO);
    wprintw(window->win, "Room message: ");
    wattroff(window->win, COLOUR_ROOMINFO);
    wprintw(window->win, "%s\n", message);

    // currently in groupchat window
    if (wins_is_current(window)) {
        status_bar_active(num);
        wins_refresh_current();

    // not currenlty on groupchat window
    } else {
        status_bar_new(num);
    }
}

void
ui_status(void)
{
    char *recipient = ui_current_recipient();
    PContact pcontact = roster_get_contact(recipient);
    ProfWin *current = wins_get_current();

    if (pcontact != NULL) {
        win_show_contact(current, pcontact);
    } else {
        ui_current_print_line("Error getting contact info.");
    }
}

void
ui_status_private(void)
{
    Jid *jid = jid_create(ui_current_recipient());
    PContact pcontact = muc_get_participant(jid->barejid, jid->resourcepart);
    ProfWin *current = wins_get_current();

    if (pcontact != NULL) {
        win_show_contact(current, pcontact);
    } else {
        ui_current_print_line("Error getting contact info.");
    }

    jid_destroy(jid);
}

void
ui_status_room(const char * const contact)
{
    PContact pcontact = muc_get_participant(ui_current_recipient(), contact);
    ProfWin *current = wins_get_current();

    if (pcontact != NULL) {
        win_show_contact(current, pcontact);
    } else {
        ui_current_print_line("No such participant \"%s\" in room.", contact);
    }
}

gint
ui_unread(void)
{
    return wins_get_total_unread();
}

int
ui_win_unread(int index)
{
    ProfWin *window = wins_get_by_num(index);
    if (window != NULL) {
        return window->unread;
    } else {
        return 0;
    }
}

static void
_ui_draw_win_title(void)
{
    char new_win_title[100];

    GString *version_str = g_string_new("");

    if (prefs_get_boolean(PREF_TITLEBARVERSION)) {
        g_string_append(version_str, " ");
        g_string_append(version_str, PACKAGE_VERSION);
        if (strcmp(PACKAGE_STATUS, "development") == 0) {
#ifdef HAVE_GIT_VERSION
            g_string_append(version_str, "dev.");
            g_string_append(version_str, PROF_GIT_BRANCH);
            g_string_append(version_str, ".");
            g_string_append(version_str, PROF_GIT_REVISION);
#else
            g_string_append(version_str, "dev");
#endif
        }
    }

    jabber_conn_status_t status = jabber_get_connection_status();

    if (status == JABBER_CONNECTED) {
        const char * const jid = jabber_get_fulljid();
        gint unread = ui_unread();

        if (unread != 0) {
            snprintf(new_win_title, sizeof(new_win_title), "%c]0;%s%s (%d) - %s%c", '\033', "Profanity", version_str->str, unread, jid, '\007');
        } else {
            snprintf(new_win_title, sizeof(new_win_title), "%c]0;%s%s - %s%c", '\033', "Profanity", version_str->str, jid, '\007');
        }
    } else {
        snprintf(new_win_title, sizeof(new_win_title), "%c]0;%s%s%c", '\033', "Profanity", version_str->str, '\007');
    }

    g_string_free(version_str, TRUE);

    if (g_strcmp0(win_title, new_win_title) != 0) {
        // print to x-window title bar
        printf("%s", new_win_title);
        if (win_title != NULL) {
            free(win_title);
        }
        win_title = strdup(new_win_title);
    }
}

static void
_win_show_user(WINDOW *win, const char * const user, const int colour)
{
    if (colour)
        wattron(win, COLOUR_THEM);
    else
        wattron(win, COLOUR_ME);
    wprintw(win, "%s: ", user);
    if (colour)
        wattroff(win, COLOUR_THEM);
    else
        wattroff(win, COLOUR_ME);
}

static void
_win_show_message(WINDOW *win, const char * const message)
{
    waddstr(win, message);
    wprintw(win, "\n");
}

static void
_win_show_error_msg(WINDOW *win, const char * const message)
{
    wattron(win, COLOUR_ERROR);
    wprintw(win, "%s\n", message);
    wattroff(win, COLOUR_ERROR);
}

static void
_win_handle_switch(const wint_t * const ch)
{
    if (*ch == KEY_F(1)) {
        ui_switch_win(1);
    } else if (*ch == KEY_F(2)) {
        ui_switch_win(2);
    } else if (*ch == KEY_F(3)) {
        ui_switch_win(3);
    } else if (*ch == KEY_F(4)) {
        ui_switch_win(4);
    } else if (*ch == KEY_F(5)) {
        ui_switch_win(5);
    } else if (*ch == KEY_F(6)) {
        ui_switch_win(6);
    } else if (*ch == KEY_F(7)) {
        ui_switch_win(7);
    } else if (*ch == KEY_F(8)) {
        ui_switch_win(8);
    } else if (*ch == KEY_F(9)) {
        ui_switch_win(9);
    } else if (*ch == KEY_F(10)) {
        ui_switch_win(0);
    }
}

static void
_win_handle_page(const wint_t * const ch)
{
    ProfWin *current = wins_get_current();
    int rows = getmaxy(stdscr);
    int y = getcury(current->win);

    int page_space = rows - 4;
    int *page_start = &(current->y_pos);

    if (prefs_get_boolean(PREF_MOUSE)) {
        MEVENT mouse_event;

        if (*ch == KEY_MOUSE) {
            if (getmouse(&mouse_event) == OK) {

#ifdef PLATFORM_CYGWIN
                if (mouse_event.bstate & BUTTON5_PRESSED) { // mouse wheel down
#else
                if (mouse_event.bstate & BUTTON2_PRESSED) { // mouse wheel down
#endif
                    *page_start += 4;

                    // only got half a screen, show full screen
                    if ((y - (*page_start)) < page_space)
                        *page_start = y - page_space;

                    // went past end, show full screen
                    else if (*page_start >= y)
                        *page_start = y - page_space;

                    current->paged = 1;
                    wins_refresh_current();
                } else if (mouse_event.bstate & BUTTON4_PRESSED) { // mouse wheel up
                    *page_start -= 4;

                    // went past beginning, show first page
                    if (*page_start < 0)
                        *page_start = 0;

                    current->paged = 1;
                    wins_refresh_current();
                }
            }
        }
    }

    // page up
    if (*ch == KEY_PPAGE) {
        *page_start -= page_space;

        // went past beginning, show first page
        if (*page_start < 0)
            *page_start = 0;

        current->paged = 1;
        wins_refresh_current();

    // page down
    } else if (*ch == KEY_NPAGE) {
        *page_start += page_space;

        // only got half a screen, show full screen
        if ((y - (*page_start)) < page_space)
            *page_start = y - page_space;

        // went past end, show full screen
        else if (*page_start >= y)
            *page_start = y - page_space;

        current->paged = 1;
        wins_refresh_current();
    }
}

static void
_win_show_history(WINDOW *win, int win_index, const char * const contact)
{
    ProfWin *window = wins_get_by_num(win_index);
    if (!window->history_shown) {
        GSList *history = NULL;
        Jid *jid = jid_create(jabber_get_fulljid());
        history = chat_log_get_previous(jid->barejid, contact, history);
        jid_destroy(jid);
        while (history != NULL) {
            wprintw(win, "%s\n", history->data);
            history = g_slist_next(history);
        }
        window->history_shown = 1;

        g_slist_free_full(history, free);
    }
}
