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

static void _win_show_user(WINDOW *win, const char * const user, const int colour);
static void _win_show_message(WINDOW *win, const char * const message);
static void _win_handle_switch(const wint_t * const ch);
static void _win_handle_page(const wint_t * const ch);
static void _win_show_history(WINDOW *win, int win_index,
    const char * const contact);
static void _ui_draw_term_title(void);

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
    notifier_init();
#ifdef HAVE_LIBXSS
    display = XOpenDisplay(0);
#endif
    ui_idle_time = g_timer_new();
    ProfWin *window = wins_get_current();
    win_update_virtual(window);
}

static void
_ui_update_screen(void)
{
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
    if (ui_current_win_type() != WIN_CONSOLE) {
        status_bar_new(1);
    }
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
_ui_get_char(char *input, int *size)
{
    wint_t ch = inp_get_char(input, size);
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
_ui_resize(const int ch, const char * const input, const int size)
{
    log_info("Resizing UI");
    title_bar_resize();
    status_bar_resize();
    wins_resize_all();
    inp_win_resize(input, size);
    ProfWin *window = wins_get_current();
    win_update_virtual(window);
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
_ui_duck_exists(void)
{
    return wins_duck_exists();
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
            win_print_line_no_time(xmlconsole, 0, "SENT:");
            win_print_line_no_time(xmlconsole, COLOUR_ONLINE, &msg[6]);
            win_print_line_no_time(xmlconsole, COLOUR_ONLINE, "");
        } else if (g_str_has_prefix(msg, "RECV:")) {
            win_print_line_no_time(xmlconsole, 0, "RECV:");
            win_print_line_no_time(xmlconsole, COLOUR_AWAY, &msg[6]);
            win_print_line_no_time(xmlconsole, COLOUR_ONLINE, "");
        }

        if (wins_is_current(xmlconsole)) {
            win_update_virtual(xmlconsole);
            ui_current_page_off();
        }
    }
}

static void
_ui_contact_typing(const char * const barejid)
{
    ProfWin *window = wins_get_by_recipient(barejid);
    ProfWin *current = wins_get_current();

    if (prefs_get_boolean(PREF_INTYPE)) {
        // no chat window for user
        if (window == NULL) {
            cons_show_typing(barejid);

        // have chat window but not currently in it
        } else if (!wins_is_current(window)) {
            cons_show_typing(barejid);
            win_update_virtual(current);

        // in chat window with user
        } else {
            title_bar_set_typing(TRUE);

            int num = wins_get_num(window);
            status_bar_active(num);
            win_update_virtual(current);
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

static GSList *
_ui_get_recipients(void)
{
    GSList *recipients = wins_get_chat_recipients();
    return recipients;
}

static void
_ui_incoming_msg(const char * const from, const char * const message,
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
#ifdef HAVE_LIBOTR
        if (otr_is_secure(from)) {
            window->is_otr = TRUE;
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
        win_update_virtual(window);

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

        win_print_incoming_message(window, tv_stamp, display_from, message);
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

    ProfWin *current = wins_get_current();
    if (!current->paged) {
        win_move_to_end(current);
        win_update_virtual(current);
    }
}

static void
_ui_roster_add(const char * const barejid, const char * const name)
{
    if (name != NULL) {
        cons_show("Roster item added: %s (%s)", barejid, name);
    } else {
        cons_show("Roster item added: %s", barejid);
    }
}

static void
_ui_roster_remove(const char * const barejid)
{
    cons_show("Roster item removed: %s", barejid);
}

static void
_ui_contact_already_in_group(const char * const contact, const char * const group)
{
    cons_show("%s already in group %s", contact, group);
}

static void
_ui_contact_not_in_group(const char * const contact, const char * const group)
{
    cons_show("%s is not currently in group %s", contact, group);
}

static void
_ui_group_added(const char * const contact, const char * const group)
{
    cons_show("%s added to group %s", contact, group);
}

static void
_ui_group_removed(const char * const contact, const char * const group)
{
    cons_show("%s removed from group %s", contact, group);
}

static void
_ui_auto_away(void)
{
    if (prefs_get_string(PREF_AUTOAWAY_MESSAGE) != NULL) {
        int pri =
            accounts_get_priority_for_presence_type(jabber_get_account_name(),
                RESOURCE_AWAY);
        cons_show("Idle for %d minutes, status set to away (priority %d), \"%s\".",
            prefs_get_autoaway_time(), pri, prefs_get_string(PREF_AUTOAWAY_MESSAGE));
        title_bar_set_presence(CONTACT_AWAY);
        ui_current_page_off();
    } else {
        int pri =
            accounts_get_priority_for_presence_type(jabber_get_account_name(),
                RESOURCE_AWAY);
        cons_show("Idle for %d minutes, status set to away (priority %d).",
            prefs_get_autoaway_time(), pri);
        title_bar_set_presence(CONTACT_AWAY);
        ui_current_page_off();
    }
}

static void
_ui_end_auto_away(void)
{
    int pri =
        accounts_get_priority_for_presence_type(jabber_get_account_name(), RESOURCE_ONLINE);
    cons_show("No longer idle, status set to online (priority %d).", pri);
    title_bar_set_presence(CONTACT_ONLINE);
    ui_current_page_off();
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
    ui_current_page_off();
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
        win_print_line(win, '!', COLOUR_ERROR, msg->str);

    // unknown chat recipient
    } else {
        g_string_printf(msg, "Recipient %s not found: %s", recipient, err_msg);
        cons_show_error(msg->str);
        win_print_line(win, '!', COLOUR_ERROR, msg->str);
    }

    ProfWin *current = wins_get_current();
    win_update_virtual(current);

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
        win_print_line(win, '!', COLOUR_ERROR, msg->str);
    }

    ProfWin *current = wins_get_current();
    win_update_virtual(current);

    g_string_free(msg, TRUE);
}

static void
_ui_handle_error(const char * const err_msg)
{
    GString *msg = g_string_new("");
    g_string_printf(msg, "Error %s", err_msg);

    cons_show_error(msg->str);

    ProfWin *current = wins_get_current();
    win_update_virtual(current);

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
}

static void
_ui_handle_special_keys(const wint_t * const ch, const char * const inp,
    const int size)
{
    _win_handle_switch(ch);
    _win_handle_page(ch);
    if (*ch == KEY_RESIZE) {
        ui_resize(*ch, inp, size);
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
        if (window->is_otr) {
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

static int
_ui_close_read_wins(void)
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
        ProfWin *new_current = wins_get_by_num(i);
        wins_set_current_by_num(i);
        ui_current_page_off();

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
        win_update_virtual(new_current);
        return TRUE;
    } else {
        return FALSE;
    }
}

static void
_ui_current_update_virtual(void)
{
    ui_switch_win(wins_get_current_num());
}

static void
_ui_next_win(void)
{
    ui_current_page_off();
    ProfWin *new_current = wins_get_next();
    int i = wins_get_num(new_current);
    wins_set_current_by_num(i);
    ui_current_page_off();

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
    win_update_virtual(new_current);
}

static void
_ui_gone_secure(const char * const recipient, gboolean trusted)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        window = wins_new(recipient, WIN_CHAT);
    }

    window->is_otr = TRUE;
    window->is_trusted = trusted;
    if (trusted) {
        win_vprint_line(window, '!', COLOUR_OTR_STARTED_TRUSTED, "OTR session started (trusted).");
    } else {
        win_vprint_line(window, '!', COLOUR_OTR_STARTED_UNTRUSTED, "OTR session started (untrusted).");
    }

    if (wins_is_current(window)) {
        GString *recipient_str = _get_recipient_string(window);
        title_bar_set_recipient(recipient_str->str);
        g_string_free(recipient_str, TRUE);
        win_update_virtual(window);
        ui_current_page_off();
    } else {
        int num = wins_get_num(window);
        status_bar_new(num);

        int ui_index = num;
        if (ui_index == 10) {
            ui_index = 0;
        }
        cons_show("%s started an OTR session (%d).", recipient, ui_index);
        ProfWin *console = wins_get_console();
        if (wins_is_current(console)) {
            ui_current_page_off();
        }
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
        win_vprint_line(window, '!', 0, "%s wants to authenticate your identity, use '/otr secret <secret>'.", recipient);
        win_update_virtual(window);
        if (wins_is_current(window)) {
            ui_current_page_off();
        }
    }
}

static void
_ui_smp_recipient_initiated_q(const char * const recipient, const char *question)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_vprint_line(window, '!', 0, "%s wants to authenticate your identity with the following question:", recipient);
        win_vprint_line(window, '!', 0, "  %s", question);
        win_vprint_line(window, '!', 0, "use '/otr answer <answer>'.");
        win_update_virtual(window);
        if (wins_is_current(window)) {
            ui_current_page_off();
        }
    }
}

