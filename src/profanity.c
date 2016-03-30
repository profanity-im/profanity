/*
 * profanity.c
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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
#include "prof_config.h"

#ifdef PROF_HAVE_GIT_VERSION
#include "gitversion.h"
#endif

#ifdef PROF_HAVE_GTK
#include <gtk/gtk.h>
#endif
#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <glib.h>

#include "profanity.h"
#include "chat_session.h"
#include "chat_state.h"
#include "config/accounts.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "config/scripts.h"
#include "command/command.h"
#include "common.h"
#include "contact.h"
#include "roster_list.h"
#include "config/tlscerts.h"
#include "log.h"
#include "muc.h"
#include "plugins/plugins.h"
#ifdef PROF_HAVE_LIBOTR
#include "otr/otr.h"
#endif
#ifdef PROF_HAVE_LIBGPGME
#include "pgp/gpg.h"
#endif
#include "resource.h"
#include "xmpp/xmpp.h"
#include "ui/ui.h"
#include "window_list.h"
#include "event/client_events.h"
#include "config/tlscerts.h"
#ifdef PROF_HAVE_GTK
#include "tray.h"
#endif

static void _check_autoaway(void);
static void _init(char *log_level);
static void _shutdown(void);
static void _create_directories(void);
static void _connect_default(const char * const account);

typedef enum {
    ACTIVITY_ST_ACTIVE,
    ACTIVITY_ST_IDLE,
    ACTIVITY_ST_AWAY,
    ACTIVITY_ST_XA,
} activity_state_t;

activity_state_t activity_state;
resource_presence_t saved_presence;
char *saved_status;

static gboolean cont = TRUE;
static gboolean force_quit = FALSE;

void
prof_run(char *log_level, char *account_name)
{
    _init(log_level);
    plugins_on_start();
#ifdef PROF_HAVE_GTK
    gtk_main_iteration_do(false);
#endif
    _connect_default(account_name);

    ui_update();

    log_info("Starting main event loop");

    activity_state = ACTIVITY_ST_ACTIVE;
    saved_status = NULL;

    char *line = NULL;
    while(cont && !force_quit) {
        log_stderr_handler();
        _check_autoaway();

        line = inp_readline();
        if (line) {
            ProfWin *window = wins_get_current();
            cont = cmd_process_input(window, line);
            free(line);
            line = NULL;
        } else {
            cont = TRUE;
        }

#ifdef PROF_HAVE_LIBOTR
        otr_poll();
#endif
        plugins_run_timed();
        notify_remind();
        jabber_process_events(10);
        iq_autoping_check();
        ui_update();
#ifdef PROF_HAVE_GTK
        gtk_main_iteration_do(false);
#endif
    }
}

void
prof_set_quit(void)
{
    force_quit = TRUE;
}

void
prof_handle_idle(void)
{
    jabber_conn_status_t status = jabber_get_connection_status();
    if (status == JABBER_CONNECTED) {
        GSList *recipients = wins_get_chat_recipients();
        GSList *curr = recipients;

        while (curr) {
            char *barejid = curr->data;
            ProfChatWin *chatwin = wins_get_chat(barejid);
            chat_state_handle_idle(chatwin->barejid, chatwin->state);
            curr = g_slist_next(curr);
        }

        if (recipients) {
            g_slist_free(recipients);
        }
    }
}

void
prof_handle_activity(void)
{
    jabber_conn_status_t status = jabber_get_connection_status();
    ProfWin *current = wins_get_current();

    if ((status == JABBER_CONNECTED) && (current->type == WIN_CHAT)) {
        ProfChatWin *chatwin = (ProfChatWin*)current;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        chat_state_handle_typing(chatwin->barejid, chatwin->state);
    }
}

static void
_connect_default(const char *const account)
{
    ProfWin *window = wins_get_current();
    if (account) {
        cmd_execute_connect(window, account);
    } else {
        char *pref_connect_account = prefs_get_string(PREF_CONNECT_ACCOUNT);
        if (pref_connect_account) {
            cmd_execute_connect(window, pref_connect_account);
            prefs_free_string(pref_connect_account);
        }
    }
}

static void
_check_autoaway(void)
{
    jabber_conn_status_t conn_status = jabber_get_connection_status();
    if (conn_status != JABBER_CONNECTED) {
        return;
    }

    char *mode = prefs_get_string(PREF_AUTOAWAY_MODE);
    gboolean check = prefs_get_boolean(PREF_AUTOAWAY_CHECK);
    gint away_time = prefs_get_autoaway_time();
    gint xa_time = prefs_get_autoxa_time();
    int away_time_ms = away_time * 60000;
    int xa_time_ms = xa_time * 60000;

    char *account = jabber_get_account_name();
    resource_presence_t curr_presence = accounts_get_last_presence(account);
    char *curr_status = accounts_get_last_status(account);

    unsigned long idle_ms = ui_get_idle_time();

    switch (activity_state) {
    case ACTIVITY_ST_ACTIVE:
        if (idle_ms >= away_time_ms) {
            if (g_strcmp0(mode, "away") == 0) {
                if ((curr_presence == RESOURCE_ONLINE) || (curr_presence == RESOURCE_CHAT) || (curr_presence == RESOURCE_DND)) {
                    activity_state = ACTIVITY_ST_AWAY;

                    // save current presence
                    saved_presence = curr_presence;
                    if (saved_status) {
                        free(saved_status);
                    }
                    saved_status = curr_status;

                    // send away presence with last activity
                    char *message = prefs_get_string(PREF_AUTOAWAY_MESSAGE);
                    if (prefs_get_boolean(PREF_LASTACTIVITY)) {
                        cl_ev_presence_send(RESOURCE_AWAY, message, idle_ms / 1000);
                    } else {
                        cl_ev_presence_send(RESOURCE_AWAY, message, 0);
                    }

                    int pri = accounts_get_priority_for_presence_type(account, RESOURCE_AWAY);
                    if (message) {
                        cons_show("Idle for %d minutes, status set to away (priority %d), \"%s\".", away_time, pri, message);
                    } else {
                        cons_show("Idle for %d minutes, status set to away (priority %d).", away_time, pri);
                    }
                    prefs_free_string(message);

                    title_bar_set_presence(CONTACT_AWAY);
                }
            } else if (g_strcmp0(mode, "idle") == 0) {
                activity_state = ACTIVITY_ST_IDLE;

                // send current presence with last activity
                cl_ev_presence_send(curr_presence, curr_status, idle_ms / 1000);
            }
        }
        break;
    case ACTIVITY_ST_IDLE:
        if (check && (idle_ms < away_time_ms)) {
            activity_state = ACTIVITY_ST_ACTIVE;

            cons_show("No longer idle.");

            // send current presence without last activity
            cl_ev_presence_send(curr_presence, curr_status, 0);
        }
        break;
    case ACTIVITY_ST_AWAY:
        if (xa_time_ms > 0 && (idle_ms >= xa_time_ms)) {
            activity_state = ACTIVITY_ST_XA;

            // send extended away presence with last activity
            char *message = prefs_get_string(PREF_AUTOXA_MESSAGE);
            if (prefs_get_boolean(PREF_LASTACTIVITY)) {
                cl_ev_presence_send(RESOURCE_XA, message, idle_ms / 1000);
            } else {
                cl_ev_presence_send(RESOURCE_XA, message, 0);
            }

            int pri = accounts_get_priority_for_presence_type(account, RESOURCE_XA);
            if (message) {
                cons_show("Idle for %d minutes, status set to xa (priority %d), \"%s\".", xa_time, pri, message);
            } else {
                cons_show("Idle for %d minutes, status set to xa (priority %d).", xa_time, pri);
            }
            prefs_free_string(message);

            title_bar_set_presence(CONTACT_XA);
        } else if (check && (idle_ms < away_time_ms)) {
            activity_state = ACTIVITY_ST_ACTIVE;

            cons_show("No longer idle.");

            // send saved presence without last activity
            cl_ev_presence_send(saved_presence, saved_status, 0);
            contact_presence_t contact_pres = contact_presence_from_resource_presence(saved_presence);
            title_bar_set_presence(contact_pres);
        }
        break;
    case ACTIVITY_ST_XA:
        if (check && (idle_ms < away_time_ms)) {
            activity_state = ACTIVITY_ST_ACTIVE;

            cons_show("No longer idle.");

            // send saved presence without last activity
            cl_ev_presence_send(saved_presence, saved_status, 0);
            contact_presence_t contact_pres = contact_presence_from_resource_presence(saved_presence);
            title_bar_set_presence(contact_pres);
        }
        break;
    }

    prefs_free_string(mode);
}

static void
_init(char *log_level)
{
    setlocale(LC_ALL, "");
    // ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGWINCH, ui_sigwinch_handler);
    _create_directories();
    log_level_t prof_log_level = log_level_from_string(log_level);
    prefs_load();
    log_init(prof_log_level);
    log_stderr_init(PROF_LEVEL_ERROR);
    if (strcmp(PROF_PACKAGE_STATUS, "development") == 0) {
#ifdef PROF_HAVE_GIT_VERSION
            log_info("Starting Profanity (%sdev.%s.%s)...", PROF_PACKAGE_VERSION, PROF_GIT_BRANCH, PROF_GIT_REVISION);
#else
            log_info("Starting Profanity (%sdev)...", PROF_PACKAGE_VERSION);
#endif
    } else {
        log_info("Starting Profanity (%s)...", PROF_PACKAGE_VERSION);
    }
    chat_log_init();
    groupchat_log_init();
    accounts_load();
    char *theme = prefs_get_string(PREF_THEME);
    theme_init(theme);
    prefs_free_string(theme);
    ui_init();
    jabber_init();
    cmd_init();
    log_info("Initialising contact list");
    muc_init();
    tlscerts_init();
    scripts_init();
#ifdef PROF_HAVE_LIBOTR
    otr_init();
#endif
#ifdef PROF_HAVE_LIBGPGME
    p_gpg_init();
#endif
    atexit(_shutdown);
    plugins_init();
#ifdef PROF_HAVE_GTK
    create_tray();
#endif
    inp_nonblocking(TRUE);
}

static void
_shutdown(void)
{
    if (prefs_get_boolean(PREF_TITLEBAR_SHOW)) {
        if (prefs_get_boolean(PREF_TITLEBAR_GOODBYE)) {
            ui_goodbye_title();
        } else {
            ui_clear_win_title();
        }
    }

    jabber_conn_status_t conn_status = jabber_get_connection_status();
    if (conn_status == JABBER_CONNECTED) {
        cl_ev_disconnect();
    }
#ifdef PROF_HAVE_GTK
    destroy_tray();
#endif
    jabber_shutdown();
    plugins_on_shutdown();
    muc_close();
    caps_close();
    ui_close();
#ifdef PROF_HAVE_LIBOTR
    otr_shutdown();
#endif
#ifdef PROF_HAVE_LIBGPGME
    p_gpg_close();
#endif
    chat_log_close();
    theme_close();
    accounts_close();
    tlscerts_close();
    cmd_uninit();
    log_stderr_close();
    log_close();
    plugins_shutdown();
    prefs_close();
    if (saved_status) {
        free(saved_status);
    }
}

static void
_create_directories(void)
{
    gchar *xdg_config = xdg_get_config_home();
    gchar *xdg_data = xdg_get_data_home();

    GString *themes_dir = g_string_new(xdg_config);
    g_string_append(themes_dir, "/profanity/themes");
    GString *icons_dir = g_string_new(xdg_config);
    g_string_append(icons_dir, "/profanity/icons");
    GString *chatlogs_dir = g_string_new(xdg_data);
    g_string_append(chatlogs_dir, "/profanity/chatlogs");
    GString *logs_dir = g_string_new(xdg_data);
    g_string_append(logs_dir, "/profanity/logs");
    GString *plugins_dir = g_string_new(xdg_data);
    g_string_append(plugins_dir, "/profanity/plugins");

    if (!mkdir_recursive(themes_dir->str)) {
        log_error("Error while creating directory %s", themes_dir->str);
    }
    if (!mkdir_recursive(icons_dir->str)) {
        log_error("Error while creating directory %s", icons_dir->str);
    }
    if (!mkdir_recursive(chatlogs_dir->str)) {
        log_error("Error while creating directory %s", chatlogs_dir->str);
    }
    if (!mkdir_recursive(logs_dir->str)) {
        log_error("Error while creating directory %s", logs_dir->str);
    }
    if (!mkdir_recursive(plugins_dir->str)) {
        log_error("Error while creating directory %s", plugins_dir->str);
    }

    g_string_free(themes_dir, TRUE);
    g_string_free(icons_dir, TRUE);
    g_string_free(chatlogs_dir, TRUE);
    g_string_free(logs_dir, TRUE);
    g_string_free(plugins_dir, TRUE);

    g_free(xdg_config);
    g_free(xdg_data);
}
