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

#ifdef HAVE_GIT_VERSION
#include "gitversion.c"
#endif

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
#include "log.h"
#include "muc.h"
#include "resource.h"
#include "ui/notifier.h"
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
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    char inp[INP_WIN_MAX];
    int size = 0;

    while(cmd_result == TRUE) {
        wint_t ch = ERR;
        size = 0;

        while(ch != '\n') {
            conn_status = jabber_get_connection_status();
            if (conn_status == JABBER_CONNECTED) {
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
    ui_contact_typing(from);
    ui_current_page_off();
}

void
prof_handle_incoming_message(char *from, char *message, gboolean priv)
{
    ui_incoming_msg(from, message, NULL, priv);
    ui_current_page_off();

    if (prefs_get_boolean(PREF_CHLOG) && !priv) {
        Jid *from_jid = jid_create(from);
        const char *jid = jabber_get_fulljid();
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
    ui_incoming_msg(from, message, &tv_stamp, priv);
    ui_current_page_off();

    if (prefs_get_boolean(PREF_CHLOG) && !priv) {
        Jid *from_jid = jid_create(from);
        const char *jid = jabber_get_fulljid();
        Jid *jidp = jid_create(jid);
        chat_log_chat(jidp->barejid, from_jid->barejid, message, PROF_IN_LOG, &tv_stamp);
        jid_destroy(jidp);
        jid_destroy(from_jid);
    }
}

void
prof_handle_duck_result(const char * const result)
{
    ui_duck_result(result);
    ui_current_page_off();
}

void
prof_handle_already_in_group(const char * const contact,
    const char * const group)
{
    ui_contact_already_in_group(contact, group);
    ui_current_page_off();
}

void
prof_handle_not_in_group(const char * const contact,
    const char * const group)
{
    ui_contact_not_in_group(contact, group);
    ui_current_page_off();
}

void
prof_handle_group_add(const char * const contact,
    const char * const group)
{
    ui_group_added(contact, group);
    ui_current_page_off();
}

void
prof_handle_group_remove(const char * const contact,
    const char * const group)
{
    ui_group_removed(contact, group);
    ui_current_page_off();
}

void
prof_handle_error_message(const char *from, const char *err_msg)
{
    ui_handle_error_message(from, err_msg);

    if (g_strcmp0(err_msg, "conflict") == 0) {
        // remove the room from muc
        Jid *room_jid = jid_create(from);
        if (!muc_get_roster_received(room_jid->barejid)) {
            muc_leave_room(room_jid->barejid);
        }
        jid_destroy(room_jid);
    }
}

void
prof_handle_subscription(const char *from, jabber_subscr_t type)
{
    switch (type) {
    case PRESENCE_SUBSCRIBE:
        /* TODO: auto-subscribe if needed */
        cons_show("Received authorization request from %s", from);
        log_info("Received authorization request from %s", from);
        ui_print_system_msg_from_recipient(from, "Authorization request, type '/sub allow' to accept or '/sub deny' to reject");
        ui_current_page_off();
        if (prefs_get_boolean(PREF_NOTIFY_SUB)) {
            notify_subscription(from);
        }
        break;
    case PRESENCE_SUBSCRIBED:
        cons_show("Subscription received from %s", from);
        log_info("Subscription received from %s", from);
        ui_print_system_msg_from_recipient(from, "Subscribed");
        ui_current_page_off();
        break;
    case PRESENCE_UNSUBSCRIBED:
        cons_show("%s deleted subscription", from);
        log_info("%s deleted subscription", from);
        ui_print_system_msg_from_recipient(from, "Unsubscribed");
        ui_current_page_off();
        break;
    default:
        /* unknown type */
        break;
    }
}

void
prof_handle_roster_add(const char * const barejid, const char * const name)
{
    ui_roster_add(barejid, name);
    ui_current_page_off();
}

void
prof_handle_roster_remove(const char * const barejid)
{
    ui_roster_remove(barejid);
    ui_current_page_off();
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
    ui_current_page_off();
    status_bar_print_message(account->jid);
    status_bar_refresh();

    accounts_free_account(account);
}

void
prof_handle_gone(const char * const from)
{
    ui_recipient_gone(from);
    ui_current_page_off();
}

void
prof_handle_failed_login(void)
{
    cons_show_error("Login failed.");
    log_info("Login failed");
    ui_current_page_off();
}

void
prof_handle_lost_connection(void)
{
    cons_show_error("Lost connection.");
    roster_clear();
    muc_clear_invites();
    chat_sessions_clear();
    ui_disconnected();
    ui_current_page_off();
}

void
prof_handle_disconnect(const char * const jid)
{
    cons_show("%s logged out successfully.", jid);
    jabber_disconnect();
    roster_clear();
    muc_clear_invites();
    chat_sessions_clear();
    ui_disconnected();
    ui_current_page_off();
}

void
prof_handle_room_history(const char * const room_jid, const char * const nick,
    GTimeVal tv_stamp, const char * const message)
{
    ui_room_history(room_jid, nick, tv_stamp, message);
    ui_current_page_off();
}

void
prof_handle_room_message(const char * const room_jid, const char * const nick,
    const char * const message)
{
    ui_room_message(room_jid, nick, message);
    ui_current_page_off();

    if (prefs_get_boolean(PREF_GRLOG)) {
        Jid *jid = jid_create(jabber_get_fulljid());
        groupchat_log_chat(jid->barejid, room_jid, nick, message);
        jid_destroy(jid);
    }
}

void
prof_handle_room_subject(const char * const room_jid, const char * const subject)
{
    ui_room_subject(room_jid, subject);
    ui_current_page_off();
}

void
prof_handle_room_broadcast(const char *const room_jid,
    const char * const message)
{
    ui_room_broadcast(room_jid, message);
    ui_current_page_off();
}

void
prof_handle_room_roster_complete(const char * const room)
{
    muc_set_roster_received(room);
    GList *roster = muc_get_roster(room);
    ui_room_roster(room, roster, NULL);
    ui_current_page_off();
}

void
prof_handle_room_member_presence(const char * const room,
    const char * const nick, const char * const show,
    const char * const status, const char * const caps_str)
{
    gboolean updated = muc_add_to_roster(room, nick, show, status, caps_str);

    if (updated) {
        ui_room_member_presence(room, nick, show, status);
        ui_current_page_off();
    }
}

void
prof_handle_room_member_online(const char * const room, const char * const nick,
    const char * const show, const char * const status,
    const char * const caps_str)
{
    muc_add_to_roster(room, nick, show, status, caps_str);
    ui_room_member_online(room, nick, show, status);
    ui_current_page_off();
}

void
prof_handle_room_member_offline(const char * const room, const char * const nick,
    const char * const show, const char * const status)
{
    muc_remove_from_roster(room, nick);
    ui_room_member_offline(room, nick);
    ui_current_page_off();
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
    Jid *room_jid = jid_create(room);
    if (!muc_room_is_active(room_jid) && !muc_invites_include(room)) {
        cons_show_room_invite(invitor, room, reason);
        muc_add_invite(room);
        ui_current_page_off();
    }
    jid_destroy(room_jid);
}

void
prof_handle_contact_online(char *contact, Resource *resource,
    GDateTime *last_activity)
{
    gboolean updated = roster_update_presence(contact, resource, last_activity);

    if (updated) {
        PContact result = roster_get_contact(contact);
        if (p_contact_subscription(result) != NULL) {
            if (strcmp(p_contact_subscription(result), "none") != 0) {
                const char *show = string_from_resource_presence(resource->presence);
                ui_contact_online(contact, resource->name, show, resource->status, last_activity);
                ui_current_page_off();
            }
        }
    }
}

void
prof_handle_contact_offline(char *contact, char *resource, char *status)
{
    gboolean updated = roster_contact_offline(contact, resource, status);

    if (resource != NULL && updated) {
        Jid *jid = jid_create_from_bare_and_resource(contact, resource);
        PContact result = roster_get_contact(contact);
        if (p_contact_subscription(result) != NULL) {
            if (strcmp(p_contact_subscription(result), "none") != 0) {
                ui_contact_offline(jid->fulljid, "offline", status);
                ui_current_page_off();
            }
        }
        jid_destroy(jid);
    }
}

void
prof_handle_room_member_nick_change(const char * const room,
    const char * const old_nick, const char * const nick)
{
    ui_room_member_nick_change(room, old_nick, nick);
    ui_current_page_off();
}

void
prof_handle_room_nick_change(const char * const room,
    const char * const nick)
{
    ui_room_nick_change(room, nick);
    ui_current_page_off();
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
        char *recipient = ui_current_recipient();
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
    ui_current_page_off();
}

void
prof_handle_room_list(GSList *rooms, const char *conference_node)
{
    cons_show_room_list(rooms, conference_node);
    ui_current_page_off();
}

void
prof_handle_disco_items(GSList *items, const char *jid)
{
    cons_show_disco_items(items, jid);
    ui_current_page_off();
}

void
prof_handle_disco_info(const char *from, GSList *identities, GSList *features)
{
    cons_show_disco_info(from, identities, features);
    ui_current_page_off();
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
    roster_reset_search_attempts();
    ui_current_page_off();

    return result;
}

static void
_handle_idle_time()
{
    gint prefs_time = prefs_get_autoaway_time() * 60000;
    resource_presence_t current_presence = accounts_get_last_presence(jabber_get_account_name());
    unsigned long idle_ms = ui_get_idle_time();

    if (!idle) {
        if ((current_presence == RESOURCE_ONLINE) || (current_presence == RESOURCE_CHAT)) {
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
                        ui_current_page_off();
                    } else {
                        int pri =
                            accounts_get_priority_for_presence_type(jabber_get_account_name(),
                                RESOURCE_AWAY);
                        cons_show("Idle for %d minutes, status set to away (priority %d).",
                            prefs_get_autoaway_time(), pri);
                        title_bar_set_status(CONTACT_AWAY);
                        ui_current_page_off();
                    }

                // handle idle mode
                } else if (strcmp(prefs_get_string(PREF_AUTOAWAY_MODE), "idle") == 0) {
                    presence_update(RESOURCE_ONLINE,
                        prefs_get_string(PREF_AUTOAWAY_MESSAGE), idle_ms / 1000);
                }
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
                    ui_current_page_off();
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
#ifdef HAVE_GIT_VERSION
            log_info("Starting Profanity (%sdev.%s.%s)...", PACKAGE_VERSION, PROF_GIT_BRANCH, PROF_GIT_REVISION);
#else
            log_info("Starting Profanity (%sdev)...", PACKAGE_VERSION);
#endif
    } else {
        log_info("Starting Profanity (%s)...", PACKAGE_VERSION);
    }
    chat_log_init();
    groupchat_log_init();
    prefs_load();
    accounts_load();
    gchar *theme = prefs_get_string(PREF_THEME);
    theme_init(theme);
    g_free(theme);
    ui_init();
    jabber_init(disable_tls);
    cmd_init();
    log_info("Initialising contact list");
    roster_init();
    muc_init();
    atexit(_shutdown);
}

static void
_shutdown(void)
{
    jabber_disconnect();
    jabber_shutdown();
    roster_free();
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

    if (!mkdir_recursive(themes_dir->str)) {
        log_error("Error while creating directory %s", themes_dir->str);
    }
    if (!mkdir_recursive(chatlogs_dir->str)) {
        log_error("Error while creating directory %s", chatlogs_dir->str);
    }
    if (!mkdir_recursive(logs_dir->str)) {
        log_error("Error while creating directory %s", logs_dir->str);
    }

    g_string_free(themes_dir, TRUE);
    g_string_free(chatlogs_dir, TRUE);
    g_string_free(logs_dir, TRUE);

    g_free(xdg_config);
    g_free(xdg_data);
}