static void
_ui_smp_unsuccessful_sender(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_vprint_line(window, '!', 0, "Authentication failed, the secret you entered does not match the secret entered by %s.", recipient);
        win_update_virtual(window);
        if (wins_is_current(window)) {
            ui_current_page_off();
        }
    }
}

static void
_ui_smp_unsuccessful_receiver(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_vprint_line(window, '!', 0, "Authentication failed, the secret entered by %s does not match yours.", recipient);
        win_update_virtual(window);
        if (wins_is_current(window)) {
            ui_current_page_off();
        }
    }
}

static void
_ui_smp_aborted(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_vprint_line(window, '!', 0, "SMP session aborted.");
        win_update_virtual(window);
        if (wins_is_current(window)) {
            ui_current_page_off();
        }
    }
}

static void
_ui_smp_successful(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_vprint_line(window, '!', 0, "Authentication successful.");
        win_update_virtual(window);
        if (wins_is_current(window)) {
            ui_current_page_off();
        }
    }
}

static void
_ui_smp_answer_success(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_vprint_line(window, '!', 0, "%s successfully authenticated you.", recipient);
        win_update_virtual(window);
        if (wins_is_current(window)) {
            ui_current_page_off();
        }
    }
}

static void
_ui_smp_answer_failure(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_vprint_line(window, '!', 0, "%s failed to authenticated you.", recipient);
        win_update_virtual(window);
        if (wins_is_current(window)) {
            ui_current_page_off();
        }
    }
}

