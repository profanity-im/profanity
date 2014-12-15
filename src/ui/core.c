/*
 * core.c
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

#ifdef HAVE_GIT_VERSION
#include "gitversion.h"
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
#include "roster_list.h"
#include "jid.h"
#include "log.h"
#include "muc.h"
#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif
#include "ui/ui.h"
#include "ui/titlebar.h"
#include "ui/statusbar.h"
#include "ui/inputwin.h"
#include "ui/window.h"
#include "ui/windows.h"
#include "xmpp/xmpp.h"

static char *win_title;

#ifdef HAVE_LIBXSS
static Display *display;
#endif

static GTimer *ui_idle_time;

static void _win_handle_switch(const wint_t * const ch);
static void _win_handle_page(const wint_t * const ch, const int result);
static void _win_show_history(int win_index, const char * const contact);
static void _ui_draw_term_title(void);
static void _ui_roster_contact(PContact contact);

static void
_ui_init(void)
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
#ifdef HAVE_LIBXSS
    display = XOpenDisplay(0);
#endif
    ui_idle_time = g_timer_new();
    ProfWin *window = wins_get_current();
    win_update_virtual(window);
}

static void
_ui_update(void)
{
    ProfWin *current = wins_get_current();
    if (current->paged == 0) {
        win_move_to_end(current);
    }

    win_update_virtual(current);

    if (prefs_get_boolean(PREF_TITLEBAR)) {
        _ui_draw_term_title();
    }
    title_bar_update_virtual();
    status_bar_update_virtual();
    inp_put_back();
    doupdate();
}

static void
_ui_about(void)
{
    cons_show("");
    cons_about();
}

static unsigned long
_ui_get_idle_time(void)
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
    if (info != NULL) {
        XFree(info);
    }
// if no libxss or xss idle time failed, use profanity idle time
#endif
    gdouble seconds_elapsed = g_timer_elapsed(ui_idle_time, NULL);
    unsigned long ms_elapsed = seconds_elapsed * 1000.0;
    return ms_elapsed;
}

static void
_ui_reset_idle_time(void)
{
    g_timer_start(ui_idle_time);
}

static void
_ui_close(void)
{
    notifier_uninit();
    wins_destroy();
    endwin();
}

static wint_t
_ui_get_char(char *input, int *size, int *result)
{
    wint_t ch = inp_get_char(input, size, result);
    if (ch != ERR) {
        ui_reset_idle_time();
    }
    return ch;
}

static void
_ui_input_clear(void)
{
    inp_win_reset();
}

static void
_ui_replace_input(char *input, const char * const new_input, int *size)
{
    inp_replace_input(input, new_input, size);
}

static void
_ui_input_nonblocking(void)
{
    inp_non_block();
}

static void
_ui_resize(void)
{
    log_info("Resizing UI");
    erase();
    refresh();
    title_bar_resize();
    wins_resize_all();
    status_bar_resize();
    inp_win_resize();
    ProfWin *window = wins_get_current();
    win_update_virtual(window);
}

static void
_ui_redraw(void)
{
    title_bar_resize();
    wins_resize_all();
    status_bar_resize();
    inp_win_resize();
}

static void
_ui_load_colours(void)
{
    if (has_colors()) {
        use_default_colors();
        start_color();
        theme_init_colours();
    }
}

static gboolean
_ui_win_exists(int index)
{
    ProfWin *window = wins_get_by_num(index);
    return (window != NULL);
}

static gboolean
_ui_xmlconsole_exists(void)
{
    return wins_xmlconsole_exists();
}

static void
_ui_handle_stanza(const char * const msg)
{
    if (ui_xmlconsole_exists()) {
        ProfWin *xmlconsole = wins_get_xmlconsole();

        if (g_str_has_prefix(msg, "SENT:")) {
            win_save_print(xmlconsole, '-', NULL, 0, 0, "", "SENT:");
            win_save_print(xmlconsole, '-', NULL, 0, THEME_ONLINE, "", &msg[6]);
            win_save_print(xmlconsole, '-', NULL, 0, THEME_ONLINE, "", "");
        } else if (g_str_has_prefix(msg, "RECV:")) {
            win_save_print(xmlconsole, '-', NULL, 0, 0, "", "RECV:");
            win_save_print(xmlconsole, '-', NULL, 0, THEME_AWAY, "", &msg[6]);
            win_save_print(xmlconsole, '-', NULL, 0, THEME_AWAY, "", "");
        }
    }
}

static void
_ui_contact_typing(const char * const barejid)
{
    ProfWin *window = wins_get_by_recipient(barejid);

    if (prefs_get_boolean(PREF_INTYPE)) {
        // no chat window for user
        if (window == NULL) {
            cons_show_typing(barejid);

        // have chat window but not currently in it
        } else if (!wins_is_current(window)) {
            cons_show_typing(barejid);

        // in chat window with user
        } else {
            title_bar_set_typing(TRUE);

            int num = wins_get_num(window);
            status_bar_active(num);
       }
    }

    if (prefs_get_boolean(PREF_NOTIFY_TYPING)) {
        gboolean is_current = FALSE;
        if (window != NULL) {
            is_current = wins_is_current(window);
        }
        if ( !is_current || (is_current && prefs_get_boolean(PREF_NOTIFY_TYPING_CURRENT)) ) {
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
}

static GSList *
_ui_get_recipients(void)
{
    GSList *recipients = wins_get_chat_recipients();
    return recipients;
}

static void
_ui_incoming_msg(const char * const barejid, const char * const message, GTimeVal *tv_stamp)
{
    gboolean win_created = FALSE;
    char *display_from = NULL;

    PContact contact = roster_get_contact(barejid);
    if (contact != NULL) {
        if (p_contact_name(contact) != NULL) {
            display_from = strdup(p_contact_name(contact));
        } else {
            display_from = strdup(barejid);
        }
    } else {
        display_from = strdup(barejid);
    }

    ProfWin *window = wins_get_by_recipient(barejid);
    if (window == NULL) {
        window = wins_new_chat(barejid);
#ifdef HAVE_LIBOTR
        if (otr_is_secure(barejid)) {
            window->wins.chat.is_otr = TRUE;
        }
#endif
        win_created = TRUE;
    }

    int num = wins_get_num(window);

    // currently viewing chat window with sender
    if (wins_is_current(window)) {
        win_print_incoming_message(window, tv_stamp, display_from, message);
        title_bar_set_typing(FALSE);
        status_bar_active(num);

    // not currently viewing chat window with sender
    } else {
        status_bar_new(num);
        cons_show_incoming_message(display_from, num);

        if (prefs_get_boolean(PREF_FLASH)) {
            flash();
        }

        window->unread++;
        if (prefs_get_boolean(PREF_CHLOG) && prefs_get_boolean(PREF_HISTORY)) {
            _win_show_history(num, barejid);
        }

        // show users status first, when receiving message via delayed delivery
        if ((tv_stamp != NULL) && (win_created)) {
            PContact pcontact = roster_get_contact(barejid);
            if (pcontact != NULL) {
                win_show_contact(window, pcontact);
            }
        }

        win_print_incoming_message(window, tv_stamp, display_from, message);
    }

    int ui_index = num;
    if (ui_index == 10) {
        ui_index = 0;
    }

    if (prefs_get_boolean(PREF_BEEP)) {
        beep();
    }

    if (prefs_get_boolean(PREF_NOTIFY_MESSAGE)) {
        gboolean is_current = wins_is_current(window);
        if ( !is_current || (is_current && prefs_get_boolean(PREF_NOTIFY_MESSAGE_CURRENT)) ) {
            if (prefs_get_boolean(PREF_NOTIFY_MESSAGE_TEXT)) {
                notify_message(display_from, ui_index, message);
            } else {
                notify_message(display_from, ui_index, NULL);
            }
        }
    }

    free(display_from);
}

static void
_ui_incoming_private_msg(const char * const fulljid, const char * const message, GTimeVal *tv_stamp)
{
    char *display_from = NULL;
    display_from = get_nick_from_full_jid(fulljid);

    ProfWin *window = wins_get_by_recipient(fulljid);
    if (window == NULL) {
        window = wins_new_private(fulljid);
    }

    int num = wins_get_num(window);

    // currently viewing chat window with sender
    if (wins_is_current(window)) {
        win_print_incoming_message(window, tv_stamp, display_from, message);
        title_bar_set_typing(FALSE);
        status_bar_active(num);

    // not currently viewing chat window with sender
    } else {
        status_bar_new(num);
        cons_show_incoming_message(display_from, num);

        if (prefs_get_boolean(PREF_FLASH)) {
            flash();
        }

        window->unread++;
        if (prefs_get_boolean(PREF_CHLOG) && prefs_get_boolean(PREF_HISTORY)) {
            _win_show_history(num, fulljid);
        }

        win_print_incoming_message(window, tv_stamp, display_from, message);
    }

    int ui_index = num;
    if (ui_index == 10) {
        ui_index = 0;
    }

    if (prefs_get_boolean(PREF_BEEP)) {
        beep();
    }

    if (prefs_get_boolean(PREF_NOTIFY_MESSAGE)) {
        gboolean is_current = wins_is_current(window);
        if ( !is_current || (is_current && prefs_get_boolean(PREF_NOTIFY_MESSAGE_CURRENT)) ) {
            if (prefs_get_boolean(PREF_NOTIFY_MESSAGE_TEXT)) {
                notify_message(display_from, ui_index, message);
            } else {
                notify_message(display_from, ui_index, NULL);
            }
        }
    }

    free(display_from);
}

static void
_ui_roster_add(const char * const barejid, const char * const name)
{
    if (name != NULL) {
        cons_show("Roster item added: %s (%s)", barejid, name);
    } else {
        cons_show("Roster item added: %s", barejid);
    }
    ui_roster();
}

static void
_ui_roster_remove(const char * const barejid)
{
    cons_show("Roster item removed: %s", barejid);
    ui_roster();
}

static void
_ui_contact_already_in_group(const char * const contact, const char * const group)
{
    cons_show("%s already in group %s", contact, group);
    ui_roster();
}

static void
_ui_contact_not_in_group(const char * const contact, const char * const group)
{
    cons_show("%s is not currently in group %s", contact, group);
    ui_roster();
}

static void
_ui_group_added(const char * const contact, const char * const group)
{
    cons_show("%s added to group %s", contact, group);
    ui_roster();
}

static void
_ui_group_removed(const char * const contact, const char * const group)
{
    cons_show("%s removed from group %s", contact, group);
    ui_roster();
}

static void
_ui_auto_away(void)
{
    char *pref_autoaway_message = prefs_get_string(PREF_AUTOAWAY_MESSAGE);
    if (pref_autoaway_message != NULL) {
        int pri =
            accounts_get_priority_for_presence_type(jabber_get_account_name(),
                RESOURCE_AWAY);
        cons_show("Idle for %d minutes, status set to away (priority %d), \"%s\".",
            prefs_get_autoaway_time(), pri, pref_autoaway_message);
        title_bar_set_presence(CONTACT_AWAY);
    } else {
        int pri =
            accounts_get_priority_for_presence_type(jabber_get_account_name(),
                RESOURCE_AWAY);
        cons_show("Idle for %d minutes, status set to away (priority %d).",
            prefs_get_autoaway_time(), pri);
        title_bar_set_presence(CONTACT_AWAY);
    }
    prefs_free_string(pref_autoaway_message);
}

static void
_ui_end_auto_away(void)
{
    int pri =
        accounts_get_priority_for_presence_type(jabber_get_account_name(), RESOURCE_ONLINE);
    cons_show("No longer idle, status set to online (priority %d).", pri);
    title_bar_set_presence(CONTACT_ONLINE);
}

static void
_ui_titlebar_presence(contact_presence_t presence)
{
    title_bar_set_presence(presence);
}

static void
_ui_handle_login_account_success(ProfAccount *account)
{
    resource_presence_t resource_presence = accounts_get_login_presence(account->name);
    contact_presence_t contact_presence = contact_presence_from_resource_presence(resource_presence);
    cons_show_login_success(account);
    title_bar_set_presence(contact_presence);
    status_bar_print_message(account->jid);
    status_bar_update_virtual();
}

static void
_ui_update_presence(const resource_presence_t resource_presence,
    const char * const message, const char * const show)
{
    contact_presence_t contact_presence = contact_presence_from_resource_presence(resource_presence);
    title_bar_set_presence(contact_presence);
    gint priority = accounts_get_priority_for_presence_type(jabber_get_account_name(), resource_presence);
    if (message != NULL) {
        cons_show("Status set to %s (priority %d), \"%s\".", show, priority, message);
    } else {
        cons_show("Status set to %s (priority %d).", show, priority);
    }
}

static void
_ui_handle_recipient_not_found(const char * const recipient, const char * const err_msg)
{
    ProfWin *win = wins_get_by_recipient(recipient);
    GString *msg = g_string_new("");

    // no window for intended recipient, show message in current and console
    if (win == NULL) {
        g_string_printf(msg, "Recipient %s not found: %s", recipient, err_msg);
        cons_show_error(msg->str);

    // intended recipient was invalid chat room
    } else if (win->type == WIN_MUC) {
        g_string_printf(msg, "Room %s not found: %s", recipient, err_msg);
        cons_show_error(msg->str);
        win_save_print(win, '!', NULL, 0, THEME_ERROR, "", msg->str);

    // unknown chat recipient
    } else {
        g_string_printf(msg, "Recipient %s not found: %s", recipient, err_msg);
        cons_show_error(msg->str);
        win_save_print(win, '!', NULL, 0, THEME_ERROR, "", msg->str);
    }

    g_string_free(msg, TRUE);
}

static void
_ui_handle_recipient_error(const char * const recipient, const char * const err_msg)
{
    ProfWin *win = wins_get_by_recipient(recipient);
    GString *msg = g_string_new("");
    g_string_printf(msg, "Error from %s: %s", recipient, err_msg);

    // always show in console
    cons_show_error(msg->str);

    // show in window if exists for recipient
    if (win != NULL)  {
        win_save_print(win, '!', NULL, 0, THEME_ERROR, "", msg->str);
    }

    g_string_free(msg, TRUE);
}

static void
_ui_handle_error(const char * const err_msg)
{
    GString *msg = g_string_new("");
    g_string_printf(msg, "Error %s", err_msg);

    cons_show_error(msg->str);

    g_string_free(msg, TRUE);
}

static void
_ui_invalid_command_usage(const char * const usage, void (**setting_func)(void))
{
    if (setting_func != NULL) {
        cons_show("");
        (*setting_func)();
        cons_show("Usage: %s", usage);
    } else {
        cons_show("");
        cons_show("Usage: %s", usage);
        if (ui_current_win_type() == WIN_CHAT) {
            char usage_cpy[strlen(usage) + 8];
            sprintf(usage_cpy, "Usage: %s", usage);
            ui_current_print_line(usage_cpy);
        }
    }
}

static void
_ui_disconnected(void)
{
    wins_lost_connection();
    title_bar_set_presence(CONTACT_OFFLINE);
    status_bar_clear_message();
    status_bar_update_virtual();
    ui_hide_roster();
}

static void
_ui_handle_special_keys(const wint_t * const ch, const int result)
{
    _win_handle_switch(ch);
    _win_handle_page(ch, result);
    if (*ch == KEY_RESIZE) {
        ui_resize();
    }
}

static void
_ui_close_connected_win(int index)
{
    win_type_t win_type = ui_win_type(index);
    if (win_type == WIN_MUC) {
        char *room_jid = ui_recipient(index);
        presence_leave_chat_room(room_jid);
    } else if ((win_type == WIN_CHAT) || (win_type == WIN_PRIVATE)) {
#ifdef HAVE_LIBOTR
        ProfWin *window = wins_get_by_num(index);
        if (win_is_otr(window)) {
            otr_end_session(window->from);
        }
#endif
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

static int
_ui_close_all_wins(void)
{
    int count = 0;
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    GList *win_nums = wins_get_nums();
    GList *curr = win_nums;

    while (curr != NULL) {
        int num = GPOINTER_TO_INT(curr->data);
        if ((num != 1) && (!ui_win_has_unsaved_form(num))) {
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

static int
_ui_close_read_wins(void)
{
    int count = 0;
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    GList *win_nums = wins_get_nums();
    GList *curr = win_nums;

    while (curr != NULL) {
        int num = GPOINTER_TO_INT(curr->data);
        if ((num != 1) && (ui_win_unread(num) == 0) && (!ui_win_has_unsaved_form(num))) {
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

static void
_ui_redraw_all_room_rosters(void)
{
    GList *win_nums = wins_get_nums();
    GList *curr = win_nums;

    while (curr != NULL) {
        int num = GPOINTER_TO_INT(curr->data);
        ProfWin *window = wins_get_by_num(num);
        if (window->type == WIN_MUC && window->wins.muc.subwin) {
            char *room = window->from;
            ui_muc_roster(room);
        }
        curr = g_list_next(curr);
    }

    g_list_free(curr);
    g_list_free(win_nums);
}

static void
_ui_hide_all_room_rosters(void)
{
    GList *win_nums = wins_get_nums();
    GList *curr = win_nums;

    while (curr != NULL) {
        int num = GPOINTER_TO_INT(curr->data);
        ProfWin *window = wins_get_by_num(num);
        if (window->type == WIN_MUC && window->wins.muc.subwin) {
            char *room = window->from;
            ui_room_hide_occupants(room);
        }
        curr = g_list_next(curr);
    }

    g_list_free(curr);
    g_list_free(win_nums);
}

static void
_ui_show_all_room_rosters(void)
{
    GList *win_nums = wins_get_nums();
    GList *curr = win_nums;

    while (curr != NULL) {
        int num = GPOINTER_TO_INT(curr->data);
        ProfWin *window = wins_get_by_num(num);
        if (window->type == WIN_MUC && window->wins.muc.subwin == NULL) {
            char *room = window->from;
            ui_room_show_occupants(room);
        }
        curr = g_list_next(curr);
    }

    g_list_free(curr);
    g_list_free(win_nums);
}

static gboolean
_ui_win_has_unsaved_form(int num)
{
    ProfWin *window = wins_get_by_num(num);

    if (window->type != WIN_MUC_CONFIG) {
        return FALSE;
    }
    if (window->wins.conf.form == NULL) {
        return FALSE;
    }

    return window->wins.conf.form->modified;
}

GString *
_get_recipient_string(ProfWin *window)
{
    GString *result = g_string_new("");
    PContact contact = roster_get_contact(window->from);
    if (contact != NULL) {
        if (p_contact_name(contact) != NULL) {
            g_string_append(result, p_contact_name(contact));
        } else {
            g_string_append(result, window->from);
        }
    } else {
        g_string_append(result, window->from);
    }

    return result;
}

static gboolean
_ui_switch_win(const int i)
{
    if (ui_win_exists(i)) {
        ProfWin *old_current = wins_get_current();
        if (old_current->type == WIN_MUC_CONFIG) {
            cmd_autocomplete_remove_form_fields(old_current->wins.conf.form);
        }

        ProfWin *new_current = wins_get_by_num(i);
        if (new_current->type == WIN_MUC_CONFIG) {
            cmd_autocomplete_add_form_fields(new_current->wins.conf.form);
        }

        wins_set_current_by_num(i);

        new_current->unread = 0;

        if (i == 1) {
            title_bar_console();
            status_bar_current(1);
            status_bar_active(1);
        } else {
            GString *recipient_str = _get_recipient_string(new_current);
            title_bar_set_recipient(recipient_str->str);
            g_string_free(recipient_str, TRUE);
            status_bar_current(i);
            status_bar_active(i);
        }
        return TRUE;
    } else {
        return FALSE;
    }
}

static void
_ui_previous_win(void)
{
    ProfWin *old_current = wins_get_current();
    if (old_current->type == WIN_MUC_CONFIG) {
        cmd_autocomplete_remove_form_fields(old_current->wins.conf.form);
    }

    ProfWin *new_current = wins_get_previous();
    if (new_current->type == WIN_MUC_CONFIG) {
        cmd_autocomplete_add_form_fields(new_current->wins.conf.form);
    }

    int i = wins_get_num(new_current);
    wins_set_current_by_num(i);

    new_current->unread = 0;

    if (i == 1) {
        title_bar_console();
        status_bar_current(1);
        status_bar_active(1);
    } else {
        GString *recipient_str = _get_recipient_string(new_current);
        title_bar_set_recipient(recipient_str->str);
        g_string_free(recipient_str, TRUE);
        status_bar_current(i);
        status_bar_active(i);
    }
}

static void
_ui_next_win(void)
{
    ProfWin *old_current = wins_get_current();
    if (old_current->type == WIN_MUC_CONFIG) {
        cmd_autocomplete_remove_form_fields(old_current->wins.conf.form);
    }

    ProfWin *new_current = wins_get_next();
    if (new_current->type == WIN_MUC_CONFIG) {
        cmd_autocomplete_add_form_fields(new_current->wins.conf.form);
    }

    int i = wins_get_num(new_current);
    wins_set_current_by_num(i);

    new_current->unread = 0;

    if (i == 1) {
        title_bar_console();
        status_bar_current(1);
        status_bar_active(1);
    } else {
        GString *recipient_str = _get_recipient_string(new_current);
        title_bar_set_recipient(recipient_str->str);
        g_string_free(recipient_str, TRUE);
        status_bar_current(i);
        status_bar_active(i);
    }
}

static void
_ui_gone_secure(const char * const barejid, gboolean trusted)
{
    ProfWin *window = wins_get_by_recipient(barejid);
    if (window == NULL) {
        window = wins_new_chat(barejid);
    }

    if (window->type != WIN_CHAT) {
        return;
    }

    FREE_SET_NULL(window->wins.chat.resource);

    window->wins.chat.is_otr = TRUE;
    window->wins.chat.is_trusted = trusted;
    if (trusted) {
        win_save_print(window, '!', NULL, 0, THEME_OTR_STARTED_TRUSTED, "", "OTR session started (trusted).");
    } else {
        win_save_print(window, '!', NULL, 0, THEME_OTR_STARTED_UNTRUSTED, "", "OTR session started (untrusted).");
    }

    if (wins_is_current(window)) {
        GString *recipient_str = _get_recipient_string(window);
        title_bar_set_recipient(recipient_str->str);
        g_string_free(recipient_str, TRUE);
    } else {
        int num = wins_get_num(window);
        status_bar_new(num);

        int ui_index = num;
        if (ui_index == 10) {
            ui_index = 0;
        }
        cons_show("%s started an OTR session (%d).", barejid, ui_index);
        cons_alert();
    }
}

static void
_ui_smp_recipient_initiated(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_save_vprint(window, '!', NULL, 0, 0, "", "%s wants to authenticate your identity, use '/otr secret <secret>'.", recipient);
    }
}

static void
_ui_smp_recipient_initiated_q(const char * const recipient, const char *question)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_save_vprint(window, '!', NULL, 0, 0, "", "%s wants to authenticate your identity with the following question:", recipient);
        win_save_vprint(window, '!', NULL, 0, 0, "", "  %s", question);
        win_save_print(window, '!', NULL, 0, 0, "", "use '/otr answer <answer>'.");
    }
}

static void
_ui_smp_unsuccessful_sender(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_save_vprint(window, '!', NULL, 0, 0, "", "Authentication failed, the secret you entered does not match the secret entered by %s.", recipient);
    }
}

static void
_ui_smp_unsuccessful_receiver(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_save_vprint(window, '!', NULL, 0, 0, "", "Authentication failed, the secret entered by %s does not match yours.", recipient);
    }
}

static void
_ui_smp_aborted(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_save_print(window, '!', NULL, 0, 0, "", "SMP session aborted.");
    }
}

static void
_ui_smp_successful(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_save_print(window, '!', NULL, 0, 0, "", "Authentication successful.");
    }
}

static void
_ui_smp_answer_success(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_save_vprint(window, '!', NULL, 0, 0, "", "%s successfully authenticated you.", recipient);
    }
}

static void
_ui_smp_answer_failure(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_save_vprint(window, '!', NULL, 0, 0, "", "%s failed to authenticate you.", recipient);
    }
}

static void
_ui_gone_insecure(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window != NULL) {
        if (window->type != WIN_CHAT) {
            return;
        }
        window->wins.chat.is_otr = FALSE;
        window->wins.chat.is_trusted = FALSE;
        win_save_print(window, '!', NULL, 0, THEME_OTR_ENDED, "", "OTR session ended.");

        if (wins_is_current(window)) {
            GString *recipient_str = _get_recipient_string(window);
            title_bar_set_recipient(recipient_str->str);
            g_string_free(recipient_str, TRUE);
        }
    }
}

static void
_ui_trust(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window != NULL) {
        if (window->type != WIN_CHAT) {
            return;
        }
        window->wins.chat.is_otr = TRUE;
        window->wins.chat.is_trusted = TRUE;
        win_save_print(window, '!', NULL, 0, THEME_OTR_TRUSTED, "", "OTR session trusted.");

        if (wins_is_current(window)) {
            GString *recipient_str = _get_recipient_string(window);
            title_bar_set_recipient(recipient_str->str);
            g_string_free(recipient_str, TRUE);
        }
    }
}

static void
_ui_untrust(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window != NULL) {
        if (window->type != WIN_CHAT) {
            return;
        }
        window->wins.chat.is_otr = TRUE;
        window->wins.chat.is_trusted = FALSE;
        win_save_print(window, '!', NULL, 0, THEME_OTR_UNTRUSTED, "", "OTR session untrusted.");

        if (wins_is_current(window)) {
            GString *recipient_str = _get_recipient_string(window);
            title_bar_set_recipient(recipient_str->str);
            g_string_free(recipient_str, TRUE);
        }
    }
}

static void
_ui_clear_current(void)
{
    wins_clear_current();
}

static void
_ui_close_current(void)
{
    int current_index = wins_get_current_num();
    status_bar_inactive(current_index);
    wins_close_current();
    title_bar_console();
    status_bar_current(1);
    status_bar_active(1);
}

static void
_ui_close_win(int index)
{
    ProfWin *window = wins_get_by_num(index);
    if (window) {
        if (window->type == WIN_MUC_CONFIG && window->wins.conf.form) {
            cmd_autocomplete_remove_form_fields(window->wins.conf.form);
        }
    }

    wins_close_by_num(index);
    title_bar_console();
    status_bar_current(1);
    status_bar_active(1);
}

static void
_ui_tidy_wins(void)
{
    gboolean tidied = wins_tidy();

    if (tidied) {
        cons_show("Windows tidied.");
    } else {
        cons_show("No tidy needed.");
    }
}

static void
_ui_prune_wins(void)
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

static gboolean
_ui_swap_wins(int source_win, int target_win)
{
    return wins_swap(source_win, target_win);
}

static win_type_t
_ui_current_win_type(void)
{
    ProfWin *current = wins_get_current();
    return current->type;
}

static gboolean
_ui_current_win_is_otr(void)
{
    ProfWin *current = wins_get_current();
    return win_is_otr(current);
}

static void
_ui_current_set_otr(gboolean value)
{
    ProfWin *current = wins_get_current();
    if (current->type == WIN_CHAT) {
        current->wins.chat.is_otr = value;
    }
}

static void
_ui_otr_authenticating(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_save_vprint(window, '!', NULL, 0, 0, "", "Authenticating %s...", recipient);
    }
}

static void
_ui_otr_authetication_waiting(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_save_vprint(window, '!', NULL, 0, 0, "", "Awaiting authentication from %s...", recipient);
    }
}

static int
_ui_current_win_index(void)
{
    return wins_get_current_num();
}

static win_type_t
_ui_win_type(int index)
{
    ProfWin *window = wins_get_by_num(index);
    return window->type;
}

static char *
_ui_recipient(int index)
{
    ProfWin *window = wins_get_by_num(index);
    return window->from;
}

static char *
_ui_current_recipient(void)
{
    ProfWin *current = wins_get_current();
    return current->from;
}

static void
_ui_current_print_line(const char * const msg, ...)
{
    ProfWin *window = wins_get_current();
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    win_save_println(window, fmt_msg->str);
    va_end(arg);
    g_string_free(fmt_msg, TRUE);
}

static void
_ui_current_print_formatted_line(const char show_char, int attrs, const char * const msg, ...)
{
    ProfWin *current = wins_get_current();
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    win_save_print(current, show_char, NULL, 0, attrs, "", fmt_msg->str);
    va_end(arg);
    g_string_free(fmt_msg, TRUE);
}

static void
_ui_current_error_line(const char * const msg)
{
    ProfWin *current = wins_get_current();
    win_save_print(current, '-', NULL, 0, THEME_ERROR, "", msg);
}

static void
_ui_print_system_msg_from_recipient(const char * const barejid, const char *message)
{
    if (barejid == NULL || message == NULL)
        return;

    ProfWin *window = wins_get_by_recipient(barejid);
    if (window == NULL) {
        int num = 0;
        window = wins_new_chat(barejid);
        if (window != NULL) {
            num = wins_get_num(window);
            status_bar_active(num);
        } else {
            num = 0;
            window = wins_get_console();
            status_bar_active(1);
        }
    }

    win_save_vprint(window, '-', NULL, 0, 0, "", "*%s %s", barejid, message);
}

static void
_ui_recipient_gone(const char * const barejid)
{
    if (barejid == NULL)
        return;

    const char * display_usr = NULL;
    PContact contact = roster_get_contact(barejid);
    if (contact != NULL) {
        if (p_contact_name(contact) != NULL) {
            display_usr = p_contact_name(contact);
        } else {
            display_usr = barejid;
        }
    } else {
        display_usr = barejid;
    }

    ProfWin *window = wins_get_by_recipient(barejid);
    if (window != NULL) {
        win_save_vprint(window, '!', NULL, 0, THEME_GONE, "", "<- %s has left the conversation.", display_usr);
    }
}

static void
_ui_new_chat_win(const char * const to)
{
    // if the contact is offline, show a message
    PContact contact = roster_get_contact(to);
    ProfWin *window = wins_get_by_recipient(to);
    int num = 0;

    // create new window
    if (window == NULL) {
        Jid *jid = jid_create(to);

        if (muc_active(jid->barejid)) {
            window = wins_new_private(to);
        } else {
            window = wins_new_chat(to);
        }

        jid_destroy(jid);
        num = wins_get_num(window);

        if (prefs_get_boolean(PREF_CHLOG) && prefs_get_boolean(PREF_HISTORY)) {
            _win_show_history(num, to);
        }

        if (contact != NULL) {
            if (strcmp(p_contact_presence(contact), "offline") == 0) {
                const char * const show = p_contact_presence(contact);
                const char * const status = p_contact_status(contact);
                win_show_status_string(window, to, show, status, NULL, "--", "offline");
            }
        }
    } else {
        num = wins_get_num(window);
    }

    ui_switch_win(num);
}

static void
_ui_create_xmlconsole_win(void)
{
    ProfWin *window = wins_new_xmlconsole();
    int num = wins_get_num(window);
    ui_switch_win(num);
}

static void
_ui_open_xmlconsole_win(void)
{
    ProfWin *window = wins_get_by_recipient("XML Console");
    if (window != NULL) {
        int num = wins_get_num(window);
        ui_switch_win(num);
    }
}

static void
_ui_outgoing_msg(const char * const from, const char * const to,
    const char * const message)
{
    PContact contact = roster_get_contact(to);
    ProfWin *window = wins_get_by_recipient(to);
    int num = 0;

    // create new window
    if (window == NULL) {
        Jid *jid = jid_create(to);

        if (muc_active(jid->barejid)) {
            window = wins_new_private(to);
        } else {
            window = wins_new_chat(to);
#ifdef HAVE_LIBOTR
            if (otr_is_secure(to)) {
                window->wins.chat.is_otr = TRUE;
            }
#endif
        }

        jid_destroy(jid);
        num = wins_get_num(window);

        if (prefs_get_boolean(PREF_CHLOG) && prefs_get_boolean(PREF_HISTORY)) {
            _win_show_history(num, to);
        }

        if (contact != NULL) {
            if (strcmp(p_contact_presence(contact), "offline") == 0) {
                const char *show = p_contact_presence(contact);
                const char *status = p_contact_status(contact);
                win_show_status_string(window, to, show, status, NULL, "--", "offline");
            }
        }

    // use existing window
    } else {
        num = wins_get_num(window);
    }

    win_save_print(window, '-', NULL, 0, THEME_TEXT_ME, from, message);
    ui_switch_win(num);
}

static void
_ui_room_join(const char * const room, gboolean focus)
{
    ProfWin *window = wins_get_by_recipient(room);
    int num = 0;

    // create new window
    if (window == NULL) {
        window = wins_new_muc(room);
    }

    char *nick = muc_nick(room);
    win_save_vprint(window, '!', NULL, NO_EOL, THEME_ROOMINFO, "", "-> You have joined the room as %s", nick);
    if (prefs_get_boolean(PREF_MUC_PRIVILEGES)) {
        char *role = muc_role_str(room);
        char *affiliation = muc_affiliation_str(room);
        if (role) {
            win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", role: %s", role);
        }
        if (affiliation) {
            win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", affiliation: %s", affiliation);
        }
    }
    win_save_print(window, '!', NULL, NO_DATE, THEME_ROOMINFO, "", "");

    num = wins_get_num(window);

    if (focus) {
        ui_switch_win(num);
    } else {
        status_bar_active(num);
        ProfWin *console = wins_get_console();
        char *nick = muc_nick(room);
        win_save_vprint(console, '!', NULL, 0, THEME_TYPING, "", "-> Autojoined %s as %s (%d).", room, nick, num);
    }
}

static void
_ui_switch_to_room(const char * const room)
{
    ProfWin *window = wins_get_by_recipient(room);
    int num = wins_get_num(window);
    num = wins_get_num(window);
    ui_switch_win(num);
}

static void
_ui_room_role_change(const char * const room, const char * const role, const char * const actor,
    const char * const reason)
{
    ProfWin *window = wins_get_by_recipient(room);
    win_save_vprint(window, '!', NULL, NO_EOL, THEME_ROOMINFO, "", "Your role has been changed to: %s", role);
    if (actor) {
        win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", by: %s", actor);
    }
    if (reason) {
        win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", reason: %s", reason);
    }
    win_save_print(window, '!', NULL, NO_DATE, THEME_ROOMINFO, "", "");
}

static void
_ui_room_affiliation_change(const char * const room, const char * const affiliation, const char * const actor,
    const char * const reason)
{
    ProfWin *window = wins_get_by_recipient(room);
    win_save_vprint(window, '!', NULL, NO_EOL, THEME_ROOMINFO, "", "Your affiliation has been changed to: %s", affiliation);
    if (actor) {
        win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", by: %s", actor);
    }
    if (reason) {
        win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", reason: %s", reason);
    }
    win_save_print(window, '!', NULL, NO_DATE, THEME_ROOMINFO, "", "");
}

static void
_ui_room_role_and_affiliation_change(const char * const room, const char * const role, const char * const affiliation,
    const char * const actor, const char * const reason)
{
    ProfWin *window = wins_get_by_recipient(room);
    win_save_vprint(window, '!', NULL, NO_EOL, THEME_ROOMINFO, "", "Your role and affiliation have been changed, role: %s, affiliation: %s", role, affiliation);
    if (actor) {
        win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", by: %s", actor);
    }
    if (reason) {
        win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", reason: %s", reason);
    }
    win_save_print(window, '!', NULL, NO_DATE, THEME_ROOMINFO, "", "");
}


static void
_ui_room_occupant_role_change(const char * const room, const char * const nick, const char * const role,
    const char * const actor, const char * const reason)
{
    ProfWin *window = wins_get_by_recipient(room);
    win_save_vprint(window, '!', NULL, NO_EOL, THEME_ROOMINFO, "", "%s's role has been changed to: %s", nick, role);
    if (actor) {
        win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", by: %s", actor);
    }
    if (reason) {
        win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", reason: %s", reason);
    }
    win_save_print(window, '!', NULL, NO_DATE, THEME_ROOMINFO, "", "");
}

static void
_ui_room_occupant_affiliation_change(const char * const room, const char * const nick, const char * const affiliation,
    const char * const actor, const char * const reason)
{
    ProfWin *window = wins_get_by_recipient(room);
    win_save_vprint(window, '!', NULL, NO_EOL, THEME_ROOMINFO, "", "%s's affiliation has been changed to: %s", nick, affiliation);
    if (actor) {
        win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", by: %s", actor);
    }
    if (reason) {
        win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", reason: %s", reason);
    }
    win_save_print(window, '!', NULL, NO_DATE, THEME_ROOMINFO, "", "");
}

static void
_ui_room_occupant_role_and_affiliation_change(const char * const room, const char * const nick, const char * const role,
    const char * const affiliation, const char * const actor, const char * const reason)
{
    ProfWin *window = wins_get_by_recipient(room);
    win_save_vprint(window, '!', NULL, NO_EOL, THEME_ROOMINFO, "", "%s's role and affiliation have been changed, role: %s, affiliation: %s", nick, role, affiliation);
    if (actor) {
        win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", by: %s", actor);
    }
    if (reason) {
        win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ROOMINFO, "", ", reason: %s", reason);
    }
    win_save_print(window, '!', NULL, NO_DATE, THEME_ROOMINFO, "", "");
}

static void
_ui_handle_room_info_error(const char * const room, const char * const error)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window) {
        win_save_vprint(window, '!', NULL, 0, 0, "", "Room info request failed: %s", error);
        win_save_print(window, '-', NULL, 0, 0, "", "");
    }
}

static void
_ui_show_room_disco_info(const char * const room, GSList *identities, GSList *features)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window) {
        if (((identities != NULL) && (g_slist_length(identities) > 0)) ||
            ((features != NULL) && (g_slist_length(features) > 0))) {
            if (identities != NULL) {
                win_save_print(window, '!', NULL, 0, 0, "", "Identities:");
            }
            while (identities != NULL) {
                DiscoIdentity *identity = identities->data;  // anme trpe, cat
                GString *identity_str = g_string_new("  ");
                if (identity->name != NULL) {
                    identity_str = g_string_append(identity_str, identity->name);
                    identity_str = g_string_append(identity_str, " ");
                }
                if (identity->type != NULL) {
                    identity_str = g_string_append(identity_str, identity->type);
                    identity_str = g_string_append(identity_str, " ");
                }
                if (identity->category != NULL) {
                    identity_str = g_string_append(identity_str, identity->category);
                }
                win_save_print(window, '!', NULL, 0, 0, "", identity_str->str);
                g_string_free(identity_str, TRUE);
                identities = g_slist_next(identities);
            }

            if (features != NULL) {
                win_save_print(window, '!', NULL, 0, 0, "", "Features:");
            }
            while (features != NULL) {
                win_save_vprint(window, '!', NULL, 0, 0, "", "  %s", features->data);
                features = g_slist_next(features);
            }
            win_save_print(window, '-', NULL, 0, 0, "", "");
        }
    }
}

static void
_ui_room_roster(const char * const room, GList *roster, const char * const presence)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window == NULL) {
        log_error("Received room roster but no window open for %s.", room);
    } else {
        if ((roster == NULL) || (g_list_length(roster) == 0)) {
            if (presence == NULL) {
                win_save_print(window, '!', NULL, 0, THEME_ROOMINFO, "", "Room is empty.");
            } else {
                win_save_vprint(window, '!', NULL, 0, THEME_ROOMINFO, "", "No occupants %s.", presence);
            }
        } else {
            int length = g_list_length(roster);
            if (presence == NULL) {
                win_save_vprint(window, '!', NULL, NO_EOL, THEME_ROOMINFO, "", "%d occupants: ", length);
            } else {
                win_save_vprint(window, '!', NULL, NO_EOL, THEME_ROOMINFO, "", "%d %s: ", length, presence);
            }

            while (roster != NULL) {
                Occupant *occupant = roster->data;
                const char *presence_str = string_from_resource_presence(occupant->presence);

                theme_item_t presence_colour = theme_main_presence_attrs(presence_str);
                win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, presence_colour, "", "%s", occupant->nick);

                if (roster->next != NULL) {
                    win_save_print(window, '!', NULL, NO_DATE | NO_EOL, 0, "", ", ");
                }

                roster = g_list_next(roster);
            }
            win_save_print(window, '!', NULL, NO_DATE, THEME_ONLINE, "", "");

        }
    }
}

static void
_ui_handle_room_join_error(const char * const room, const char * const err)
{
    cons_show_error("Error joining room %s, reason: %s", room, err);
}

static void
_ui_room_member_offline(const char * const room, const char * const nick)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window == NULL) {
        log_error("Received offline presence for room participant %s, but no window open for %s.", nick, room);
    } else {
        win_save_vprint(window, '!', NULL, 0, THEME_OFFLINE, "", "<- %s has left the room.", nick);
    }
}

static void
_ui_room_member_kicked(const char * const room, const char * const nick, const char * const actor,
    const char * const reason)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window == NULL) {
        log_error("Received kick for room participant %s, but no window open for %s.", nick, room);
    } else {
        GString *message = g_string_new(nick);
        g_string_append(message, " has been kicked from the room");
        if (actor) {
            g_string_append(message, " by ");
            g_string_append(message, actor);
        }
        if (reason) {
            g_string_append(message, ", reason: ");
            g_string_append(message, reason);
        }

        win_save_vprint(window, '!', NULL, 0, THEME_OFFLINE, "", "<- %s", message->str);
        g_string_free(message, TRUE);
    }
}

static void
_ui_room_member_banned(const char * const room, const char * const nick, const char * const actor,
    const char * const reason)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window == NULL) {
        log_error("Received ban for room participant %s, but no window open for %s.", nick, room);
    } else {
        GString *message = g_string_new(nick);
        g_string_append(message, " has been banned from the room");
        if (actor) {
            g_string_append(message, " by ");
            g_string_append(message, actor);
        }
        if (reason) {
            g_string_append(message, ", reason: ");
            g_string_append(message, reason);
        }

        win_save_vprint(window, '!', NULL, 0, THEME_OFFLINE, "", "<- %s", message->str);
        g_string_free(message, TRUE);
    }
}

static void
_ui_room_member_online(const char * const room, const char * const nick, const char * const role,
    const char * const affiliation, const char * const show, const char * const status)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window == NULL) {
        log_error("Received online presence for room participant %s, but no window open for %s.", nick, room);
    } else {
        win_save_vprint(window, '!', NULL, NO_EOL, THEME_ONLINE, "", "-> %s has joined the room", nick);
        if (prefs_get_boolean(PREF_MUC_PRIVILEGES)) {
            if (role) {
                win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ONLINE, "", ", role: %s", role);
            }
            if (affiliation) {
                win_save_vprint(window, '!', NULL, NO_DATE | NO_EOL, THEME_ONLINE, "", ", affiliation: %s", affiliation);
            }
        }
        win_save_print(window, '!', NULL, NO_DATE, THEME_ROOMINFO, "", "");
    }
}

static void
_ui_room_member_presence(const char * const room, const char * const nick,
    const char * const show, const char * const status)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window == NULL) {
        log_error("Received presence for room participant %s, but no window open for %s.", nick, room);
    } else {
        win_show_status_string(window, nick, show, status, NULL, "++", "online");
    }
}

static void
_ui_room_member_nick_change(const char * const room,
    const char * const old_nick, const char * const nick)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window == NULL) {
        log_error("Received nick change for room participant %s, but no window open for %s.", old_nick, room);
    } else {
        win_save_vprint(window, '!', NULL, 0, THEME_THEM, "", "** %s is now known as %s", old_nick, nick);
    }
}

static void
_ui_room_nick_change(const char * const room, const char * const nick)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window == NULL) {
        log_error("Received self nick change %s, but no window open for %s.", nick, room);
    } else {
        win_save_vprint(window, '!', NULL, 0, THEME_ME, "", "** You are now known as %s", nick);
    }
}

static void
_ui_room_history(const char * const room_jid, const char * const nick,
    GTimeVal tv_stamp, const char * const message)
{
    ProfWin *window = wins_get_by_recipient(room_jid);
    if (window == NULL) {
        log_error("Room history message received from %s, but no window open for %s", nick, room_jid);
    } else {
        GString *line = g_string_new("");

        if (strncmp(message, "/me ", 4) == 0) {
            g_string_append(line, "*");
            g_string_append(line, nick);
            g_string_append(line, " ");
            g_string_append(line, message + 4);
        } else {
            g_string_append(line, nick);
            g_string_append(line, ": ");
            g_string_append(line, message);
        }

        win_save_print(window, '-', &tv_stamp, NO_COLOUR_DATE, 0, "", line->str);
        g_string_free(line, TRUE);
    }
}

static void
_ui_room_message(const char * const room_jid, const char * const nick,
    const char * const message)
{
    ProfWin *window = wins_get_by_recipient(room_jid);
    if (window == NULL) {
        log_error("Room message received from %s, but no window open for %s", nick, room_jid);
    } else {
        int num = wins_get_num(window);
        char *my_nick = muc_nick(room_jid);

        if (g_strcmp0(nick, my_nick) != 0) {
            if (g_strrstr(message, my_nick) != NULL) {
                win_save_print(window, '-', NULL, NO_ME, THEME_ROOMMENTION, nick, message);
            } else {
                win_save_print(window, '-', NULL, NO_ME, THEME_TEXT_THEM, nick, message);
            }
        } else {
            win_save_print(window, '-', NULL, 0, THEME_TEXT_ME, nick, message);
        }

        // currently in groupchat window
        if (wins_is_current(window)) {
            status_bar_active(num);

        // not currenlty on groupchat window
        } else {
            status_bar_new(num);
            cons_show_incoming_message(nick, num);

            if (strcmp(nick, my_nick) != 0) {
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

        if (strcmp(nick, muc_nick(room_jid)) != 0) {
            if (prefs_get_boolean(PREF_BEEP)) {
                beep();
            }

            gboolean notify = FALSE;
            char *room_setting = prefs_get_string(PREF_NOTIFY_ROOM);
            if (g_strcmp0(room_setting, "on") == 0) {
                notify = TRUE;
            }
            if (g_strcmp0(room_setting, "mention") == 0) {
                char *message_lower = g_utf8_strdown(message, -1);
                char *nick_lower = g_utf8_strdown(nick, -1);
                if (g_strrstr(message_lower, nick_lower) != NULL) {
                    notify = TRUE;
                }
                g_free(message_lower);
                g_free(nick_lower);
            }
            prefs_free_string(room_setting);

            if (notify) {
                gboolean is_current = wins_is_current(window);
                if ( !is_current || (is_current && prefs_get_boolean(PREF_NOTIFY_ROOM_CURRENT)) ) {
                    Jid *jidp = jid_create(room_jid);
                    if (prefs_get_boolean(PREF_NOTIFY_ROOM_TEXT)) {
                        notify_room_message(nick, jidp->localpart, ui_index, message);
                    } else {
                        notify_room_message(nick, jidp->localpart, ui_index, NULL);
                    }
                    jid_destroy(jidp);
                }
            }
        }
    }
}

static void
_ui_room_requires_config(const char * const room_jid)
{
    ProfWin *window = wins_get_by_recipient(room_jid);
    if (window == NULL) {
        log_error("Received room config request, but no window open for %s.", room_jid);
    } else {
        int num = wins_get_num(window);
        int ui_index = num;
        if (ui_index == 10) {
            ui_index = 0;
        }

        win_save_print(window, '-', NULL, 0, 0, "", "");
        win_save_vprint(window, '!', NULL, 0, THEME_ROOMINFO, "",
            "Room locked, requires configuration.");
        win_save_vprint(window, '!', NULL, 0, THEME_ROOMINFO, "",
            "Use '/room accept' to accept the defaults");
        win_save_vprint(window, '!', NULL, 0, THEME_ROOMINFO, "",
            "Use '/room destroy' to cancel and destroy the room");
        win_save_vprint(window, '!', NULL, 0, THEME_ROOMINFO, "",
            "Use '/room config' to edit the room configuration");
        win_save_print(window, '-', NULL, 0, 0, "", "");

        // currently in groupchat window
        if (wins_is_current(window)) {
            status_bar_active(num);

        // not currenlty on groupchat window
        } else {
            status_bar_new(num);
        }
    }
}

static void
_ui_room_destroy(const char * const room_jid)
{
    ProfWin *window = wins_get_by_recipient(room_jid);
    if (window == NULL) {
        log_error("Received room destroy result, but no window open for %s.", room_jid);
    } else {
        int num = wins_get_num(window);
        ui_close_win(num);
        cons_show("Room destroyed: %s", room_jid);
    }
}

static void
_ui_leave_room(const char * const room)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window) {
        int num = wins_get_num(window);
        ui_close_win(num);
    }
}

static void
_ui_room_destroyed(const char * const room, const char * const reason, const char * const new_jid,
    const char * const password)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window == NULL) {
        log_error("Received room destroy, but no window open for %s.", room);
    } else {
        int num = wins_get_num(window);
        ui_close_win(num);
        ProfWin *console = wins_get_console();

        if (reason) {
            win_save_vprint(console, '!', NULL, 0, THEME_TYPING, "", "<- Room destroyed: %s, reason: %s", room, reason);
        } else {
            win_save_vprint(console, '!', NULL, 0, THEME_TYPING, "", "<- Room destroyed: %s", room);
        }

        if (new_jid) {
            if (password) {
                win_save_vprint(console, '!', NULL, 0, THEME_TYPING, "", "Replacement room: %s, password: %s", new_jid, password);
            } else {
                win_save_vprint(console, '!', NULL, 0, THEME_TYPING, "", "Replacement room: %s", new_jid);
            }
        }
    }
}

static void
_ui_room_kicked(const char * const room, const char * const actor, const char * const reason)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window == NULL) {
        log_error("Received kick, but no window open for %s.", room);
    } else {
        int num = wins_get_num(window);
        ui_close_win(num);

        GString *message = g_string_new("Kicked from ");
        g_string_append(message, room);
        if (actor) {
            g_string_append(message, " by ");
            g_string_append(message, actor);
        }
        if (reason) {
            g_string_append(message, ", reason: ");
            g_string_append(message, reason);
        }

        ProfWin *console = wins_get_console();
        win_save_vprint(console, '!', NULL, 0, THEME_TYPING, "", "<- %s", message->str);
        g_string_free(message, TRUE);
    }
}

static void
_ui_room_banned(const char * const room, const char * const actor, const char * const reason)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window == NULL) {
        log_error("Received ban, but no window open for %s.", room);
    } else {
        int num = wins_get_num(window);
        ui_close_win(num);

        GString *message = g_string_new("Banned from ");
        g_string_append(message, room);
        if (actor) {
            g_string_append(message, " by ");
            g_string_append(message, actor);
        }
        if (reason) {
            g_string_append(message, ", reason: ");
            g_string_append(message, reason);
        }

        ProfWin *console = wins_get_console();
        win_save_vprint(console, '!', NULL, 0, THEME_TYPING, "", "<- %s", message->str);
        g_string_free(message, TRUE);
    }
}

static void
_ui_room_subject(const char * const room, const char * const nick, const char * const subject)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window == NULL) {
        log_error("Received room subject, but no window open for %s.", room);
    } else {
        int num = wins_get_num(window);

        if (subject) {
            if (nick) {
                win_save_vprint(window, '!', NULL, NO_EOL, THEME_ROOMINFO, "", "*%s has set the room subject: ", nick);
                win_save_vprint(window, '!', NULL, NO_DATE, 0, "", "%s", subject);
            } else {
                win_save_vprint(window, '!', NULL, NO_EOL, THEME_ROOMINFO, "", "Room subject: ");
                win_save_vprint(window, '!', NULL, NO_DATE, 0, "", "%s", subject);
            }
        } else {
            if (nick) {
                win_save_vprint(window, '!', NULL, 0, THEME_ROOMINFO, "", "*%s has cleared the room subject: ", nick);
            } else {
                win_save_vprint(window, '!', NULL, 0, THEME_ROOMINFO, "", "Room subject cleared");
            }
        }

        // currently in groupchat window
        if (wins_is_current(window)) {
            status_bar_active(num);

        // not currenlty on groupchat window
        } else {
            status_bar_active(num);
        }
    }
}

static void
_ui_handle_room_kick_error(const char * const room, const char * const nick, const char * const error)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window == NULL) {
        log_error("Kick error received for %s, but no window open for %s.", nick, room);
    } else {
        win_save_vprint(window, '!', NULL, 0, THEME_ERROR, "", "Error kicking %s: %s", nick, error);
    }
}

static void
_ui_room_broadcast(const char * const room_jid, const char * const message)
{
    ProfWin *window = wins_get_by_recipient(room_jid);
    if (window == NULL) {
        log_error("Received room broadcast, but no window open for %s.", room_jid);
    } else {
        int num = wins_get_num(window);

        win_save_vprint(window, '!', NULL, NO_EOL, THEME_ROOMINFO, "", "Room message: ");
        win_save_vprint(window, '!', NULL, NO_DATE, 0, "", "%s", message);

        // currently in groupchat window
        if (wins_is_current(window)) {
            status_bar_active(num);

        // not currenlty on groupchat window
        } else {
            status_bar_new(num);
        }
    }
}

static void
_ui_handle_room_affiliation_list_error(const char * const room, const char * const affiliation,
    const char * const error)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window) {
        win_save_vprint(window, '!', NULL, 0, THEME_ERROR, "", "Error retrieving %s list: %s", affiliation, error);
    }
}

static void
_ui_handle_room_affiliation_list(const char * const room, const char * const affiliation, GSList *jids)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window) {
        if (jids) {
            win_save_vprint(window, '!', NULL, 0, 0, "", "Affiliation: %s", affiliation);
            GSList *curr_jid = jids;
            while (curr_jid) {
                char *jid = curr_jid->data;
                win_save_vprint(window, '!', NULL, 0, 0, "", "  %s", jid);
                curr_jid = g_slist_next(curr_jid);
            }
            win_save_print(window, '!', NULL, 0, 0, "", "");
        } else {
            win_save_vprint(window, '!', NULL, 0, 0, "", "No users found with affiliation: %s", affiliation);
            win_save_print(window, '!', NULL, 0, 0, "", "");
        }
    }
}

static void
_ui_handle_room_role_list_error(const char * const room, const char * const role, const char * const error)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window) {
        win_save_vprint(window, '!', NULL, 0, THEME_ERROR, "", "Error retrieving %s list: %s", role, error);
    }
}

static void
_ui_handle_room_role_list(const char * const room, const char * const role, GSList *nicks)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window) {
        if (nicks) {
            win_save_vprint(window, '!', NULL, 0, 0, "", "Role: %s", role);
            GSList *curr_nick = nicks;
            while (curr_nick) {
                char *nick = curr_nick->data;
                Occupant *occupant = muc_roster_item(room, nick);
                if (occupant) {
                    if (occupant->jid) {
                        win_save_vprint(window, '!', NULL, 0, 0, "", "  %s (%s)", nick, occupant->jid);
                    } else {
                        win_save_vprint(window, '!', NULL, 0, 0, "", "  %s", nick);
                    }
                } else {
                    win_save_vprint(window, '!', NULL, 0, 0, "", "  %s", nick);
                }
                curr_nick = g_slist_next(curr_nick);
            }
            win_save_print(window, '!', NULL, 0, 0, "", "");
        } else {
            win_save_vprint(window, '!', NULL, 0, 0, "", "No occupants found with role: %s", role);
            win_save_print(window, '!', NULL, 0, 0, "", "");
        }
    }
}

static void
_ui_handle_room_affiliation_set_error(const char * const room, const char * const jid, const char * const affiliation,
    const char * const error)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window) {
        win_save_vprint(window, '!', NULL, 0, THEME_ERROR, "", "Error setting %s affiliation for %s: %s", affiliation, jid, error);
    }
}

static void
_ui_handle_room_role_set_error(const char * const room, const char * const nick, const char * const role,
    const char * const error)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window) {
        win_save_vprint(window, '!', NULL, 0, THEME_ERROR, "", "Error setting %s role for %s: %s", role, nick, error);
    }
}

static void
_ui_status(void)
{
    char *recipient = ui_current_recipient();
    PContact pcontact = roster_get_contact(recipient);
    ProfWin *window = wins_get_current();

    if (pcontact != NULL) {
        win_show_contact(window, pcontact);
    } else {
        win_save_println(window, "Error getting contact info.");
    }
}

static void
_ui_info(void)
{
    char *recipient = ui_current_recipient();
    PContact pcontact = roster_get_contact(recipient);
    ProfWin *window = wins_get_current();

    if (pcontact != NULL) {
        win_show_info(window, pcontact);
    } else {
        win_save_println(window, "Error getting contact info.");
    }
}

static void
_ui_status_private(void)
{
    Jid *jid = jid_create(ui_current_recipient());
    Occupant *occupant = muc_roster_item(jid->barejid, jid->resourcepart);
    ProfWin *window = wins_get_current();

    if (occupant) {
        win_show_occupant(window, occupant);
    } else {
        win_save_println(window, "Error getting contact info.");
    }

    jid_destroy(jid);
}

static void
_ui_info_private(void)
{
    Jid *jid = jid_create(ui_current_recipient());
    Occupant *occupant = muc_roster_item(jid->barejid, jid->resourcepart);
    ProfWin *window = wins_get_current();

    if (occupant) {
        win_show_occupant_info(window, jid->barejid, occupant);
    } else {
        win_save_println(window, "Error getting contact info.");
    }

    jid_destroy(jid);
}

static void
_ui_status_room(const char * const contact)
{
    Occupant *occupant = muc_roster_item(ui_current_recipient(), contact);
    ProfWin *current = wins_get_current();

    if (occupant) {
        win_show_occupant(current, occupant);
    } else {
        win_save_vprint(current, '-', NULL, 0, 0, "", "No such participant \"%s\" in room.", contact);
    }
}

static void
_ui_info_room(const char * const room, Occupant *occupant)
{
    ProfWin *current = wins_get_current();
    win_show_occupant_info(current, room, occupant);
}

static gint
_ui_unread(void)
{
    return wins_get_total_unread();
}

static int
_ui_win_unread(int index)
{
    ProfWin *window = wins_get_by_num(index);
    if (window != NULL) {
        return window->unread;
    } else {
        return 0;
    }
}

static char *
_ui_ask_password(void)
{
  char *passwd = malloc(sizeof(char) * (MAX_PASSWORD_SIZE + 1));
  status_bar_get_password();
  status_bar_update_virtual();
  inp_block();
  inp_get_password(passwd);
  inp_non_block();

  return passwd;
}

static void
_ui_chat_win_contact_online(PContact contact, Resource *resource, GDateTime *last_activity)
{
    const char *show = string_from_resource_presence(resource->presence);
    char *display_str = p_contact_create_display_string(contact, resource->name);
    const char *barejid = p_contact_barejid(contact);

    ProfWin *window = wins_get_by_recipient(barejid);
    if (window != NULL) {
        win_show_status_string(window, display_str, show, resource->status,
            last_activity, "++", "online");

    }

    free(display_str);
}

static void
_ui_chat_win_contact_offline(PContact contact, char *resource, char *status)
{
    char *display_str = p_contact_create_display_string(contact, resource);
    const char *barejid = p_contact_barejid(contact);

    ProfWin *window = wins_get_by_recipient(barejid);
    if (window != NULL) {
        win_show_status_string(window, display_str, "offline", status, NULL, "--",
            "offline");
    }

    free(display_str);
}

static void
_ui_clear_win_title(void)
{
    printf("%c]0;%c", '\033', '\007');
}

static void
_ui_statusbar_new(const int win)
{
    status_bar_new(win);
}

static void
_ui_draw_term_title(void)
{
    char new_win_title[100];
    jabber_conn_status_t status = jabber_get_connection_status();

    if (status == JABBER_CONNECTED) {
        const char * const jid = jabber_get_fulljid();
        gint unread = ui_unread();

        if (unread != 0) {
            snprintf(new_win_title, sizeof(new_win_title),
                "%c]0;%s (%d) - %s%c", '\033', "Profanity",
                unread, jid, '\007');
        } else {
            snprintf(new_win_title, sizeof(new_win_title),
                "%c]0;%s - %s%c", '\033', "Profanity", jid,
                '\007');
        }
    } else {
        snprintf(new_win_title, sizeof(new_win_title), "%c]0;%s%c", '\033',
            "Profanity", '\007');
    }

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
_ui_show_room_info(ProfWin *window, const char * const room)
{
    char *role = muc_role_str(room);
    char *affiliation = muc_affiliation_str(room);

    win_save_vprint(window, '!', NULL, 0, 0, "", "Room: %s", room);
    win_save_vprint(window, '!', NULL, 0, 0, "", "Affiliation: %s", affiliation);
    win_save_vprint(window, '!', NULL, 0, 0, "", "Role: %s", role);
    win_save_print(window, '-', NULL, 0, 0, "", "");
}

static void
_ui_show_room_role_list(ProfWin *window, const char * const room, muc_role_t role)
{
    GSList *occupants = muc_occupants_by_role(room, role);

    if (!occupants) {
        switch (role) {
            case MUC_ROLE_MODERATOR:
                win_save_print(window, '!', NULL, 0, 0, "", "No moderators found.");
                break;
            case MUC_ROLE_PARTICIPANT:
                win_save_print(window, '!', NULL, 0, 0, "", "No participants found.");
                break;
            case MUC_ROLE_VISITOR:
                win_save_print(window, '!', NULL, 0, 0, "", "No visitors found.");
                break;
            default:
                break;
        }
        win_save_print(window, '-', NULL, 0, 0, "", "");
    } else {
        switch (role) {
            case MUC_ROLE_MODERATOR:
                win_save_print(window, '!', NULL, 0, 0, "", "Moderators:");
                break;
            case MUC_ROLE_PARTICIPANT:
                win_save_print(window, '!', NULL, 0, 0, "", "Participants:");
                break;
            case MUC_ROLE_VISITOR:
                win_save_print(window, '!', NULL, 0, 0, "", "Visitors:");
                break;
            default:
                break;
        }

        GSList *curr_occupant = occupants;
        while(curr_occupant) {
            Occupant *occupant = curr_occupant->data;
            if (occupant->role == role) {
                if (occupant->jid) {
                    win_save_vprint(window, '!', NULL, 0, 0, "", "  %s (%s)", occupant->nick, occupant->jid);
                } else {
                    win_save_vprint(window, '!', NULL, 0, 0, "", "  %s", occupant->nick);
                }
            }

            curr_occupant = g_slist_next(curr_occupant);
        }

        win_save_print(window, '-', NULL, 0, 0, "", "");
    }
}

static void
_ui_show_room_affiliation_list(ProfWin *window, const char * const room, muc_affiliation_t affiliation)
{
    GSList *occupants = muc_occupants_by_affiliation(room, affiliation);

    if (!occupants) {
        switch (affiliation) {
            case MUC_AFFILIATION_OWNER:
                win_save_print(window, '!', NULL, 0, 0, "", "No owners found.");
                break;
            case MUC_AFFILIATION_ADMIN:
                win_save_print(window, '!', NULL, 0, 0, "", "No admins found.");
                break;
            case MUC_AFFILIATION_MEMBER:
                win_save_print(window, '!', NULL, 0, 0, "", "No members found.");
                break;
            case MUC_AFFILIATION_OUTCAST:
                win_save_print(window, '!', NULL, 0, 0, "", "No outcasts found.");
                break;
            default:
                break;
        }
        win_save_print(window, '-', NULL, 0, 0, "", "");
    } else {
        switch (affiliation) {
            case MUC_AFFILIATION_OWNER:
                win_save_print(window, '!', NULL, 0, 0, "", "Owners:");
                break;
            case MUC_AFFILIATION_ADMIN:
                win_save_print(window, '!', NULL, 0, 0, "", "Admins:");
                break;
            case MUC_AFFILIATION_MEMBER:
                win_save_print(window, '!', NULL, 0, 0, "", "Members:");
                break;
            case MUC_AFFILIATION_OUTCAST:
                win_save_print(window, '!', NULL, 0, 0, "", "Outcasts:");
                break;
            default:
                break;
        }

        GSList *curr_occupant = occupants;
        while(curr_occupant) {
            Occupant *occupant = curr_occupant->data;
            if (occupant->affiliation == affiliation) {
                if (occupant->jid) {
                    win_save_vprint(window, '!', NULL, 0, 0, "", "  %s (%s)", occupant->nick, occupant->jid);
                } else {
                    win_save_vprint(window, '!', NULL, 0, 0, "", "  %s", occupant->nick);
                }
            }

            curr_occupant = g_slist_next(curr_occupant);
        }

        win_save_print(window, '-', NULL, 0, 0, "", "");
    }
}

static void
_ui_handle_form_field(ProfWin *window, char *tag, FormField *field)
{
    win_save_vprint(window, '-', NULL, NO_EOL, THEME_AWAY, "", "[%s] ", tag);
    win_save_vprint(window, '-', NULL, NO_EOL | NO_DATE, 0, "", "%s", field->label);
    if (field->required) {
        win_save_print(window, '-', NULL, NO_DATE | NO_EOL, 0, "", " (required): ");
    } else {
        win_save_print(window, '-', NULL, NO_DATE | NO_EOL, 0, "", ": ");
    }

    GSList *values = field->values;
    GSList *curr_value = values;

    switch (field->type_t) {
    case FIELD_HIDDEN:
        break;
    case FIELD_TEXT_SINGLE:
        if (curr_value != NULL) {
            char *value = curr_value->data;
            if (value != NULL) {
                if (g_strcmp0(field->var, "muc#roomconfig_roomsecret") == 0) {
                    win_save_print(window, '-', NULL, NO_DATE | NO_EOL, THEME_ONLINE, "", "[hidden]");
                } else {
                    win_save_print(window, '-', NULL, NO_DATE | NO_EOL, THEME_ONLINE, "", value);
                }
            }
        }
        win_save_newline(window);
        break;
    case FIELD_TEXT_PRIVATE:
        if (curr_value != NULL) {
            char *value = curr_value->data;
            if (value != NULL) {
                win_save_print(window, '-', NULL, NO_DATE | NO_EOL, THEME_ONLINE, "", "[hidden]");
            }
        }
        win_save_newline(window);
        break;
    case FIELD_TEXT_MULTI:
        win_save_newline(window);
        int index = 1;
        while (curr_value != NULL) {
            char *value = curr_value->data;
            GString *val_tag = g_string_new("");
            g_string_printf(val_tag, "val%d", index++);
            win_save_vprint(window, '-', NULL, 0, THEME_ONLINE, "", "  [%s] %s", val_tag->str, value);
            g_string_free(val_tag, TRUE);
            curr_value = g_slist_next(curr_value);
        }
        break;
    case FIELD_BOOLEAN:
        if (curr_value == NULL) {
            win_save_print(window, '-', NULL, NO_DATE, THEME_OFFLINE, "", "FALSE");
        } else {
            char *value = curr_value->data;
            if (value == NULL) {
                win_save_print(window, '-', NULL, NO_DATE, THEME_OFFLINE, "", "FALSE");
            } else {
                if (g_strcmp0(value, "0") == 0) {
                    win_save_print(window, '-', NULL, NO_DATE, THEME_OFFLINE, "", "FALSE");
                } else {
                    win_save_print(window, '-', NULL, NO_DATE, THEME_ONLINE, "", "TRUE");
                }
            }
        }
        break;
    case FIELD_LIST_SINGLE:
        if (curr_value != NULL) {
            win_save_newline(window);
            char *value = curr_value->data;
            GSList *options = field->options;
            GSList *curr_option = options;
            while (curr_option != NULL) {
                FormOption *option = curr_option->data;
                if (g_strcmp0(option->value, value) == 0) {
                    win_save_vprint(window, '-', NULL, 0, THEME_ONLINE, "", "  [%s] %s", option->value, option->label);
                } else {
                    win_save_vprint(window, '-', NULL, 0, THEME_OFFLINE, "", "  [%s] %s", option->value, option->label);
                }
                curr_option = g_slist_next(curr_option);
            }
        }
        break;
    case FIELD_LIST_MULTI:
        if (curr_value != NULL) {
            win_save_newline(window);
            GSList *options = field->options;
            GSList *curr_option = options;
            while (curr_option != NULL) {
                FormOption *option = curr_option->data;
                if (g_slist_find_custom(curr_value, option->value, (GCompareFunc)g_strcmp0) != NULL) {
                    win_save_vprint(window, '-', NULL, 0, THEME_ONLINE, "", "  [%s] %s", option->value, option->label);
                } else {
                    win_save_vprint(window, '-', NULL, 0, THEME_OFFLINE, "", "  [%s] %s", option->value, option->label);
                }
                curr_option = g_slist_next(curr_option);
            }
        }
        break;
    case FIELD_JID_SINGLE:
        if (curr_value != NULL) {
            char *value = curr_value->data;
            if (value != NULL) {
                win_save_print(window, '-', NULL, NO_DATE | NO_EOL, THEME_ONLINE, "", value);
            }
        }
        win_save_newline(window);
        break;
    case FIELD_JID_MULTI:
        win_save_newline(window);
        while (curr_value != NULL) {
            char *value = curr_value->data;
            win_save_vprint(window, '-', NULL, 0, THEME_ONLINE, "", "  %s", value);
            curr_value = g_slist_next(curr_value);
        }
        break;
    case FIELD_FIXED:
        if (curr_value != NULL) {
            char *value = curr_value->data;
            if (value != NULL) {
                win_save_print(window, '-', NULL, NO_DATE | NO_EOL, 0, "", value);
            }
        }
        win_save_newline(window);
        break;
    default:
        break;
    }
}

static void
_ui_show_form(ProfWin *window, const char * const room, DataForm *form)
{
    if (form->title != NULL) {
        win_save_print(window, '-', NULL, NO_EOL, 0, "", "Form title: ");
        win_save_print(window, '-', NULL, NO_DATE, 0, "", form->title);
    } else {
        win_save_vprint(window, '-', NULL, 0, 0, "", "Configuration for room %s.", room);
    }
    win_save_print(window, '-', NULL, 0, 0, "", "");

    ui_show_form_help(window, form);

    GSList *fields = form->fields;
    GSList *curr_field = fields;
    while (curr_field != NULL) {
        FormField *field = curr_field->data;

        if ((g_strcmp0(field->type, "fixed") == 0) && field->values) {
            if (field->values) {
                char *value = field->values->data;
                win_save_print(window, '-', NULL, 0, 0, "", value);
            }
        } else if (g_strcmp0(field->type, "hidden") != 0 && field->var) {
            char *tag = g_hash_table_lookup(form->var_to_tag, field->var);
            _ui_handle_form_field(window, tag, field);
        }

        curr_field = g_slist_next(curr_field);
    }
}

static void
_ui_show_form_field(ProfWin *window, DataForm *form, char *tag)
{
    FormField *field = form_get_field_by_tag(form, tag);
    _ui_handle_form_field(window, tag, field);
    win_save_println(window, "");
}

static void
_ui_handle_room_configuration(const char * const room, DataForm *form)
{
    GString *title = g_string_new(room);
    g_string_append(title, " config");
    ProfWin *window = wins_new_muc_config(title->str, form);
    g_string_free(title, TRUE);

    int num = wins_get_num(window);
    ui_switch_win(num);

    ui_show_form(window, room, form);

    win_save_print(window, '-', NULL, 0, 0, "", "");
    win_save_print(window, '-', NULL, 0, 0, "", "Use '/form submit' to save changes.");
    win_save_print(window, '-', NULL, 0, 0, "", "Use '/form cancel' to cancel changes.");
    win_save_print(window, '-', NULL, 0, 0, "", "See '/form help' for more information.");
    win_save_print(window, '-', NULL, 0, 0, "", "");
}

static void
_ui_handle_room_configuration_form_error(const char * const room, const char * const message)
{
    ProfWin *window = NULL;
    GString *message_str = g_string_new("");

    if (room) {
        window = wins_get_by_recipient(room);
        g_string_printf(message_str, "Could not get room configuration for %s", room);
    } else {
        window = wins_get_console();
        g_string_printf(message_str, "Could not get room configuration");
    }

    if (message) {
        g_string_append(message_str, ": ");
        g_string_append(message_str, message);
    }

    win_save_print(window, '-', NULL, 0, THEME_ERROR, "", message_str->str);

    g_string_free(message_str, TRUE);
}

static void
_ui_handle_room_config_submit_result(const char * const room)
{
    ProfWin *muc_window = NULL;
    ProfWin *form_window = NULL;
    int num;

    if (room) {
        GString *form_recipient = g_string_new(room);
        g_string_append(form_recipient, " config");

        muc_window = wins_get_by_recipient(room);
        form_window = wins_get_by_recipient(form_recipient->str);
        g_string_free(form_recipient, TRUE);

        if (form_window) {
            num = wins_get_num(form_window);
            wins_close_by_num(num);
        }

        if (muc_window) {
            int num = wins_get_num(muc_window);
            ui_switch_win(num);
            win_save_print(muc_window, '!', NULL, 0, THEME_ROOMINFO, "", "Room configuration successful");
        } else {
            ui_switch_win(1);
            cons_show("Room configuration successful: %s", room);
        }
    } else {
        cons_show("Room configuration successful");
    }
}

static void
_ui_handle_room_config_submit_result_error(const char * const room, const char * const message)
{
    ProfWin *console = wins_get_console();
    ProfWin *muc_window = NULL;
    ProfWin *form_window = NULL;

    if (room) {
        GString *form_recipient = g_string_new(room);
        g_string_append(form_recipient, " config");

        muc_window = wins_get_by_recipient(room);
        form_window = wins_get_by_recipient(form_recipient->str);
        g_string_free(form_recipient, TRUE);

        if (form_window) {
            if (message) {
                win_save_vprint(form_window, '!', NULL, 0, THEME_ERROR, "", "Configuration error: %s", message);
            } else {
                win_save_print(form_window, '!', NULL, 0, THEME_ERROR, "", "Configuration error");
            }
        } else if (muc_window) {
            if (message) {
                win_save_vprint(muc_window, '!', NULL, 0, THEME_ERROR, "", "Configuration error: %s", message);
            } else {
                win_save_print(muc_window, '!', NULL, 0, THEME_ERROR, "", "Configuration error");
            }
        } else {
            if (message) {
                win_save_vprint(console, '!', NULL, 0, THEME_ERROR, "", "Configuration error for %s: %s", room, message);
            } else {
                win_save_vprint(console, '!', NULL, 0, THEME_ERROR, "", "Configuration error for %s", room);
            }
        }
    } else {
        win_save_print(console, '!', NULL, 0, THEME_ERROR, "", "Configuration error");
    }
}

static void
_ui_show_form_field_help(ProfWin *window, DataForm *form, char *tag)
{
    FormField *field = form_get_field_by_tag(form, tag);
    if (field != NULL) {
        win_save_print(window, '-', NULL, NO_EOL, 0, "", field->label);
        if (field->required) {
            win_save_print(window, '-', NULL, NO_DATE, 0, "", " (Required):");
        } else {
            win_save_print(window, '-', NULL, NO_DATE, 0, "", ":");
        }
        if (field->description != NULL) {
            win_save_vprint(window, '-', NULL, 0, 0, "", "  Description : %s", field->description);
        }
        win_save_vprint(window, '-', NULL, 0, 0, "", "  Type        : %s", field->type);

        int num_values = 0;
        GSList *curr_option = NULL;
        FormOption *option = NULL;

        switch (field->type_t) {
        case FIELD_TEXT_SINGLE:
        case FIELD_TEXT_PRIVATE:
            win_save_vprint(window, '-', NULL, 0, 0, "", "  Set         : /%s <value>", tag);
            win_save_print(window, '-', NULL, 0, 0, "", "  Where       : <value> is any text");
            break;
        case FIELD_TEXT_MULTI:
            num_values = form_get_value_count(form, tag);
            win_save_vprint(window, '-', NULL, 0, 0, "", "  Add         : /%s add <value>", tag);
            win_save_print(window, '-', NULL, 0, 0, "", "  Where       : <value> is any text");
            if (num_values > 0) {
                win_save_vprint(window, '-', NULL, 0, 0, "", "  Remove      : /%s remove <value>", tag);
                win_save_vprint(window, '-', NULL, 0, 0, "", "  Where       : <value> between 'val1' and 'val%d'", num_values);
            }
            break;
        case FIELD_BOOLEAN:
            win_save_vprint(window, '-', NULL, 0, 0, "", "  Set         : /%s <value>", tag);
            win_save_print(window, '-', NULL, 0, 0, "", "  Where       : <value> is either 'on' or 'off'");
            break;
        case FIELD_LIST_SINGLE:
            win_save_vprint(window, '-', NULL, 0, 0, "", "  Set         : /%s <value>", tag);
            win_save_print(window, '-', NULL, 0, 0, "", "  Where       : <value> is one of");
            curr_option = field->options;
            while (curr_option != NULL) {
                option = curr_option->data;
                win_save_vprint(window, '-', NULL, 0, 0, "", "                  %s", option->value);
                curr_option = g_slist_next(curr_option);
            }
            break;
        case FIELD_LIST_MULTI:
            win_save_vprint(window, '-', NULL, 0, 0, "", "  Add         : /%s add <value>", tag);
            win_save_vprint(window, '-', NULL, 0, 0, "", "  Remove      : /%s remove <value>", tag);
            win_save_print(window, '-', NULL, 0, 0, "", "  Where       : <value> is one of");
            curr_option = field->options;
            while (curr_option != NULL) {
                option = curr_option->data;
                win_save_vprint(window, '-', NULL, 0, 0, "", "                  %s", option->value);
                curr_option = g_slist_next(curr_option);
            }
            break;
        case FIELD_JID_SINGLE:
            win_save_vprint(window, '-', NULL, 0, 0, "", "  Set         : /%s <value>", tag);
            win_save_print(window, '-', NULL, 0, 0, "", "  Where       : <value> is a valid Jabber ID");
            break;
        case FIELD_JID_MULTI:
            win_save_vprint(window, '-', NULL, 0, 0, "", "  Add         : /%s add <value>", tag);
            win_save_vprint(window, '-', NULL, 0, 0, "", "  Remove      : /%s remove <value>", tag);
            win_save_print(window, '-', NULL, 0, 0, "", "  Where       : <value> is a valid Jabber ID");
            break;
        case FIELD_FIXED:
        case FIELD_UNKNOWN:
        case FIELD_HIDDEN:
        default:
            break;
        }
    } else {
        win_save_vprint(window, '-', NULL, 0, 0, "", "No such field %s", tag);
    }
}

static void
_ui_show_form_help(ProfWin *window, DataForm *form)
{
    if (form->instructions != NULL) {
        win_save_print(window, '-', NULL, 0, 0, "", "Supplied instructions:");
        win_save_print(window, '-', NULL, 0, 0, "", form->instructions);
        win_save_print(window, '-', NULL, 0, 0, "", "");
    }
}

static void
_ui_show_lines(ProfWin *window, const gchar** lines)
{
    if (lines != NULL) {
        int i;
        for (i = 0; lines[i] != NULL; i++) {
            win_save_print(window, '-', NULL, 0, 0, "", lines[i]);
        }
    }
}

static void
_ui_roster_contact(PContact contact)
{
    if (p_contact_subscribed(contact)) {
        ProfWin *window = wins_get_console();
        const char *name = p_contact_name_or_jid(contact);
        const char *presence = p_contact_presence(contact);

        if ((g_strcmp0(presence, "offline") != 0) || ((g_strcmp0(presence, "offline") == 0) &&
                (prefs_get_boolean(PREF_ROSTER_OFFLINE)))) {
            theme_item_t presence_colour = theme_main_presence_attrs(presence);

            wattron(window->wins.cons.subwin, theme_attrs(presence_colour));

            GString *msg = g_string_new("   ");
            g_string_append(msg, name);
            win_printline_nowrap(window->wins.cons.subwin, msg->str);
            g_string_free(msg, TRUE);

            wattroff(window->wins.cons.subwin, theme_attrs(presence_colour));

            if (prefs_get_boolean(PREF_ROSTER_RESOURCE)) {
                GList *resources = p_contact_get_available_resources(contact);
                GList *curr_resource = resources;
                while (curr_resource) {
                    Resource *resource = curr_resource->data;
                    const char *resource_presence = string_from_resource_presence(resource->presence);
                    theme_item_t resource_presence_colour = theme_main_presence_attrs(resource_presence);
                    wattron(window->wins.cons.subwin, theme_attrs(resource_presence_colour));

                    GString *msg = g_string_new("     ");
                    g_string_append(msg, resource->name);
                    win_printline_nowrap(window->wins.cons.subwin, msg->str);
                    g_string_free(msg, TRUE);

                    wattroff(window->wins.cons.subwin, theme_attrs(resource_presence_colour));

                    curr_resource = g_list_next(curr_resource);
                }
                g_list_free(resources);
            }
        }
    }
}

static void
_ui_roster_contacts_by_presence(const char * const presence, char *title)
{
    ProfWin *window = wins_get_console();
    wattron(window->wins.cons.subwin, theme_attrs(THEME_ROSTER_HEADER));
    win_printline_nowrap(window->wins.cons.subwin, title);
    wattroff(window->wins.cons.subwin, theme_attrs(THEME_ROSTER_HEADER));
    GSList *contacts = roster_get_contacts_by_presence(presence);
    if (contacts) {
        GSList *curr_contact = contacts;
        while (curr_contact) {
            PContact contact = curr_contact->data;
            _ui_roster_contact(contact);
            curr_contact = g_slist_next(curr_contact);
        }
    }
    g_slist_free(contacts);
}

static void
_ui_roster_contacts_by_group(char *group)
{
    ProfWin *window = wins_get_console();
    wattron(window->wins.cons.subwin, theme_attrs(THEME_ROSTER_HEADER));
    GString *title = g_string_new(" -");
    g_string_append(title, group);
    win_printline_nowrap(window->wins.cons.subwin, title->str);
    g_string_free(title, TRUE);
    wattroff(window->wins.cons.subwin, theme_attrs(THEME_ROSTER_HEADER));
    GSList *contacts = roster_get_group(group);
    if (contacts) {
        GSList *curr_contact = contacts;
        while (curr_contact) {
            PContact contact = curr_contact->data;
            _ui_roster_contact(contact);
            curr_contact = g_slist_next(curr_contact);
        }
    }
    g_slist_free(contacts);
}

static void
_ui_roster_contacts_by_no_group(void)
{
    ProfWin *window = wins_get_console();
    GSList *contacts = roster_get_nogroup();
    if (contacts) {
        wattron(window->wins.cons.subwin, theme_attrs(THEME_ROSTER_HEADER));
        win_printline_nowrap(window->wins.cons.subwin, " -no group");
        wattroff(window->wins.cons.subwin, theme_attrs(THEME_ROSTER_HEADER));
        GSList *curr_contact = contacts;
        while (curr_contact) {
            PContact contact = curr_contact->data;
            _ui_roster_contact(contact);
            curr_contact = g_slist_next(curr_contact);
        }
    }
    g_slist_free(contacts);
}

static void
_ui_roster(void)
{
    ProfWin *window = wins_get_console();
    if (window) {
        char *by = prefs_get_string(PREF_ROSTER_BY);
        if (g_strcmp0(by, "presence") == 0) {
            werase(window->wins.cons.subwin);
            _ui_roster_contacts_by_presence("chat", " -Available for chat");
            _ui_roster_contacts_by_presence("online", " -Online");
            _ui_roster_contacts_by_presence("away", " -Away");
            _ui_roster_contacts_by_presence("xa", " -Extended Away");
            _ui_roster_contacts_by_presence("dnd", " -Do not disturb");
            if (prefs_get_boolean(PREF_ROSTER_OFFLINE)) {
                _ui_roster_contacts_by_presence("offline", " -Offline");
            }
        } else if (g_strcmp0(by, "group") == 0) {
            werase(window->wins.cons.subwin);
            GSList *groups = roster_get_groups();
            GSList *curr_group = groups;
            while (curr_group) {
                _ui_roster_contacts_by_group(curr_group->data);
                curr_group = g_slist_next(curr_group);
            }
            g_slist_free_full(groups, free);
            _ui_roster_contacts_by_no_group();
        } else {
            GSList *contacts = roster_get_contacts();
            if (contacts) {
                werase(window->wins.cons.subwin);
                wattron(window->wins.cons.subwin, theme_attrs(THEME_ROSTER_HEADER));
                win_printline_nowrap(window->wins.cons.subwin, " -Roster");
                wattroff(window->wins.cons.subwin, theme_attrs(THEME_ROSTER_HEADER));
                GSList *curr_contact = contacts;
                while (curr_contact) {
                    PContact contact = curr_contact->data;
                    _ui_roster_contact(contact);
                    curr_contact = g_slist_next(curr_contact);
                }
            }
            g_slist_free(contacts);
        }
        free(by);
    }
}

static void
_ui_muc_roster(const char * const room)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window) {
        GList *occupants = muc_roster(room);
        if (occupants) {
            werase(window->wins.muc.subwin);

            if (prefs_get_boolean(PREF_MUC_PRIVILEGES)) {
                wattron(window->wins.muc.subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                win_printline_nowrap(window->wins.muc.subwin, " -Moderators");
                wattroff(window->wins.muc.subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                GList *roster_curr = occupants;
                while (roster_curr) {
                    Occupant *occupant = roster_curr->data;
                    if (occupant->role == MUC_ROLE_MODERATOR) {
                        const char *presence_str = string_from_resource_presence(occupant->presence);
                        theme_item_t presence_colour = theme_main_presence_attrs(presence_str);
                        wattron(window->wins.muc.subwin, theme_attrs(presence_colour));

                        GString *msg = g_string_new("   ");
                        g_string_append(msg, occupant->nick);
                        win_printline_nowrap(window->wins.muc.subwin, msg->str);
                        g_string_free(msg, TRUE);

                        wattroff(window->wins.muc.subwin, theme_attrs(presence_colour));
                    }
                    roster_curr = g_list_next(roster_curr);
                }

                wattron(window->wins.muc.subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                win_printline_nowrap(window->wins.muc.subwin, " -Participants");
                wattroff(window->wins.muc.subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                roster_curr = occupants;
                while (roster_curr) {
                    Occupant *occupant = roster_curr->data;
                    if (occupant->role == MUC_ROLE_PARTICIPANT) {
                        const char *presence_str = string_from_resource_presence(occupant->presence);
                        theme_item_t presence_colour = theme_main_presence_attrs(presence_str);
                        wattron(window->wins.muc.subwin, theme_attrs(presence_colour));

                        GString *msg = g_string_new("   ");
                        g_string_append(msg, occupant->nick);
                        win_printline_nowrap(window->wins.muc.subwin, msg->str);
                        g_string_free(msg, TRUE);

                        wattroff(window->wins.muc.subwin, theme_attrs(presence_colour));
                    }
                    roster_curr = g_list_next(roster_curr);
                }

                wattron(window->wins.muc.subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                win_printline_nowrap(window->wins.muc.subwin, " -Visitors");
                wattroff(window->wins.muc.subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                roster_curr = occupants;
                while (roster_curr) {
                    Occupant *occupant = roster_curr->data;
                    if (occupant->role == MUC_ROLE_VISITOR) {
                        const char *presence_str = string_from_resource_presence(occupant->presence);
                        theme_item_t presence_colour = theme_main_presence_attrs(presence_str);
                        wattron(window->wins.muc.subwin, theme_attrs(presence_colour));

                        GString *msg = g_string_new("   ");
                        g_string_append(msg, occupant->nick);
                        win_printline_nowrap(window->wins.muc.subwin, msg->str);
                        g_string_free(msg, TRUE);

                        wattroff(window->wins.muc.subwin, theme_attrs(presence_colour));
                    }
                    roster_curr = g_list_next(roster_curr);
                }
            } else {
                wattron(window->wins.muc.subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                win_printline_nowrap(window->wins.muc.subwin, " -Occupants\n");
                wattroff(window->wins.muc.subwin, theme_attrs(THEME_OCCUPANTS_HEADER));
                GList *roster_curr = occupants;
                while (roster_curr) {
                    Occupant *occupant = roster_curr->data;
                    const char *presence_str = string_from_resource_presence(occupant->presence);
                    theme_item_t presence_colour = theme_main_presence_attrs(presence_str);
                    wattron(window->wins.muc.subwin, theme_attrs(presence_colour));

                    GString *msg = g_string_new("   ");
                    g_string_append(msg, occupant->nick);
                    win_printline_nowrap(window->wins.muc.subwin, msg->str);
                    g_string_free(msg, TRUE);

                    wattroff(window->wins.muc.subwin, theme_attrs(presence_colour));
                    roster_curr = g_list_next(roster_curr);
                }
            }
        }

        g_list_free(occupants);
    }
}

static void
_ui_room_show_occupants(const char * const room)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window && !window->wins.muc.subwin) {
        wins_show_subwin(window);
        ui_muc_roster(room);
    }
}

static void
_ui_room_hide_occupants(const char * const room)
{
    ProfWin *window = wins_get_by_recipient(room);
    if (window && window->wins.muc.subwin) {
        wins_hide_subwin(window);
    }
}

static void
_ui_show_roster(void)
{
    ProfWin *window = wins_get_console();
    if (window && !window->wins.cons.subwin) {
        wins_show_subwin(window);
        ui_roster();
    }
}

static void
_ui_hide_roster(void)
{
    ProfWin *window = wins_get_console();
    if (window && window->wins.cons.subwin) {
        wins_hide_subwin(window);
    }
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
_win_handle_page(const wint_t * const ch, const int result)
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
                    win_update_virtual(current);
                } else if (mouse_event.bstate & BUTTON4_PRESSED) { // mouse wheel up
                    *page_start -= 4;

                    // went past beginning, show first page
                    if (*page_start < 0)
                        *page_start = 0;

                    current->paged = 1;
                    win_update_virtual(current);
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
        win_update_virtual(current);

    // page down
    } else if (*ch == KEY_NPAGE) {
        *page_start += page_space;

        // only got half a screen, show full screen
        if ((y - (*page_start)) < page_space)
            *page_start = y - page_space;

        // went past end, show full screen
        else if (*page_start >= y)
            *page_start = y - page_space - 1;

        current->paged = 1;
        win_update_virtual(current);
    }

    // switch off page if last line and space line visible
    if ((y) - *page_start == page_space) {
        current->paged = 0;
    }

    if ((current->type == WIN_MUC) || (current->type == WIN_CONSOLE)) {
        int sub_y = 0;
        int *sub_y_pos = NULL;

        if (current->type == WIN_MUC) {
            sub_y = getcury(current->wins.muc.subwin);
            sub_y_pos = &(current->wins.muc.sub_y_pos);
        } else if (current->type == WIN_CONSOLE) {
            sub_y = getcury(current->wins.cons.subwin);
            sub_y_pos = &(current->wins.cons.sub_y_pos);
        }

        // alt up arrow
        if ((result == KEY_CODE_YES) && ((*ch == 565) || (*ch == 337))) {
            *sub_y_pos -= page_space;

            // went past beginning, show first page
            if (*sub_y_pos < 0)
                *sub_y_pos = 0;

            win_update_virtual(current);

        // alt down arrow
        } else if ((result == KEY_CODE_YES) && ((*ch == 524) || (*ch == 336))) {
            *sub_y_pos += page_space;

            // only got half a screen, show full screen
            if ((sub_y- (*sub_y_pos)) < page_space)
                *sub_y_pos = sub_y - page_space;

            // went past end, show full screen
            else if (*sub_y_pos >= sub_y)
                *sub_y_pos = sub_y - page_space - 1;

            win_update_virtual(current);
        }
    }
}

static void
_win_show_history(int win_index, const char * const contact)
{
    ProfWin *window = wins_get_by_num(win_index);
    if (window->type == WIN_CHAT && !window->wins.chat.history_shown) {
        Jid *jid = jid_create(jabber_get_fulljid());
        GSList *history = chat_log_get_previous(jid->barejid, contact);
        jid_destroy(jid);
        GSList *curr = history;
        while (curr != NULL) {
            char *line = curr->data;
            // entry
            if (line[2] == ':') {
                char hh[3]; memcpy(hh, &line[0], 2); hh[2] = '\0'; int ihh = atoi(hh);
                char mm[3]; memcpy(mm, &line[3], 2); mm[2] = '\0'; int imm = atoi(mm);
                char ss[3]; memcpy(ss, &line[6], 2); ss[2] = '\0'; int iss = atoi(ss);
                GDateTime *time = g_date_time_new_local(2000, 1, 1, ihh, imm, iss);
                GTimeVal tv;
                g_date_time_to_timeval(time, &tv);
                win_save_print(window, '-', &tv, NO_COLOUR_DATE, 0, "", curr->data+11);
                g_date_time_unref(time);
            // header
            } else {
                win_save_print(window, '-', NULL, 0, 0, "", curr->data);
            }
            curr = g_slist_next(curr);
        }
        window->wins.chat.history_shown = TRUE;

        g_slist_free_full(history, free);
    }
}

void
ui_init_module(void)
{
    ui_init = _ui_init;
    ui_get_idle_time = _ui_get_idle_time;
    ui_reset_idle_time = _ui_reset_idle_time;
    ui_close = _ui_close;
    ui_resize = _ui_resize;
    ui_load_colours = _ui_load_colours;
    ui_win_exists = _ui_win_exists;
    ui_contact_typing = _ui_contact_typing;
    ui_get_recipients = _ui_get_recipients;
    ui_incoming_msg = _ui_incoming_msg;
    ui_incoming_private_msg = _ui_incoming_private_msg;
    ui_roster_add = _ui_roster_add;
    ui_roster_remove = _ui_roster_remove;
    ui_contact_already_in_group = _ui_contact_already_in_group;
    ui_contact_not_in_group = _ui_contact_not_in_group;
    ui_group_added = _ui_group_added;
    ui_group_removed = _ui_group_removed;
    ui_disconnected = _ui_disconnected;
    ui_handle_special_keys = _ui_handle_special_keys;
    ui_close_connected_win = _ui_close_connected_win;
    ui_close_all_wins = _ui_close_all_wins;
    ui_close_read_wins = _ui_close_read_wins;
    ui_switch_win = _ui_switch_win;
    ui_next_win = _ui_next_win;
    ui_previous_win = _ui_previous_win;
    ui_clear_current = _ui_clear_current;
    ui_close_current = _ui_close_current;
    ui_close_win = _ui_close_win;
    ui_tidy_wins = _ui_tidy_wins;
    ui_prune_wins = _ui_prune_wins;
    ui_current_win_type = _ui_current_win_type;
    ui_current_win_index = _ui_current_win_index;
    ui_win_type = _ui_win_type;
    ui_recipient = _ui_recipient;
    ui_current_recipient = _ui_current_recipient;
    ui_current_print_line = _ui_current_print_line;
    ui_current_print_formatted_line = _ui_current_print_formatted_line;
    ui_current_error_line = _ui_current_error_line;
    ui_print_system_msg_from_recipient = _ui_print_system_msg_from_recipient;
    ui_recipient_gone = _ui_recipient_gone;
    ui_new_chat_win = _ui_new_chat_win;
    ui_outgoing_msg = _ui_outgoing_msg;
    ui_room_join = _ui_room_join;
    ui_room_roster = _ui_room_roster;
    ui_room_member_offline = _ui_room_member_offline;
    ui_room_member_online = _ui_room_member_online;
    ui_room_member_presence = _ui_room_member_presence;
    ui_room_member_nick_change = _ui_room_member_nick_change;
    ui_room_nick_change = _ui_room_nick_change;
    ui_room_history = _ui_room_history;
    ui_room_message = _ui_room_message;
    ui_room_subject = _ui_room_subject;
    ui_room_broadcast = _ui_room_broadcast;
    ui_status = _ui_status;
    ui_info = _ui_info;
    ui_status_private = _ui_status_private;
    ui_info_private = _ui_info_private;
    ui_status_room = _ui_status_room;
    ui_info_room = _ui_info_room;
    ui_unread = _ui_unread;
    ui_win_unread = _ui_win_unread;
    ui_ask_password = _ui_ask_password;
    ui_current_win_is_otr = _ui_current_win_is_otr;
    ui_current_set_otr = _ui_current_set_otr;
    ui_otr_authenticating = _ui_otr_authenticating;
    ui_otr_authetication_waiting = _ui_otr_authetication_waiting;
    ui_gone_secure = _ui_gone_secure;
    ui_gone_insecure = _ui_gone_insecure;
    ui_trust = _ui_trust;
    ui_untrust = _ui_untrust;
    ui_smp_recipient_initiated = _ui_smp_recipient_initiated;
    ui_smp_recipient_initiated_q = _ui_smp_recipient_initiated_q;
    ui_smp_successful = _ui_smp_successful;
    ui_smp_unsuccessful_sender = _ui_smp_unsuccessful_sender;
    ui_smp_unsuccessful_receiver = _ui_smp_unsuccessful_receiver;
    ui_smp_aborted = _ui_smp_aborted;
    ui_smp_answer_success = _ui_smp_answer_success;
    ui_smp_answer_failure = _ui_smp_answer_failure;
    ui_chat_win_contact_online = _ui_chat_win_contact_online;
    ui_chat_win_contact_offline = _ui_chat_win_contact_offline;
    ui_handle_recipient_not_found = _ui_handle_recipient_not_found;
    ui_handle_recipient_error = _ui_handle_recipient_error;
    ui_handle_error = _ui_handle_error;
    ui_clear_win_title = _ui_clear_win_title;
    ui_auto_away = _ui_auto_away;
    ui_end_auto_away = _ui_end_auto_away;
    ui_titlebar_presence = _ui_titlebar_presence;
    ui_handle_login_account_success = _ui_handle_login_account_success;
    ui_update_presence =_ui_update_presence;
    ui_about = _ui_about;
    ui_statusbar_new = _ui_statusbar_new;
    ui_get_char = _ui_get_char;
    ui_input_clear = _ui_input_clear;
    ui_input_nonblocking = _ui_input_nonblocking;
    ui_replace_input = _ui_replace_input;
    ui_invalid_command_usage = _ui_invalid_command_usage;
    ui_handle_stanza = _ui_handle_stanza;
    ui_create_xmlconsole_win = _ui_create_xmlconsole_win;
    ui_xmlconsole_exists = _ui_xmlconsole_exists;
    ui_open_xmlconsole_win = _ui_open_xmlconsole_win;
    ui_handle_room_join_error = _ui_handle_room_join_error;
    ui_swap_wins = _ui_swap_wins;
    ui_update = _ui_update;
    ui_room_requires_config = _ui_room_requires_config;
    ui_room_destroy = _ui_room_destroy;
    ui_handle_room_configuration = _ui_handle_room_configuration;
    ui_handle_room_config_submit_result = _ui_handle_room_config_submit_result;
    ui_handle_room_config_submit_result_error = _ui_handle_room_config_submit_result_error;
    ui_win_has_unsaved_form = _ui_win_has_unsaved_form;
    ui_show_form = _ui_show_form;
    ui_show_form_field = _ui_show_form_field;
    ui_show_form_help = _ui_show_form_help;
    ui_show_form_field_help = _ui_show_form_field_help;
    ui_show_lines = _ui_show_lines;
    ui_handle_room_configuration_form_error = _ui_handle_room_configuration_form_error;
    ui_show_room_info = _ui_show_room_info;
    ui_show_room_role_list = _ui_show_room_role_list;
    ui_show_room_affiliation_list = _ui_show_room_affiliation_list;
    ui_handle_room_info_error = _ui_handle_room_info_error;
    ui_show_room_disco_info = _ui_show_room_disco_info;
    ui_handle_room_affiliation_list_error = _ui_handle_room_affiliation_list_error;
    ui_handle_room_affiliation_list = _ui_handle_room_affiliation_list;
    ui_handle_room_affiliation_set_error = _ui_handle_room_affiliation_set_error;
    ui_handle_room_kick_error = _ui_handle_room_kick_error;
    ui_room_destroyed = _ui_room_destroyed;
    ui_room_kicked = _ui_room_kicked;
    ui_room_banned = _ui_room_banned;
    ui_leave_room = _ui_leave_room;
    ui_room_member_kicked = _ui_room_member_kicked;
    ui_room_member_banned = _ui_room_member_banned;
    ui_handle_room_role_set_error = _ui_handle_room_role_set_error;
    ui_handle_room_role_list_error = _ui_handle_room_role_list_error;
    ui_handle_room_role_list = _ui_handle_room_role_list;
    ui_muc_roster = _ui_muc_roster;
    ui_roster = _ui_roster;
    ui_room_show_occupants = _ui_room_show_occupants;
    ui_room_hide_occupants = _ui_room_hide_occupants;
    ui_show_roster = _ui_show_roster;
    ui_hide_roster = _ui_hide_roster;
    ui_room_role_change = _ui_room_role_change;
    ui_room_affiliation_change = _ui_room_affiliation_change;
    ui_switch_to_room = _ui_switch_to_room;
    ui_room_role_and_affiliation_change = _ui_room_role_and_affiliation_change;
    ui_room_occupant_role_change = _ui_room_occupant_role_change;
    ui_room_occupant_affiliation_change = _ui_room_occupant_affiliation_change;
    ui_room_occupant_role_and_affiliation_change = _ui_room_occupant_role_and_affiliation_change;
    ui_redraw_all_room_rosters = _ui_redraw_all_room_rosters;
    ui_redraw = _ui_redraw;
    ui_show_all_room_rosters = _ui_show_all_room_rosters;
    ui_hide_all_room_rosters = _ui_hide_all_room_rosters;
}

