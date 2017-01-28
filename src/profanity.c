/*
 * profanity.c
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

#ifdef HAVE_GTK
#include "ui/tray.h"
#endif

#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "profanity.h"
#include "common.h"
#include "log.h"
#include "config/files.h"
#include "config/tlscerts.h"
#include "config/accounts.h"
#include "config/preferences.h"
#include "config/theme.h"
#include "config/tlscerts.h"
#include "config/scripts.h"
#include "command/cmd_defs.h"
#include "plugins/plugins.h"
#include "event/client_events.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/resource.h"
#include "xmpp/session.h"
#include "xmpp/xmpp.h"
#include "xmpp/muc.h"
#include "xmpp/chat_session.h"
#include "xmpp/chat_state.h"
#include "xmpp/contact.h"
#include "xmpp/roster_list.h"

#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif

#ifdef HAVE_LIBGPGME
#include "pgp/gpg.h"
#endif

static void _init(char *log_level);
static void _shutdown(void);
static void _connect_default(const char * const account);

static gboolean cont = TRUE;
static gboolean force_quit = FALSE;

void
prof_run(char *log_level, char *account_name)
{
    _init(log_level);
    plugins_on_start();
    _connect_default(account_name);

    ui_update();

    log_info("Starting main event loop");

    session_init_activity();

    char *line = NULL;
    while(cont && !force_quit) {
        log_stderr_handler();
        session_check_autoaway();

        line = inp_readline();
        if (line) {
            ProfWin *window = wins_get_current();
            cont = cmd_process_input(window, line);
            free(line);
            line = NULL;
        } else {
            cont = TRUE;
        }

#ifdef HAVE_LIBOTR
        otr_poll();
#endif
        plugins_run_timed();
        notify_remind();
        session_process_events();
        iq_autoping_check();
        ui_update();
#ifdef HAVE_GTK
        tray_update();
#endif
    }
}

void
prof_set_quit(void)
{
    force_quit = TRUE;
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
_init(char *log_level)
{
    setlocale(LC_ALL, "");
    // ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGWINCH, ui_sigwinch_handler);
    if (pthread_mutex_init(&lock, NULL) != 0) {
        log_error("Mutex init failed");
        exit(1);
    }
    pthread_mutex_lock(&lock);
    files_create_directories();
    log_level_t prof_log_level = log_level_from_string(log_level);
    prefs_load();
    log_init(prof_log_level);
    log_stderr_init(PROF_LEVEL_ERROR);
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
    accounts_load();
    char *theme = prefs_get_string(PREF_THEME);
    theme_init(theme);
    prefs_free_string(theme);
    ui_init();
    session_init();
    cmd_init();
    log_info("Initialising contact list");
    muc_init();
    tlscerts_init();
    scripts_init();
#ifdef HAVE_LIBOTR
    otr_init();
#endif
#ifdef HAVE_LIBGPGME
    p_gpg_init();
#endif
    atexit(_shutdown);
    plugins_init();
#ifdef HAVE_GTK
    tray_init();
#endif
    inp_nonblocking(TRUE);
    ui_resize();
}

static void
_shutdown(void)
{
    if (prefs_get_boolean(PREF_WINTITLE_SHOW)) {
        if (prefs_get_boolean(PREF_WINTITLE_GOODBYE)) {
            ui_goodbye_title();
        } else {
            ui_clear_win_title();
        }
    }

    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status == JABBER_CONNECTED) {
        cl_ev_disconnect();
    }
#ifdef HAVE_GTK
    tray_shutdown();
#endif
    session_shutdown();
    plugins_on_shutdown();
    muc_close();
    caps_close();
#ifdef HAVE_LIBOTR
    otr_shutdown();
#endif
#ifdef HAVE_LIBGPGME
    p_gpg_close();
#endif
    chat_log_close();
    theme_close();
    accounts_close();
    tlscerts_close();
    log_stderr_close();
    log_close();
    plugins_shutdown();
    cmd_uninit();
    ui_close();
    prefs_close();
}
