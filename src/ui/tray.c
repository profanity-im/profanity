/*
 * tray.c
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

#ifdef HAVE_GTK
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>

#include "log.h"
#include "config/preferences.h"
#include "config/files.h"
#include "ui/tray.h"
#include "ui/window_list.h"

static gboolean gtk_ready = FALSE;
static GtkStatusIcon *prof_tray = NULL;
static GString *icon_filename = NULL;
static GString *icon_msg_filename = NULL;
static gint unread_messages;
static gboolean shutting_down;
static guint timer;

/* {{{ Privates */

/*
 * Get icons from installation share folder or (if defined) .locale user's folder
 *
 * As implementation, looking through all the entries in the .locale folder is chosen.
 * While useless as now, it might be useful in case an association name-icon is created.
 * As now, with 2 icons only, this is pretty useless, but it is not harming ;)
 *
 */
static void
_get_icons(void)
{
    GString *icons_dir =  NULL;

#ifdef ICONS_PATH

    icons_dir = g_string_new(ICONS_PATH);
    icon_filename = g_string_new(icons_dir->str);
    icon_msg_filename = g_string_new(icons_dir->str);
    g_string_append(icon_filename, "/proIcon.png");
    g_string_append(icon_msg_filename, "/proIconMsg.png");
    g_string_free(icons_dir, TRUE);

#endif /* ICONS_PATH */

    char *icons_dir_s = files_get_config_path(DIR_ICONS);
    icons_dir = g_string_new(icons_dir_s);
    free(icons_dir_s);
    GError *err = NULL;
    if (!g_file_test(icons_dir->str, G_FILE_TEST_IS_DIR)) {
        return;
    }
    GDir *dir = g_dir_open(icons_dir->str, 0, &err);
    if (dir) {
        GString *name = g_string_new(g_dir_read_name(dir));
        while (name->len) {
            if (g_strcmp0("proIcon.png", name->str) == 0) {
                if (icon_filename) {
                    g_string_free(icon_filename, TRUE);
                }
                icon_filename = g_string_new(icons_dir->str);
                g_string_append(icon_filename, "/proIcon.png");
            } else if (g_strcmp0("proIconMsg.png", name->str) == 0) {
                if (icon_msg_filename) {
                    g_string_free(icon_msg_filename, TRUE);
                }
                icon_msg_filename = g_string_new(icons_dir->str);
                g_string_append(icon_msg_filename, "/proIconMsg.png");
            }
            g_string_free(name, TRUE);
            name = g_string_new(g_dir_read_name(dir));
        }
        g_string_free(name, TRUE);
    } else {
        log_error("Unable to open dir: %s", err->message);
        g_error_free(err);
    }
    g_dir_close(dir);
    g_string_free(icons_dir, TRUE);
}

/*
 * Callback for the timer
 *
 * This is the callback that the timer is calling in order to check if messages are there.
 *
 */
gboolean
_tray_change_icon(gpointer data)
{
    if (shutting_down) {
        return FALSE;
    }

    unread_messages = wins_get_total_unread();

    if (unread_messages) {
        if (!prof_tray) {
            prof_tray = gtk_status_icon_new_from_file(icon_msg_filename->str);
        } else {
            gtk_status_icon_set_from_file(prof_tray, icon_msg_filename->str);
        }
    } else {
        if (prefs_get_boolean(PREF_TRAY_READ)) {
            if (!prof_tray) {
                prof_tray = gtk_status_icon_new_from_file(icon_filename->str);
            } else {
                gtk_status_icon_set_from_file(prof_tray, icon_filename->str);
            }
        } else {
            g_clear_object(&prof_tray);
            prof_tray = NULL;
        }
    }

    return TRUE;
}

/* }}} */
/* {{{ Public */

void
tray_init(void)
{
    _get_icons();
    gtk_ready = gtk_init_check(0, NULL);
    log_debug("Env is GTK-ready: %s", gtk_ready ? "true" : "false");
    if (!gtk_ready) {
        return;
    }

    gtk_init(0, NULL);
    if (prefs_get_boolean(PREF_TRAY)) {
        log_debug("Building GTK icon");
        tray_enable();
    }

    gtk_main_iteration_do(FALSE);
}

void
tray_update(void)
{
    if (gtk_ready) {
        gtk_main_iteration_do(FALSE);
    }
}

void
tray_shutdown(void)
{
    if (gtk_ready && prefs_get_boolean(PREF_TRAY)) {
        tray_disable();
    }
    g_string_free(icon_filename, TRUE);
    g_string_free(icon_msg_filename, TRUE);
}

void
tray_set_timer(int interval)
{
    g_source_remove(timer);
    _tray_change_icon(NULL);
    timer = g_timeout_add(interval * 1000, _tray_change_icon, NULL);
}

void
tray_enable(void)
{
    prof_tray = gtk_status_icon_new_from_file(icon_filename->str);
    shutting_down = FALSE;
    _tray_change_icon(NULL);
    int interval = prefs_get_tray_timer() * 1000;
    timer = g_timeout_add(interval, _tray_change_icon, NULL);
}

void
tray_disable(void)
{
    shutting_down = TRUE;
    g_source_remove(timer);
    if (prof_tray) {
        g_clear_object(&prof_tray);
        prof_tray = NULL;
    }
}

/* }}} */
#endif
