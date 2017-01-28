/*
 * core.c
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

#include "config.h"

#ifdef HAVE_GIT_VERSION
#include "gitversion.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <glib.h>

#ifdef HAVE_LIBXSS
#include <X11/extensions/scrnsaver.h>
#endif

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "log.h"
#include "common.h"
#include "command/cmd_defs.h"
#include "command/cmd_ac.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "ui/ui.h"
#include "ui/titlebar.h"
#include "ui/statusbar.h"
#include "ui/inputwin.h"
#include "ui/window.h"
#include "ui/window_list.h"
#include "xmpp/xmpp.h"
#include "xmpp/muc.h"
#include "xmpp/chat_session.h"
#include "xmpp/contact.h"
#include "xmpp/roster_list.h"
#include "xmpp/jid.h"

#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif

static char *win_title;
static int inp_size;
static gboolean perform_resize = FALSE;
static GTimer *ui_idle_time;

#ifdef HAVE_LIBXSS
static Display *display;
#endif

static void _ui_draw_term_title(void);

void
ui_init(void)
{
    log_info("Initialising UI");
    initscr();
    nonl();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    ui_load_colours();
    refresh();
    create_title_bar();
    create_status_bar();
    status_bar_active(1);
    create_input_window();
    wins_init();
    notifier_initialise();
    cons_about();
#ifdef HAVE_LIBXSS
    display = XOpenDisplay(0);
#endif
    ui_idle_time = g_timer_new();
    inp_size = 0;
    ProfWin *window = wins_get_current();
    win_update_virtual(window);
}

void
ui_sigwinch_handler(int sig)
{
    perform_resize = TRUE;
}

void
ui_update(void)
{
    ProfWin *current = wins_get_current();
    if (current->layout->paged == 0) {
        win_move_to_end(current);
    }

    win_update_virtual(current);

    if (prefs_get_boolean(PREF_WINTITLE_SHOW)) {
        _ui_draw_term_title();
    }
    title_bar_update_virtual();
    status_bar_update_virtual();
    inp_put_back();
    doupdate();

    if (perform_resize) {
        signal(SIGWINCH, SIG_IGN);
        ui_resize();
        perform_resize = FALSE;
        signal(SIGWINCH, ui_sigwinch_handler);
    }
}

unsigned long
ui_get_idle_time(void)
{
// if compiled with libxss, get the x sessions idle time
#ifdef HAVE_LIBXSS
    XScreenSaverInfo *info = XScreenSaverAllocInfo();
    if (info && display) {
        XScreenSaverQueryInfo(display, DefaultRootWindow(display), info);
        unsigned long result = info->idle;
        XFree(info);
        return result;
    }
    if (info) {
        XFree(info);
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
    inp_close();
    endwin();
}

void
ui_resize(void)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    erase();
    resizeterm(w.ws_row, w.ws_col);
    refresh();

    log_debug("Resizing UI");
    title_bar_resize();
    wins_resize_all();
    status_bar_resize();
    inp_win_resize();
    ProfWin *window = wins_get_current();
    win_update_virtual(window);
}

void
ui_redraw(void)
{
    title_bar_resize();
    wins_resize_all();
    status_bar_resize();
    inp_win_resize();
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

void
ui_contact_online(char *barejid, Resource *resource, GDateTime *last_activity)
{
    char *show_console = prefs_get_string(PREF_STATUSES_CONSOLE);
    char *show_chat_win = prefs_get_string(PREF_STATUSES_CHAT);
    PContact contact = roster_get_contact(barejid);

    // show nothing
    if (g_strcmp0(p_contact_subscription(contact), "none") == 0) {
        free(show_console);
        free(show_chat_win);
        return;
    }

    // show in console if "all"
    if (g_strcmp0(show_console, "all") == 0) {
        cons_show_contact_online(contact, resource, last_activity);

    // show in console of "online" and presence online
    } else if (g_strcmp0(show_console, "online") == 0 && resource->presence == RESOURCE_ONLINE) {
        cons_show_contact_online(contact, resource, last_activity);
    }

    // show in chat win if "all"
    if (g_strcmp0(show_chat_win, "all") == 0) {
        ProfChatWin *chatwin = wins_get_chat(barejid);
        if (chatwin) {
            chatwin_contact_online(chatwin, resource, last_activity);
        }

    // show in char win if "online" and presence online
    } else if (g_strcmp0(show_chat_win, "online") == 0 && resource->presence == RESOURCE_ONLINE) {
        ProfChatWin *chatwin = wins_get_chat(barejid);
        if (chatwin) {
            chatwin_contact_online(chatwin, resource, last_activity);
        }
    }

    free(show_console);
    free(show_chat_win);
}

void
ui_contact_typing(const char *const barejid, const char *const resource)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    ProfWin *window = (ProfWin*) chatwin;
    ChatSession *session = chat_session_get(barejid);

    if (prefs_get_boolean(PREF_INTYPE)) {
        // no chat window for user
        if (chatwin == NULL) {
            cons_show_typing(barejid);

        // have chat window but not currently in it
        } else if (!wins_is_current(window)) {
            cons_show_typing(barejid);

        // in chat window with user, no session or session with resource
        } else if (!session || (session && g_strcmp0(session->resource, resource) == 0)) {
            title_bar_set_typing(TRUE);

            int num = wins_get_num(window);
            status_bar_active(num);
       }
    }

    if (prefs_get_boolean(PREF_NOTIFY_TYPING)) {
        gboolean is_current = FALSE;
        if (window) {
            is_current = wins_is_current(window);
        }
        if ( !is_current || (is_current && prefs_get_boolean(PREF_NOTIFY_TYPING_CURRENT)) ) {
            PContact contact = roster_get_contact(barejid);
            char const *display_usr = NULL;
            if (contact) {
                if (p_contact_name(contact)) {
                    display_usr = p_contact_name(contact);
                } else {
                    display_usr = barejid;
                }
            } else {
                display_usr = barejid;
            }
            notify_typing(display_usr);
        }
    }
}

void
ui_roster_add(const char *const barejid, const char *const name)
{
    if (name) {
        cons_show("Roster item added: %s (%s)", barejid, name);
    } else {
        cons_show("Roster item added: %s", barejid);
    }
    rosterwin_roster();
}

void
ui_roster_remove(const char *const barejid)
{
    cons_show("Roster item removed: %s", barejid);
    rosterwin_roster();
}

void
ui_contact_already_in_group(const char *const contact, const char *const group)
{
    cons_show("%s already in group %s", contact, group);
    rosterwin_roster();
}

void
ui_contact_not_in_group(const char *const contact, const char *const group)
{
    cons_show("%s is not currently in group %s", contact, group);
    rosterwin_roster();
}

void
ui_group_added(const char *const contact, const char *const group)
{
    cons_show("%s added to group %s", contact, group);
    rosterwin_roster();
}

void
ui_group_removed(const char *const contact, const char *const group)
{
    cons_show("%s removed from group %s", contact, group);
    rosterwin_roster();
}

void
ui_handle_login_account_success(ProfAccount *account, gboolean secured)
{
    if (account->theme) {
        if (theme_load(account->theme)) {
            ui_load_colours();
            if (prefs_get_boolean(PREF_ROSTER)) {
                ui_show_roster();
            } else {
                ui_hide_roster();
            }
            if (prefs_get_boolean(PREF_OCCUPANTS)) {
                ui_show_all_room_rosters();
            } else {
                ui_hide_all_room_rosters();
            }
            ui_resize();
        } else {
            cons_show("Couldn't find account theme: %s", account->theme);
        }
    }

    resource_presence_t resource_presence = accounts_get_login_presence(account->name);
    contact_presence_t contact_presence = contact_presence_from_resource_presence(resource_presence);
    cons_show_login_success(account, secured);
    title_bar_set_presence(contact_presence);
    title_bar_set_connected(TRUE);
    title_bar_set_tls(secured);

    status_bar_print_message(connection_get_fulljid());
    status_bar_update_virtual();
}

void
ui_update_presence(const resource_presence_t resource_presence,
    const char *const message, const char *const show)
{
    contact_presence_t contact_presence = contact_presence_from_resource_presence(resource_presence);
    title_bar_set_presence(contact_presence);
    gint priority = accounts_get_priority_for_presence_type(session_get_account_name(), resource_presence);
    if (message) {
        cons_show("Status set to %s (priority %d), \"%s\".", show, priority, message);
    } else {
        cons_show("Status set to %s (priority %d).", show, priority);
    }
}

void
ui_handle_recipient_error(const char *const recipient, const char *const err_msg)
{
    // always show in console
    cons_show_error("Error from %s: %s", recipient, err_msg);

    ProfChatWin *chatwin = wins_get_chat(recipient);
    if (chatwin) {
        win_println((ProfWin*)chatwin, THEME_ERROR, '!', "Error from %s: %s", recipient, err_msg);
        return;
    }

    ProfMucWin *mucwin = wins_get_muc(recipient);
    if (mucwin) {
        win_println((ProfWin*)mucwin, THEME_ERROR, '!', "Error from %s: %s", recipient, err_msg);
        return;
    }

    ProfPrivateWin *privatewin = wins_get_private(recipient);
    if (privatewin) {
        win_println((ProfWin*)privatewin, THEME_ERROR, '!', "Error from %s: %s", recipient, err_msg);
        return;
    }
}

void
ui_handle_otr_error(const char *const barejid, const char *const message)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin) {
        win_println((ProfWin*)chatwin, THEME_ERROR, '!', "%s", message);
    } else {
        cons_show_error("%s - %s", barejid, message);
    }
}

void
ui_handle_error(const char *const err_msg)
{
    GString *msg = g_string_new("");
    g_string_printf(msg, "Error %s", err_msg);

    cons_show_error(msg->str);

    g_string_free(msg, TRUE);
}

void
ui_invalid_command_usage(const char *const cmd, void (*setting_func)(void))
{
    GString *msg = g_string_new("");
    g_string_printf(msg, "Invalid usage, see '/help %s' for details.", &cmd[1]);

    if (setting_func) {
        cons_show("");
        (*setting_func)();
        cons_show(msg->str);
    } else {
        cons_show("");
        cons_show(msg->str);
        ProfWin *current = wins_get_current();
        if (current->type == WIN_CHAT) {
            win_println(current, THEME_DEFAULT, '-', "%s", msg->str);
        }
    }

    g_string_free(msg, TRUE);
}

void
ui_disconnected(void)
{
    wins_lost_connection();
    title_bar_set_connected(FALSE);
    title_bar_set_tls(FALSE);
    title_bar_set_presence(CONTACT_OFFLINE);
    status_bar_clear_message();
    status_bar_update_virtual();
    ui_hide_roster();
}

void
ui_close_connected_win(int index)
{
    ProfWin *window = wins_get_by_num(index);
    if (window) {
        if (window->type == WIN_MUC) {
            ProfMucWin *mucwin = (ProfMucWin*) window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            presence_leave_chat_room(mucwin->roomjid);
            muc_leave(mucwin->roomjid);
            ui_leave_room(mucwin->roomjid);
        } else if (window->type == WIN_CHAT) {
            ProfChatWin *chatwin = (ProfChatWin*) window;
            assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
#ifdef HAVE_LIBOTR
            if (chatwin->is_otr) {
                otr_end_session(chatwin->barejid);
            }
#endif
            chat_state_gone(chatwin->barejid, chatwin->state);
            chat_session_remove(chatwin->barejid);
        }
    }
}

int
ui_close_all_wins(void)
{
    int count = 0;
    jabber_conn_status_t conn_status = connection_get_status();

    GList *win_nums = wins_get_nums();
    GList *curr = win_nums;

    while (curr) {
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

int
ui_close_read_wins(void)
{
    int count = 0;
    jabber_conn_status_t conn_status = connection_get_status();

    GList *win_nums = wins_get_nums();
    GList *curr = win_nums;

    while (curr) {
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

void
ui_redraw_all_room_rosters(void)
{
    GList *win_nums = wins_get_nums();
    GList *curr = win_nums;

    while (curr) {
        int num = GPOINTER_TO_INT(curr->data);
        ProfWin *window = wins_get_by_num(num);
        if (window->type == WIN_MUC && win_has_active_subwin(window)) {
            ProfMucWin *mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            occupantswin_occupants(mucwin->roomjid);
        }
        curr = g_list_next(curr);
    }

    g_list_free(curr);
    g_list_free(win_nums);
}

void
ui_hide_all_room_rosters(void)
{
    GList *win_nums = wins_get_nums();
    GList *curr = win_nums;

    while (curr) {
        int num = GPOINTER_TO_INT(curr->data);
        ProfWin *window = wins_get_by_num(num);
        if (window->type == WIN_MUC && win_has_active_subwin(window)) {
            ProfMucWin *mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            mucwin_hide_occupants(mucwin);
        }
        curr = g_list_next(curr);
    }

    g_list_free(curr);
    g_list_free(win_nums);
}

void
ui_show_all_room_rosters(void)
{
    GList *win_nums = wins_get_nums();
    GList *curr = win_nums;

    while (curr) {
        int num = GPOINTER_TO_INT(curr->data);
        ProfWin *window = wins_get_by_num(num);
        if (window->type == WIN_MUC && !win_has_active_subwin(window)) {
            ProfMucWin *mucwin = (ProfMucWin*)window;
            assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
            mucwin_show_occupants(mucwin);
        }
        curr = g_list_next(curr);
    }

    g_list_free(curr);
    g_list_free(win_nums);
}

gboolean
ui_win_has_unsaved_form(int num)
{
    ProfWin *window = wins_get_by_num(num);

    if (window->type == WIN_MUC_CONFIG) {
        ProfMucConfWin *confwin = (ProfMucConfWin*)window;
        assert(confwin->memcheck == PROFCONFWIN_MEMCHECK);
        return confwin->form->modified;
    } else {
        return FALSE;
    }
}

void
ui_focus_win(ProfWin *window)
{
    assert(window != NULL);

    if (wins_is_current(window)) {
        return;
    }

    ProfWin *old_current = wins_get_current();
    if (old_current->type == WIN_MUC_CONFIG) {
        ProfMucConfWin *confwin = (ProfMucConfWin*)old_current;
        cmd_ac_remove_form_fields(confwin->form);
    }

    if (window->type == WIN_MUC_CONFIG) {
        ProfMucConfWin *confwin = (ProfMucConfWin*)window;
        cmd_ac_add_form_fields(confwin->form);
    }

    int i = wins_get_num(window);
    wins_set_current_by_num(i);

    if (i == 1) {
        title_bar_console();
        rosterwin_roster();
    } else {
        title_bar_switch();
    }
    status_bar_current(i);
    status_bar_active(i);
}

void
ui_close_win(int index)
{
    ProfWin *window = wins_get_by_num(index);
    if (window && window->type == WIN_MUC_CONFIG) {
        ProfMucConfWin *confwin = (ProfMucConfWin*)window;
        if (confwin->form) {
            cmd_ac_remove_form_fields(confwin->form);
        }
    }

    wins_close_by_num(index);
    title_bar_console();
    status_bar_current(1);
    status_bar_active(1);
}

void
ui_prune_wins(void)
{
    jabber_conn_status_t conn_status = connection_get_status();
    gboolean pruned = FALSE;

    GSList *wins = wins_get_prune_wins();
    if (wins) {
        pruned = TRUE;
    }

    GSList *curr = wins;
    while (curr) {
        ProfWin *window = curr->data;
        if (window->type == WIN_CHAT) {
            if (conn_status == JABBER_CONNECTED) {
                ProfChatWin *chatwin = (ProfChatWin*)window;
                chat_session_remove(chatwin->barejid);
            }
        }

        int num = wins_get_num(window);
        ui_close_win(num);

        curr = g_slist_next(curr);
    }

    if (wins) {
        g_slist_free(wins);
    }

    wins_tidy();
    if (pruned) {
        cons_show("Windows pruned.");
    } else {
        cons_show("No prune needed.");
    }
}

void
ui_print_system_msg_from_recipient(const char *const barejid, const char *message)
{
    if (barejid == NULL || message == NULL)
        return;

    ProfWin *window = (ProfWin*)wins_get_chat(barejid);
    if (window == NULL) {
        int num = 0;
        window = wins_new_chat(barejid);
        if (window) {
            num = wins_get_num(window);
            status_bar_active(num);
        } else {
            num = 0;
            window = wins_get_console();
            status_bar_active(1);
        }
    }

    win_println(window, THEME_DEFAULT, '-', "*%s %s", barejid, message);
}

void
ui_room_join(const char *const roomjid, gboolean focus)
{
    ProfWin *window = (ProfWin*)wins_get_muc(roomjid);
    if (!window) {
        window = wins_new_muc(roomjid);
    }

    char *nick = muc_nick(roomjid);
    win_print(window, THEME_ROOMINFO, '!', "-> You have joined the room as %s", nick);
    if (prefs_get_boolean(PREF_MUC_PRIVILEGES)) {
        char *role = muc_role_str(roomjid);
        char *affiliation = muc_affiliation_str(roomjid);
        if (role) {
            win_append(window, THEME_ROOMINFO, ", role: %s", role);
        }
        if (affiliation) {
            win_append(window, THEME_ROOMINFO, ", affiliation: %s", affiliation);
        }
    }
    win_appendln(window, THEME_ROOMINFO, "");

    if (focus) {
        ui_focus_win(window);
    } else {
        int num = wins_get_num(window);
        status_bar_active(num);
        ProfWin *console = wins_get_console();
        char *nick = muc_nick(roomjid);
        win_println(console, THEME_TYPING, '!', "-> Autojoined %s as %s (%d).", roomjid, nick, num);
    }

    GList *privwins = wins_get_private_chats(roomjid);
    GList *curr = privwins;
    while (curr) {
        ProfPrivateWin *privwin = curr->data;
        privwin_room_joined(privwin);
        curr = g_list_next(curr);
    }
    g_list_free(privwins);
}

void
ui_switch_to_room(const char *const roomjid)
{
    ProfWin *window = (ProfWin*)wins_get_muc(roomjid);
    ui_focus_win(window);
}

void
ui_room_destroy(const char *const roomjid)
{
    ProfWin *window = (ProfWin*)wins_get_muc(roomjid);
    if (window == NULL) {
        log_error("Received room destroy result, but no window open for %s.", roomjid);
    } else {
        int num = wins_get_num(window);
        ui_close_win(num);
        cons_show("Room destroyed: %s", roomjid);
    }

    GList *privwins = wins_get_private_chats(roomjid);
    GList *curr = privwins;
    while (curr) {
        ProfPrivateWin *privwin = curr->data;
        privwin_room_destroyed(privwin);
        curr = g_list_next(curr);
    }
    g_list_free(privwins);
}

void
ui_leave_room(const char *const roomjid)
{
    ProfWin *window = (ProfWin*)wins_get_muc(roomjid);
    if (window) {
        int num = wins_get_num(window);
        ui_close_win(num);
    }

    GList *privwins = wins_get_private_chats(roomjid);
    GList *curr = privwins;
    while (curr) {
        ProfPrivateWin *privwin = curr->data;
        privwin_room_left(privwin);
        curr = g_list_next(curr);
    }
    g_list_free(privwins);

}

void
ui_room_destroyed(const char *const roomjid, const char *const reason, const char *const new_jid,
    const char *const password)
{
    ProfWin *window = (ProfWin*)wins_get_muc(roomjid);
    if (window == NULL) {
        log_error("Received room destroy, but no window open for %s.", roomjid);
    } else {
        int num = wins_get_num(window);
        ui_close_win(num);
        ProfWin *console = wins_get_console();

        if (reason) {
            win_println(console, THEME_TYPING, '!', "<- Room destroyed: %s, reason: %s", roomjid, reason);
        } else {
            win_println(console, THEME_TYPING, '!', "<- Room destroyed: %s", roomjid);
        }

        if (new_jid) {
            if (password) {
                win_println(console, THEME_TYPING, '!', "Replacement room: %s, password: %s", new_jid, password);
            } else {
                win_println(console, THEME_TYPING, '!', "Replacement room: %s", new_jid);
            }
        }
    }

    GList *privwins = wins_get_private_chats(roomjid);
    GList *curr = privwins;
    while (curr) {
        ProfPrivateWin *privwin = curr->data;
        privwin_room_destroyed(privwin);
        curr = g_list_next(curr);
    }
    g_list_free(privwins);
}

void
ui_room_kicked(const char *const roomjid, const char *const actor, const char *const reason)
{
    ProfWin *window = (ProfWin*)wins_get_muc(roomjid);
    if (window == NULL) {
        log_error("Received kick, but no window open for %s.", roomjid);
    } else {
        int num = wins_get_num(window);
        ui_close_win(num);

        GString *message = g_string_new("Kicked from ");
        g_string_append(message, roomjid);
        if (actor) {
            g_string_append(message, " by ");
            g_string_append(message, actor);
        }
        if (reason) {
            g_string_append(message, ", reason: ");
            g_string_append(message, reason);
        }

        ProfWin *console = wins_get_console();
        win_println(console, THEME_TYPING, '!', "<- %s", message->str);
        g_string_free(message, TRUE);
    }

    GList *privwins = wins_get_private_chats(roomjid);
    GList *curr = privwins;
    while (curr) {
        ProfPrivateWin *privwin = curr->data;
        privwin_room_kicked(privwin, actor, reason);
        curr = g_list_next(curr);
    }
    g_list_free(privwins);
}

void
ui_room_banned(const char *const roomjid, const char *const actor, const char *const reason)
{
    ProfWin *window = (ProfWin*)wins_get_muc(roomjid);
    if (window == NULL) {
        log_error("Received ban, but no window open for %s.", roomjid);
    } else {
        int num = wins_get_num(window);
        ui_close_win(num);

        GString *message = g_string_new("Banned from ");
        g_string_append(message, roomjid);
        if (actor) {
            g_string_append(message, " by ");
            g_string_append(message, actor);
        }
        if (reason) {
            g_string_append(message, ", reason: ");
            g_string_append(message, reason);
        }

        ProfWin *console = wins_get_console();
        win_println(console, THEME_TYPING, '!', "<- %s", message->str);
        g_string_free(message, TRUE);
    }

    GList *privwins = wins_get_private_chats(roomjid);
    GList *curr = privwins;
    while (curr) {
        ProfPrivateWin *privwin = curr->data;
        privwin_room_banned(privwin, actor, reason);
        curr = g_list_next(curr);
    }
    g_list_free(privwins);
}

int
ui_win_unread(int index)
{
    ProfWin *window = wins_get_by_num(index);
    if (window) {
        return win_unread(window);
    } else {
        return 0;
    }
}

char*
ui_ask_password(void)
{
    status_bar_get_password();
    status_bar_update_virtual();
    return inp_get_password();
}

char*
ui_get_line(void)
{
    status_bar_update_virtual();
    return inp_get_line();
}

char*
ui_ask_pgp_passphrase(const char *hint, int prev_fail)
{
    ProfWin *current = wins_get_current();

    win_println(current, THEME_DEFAULT, '-', "");

    if (prev_fail) {
        win_println(current, THEME_DEFAULT, '!', "Incorrect passphrase");
    }

    if (hint) {
        win_println(current, THEME_DEFAULT, '!', "Enter PGP key passphrase for %s", hint);
    } else {
        win_println(current, THEME_DEFAULT, '!', "Enter PGP key passphrase");
    }

    ui_update();

    status_bar_get_password();
    status_bar_update_virtual();
    return inp_get_password();
}

void
ui_contact_offline(char *barejid, char *resource, char *status)
{
    char *show_console = prefs_get_string(PREF_STATUSES_CONSOLE);
    char *show_chat_win = prefs_get_string(PREF_STATUSES_CHAT);
    Jid *jid = jid_create_from_bare_and_resource(barejid, resource);
    PContact contact = roster_get_contact(barejid);
    if (p_contact_subscription(contact)) {
        if (strcmp(p_contact_subscription(contact), "none") != 0) {

            // show in console if "all"
            if (g_strcmp0(show_console, "all") == 0) {
                cons_show_contact_offline(contact, resource, status);

            // show in console of "online"
            } else if (g_strcmp0(show_console, "online") == 0) {
                cons_show_contact_offline(contact, resource, status);
            }

            // show in chat win if "all"
            if (g_strcmp0(show_chat_win, "all") == 0) {
                ProfChatWin *chatwin = wins_get_chat(barejid);
                if (chatwin) {
                    chatwin_contact_offline(chatwin, resource, status);
                }

            // show in chat win if "online" and presence online
            } else if (g_strcmp0(show_chat_win, "online") == 0) {
                ProfChatWin *chatwin = wins_get_chat(barejid);
                if (chatwin) {
                    chatwin_contact_offline(chatwin, resource, status);
                }
            }
        }
    }

    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin && chatwin->resource_override && (g_strcmp0(resource, chatwin->resource_override) == 0)) {
        FREE_SET_NULL(chatwin->resource_override);
    }

    prefs_free_string(show_console);
    prefs_free_string(show_chat_win);
    jid_destroy(jid);
}

void
ui_clear_win_title(void)
{
    printf("%c]0;%c", '\033', '\007');
}

void
ui_goodbye_title(void)
{
    int result = system("/bin/echo -ne \"\033]0;Thanks for using Profanity\007\"");
    if(result == -1) log_error("Error printing title on shutdown");
}

static void
_ui_draw_term_title(void)
{
    char new_win_title[100];
    jabber_conn_status_t status = connection_get_status();

    if (status == JABBER_CONNECTED) {
        const char * const jid = connection_get_fulljid();
        gint unread = wins_get_total_unread();

        if (unread != 0) {
            snprintf(new_win_title, sizeof(new_win_title),
                "/bin/echo -n \"%c]0;%s (%d) - %s%c\"", '\033', "Profanity",
                unread, jid, '\007');
        } else {
            snprintf(new_win_title, sizeof(new_win_title),
                "/bin/echo -n \"%c]0;%s - %s%c\"", '\033', "Profanity", jid,
                '\007');
        }
    } else {
        snprintf(new_win_title, sizeof(new_win_title), "/bin/echo -n \"%c]0;%s%c\"", '\033',
            "Profanity", '\007');
    }
    if (g_strcmp0(win_title, new_win_title) != 0) {
        // print to x-window title bar
        int res = system(new_win_title);
        if (res == -1) {
            log_error("Error writing terminal window title.");
        }
        if (win_title) {
            free(win_title);
        }
        win_title = strdup(new_win_title);
    }
}

void
ui_handle_room_configuration_form_error(const char *const roomjid, const char *const message)
{
    ProfWin *window = NULL;
    GString *message_str = g_string_new("");

    if (roomjid) {
        window = (ProfWin*)wins_get_muc(roomjid);
        g_string_printf(message_str, "Could not get room configuration for %s", roomjid);
    } else {
        window = wins_get_console();
        g_string_printf(message_str, "Could not get room configuration");
    }

    if (message) {
        g_string_append(message_str, ": ");
        g_string_append(message_str, message);
    }

    win_println(window, THEME_ERROR, '-', "%s", message_str->str);

    g_string_free(message_str, TRUE);
}

void
ui_handle_room_config_submit_result(const char *const roomjid)
{
    if (roomjid) {
        ProfWin *form_window = NULL;
        ProfWin *muc_window = (ProfWin*)wins_get_muc(roomjid);

        GString *form_recipient = g_string_new(roomjid);
        g_string_append(form_recipient, " config");
        form_window = (ProfWin*) wins_get_muc_conf(form_recipient->str);
        g_string_free(form_recipient, TRUE);

        if (form_window) {
            int num = wins_get_num(form_window);
            wins_close_by_num(num);
        }

        if (muc_window) {
            ui_focus_win((ProfWin*)muc_window);
            win_println(muc_window, THEME_ROOMINFO, '!', "Room configuration successful");
        } else {
            ProfWin *console = wins_get_console();
            ui_focus_win(console);
            cons_show("Room configuration successful: %s", roomjid);
        }
    } else {
        cons_show("Room configuration successful");
    }
}

void
ui_handle_room_config_submit_result_error(const char *const roomjid, const char *const message)
{
    ProfWin *console = wins_get_console();
    if (roomjid) {
        ProfWin *muc_window = NULL;
        ProfWin *form_window = NULL;
        muc_window = (ProfWin*)wins_get_muc(roomjid);

        GString *form_recipient = g_string_new(roomjid);
        g_string_append(form_recipient, " config");
        form_window = (ProfWin*) wins_get_muc_conf(form_recipient->str);
        g_string_free(form_recipient, TRUE);

        if (form_window) {
            if (message) {
                win_println(form_window, THEME_ERROR, '!', "Configuration error: %s", message);
            } else {
                win_println(form_window, THEME_ERROR, '!', "Configuration error");
            }
        } else if (muc_window) {
            if (message) {
                win_println(muc_window, THEME_ERROR, '!', "Configuration error: %s", message);
            } else {
                win_println(muc_window, THEME_ERROR, '!', "Configuration error");
            }
        } else {
            if (message) {
                win_println(console, THEME_ERROR, '!', "Configuration error for %s: %s", roomjid, message);
            } else {
                win_println(console, THEME_ERROR, '!', "Configuration error for %s", roomjid);
            }
        }
    } else {
        win_println(console, THEME_ERROR, '!', "Configuration error");
    }
}

void
ui_show_lines(ProfWin *window, gchar** lines)
{
    if (lines) {
        int i;
        for (i = 0; lines[i] != NULL; i++) {
            win_println(window, THEME_DEFAULT, '-', "%s", lines[i]);
        }
    }
}

void
ui_show_roster(void)
{
    ProfWin *window = wins_get_console();
    if (window && !win_has_active_subwin(window)) {
        wins_show_subwin(window);
        rosterwin_roster();
    }
}

void
ui_hide_roster(void)
{
    ProfWin *window = wins_get_console();
    if (window && win_has_active_subwin(window)) {
        wins_hide_subwin(window);
    }
}

void
ui_handle_software_version_error(const char *const roomjid, const char *const message)
{
    GString *message_str = g_string_new("");

    ProfWin *window = wins_get_console();
    g_string_printf(message_str, "Could not get software version");

    if (message) {
        g_string_append(message_str, ": ");
        g_string_append(message_str, message);
    }

    win_println(window, THEME_ERROR, '-', "%s", message_str->str);

    g_string_free(message_str, TRUE);
}

void
ui_show_software_version(const char *const jid, const char *const  presence,
    const char *const name, const char *const version, const char *const os)
{
    Jid *jidp = jid_create(jid);
    ProfWin *window = NULL;
    ProfWin *chatwin = (ProfWin*)wins_get_chat(jidp->barejid);
    ProfWin *mucwin = (ProfWin*)wins_get_muc(jidp->barejid);
    ProfWin *privwin = (ProfWin*)wins_get_private(jidp->fulljid);
    ProfWin *console = wins_get_console();
    jid_destroy(jidp);

    if (chatwin) {
        if (wins_is_current(chatwin)) {
            window = chatwin;
        } else {
            window = console;
        }
    } else if (privwin) {
        if (wins_is_current(privwin)) {
            window = privwin;
        } else {
            window = console;
        }
    } else if (mucwin) {
        if (wins_is_current(mucwin)) {
            window = mucwin;
        } else {
            window = console;
        }
    } else {
        window = console;
    }

    if (name || version || os) {
        win_println(window, THEME_DEFAULT, '-', "");
        theme_item_t presence_colour = theme_main_presence_attrs(presence);
        win_print(window, presence_colour, '-', "%s", jid);
        win_appendln(window, THEME_DEFAULT, ":");
    }
    if (name) {
        win_println(window, THEME_DEFAULT, '-', "Name    : %s", name);
    }
    if (version) {
        win_println(window, THEME_DEFAULT, '-', "Version : %s", version);
    }
    if (os) {
        win_println(window, THEME_DEFAULT, '-', "OS      : %s", os);
    }
}
