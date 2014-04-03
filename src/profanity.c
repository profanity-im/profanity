/*
 * profanity.c
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
#include "roster_list.h"
#include "log.h"
#include "muc.h"
#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif
#include "resource.h"
#include "ui/ui.h"
#include "xmpp/xmpp.h"

static void _handle_idle_time(void);
static void _init(const int disable_tls, char *log_level);
static void _shutdown(void);
static void _create_directories(void);

static gboolean idle = FALSE;

void
prof_run(const int disable_tls, char *log_level, char *account_name)
{
    _init(disable_tls, log_level);
    log_info("Starting main event loop");
    inp_non_block();
    GTimer *timer = g_timer_new();
    gboolean cmd_result = TRUE;
    jabber_conn_status_t conn_status = jabber_get_connection_status();

    char inp[INP_WIN_MAX];
    int size = 0;

    ui_update_screen();

    if (account_name != NULL) {
      char *cmd = "/connect";
      snprintf(inp, sizeof(inp), "%s %s", cmd, account_name);
      process_input(inp);
    } else if (prefs_get_string(PREF_CONNECT_ACCOUNT) != NULL) {
      char *cmd = "/connect";
      snprintf(inp, sizeof(inp), "%s %s", cmd, prefs_get_string(PREF_CONNECT_ACCOUNT));
      process_input(inp);
    }

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
            ui_update_screen();
            jabber_process_events();

            ch = inp_get_char(inp, &size);
            if (ch != ERR) {
                ui_reset_idle_time();
            }
        }

        inp[size++] = '\0';
        cmd_result = process_input(inp);
    }

    g_timer_destroy(timer);
}

void
prof_handle_idle(void)
{
    jabber_conn_status_t status = jabber_get_connection_status();
    if (status == JABBER_CONNECTED) {
        GSList *recipients = ui_get_recipients();
        GSList *curr = recipients;

        while (curr != NULL) {
            char *recipient = curr->data;
            if (chat_session_get_recipient_supports(recipient)) {
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
            }

            curr = g_slist_next(curr);
        }

        if (recipients != NULL) {
            g_slist_free(recipients);
        }
    }
}

void
prof_handle_activity(void)
{
    win_type_t win_type = ui_current_win_type();
    jabber_conn_status_t status = jabber_get_connection_status();

    if ((status == JABBER_CONNECTED) && (win_type == WIN_CHAT)) {
        char *recipient = ui_current_recipient();
        if (chat_session_get_recipient_supports(recipient)) {
            chat_session_set_composing(recipient);
            if (!chat_session_get_sent(recipient) ||
                    chat_session_is_paused(recipient)) {
                message_send_composing(recipient);
            }
        }
    }
}

/*
 * Take a line of input and process it, return TRUE if profanity is to
 * continue, FALSE otherwise
 */
gboolean
process_input(char *inp)
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
        char *inp_cpy = strdup(inp);
        char *command = strtok(inp_cpy, " ");
        result = cmd_execute(command, inp);
        free(inp_cpy);

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
                    title_bar_set_presence(CONTACT_ONLINE);
                    ui_current_page_off();
                } else if (strcmp(prefs_get_string(PREF_AUTOAWAY_MODE), "idle") == 0) {
                    presence_update(RESOURCE_ONLINE, NULL, 0);
                    title_bar_set_presence(CONTACT_ONLINE);
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
#ifdef HAVE_LIBOTR
    otr_init();
#endif
    atexit(_shutdown);
}

static void
_shutdown(void)
{
    ui_clear_win_title();
    ui_close_all_wins();
    jabber_disconnect();
    jabber_shutdown();
    roster_free();
    muc_close();
    caps_close();
    ui_close();
    chat_log_close();
    prefs_close();
    theme_close();
    accounts_close();
    cmd_uninit();
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
