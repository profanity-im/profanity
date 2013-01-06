/*
 * profanity.c
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
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

#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <glib.h>

#include "accounts.h"
#include "chat_log.h"
#include "chat_session.h"
#include "command.h"
#include "common.h"
#include "contact.h"
#include "contact_list.h"
#include "files.h"
#include "history.h"
#include "log.h"
#include "preferences.h"
#include "profanity.h"
#include "room_chat.h"
#include "theme.h"
#include "jabber.h"
#include "ui.h"

static log_level_t _get_log_level(char *log_level);
static gboolean _process_input(char *inp);
static void _handle_idle_time(void);
static void _init(const int disable_tls, char *log_level);
static void _shutdown(void);

static gboolean idle = FALSE;

void
prof_run(const int disable_tls, char *log_level)
{
    _init(disable_tls, log_level);
    log_info("Starting main event loop");
    inp_non_block();
    GTimer *timer = g_timer_new();
    gboolean cmd_result = TRUE;

    char inp[INP_WIN_MAX];
    int size = 0;

    while(cmd_result == TRUE) {
        wint_t ch = ERR;
        size = 0;

        while(ch != '\n') {

            if (jabber_get_connection_status() == JABBER_CONNECTED) {
                _handle_idle_time();
            }

            gdouble elapsed = g_timer_elapsed(timer, NULL);

            gint remind_period = prefs_get_notify_remind();

            // 0 means to not remind
            if (remind_period > 0 && elapsed >= remind_period) {
                notify_remind();
                g_timer_start(timer);
            }

            ui_handle_special_keys(&ch);

            if (ch == KEY_RESIZE) {
                ui_resize(ch, inp, size);
            }

            ui_refresh();
            jabber_process_events();

            ch = inp_get_char(inp, &size);

            if (ch != ERR) {
                ui_reset_idle_time();
            }
        }

        inp[size++] = '\0';
        cmd_result = _process_input(inp);
    }

    g_timer_destroy(timer);
}

void
prof_handle_typing(char *from)
{
    ui_show_typing(from);
    win_current_page_off();
}

void
prof_handle_incoming_message(char *from, char *message, gboolean priv)
{
    ui_show_incoming_msg(from, message, NULL, priv);
    win_current_page_off();

    if (prefs_get_chlog()) {
        char from_cpy[strlen(from) + 1];
        strcpy(from_cpy, from);
        char *short_from = strtok(from_cpy, "/");
        const char *jid = jabber_get_jid();

        chat_log_chat(jid, short_from, message, PROF_IN_LOG, NULL);
    }
}

void
prof_handle_delayed_message(char *from, char *message, GTimeVal tv_stamp,
    gboolean priv)
{
    ui_show_incoming_msg(from, message, &tv_stamp, priv);
    win_current_page_off();

    if (prefs_get_chlog()) {
        char from_cpy[strlen(from) + 1];
        strcpy(from_cpy, from);
        char *short_from = strtok(from_cpy, "/");
        const char *jid = jabber_get_jid();

        chat_log_chat(jid, short_from, message, PROF_IN_LOG, &tv_stamp);
    }
}

void
prof_handle_error_message(const char *from, const char *err_msg)
{
    if (err_msg != NULL) {
        cons_bad_show("Error received from server: %s", err_msg);
    } else {
        cons_bad_show("Unknown error received from server.");
    }

    win_show_error_msg(from, err_msg);
}

void
prof_handle_subscription(const char *from, jabber_subscr_t type)
{
    switch (type) {
    case PRESENCE_SUBSCRIBE:
        /* TODO: auto-subscribe if needed */
        cons_show("Received authorization request from %s", from);
        log_info("Received authorization request from %s", from);
        win_show_system_msg(from, "Authorization request, type '/sub allow' to accept or '/sub deny' to reject");
        win_current_page_off();
        break;
    case PRESENCE_SUBSCRIBED:
        cons_show("Subscription received from %s", from);
        log_info("Subscription received from %s", from);
        win_show_system_msg(from, "Subscribed");
        win_current_page_off();
        break;
    case PRESENCE_UNSUBSCRIBED:
        cons_show("%s deleted subscription", from);
        log_info("%s deleted subscription", from);
        win_show_system_msg(from, "Unsubscribed");
        win_current_page_off();
        break;
    default:
        /* unknown type */
        break;
    }
}

void
prof_handle_login_account_success(char *account_name)
{
    ProfAccount *account = accounts_get_account(account_name);

    const char *msg = "logged in successfully.";
    cons_show("%s %s", account->jid, msg);
    title_bar_set_status(PRESENCE_ONLINE);
    log_info("%s %s", account->jid, msg);
    win_current_page_off();
    status_bar_print_message(account->jid);
    status_bar_refresh();

    accounts_free_account(account);
}

