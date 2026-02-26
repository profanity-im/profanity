/*
 * profanity.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
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
#include <unistd.h>

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
#include "chatlog.h"
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

#ifdef HAVE_OMEMO
#include "omemo/omemo.h"
#endif

static unsigned int min_runtime = 5;

static void _init(char* log_level, char* config_file, char* log_file, char* theme_name);
static void _shutdown(void);
static void _connect_default(const char* const account);

pthread_mutex_t lock;
static gboolean force_quit = FALSE;

void
prof_run(gchar* log_level, gchar* account_name, gchar* config_file, gchar* log_file, gchar* theme_name, gchar** commands)
{
    gboolean cont = TRUE;

    _init(log_level, config_file, log_file, theme_name);
    plugins_on_start();
    _connect_default(account_name);

    ui_update();

    log_info("Starting main event loop");

    session_init_activity();

    GTimer* runtime = g_timer_new();

    char* line = NULL;
    GTimer* waittimer = g_timer_new();
    g_timer_stop(waittimer);
    int waittime = 0;
    while (cont && !force_quit) {
        log_stderr_handler();
        session_check_autoaway();

        line = commands ? *commands : inp_readline();
        if (commands && line && memcmp(line, "/sleep", 6) == 0) {
            if (!g_timer_is_active(waittimer)) {
                gchar* err_msg;
                if (strtoi_range(line + 7, &waittime, 0, 300, &err_msg)) {
                    g_timer_start(waittimer);
                    /* Increase the minimal runtime by the waiting time
                     * so we can be sure there's runtime left after executing
                     * the last command.
                     */
                    min_runtime += waittime;
                } else {
                    log_error(err_msg);
                    g_free(err_msg);
                    commands = NULL;
                }
            } else if (g_timer_elapsed(waittimer, NULL) >= waittime) {
                g_timer_stop(waittimer);
                commands++;
                if (!(*commands))
                    commands = NULL;
            }
            cont = TRUE;
        } else if (line) {
            ProfWin* window = wins_get_current();
            cont = cmd_process_input(window, line);
            if (commands) {
                commands++;
                if (!(*commands))
                    commands = NULL;
            } else {
                free(line);
                line = NULL;
            }
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
    g_timer_destroy(waittimer);
    g_timer_elapsed(runtime, NULL) < min_runtime ? sleep(min_runtime) : (void)NULL;
    g_timer_destroy(runtime);
}

gboolean
prof_set_quit(void)
{
    gboolean ret = force_quit;
    force_quit = TRUE;
    return ret;
}

static void
_connect_default(const char* const account)
{
    ProfWin* window = wins_get_current();
    if (account) {
        cmd_execute_connect(window, account);
    } else {
        auto_gchar gchar* pref_connect_account = prefs_get_string(PREF_CONNECT_ACCOUNT);
        if (pref_connect_account) {
            cmd_execute_connect(window, pref_connect_account);
        }
    }
}

static void
sigterm_handler(int sig)
{
    log_info("Received signal %d, exiting", sig);
    force_quit = TRUE;
}

static void
_init(char* log_level, char* config_file, char* log_file, char* theme_name)
{
    setlocale(LC_ALL, "");
    // ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGWINCH, ui_sigwinch_handler);
    signal(SIGTERM, sigterm_handler);
    signal(SIGHUP, sigterm_handler);
    if (pthread_mutex_init(&lock, NULL) != 0) {
        log_error("Mutex init failed");
        exit(1);
    }

    pthread_mutex_lock(&lock);
    files_create_directories();
    log_level_t prof_log_level;
    log_level_from_string(log_level, &prof_log_level);
    prefs_load(config_file);
    log_init(prof_log_level, log_file);
    log_stderr_init(PROF_LEVEL_ERROR);

    auto_gchar gchar* prof_version = prof_get_version();
    log_info("Starting Profanity (%s)…", prof_version);

    chatlog_init();
    accounts_load();

    if (theme_name) {
        theme_init(theme_name);
    } else {
        auto_gchar gchar* theme = prefs_get_string(PREF_THEME);
        theme_init(theme);
    }

    ui_init();
    if (prof_log_level == PROF_LEVEL_DEBUG) {
        ProfWin* console = wins_get_console();
        win_println(console, THEME_DEFAULT, "-", "Debug mode enabled! Logging to: ");
        win_println(console, THEME_DEFAULT, "-", get_log_file_location());
    }
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
#ifdef HAVE_OMEMO
    omemo_init();
#endif
    atexit(_shutdown);
    plugins_init();
#ifdef HAVE_GTK
    tray_init();
#endif
    inp_nonblocking(TRUE);
    ui_resize();
}

static GList* shutdown_routines = NULL;

/* We have to encapsulate the function pointer, since the C standard does not guarantee
 * that a void* can be converted to a function* and vice-versa.
 */
struct shutdown_routine
{
    void (*routine)(void);
};

static gint
_cmp_shutdown_routine(const struct shutdown_routine* a, const struct shutdown_routine* b)
{
    return a->routine != b->routine;
}

void
prof_add_shutdown_routine(void (*routine)(void))
{
    struct shutdown_routine this = { .routine = routine };
    if (g_list_find_custom(shutdown_routines, &this, (GCompareFunc)_cmp_shutdown_routine)) {
        return;
    }
    struct shutdown_routine* r = malloc(sizeof *r);
    r->routine = routine;
    shutdown_routines = g_list_prepend(shutdown_routines, r);
}

static void
_call_and_free_shutdown_routine(struct shutdown_routine* r)
{
    r->routine();
    free(r);
}

void
prof_shutdown(void)
{
    if (shutdown_routines) {
        g_list_free_full(shutdown_routines, (GDestroyNotify)_call_and_free_shutdown_routine);
        shutdown_routines = NULL;
    }
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
    prof_shutdown();
    /* Prefs and logs have to be closed in swapped order, so they're no using the automatic
     * shutdown mechanism. */
    prefs_close();
    log_close();
}