static void
_ui_gone_insecure(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window != NULL) {
        window->is_otr = FALSE;
        window->is_trusted = FALSE;
        win_vprint_line(window, '!', COLOUR_OTR_ENDED, "OTR session ended.");

        if (wins_is_current(window)) {
            GString *recipient_str = _get_recipient_string(window);
            title_bar_set_recipient(recipient_str->str);
            g_string_free(recipient_str, TRUE);
            win_update_virtual(window);
            ui_current_page_off();
        }
    }
}

static void
_ui_trust(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window != NULL) {
        window->is_otr = TRUE;
        window->is_trusted = TRUE;
        win_vprint_line(window, '!', COLOUR_OTR_TRUSTED, "OTR session trusted.");

        if (wins_is_current(window)) {
            GString *recipient_str = _get_recipient_string(window);
            title_bar_set_recipient(recipient_str->str);
            g_string_free(recipient_str, TRUE);
            win_update_virtual(window);
            ui_current_page_off();
        }
    }
}

static void
_ui_untrust(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window != NULL) {
        window->is_otr = TRUE;
        window->is_trusted = FALSE;
        win_vprint_line(window, '!', COLOUR_OTR_UNTRUSTED, "OTR session untrusted.");

        if (wins_is_current(window)) {
            GString *recipient_str = _get_recipient_string(window);
            title_bar_set_recipient(recipient_str->str);
            g_string_free(recipient_str, TRUE);
            win_update_virtual(window);
            ui_current_page_off();
        }
    }
}

static void
_ui_previous_win(void)
{
    ui_current_page_off();
    ProfWin *new_current = wins_get_previous();
    int i = wins_get_num(new_current);
    wins_set_current_by_num(i);
    ui_current_page_off();

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
    win_update_virtual(new_current);
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
    wins_close_by_num(index);
    title_bar_console();
    status_bar_current(1);
    status_bar_active(1);

    ProfWin *current = wins_get_current();
    win_update_virtual(current);
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
    return current->is_otr;
}

static void
_ui_current_set_otr(gboolean value)
{
    ProfWin *current = wins_get_current();
    current->is_otr = value;
}

static void
_ui_otr_authenticating(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_vprint_line(window, '!', 0, "Authenticating %s...", recipient);
        win_update_virtual(window);
        if (wins_is_current(window)) {
            ui_current_page_off();
        }
    }
}