void
prof_handle_login_success(const char *jid, const char *altdomain)
{
    const char *msg = "logged in successfully.";
    cons_show("%s %s", jid, msg);
    title_bar_set_status(PRESENCE_ONLINE);
    log_info("%s %s", jid, msg);
    win_current_page_off();
    status_bar_print_message(jid);
    status_bar_refresh();

    accounts_add_login(jid, altdomain);
}

void
prof_handle_gone(const char * const from)
{
    win_show_gone(from);
    win_current_page_off();
}

void
prof_handle_lost_connection(void)
{
    cons_bad_show("Lost connection.");
    log_info("Lost connection");
    contact_list_clear();
    ui_disconnected();
    win_current_page_off();
    log_info("disconnected");
}

void
prof_handle_failed_login(void)
{
    cons_bad_show("Login failed.");
    log_info("Login failed");
    win_current_page_off();
    log_info("disconnected");
}

void
prof_handle_disconnect(const char * const jid)
{
    jabber_disconnect();
    contact_list_clear();
    chat_sessions_clear();
    jabber_restart();
    ui_disconnected();
    title_bar_set_status(PRESENCE_OFFLINE);
    status_bar_clear_message();
    status_bar_refresh();
    cons_show("%s logged out successfully.", jid);
    win_current_page_off();
}

void
prof_handle_room_history(const char * const room_jid, const char * const nick,
    GTimeVal tv_stamp, const char * const message)
{
    win_show_room_history(room_jid, nick, tv_stamp, message);
    win_current_page_off();
}

void
prof_handle_room_message(const char * const room_jid, const char * const nick,
    const char * const message)
{
    win_show_room_message(room_jid, nick, message);
    win_current_page_off();
}

void
prof_handle_room_subject(const char * const room_jid, const char * const subject)
{
    win_show_room_subject(room_jid, subject);
    win_current_page_off();
}

void
prof_handle_room_broadcast(const char *const room_jid,
    const char * const message)
{
    win_show_room_broadcast(room_jid, message);
    win_current_page_off();
}

void
prof_handle_room_roster_complete(const char * const room)
{
    room_set_roster_received(room);
    win_show_room_roster(room);
    win_current_page_off();
}

void
prof_handle_room_member_presence(const char * const room,
    const char * const nick, const char * const show,
    const char * const status)
{
    gboolean updated = room_add_to_roster(room, nick, show, status);

    if (updated) {
        win_show_room_member_presence(room, nick, show, status);
        win_current_page_off();
    }
}

void
prof_handle_room_member_online(const char * const room, const char * const nick,
    const char * const show, const char * const status)
{
    room_add_to_roster(room, nick, show, status);
    win_show_room_member_online(room, nick, show, status);
    win_current_page_off();
}

void
prof_handle_room_member_offline(const char * const room, const char * const nick,
    const char * const show, const char * const status)
{
    room_remove_from_roster(room, nick);
    win_show_room_member_offline(room, nick);
    win_current_page_off();
}

void
prof_handle_leave_room(const char * const room)
{
    room_leave(room);
}

void
prof_handle_contact_online(char *contact, char *show, char *status,
    GDateTime *last_activity)
{
    gboolean updated = contact_list_update_contact(contact, show, status, last_activity);

    if (updated) {
        PContact result = contact_list_get_contact(contact);
        if (p_contact_subscription(result) != NULL) {
            if (strcmp(p_contact_subscription(result), "none") != 0) {
                ui_contact_online(contact, show, status, last_activity);
                win_current_page_off();
            }
        }
    }
}

void
prof_handle_contact_offline(char *contact, char *show, char *status)
{
    gboolean updated = contact_list_update_contact(contact, "offline", status, NULL);

    if (updated) {
        PContact result = contact_list_get_contact(contact);
        if (p_contact_subscription(result) != NULL) {
            if (strcmp(p_contact_subscription(result), "none") != 0) {
                ui_contact_offline(contact, show, status);
                win_current_page_off();
            }
        }
    }
}

void
prof_handle_room_member_nick_change(const char * const room,
    const char * const old_nick, const char * const nick)
{
    win_show_room_member_nick_change(room, old_nick, nick);
    win_current_page_off();
}

void
prof_handle_room_nick_change(const char * const room,
    const char * const nick)
{
    win_show_room_nick_change(room, nick);
    win_current_page_off();
}

void
prof_handle_idle(void)
{
    jabber_conn_status_t status = jabber_get_connection_status();
    if (status == JABBER_CONNECTED) {
        ui_idle();
    }
}

