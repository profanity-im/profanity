/*
 * profanity.c
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

#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "profanity.h"

#include "chat_session.h"
#include "config/accounts.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "command/command.h"
#include "common.h"
#include "contact.h"
#include "contact_list.h"
#include "log.h"
#include "muc.h"
#include "resource.h"
#include "ui/ui.h"
#include "xmpp/xmpp.h"

static gboolean _process_input(char *inp);
static void _handle_idle_time(void);
static void _init(const int disable_tls, char *log_level);
static void _shutdown(void);
static void _create_directories(void);

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
            if (remind_period > 0 && elapsed >= remind_period) {
                notify_remind();
                g_timer_start(timer);
            }

            ui_handle_special_keys(&ch, inp, size);
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

    if (prefs_get_boolean(PREF_CHLOG) && !priv) {
        Jid *from_jid = jid_create(from);
        const char *jid = jabber_get_jid();
        Jid *jidp = jid_create(jid);
        chat_log_chat(jidp->barejid, from_jid->barejid, message, PROF_IN_LOG, NULL);
        jid_destroy(jidp);
        jid_destroy(from_jid);
    }
}

void
prof_handle_delayed_message(char *from, char *message, GTimeVal tv_stamp,
    gboolean priv)
{
    ui_show_incoming_msg(from, message, &tv_stamp, priv);
    win_current_page_off();

    if (prefs_get_boolean(PREF_CHLOG) && !priv) {
        Jid *from_jid = jid_create(from);
        const char *jid = jabber_get_jid();
        Jid *jidp = jid_create(jid);
        chat_log_chat(jidp->barejid, from_jid->barejid, message, PROF_IN_LOG, &tv_stamp);
        jid_destroy(jidp);
        jid_destroy(from_jid);
    }
}

void
prof_handle_error_message(const char *from, const char *err_msg)
{
    win_type_t win_type = ui_current_win_type();
    if (err_msg == NULL) {
        cons_show_error("Unknown error received from service.");
    } else if (strcmp(err_msg, "conflict") == 0) {
        if (win_type == WIN_MUC) {
            win_current_show("Nickname already in use.");
        } else {
            cons_show_error("Error received from server: %s", err_msg);
        }
    } else {
        cons_show_error("Error received from server: %s", err_msg);
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
    resource_presence_t resource_presence = accounts_get_login_presence(account->name);
    contact_presence_t contact_presence = contact_presence_from_resource_presence(resource_presence);
    cons_show_login_success(account);
    title_bar_set_status(contact_presence);
    log_info("%s logged in successfully", account->jid);
    win_current_page_off();
    status_bar_print_message(account->jid);
    status_bar_refresh();

    accounts_free_account(account);
}

void
prof_handle_gone(const char * const from)
{
    win_show_gone(from);
    win_current_page_off();
}

void
prof_handle_failed_login(void)
{
    cons_show_error("Login failed.");
    log_info("Login failed");
    win_current_page_off();
}

void
prof_handle_lost_connection(void)
{
    cons_show_error("Lost connection.");
    contact_list_clear();
    chat_sessions_clear();
    ui_disconnected();
    win_current_page_off();
}

void
prof_handle_disconnect(const char * const jid)
{
    cons_show("%s logged out successfully.", jid);
    jabber_disconnect();
    contact_list_clear();
    chat_sessions_clear();
    ui_disconnected();
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
    muc_set_roster_received(room);
    GList *roster = muc_get_roster(room);
    win_show_room_roster(room, roster, NULL);
    win_current_page_off();
}

void
prof_handle_room_member_presence(const char * const room,
    const char * const nick, const char * const show,
    const char * const status, const char * const caps_str)
{
    gboolean updated = muc_add_to_roster(room, nick, show, status, caps_str);

    if (updated) {
        win_show_room_member_presence(room, nick, show, status);
        win_current_page_off();
    }
}

void
prof_handle_room_member_online(const char * const room, const char * const nick,
    const char * const show, const char * const status,
    const char * const caps_str)
{
    muc_add_to_roster(room, nick, show, status, caps_str);
    win_show_room_member_online(room, nick, show, status);
    win_current_page_off();
}

void
prof_handle_room_member_offline(const char * const room, const char * const nick,
    const char * const show, const char * const status)
{
    muc_remove_from_roster(room, nick);
    win_show_room_member_offline(room, nick);
    win_current_page_off();
}

void
prof_handle_leave_room(const char * const room)
{
    muc_leave_room(room);
}

void prof_handle_room_invite(jabber_invite_t invite_type,
    const char * const invitor, const char * const room,
    const char * const reason)
{
    cons_show_room_invite(invitor, room, reason);
    win_current_page_off();
}

void
prof_handle_contact_online(char *contact, Resource *resource,
    GDateTime *last_activity)
{
    gboolean updated = contact_list_update_presence(contact, resource, last_activity);

    if (updated) {
        PContact result = contact_list_get_contact(contact);
        if (p_contact_subscription(result) != NULL) {
            if (strcmp(p_contact_subscription(result), "none") != 0) {
                const char *show = string_from_resource_presence(resource->presence);
                ui_contact_online(contact, resource->name, show, resource->status, last_activity);
                win_current_page_off();
            }
        }
    }
}

void
prof_handle_contact_offline(char *contact, char *resource, char *status)
{
    gboolean updated = contact_list_contact_offline(contact, resource, status);

    if (resource != NULL && updated) {
        Jid *jid = jid_create_from_bare_and_resource(contact, resource);
        PContact result = contact_list_get_contact(contact);
        if (p_contact_subscription(result) != NULL) {
            if (strcmp(p_contact_subscription(result), "none") != 0) {
                ui_contact_offline(jid->fulljid, "offline", status);
                win_current_page_off();
            }
        }
        jid_destroy(jid);
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
    win_type_t win_type = ui_current_win_type();
    jabber_conn_status_t status = jabber_get_connection_status();

    if ((status == JABBER_CONNECTED) && (win_type == WIN_CHAT)) {
        char *recipient = win_current_get_recipient();
        chat_session_set_composing(recipient);
        if (!chat_session_get_sent(recipient) ||
                chat_session_is_paused(recipient)) {
            message_send_composing(recipient);
        }
    }
}

void
prof_handle_version_result(const char * const jid, const char * const  presence,
    const char * const name, const char * const version, const char * const os)
{
    cons_show_software_version(jid, presence, name, version, os);
    win_current_page_off();
}

void
prof_handle_room_list(GSList *rooms, const char *conference_node)
{
    cons_show_room_list(rooms, conference_node);
    win_current_page_off();
}

void
prof_handle_disco_items(GSList *items, const char *jid)
{
    cons_show_disco_items(items, jid);
    win_current_page_off();
}

void
prof_handle_disco_info(const char *from, GSList *identities, GSList *features)
{
    cons_show_disco_info(from, identities, features);
    win_current_page_off();
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
        cmd_history_append(inp);
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
            if (strcmp(prefs_get_string(PREF_AUTOAWAY_MODE), "away") == 0) {
                presence_update(RESOURCE_AWAY, prefs_get_string(PREF_AUTOAWAY_MESSAGE), 0);
                if (prefs_get_string(PREF_AUTOAWAY_MESSAGE) != NULL) {
                    int pri =
                        accounts_get_priority_for_presence_type(jabber_get_account_name(),
                            RESOURCE_AWAY);
                    cons_show("Idle for %d minutes, status set to away (priority %d), \"%s\".",
                        prefs_get_autoaway_time(), pri, prefs_get_string(PREF_AUTOAWAY_MESSAGE));
                    title_bar_set_status(CONTACT_AWAY);
                    win_current_page_off();
                } else {
                    int pri =
                        accounts_get_priority_for_presence_type(jabber_get_account_name(),
                            RESOURCE_AWAY);
                    cons_show("Idle for %d minutes, status set to away (priority %d).",
                        prefs_get_autoaway_time(), pri);
                    title_bar_set_status(CONTACT_AWAY);
                    win_current_page_off();
                }

            // handle idle mode
            } else if (strcmp(prefs_get_string(PREF_AUTOAWAY_MODE), "idle") == 0) {
                presence_update(RESOURCE_ONLINE,
                    prefs_get_string(PREF_AUTOAWAY_MESSAGE), idle_ms / 1000);
            }
        }

    } else {
        if (idle_ms < prefs_time) {
            idle = FALSE;

            // handle check
            if (prefs_get_boolean(PREF_AUTOAWAY_CHECK)) {
                if (strcmp(prefs_get_string(PREF_AUTOAWAY_MODE), "away") == 0) {
                    presence_update(RESOURCE_ONLINE, NULL, 0);
                    int pri =
                        accounts_get_priority_for_presence_type(jabber_get_account_name(),
                            RESOURCE_ONLINE);
                    cons_show("No longer idle, status set to online (priority %d).", pri);
                    title_bar_set_status(CONTACT_ONLINE);
                    win_current_page_off();
                } else if (strcmp(prefs_get_string(PREF_AUTOAWAY_MODE), "idle") == 0) {
                    presence_update(RESOURCE_ONLINE, NULL, 0);
                    title_bar_set_status(CONTACT_ONLINE);
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
    _create_directories();
    log_level_t prof_log_level = log_level_from_string(log_level);
    log_init(prof_log_level);
    if (strcmp(PACKAGE_STATUS, "development") == 0) {
        log_info("Starting Profanity (%sdev)...", PACKAGE_VERSION);
    } else {
        log_info("Starting Profanity (%s)...", PACKAGE_VERSION);
    }
    chat_log_init();
    prefs_load();
    accounts_load();
    gchar *theme = prefs_get_string(PREF_THEME);
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
    jabber_shutdown();
    contact_list_free();
    caps_close();
    ui_close();
    chat_log_close();
    prefs_close();
    theme_close();
    accounts_close();
    cmd_close();
    log_close();
}

static void
_create_directories(void)
{
    gchar *xdg_config = xdg_get_config_home();
    gchar *xdg_data = xdg_get_data_home();

    GString *themes_dir = g_string_new(xdg_config);
    g_string_append(themes_dir, "/profanity/themes");
    GString *chatlogs_dir = g_string_new(xdg_data);
    g_string_append(chatlogs_dir, "/profanity/chatlogs");
    GString *logs_dir = g_string_new(xdg_data);
    g_string_append(logs_dir, "/profanity/logs");

    mkdir_recursive(themes_dir->str);
    mkdir_recursive(chatlogs_dir->str);
    mkdir_recursive(logs_dir->str);

    g_string_free(themes_dir, TRUE);
    g_string_free(chatlogs_dir, TRUE);
    g_string_free(logs_dir, TRUE);

    g_free(xdg_config);
    g_free(xdg_data);
}