static void
_ui_otr_authetication_waiting(const char * const recipient)
{
    ProfWin *window = wins_get_by_recipient(recipient);
    if (window == NULL) {
        return;
    } else {
        win_vprint_line(window, '!', 0, "Awaiting authentication from %s...", recipient);
        win_update_virtual(window);
        if (wins_is_current(window)) {
            ui_current_page_off();
        }
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
    ProfWin *current = wins_get_current();
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    win_print_line(current, '-', 0, fmt_msg->str);
    va_end(arg);
    g_string_free(fmt_msg, TRUE);
    win_update_virtual(current);
}

static void
_ui_current_print_formatted_line(const char show_char, int attrs, const char * const msg, ...)
{
    ProfWin *current = wins_get_current();
    va_list arg;
    va_start(arg, msg);
    GString *fmt_msg = g_string_new(NULL);
    g_string_vprintf(fmt_msg, msg, arg);
    win_print_line(current, show_char, attrs, fmt_msg->str);
    va_end(arg);
    g_string_free(fmt_msg, TRUE);
    win_update_virtual(current);
}

static void
_ui_current_error_line(const char * const msg)
{
    ProfWin *current = wins_get_current();
    win_print_line(current, '-', COLOUR_ERROR, msg);
    win_update_virtual(current);
}

static void
_ui_current_page_off(void)
{
    ProfWin *current = wins_get_current();
    win_move_to_end(current);
    win_update_virtual(current);
}

static void
_ui_print_system_msg_from_recipient(const char * const from, const char *message)
{
    if (from == NULL || message == NULL)
        return;

    Jid *jid = jid_create(from);

    ProfWin *window = wins_get_by_recipient(jid->barejid);
    if (window == NULL) {
        int num = 0;
        window = wins_new(jid->barejid, WIN_CHAT);
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
    wprintw(window->win, "*%s %s\n", jid->barejid, message);

    // this is the current window
    if (wins_is_current(window)) {
        win_update_virtual(window);
    }

    jid_destroy(jid);
}

static void
_ui_recipient_gone(const char * const barejid)
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
        win_vprint_line(window, '!', COLOUR_GONE, "<- %s has left the conversation.", display_usr);
        if (wins_is_current(window)) {
            win_update_virtual(window);
        }
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

        if (muc_room_is_active(jid->barejid)) {
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
_ui_create_duck_win(void)
{
    ProfWin *window = wins_new("DuckDuckGo search", WIN_DUCK);
    int num = wins_get_num(window);
    ui_switch_win(num);
    win_print_line(window, '-', 0, "Type ':help' to find out more.");
}

static void
_ui_create_xmlconsole_win(void)
{
    ProfWin *window = wins_new("XML Console", WIN_XML);
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
_ui_open_duck_win(void)
{
    ProfWin *window = wins_get_by_recipient("DuckDuckGo search");
    if (window != NULL) {
        int num = wins_get_num(window);
        ui_switch_win(num);
    }
}

static void
_ui_duck(const char * const query)
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

static void
_ui_duck_result(const char * const result)
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

        if (muc_room_is_active(jid->barejid)) {
            window = wins_new(to, WIN_PRIVATE);
        } else {
            window = wins_new(to, WIN_CHAT);
#ifdef HAVE_LIBOTR
            if (otr_is_secure(to)) {
                window->is_otr = TRUE;
            }
#endif
        }

        jid_destroy(jid);
        num = wins_get_num(window);

        if (prefs_get_boolean(PREF_CHLOG) && prefs_get_boolean(PREF_HISTORY)) {
            _win_show_history(window->win, num, to);
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

static void
_ui_room_join(const char * const room, gboolean focus)
{
    ProfWin *window = wins_get_by_recipient(room);
    int num = 0;

    // create new window
    if (window == NULL) {
        window = wins_new(room, WIN_MUC);
    }

    num = wins_get_num(window);

    if (focus) {
        ui_switch_win(num);
    } else {
        status_bar_active(num);
        ProfWin *console = wins_get_console();
        char *nick = muc_get_room_nick(room);
        win_vprint_line(console, '!', COLOUR_TYPING, "-> Autojoined %s as %s (%d).", room, nick, num);
        win_update_virtual(console);
    }
}

static void
_ui_room_roster(const char * const room, GList *roster, const char * const presence)
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
            const char *nick = p_contact_barejid(member);
            const char *show = p_contact_presence(member);

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
        win_update_virtual(window);
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

    win_print_time(window, '!');
    wattron(window->win, COLOUR_OFFLINE);
    wprintw(window->win, "<- %s has left the room.\n", nick);
    wattroff(window->win, COLOUR_OFFLINE);

    if (wins_is_current(window)) {
        win_update_virtual(window);
    }
}

static void
_ui_room_member_online(const char * const room, const char * const nick,
    const char * const show, const char * const status)
{
    ProfWin *window = wins_get_by_recipient(room);

    win_print_time(window, '!');
    wattron(window->win, COLOUR_ONLINE);
    wprintw(window->win, "-> %s has joined the room.\n", nick);
    wattroff(window->win, COLOUR_ONLINE);

    if (wins_is_current(window)) {
        win_update_virtual(window);
    }
}

static void
_ui_room_member_presence(const char * const room, const char * const nick,
    const char * const show, const char * const status)
{
    ProfWin *window = wins_get_by_recipient(room);

    if (window != NULL) {
        win_show_status_string(window, nick, show, status, NULL, "++", "online");
    }

    if (wins_is_current(window)) {
        win_update_virtual(window);
    }
}

static void
_ui_room_member_nick_change(const char * const room,
    const char * const old_nick, const char * const nick)
{
    ProfWin *window = wins_get_by_recipient(room);

    win_print_time(window, '!');
    wattron(window->win, COLOUR_THEM);
    wprintw(window->win, "** %s is now known as %s\n", old_nick, nick);
    wattroff(window->win, COLOUR_THEM);

    if (wins_is_current(window)) {
        win_update_virtual(window);
    }
}

static void
_ui_room_nick_change(const char * const room, const char * const nick)
{
    ProfWin *window = wins_get_by_recipient(room);

    win_print_time(window, '!');
    wattron(window->win, COLOUR_ME);
    wprintw(window->win, "** You are now known as %s\n", nick);
    wattroff(window->win, COLOUR_ME);

    if (wins_is_current(window)) {
        win_update_virtual(window);
    }
}

static void
_ui_room_history(const char * const room_jid, const char * const nick,
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
        win_update_virtual(window);
    }
}

static void
_ui_room_message(const char * const room_jid, const char * const nick,
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
        win_update_virtual(window);

    // not currenlty on groupchat window
    } else {
        status_bar_new(num);
        cons_show_incoming_message(nick, num);
        if (wins_get_current_num() == 0) {
            ProfWin *current = wins_get_current();
            win_update_virtual(current);
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

    ProfWin *current = wins_get_current();
    if (!current->paged) {
        win_move_to_end(current);
        win_update_virtual(current);
    }
}

static void
_ui_room_subject(const char * const room_jid, const char * const subject)
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
        win_update_virtual(window);

    // not currenlty on groupchat window
    } else {
        status_bar_new(num);
    }
}

static void
_ui_room_broadcast(const char * const room_jid, const char * const message)
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
        win_update_virtual(window);

    // not currenlty on groupchat window
    } else {
        status_bar_new(num);
    }
}