void
prof_handle_activity(void)
{
    jabber_conn_status_t status = jabber_get_connection_status();
    if (status == JABBER_CONNECTED) {
        if (win_current_is_chat()) {
            char *recipient = win_current_get_recipient();
            chat_session_set_composing(recipient);
            if (!chat_session_get_sent(recipient) ||
                    chat_session_is_paused(recipient)) {
                jabber_send_composing(recipient);
            }
        }
    }
}

static log_level_t
_get_log_level(char *log_level)
{
    if (strcmp(log_level, "DEBUG") == 0) {
        return PROF_LEVEL_DEBUG;
    } else if (strcmp(log_level, "INFO") == 0) {
        return PROF_LEVEL_INFO;
    } else if (strcmp(log_level, "WARN") == 0) {
        return PROF_LEVEL_WARN;
    } else {
        return PROF_LEVEL_ERROR;
    }
}

/*
 * Take a line of input and process it, return TRUE if profanity is to
 * continue, FALSE otherwise
 */
static gboolean
_process_input(char *inp)
{
    log_debug("Input recieved: %s", inp);
    gboolean result = FALSE;
    g_strstrip(inp);

    // add line to history if something typed
    if (strlen(inp) > 0) {
        history_append(inp);
    }

    // just carry on if no input
    if (strlen(inp) == 0) {
        result = TRUE;

    // habdle command if input starts with a '/'
    } else if (inp[0] == '/') {
        char inp_cpy[strlen(inp) + 1];
        strcpy(inp_cpy, inp);
        char *command = strtok(inp_cpy, " ");
        result = cmd_execute(command, inp);

    // call a default handler if input didn't start with '/'
    } else {
        result = cmd_execute_default(inp);
    }

    inp_win_reset();
    contact_list_reset_search_attempts();
    win_current_page_off();

    return result;
}

static void
_handle_idle_time()
{
    gint prefs_time = prefs_get_autoaway_time() * 60000;

    unsigned long idle_ms = ui_get_idle_time();
    if (!idle) {
        if (idle_ms >= prefs_time) {
            idle = TRUE;

            // handle away mode
            if (strcmp(prefs_get_autoaway_mode(), "away") == 0) {
                jabber_update_presence(PRESENCE_AWAY, prefs_get_autoaway_message(), 0);
                if (prefs_get_autoaway_message() != NULL) {
                    cons_show("Idle for %d minutes, status set to away, \"%s\".",
                        prefs_get_autoaway_time(), prefs_get_autoaway_message());
                    title_bar_set_status(PRESENCE_AWAY);
                    win_current_page_off();
                } else {
                    cons_show("Idle for %d minutes, status set to away.",
                        prefs_get_autoaway_time());
                    title_bar_set_status(PRESENCE_AWAY);
                    win_current_page_off();
                }

            // handle idle mode
            } else if (strcmp(prefs_get_autoaway_mode(), "idle") == 0) {
                jabber_update_presence(PRESENCE_ONLINE,
                    prefs_get_autoaway_message(), idle_ms / 1000);
            }
        }

    } else {
        if (idle_ms < prefs_time) {
            idle = FALSE;

            // handle check
            if (prefs_get_autoaway_check()) {
                if (strcmp(prefs_get_autoaway_mode(), "away") == 0) {
                    jabber_update_presence(PRESENCE_ONLINE, NULL, 0);
                    cons_show("No longer idle, status set to online.");
                    title_bar_set_status(PRESENCE_ONLINE);
                    win_current_page_off();
                } else if (strcmp(prefs_get_autoaway_mode(), "idle") == 0) {
                    jabber_update_presence(PRESENCE_ONLINE, NULL, 0);
                    title_bar_set_status(PRESENCE_ONLINE);
                }
            }
        }
    }
}

static void
_init(const int disable_tls, char *log_level)
{
    setlocale(LC_ALL, "");
    // ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    files_create_directories();
    log_level_t prof_log_level = _get_log_level(log_level);
    log_init(prof_log_level);
    log_info("Starting Profanity (%s)...", PACKAGE_VERSION);
    chat_log_init();
    prefs_load();
    accounts_load();
    gchar *theme = prefs_get_theme();
    theme_init(theme);
    g_free(theme);
    ui_init();
    jabber_init(disable_tls);
    cmd_init();
    log_info("Initialising contact list");
    contact_list_init();
    atexit(_shutdown);
}

static void
_shutdown(void)
{
    jabber_disconnect();
    contact_list_clear();
    ui_close();
    chat_log_close();
    prefs_close();
    theme_close();
    cmd_close();
    log_close();
}