static void
_ui_status(void)
{
    char *recipient = ui_current_recipient();
    PContact pcontact = roster_get_contact(recipient);
    ProfWin *current = wins_get_current();

    if (pcontact != NULL) {
        win_show_contact(current, pcontact);
    } else {
        win_print_line(current, '-', 0, "Error getting contact info.");
    }
}

static void
_ui_status_private(void)
{
    Jid *jid = jid_create(ui_current_recipient());
    PContact pcontact = muc_get_participant(jid->barejid, jid->resourcepart);
    ProfWin *current = wins_get_current();

    if (pcontact != NULL) {
        win_show_contact(current, pcontact);
    } else {
        win_print_line(current, '-', 0, "Error getting contact info.");
    }

    jid_destroy(jid);
}

static void
_ui_status_room(const char * const contact)
{
    PContact pcontact = muc_get_participant(ui_current_recipient(), contact);
    ProfWin *current = wins_get_current();

    if (pcontact != NULL) {
        win_show_contact(current, pcontact);
    } else {
        win_vprint_line(current, '-', 0, "No such participant \"%s\" in room.", contact);
    }
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

        if (wins_is_current(window)) {
            win_update_virtual(window);
            ui_current_page_off();
        }
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

        if (wins_is_current(window)) {
            win_update_virtual(window);
            ui_current_page_off();
        }
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
            *page_start = y - page_space;

        current->paged = 1;
        win_update_virtual(current);
    }

    // switch off page if last line visible
    if ((y-1) - *page_start == page_space) {
        current->paged = 0;
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

void
ui_init_module(void)
{
    ui_init = _ui_init;
    ui_update_screen = _ui_update_screen;
    ui_get_idle_time = _ui_get_idle_time;
    ui_reset_idle_time = _ui_reset_idle_time;
    ui_close = _ui_close;
    ui_resize = _ui_resize;
    ui_load_colours = _ui_load_colours;
    ui_win_exists = _ui_win_exists;
    ui_duck_exists = _ui_duck_exists;
    ui_contact_typing = _ui_contact_typing;
    ui_get_recipients = _ui_get_recipients;
    ui_incoming_msg = _ui_incoming_msg;
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
    ui_current_page_off = _ui_current_page_off;
    ui_print_system_msg_from_recipient = _ui_print_system_msg_from_recipient;
    ui_recipient_gone = _ui_recipient_gone;
    ui_new_chat_win = _ui_new_chat_win;
    ui_create_duck_win = _ui_create_duck_win;
    ui_open_duck_win = _ui_open_duck_win;
    ui_duck = _ui_duck;
    ui_duck_result = _ui_duck_result;
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
    ui_status_private = _ui_status_private;
    ui_status_room = _ui_status_room;
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
    ui_current_update_virtual = _ui_current_update_virtual;
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
}
