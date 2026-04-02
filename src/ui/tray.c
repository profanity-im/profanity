/*
 * tray.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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

#if defined(GDK_WINDOWING_X11) && defined(HAVE_XEXITHANDLER)
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#endif

static gboolean gtk_ready = FALSE;
static GtkStatusIcon* prof_tray = NULL;
static GString* icon_filename = NULL;
static GString* icon_msg_filename = NULL;
static gint unread_messages;
static gboolean shutting_down;
static guint timer;

#if defined(GDK_WINDOWING_X11) && defined(HAVE_XEXITHANDLER)
static void
_x_io_error_handler(Display* display, void* user_data)
{
    log_warning("Error: X Server connection lost.");
    gtk_ready = FALSE;
}

static int
_x_io_handler(Display* display)
{
    return 0;
}
#endif

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
    GString* icons_dir = NULL;

#ifdef ICONS_PATH

    icons_dir = g_string_new(ICONS_PATH);
    icon_filename = g_string_new(icons_dir->str);
    icon_msg_filename = g_string_new(icons_dir->str);
    g_string_append(icon_filename, "/proIcon.png");
    g_string_append(icon_msg_filename, "/proIconMsg.png");
    g_string_free(icons_dir, TRUE);

#endif /* ICONS_PATH */

    auto_gchar gchar* icons_dir_s = files_get_config_path(DIR_ICONS);
    icons_dir = g_string_new(icons_dir_s);
    auto_gerror GError* err = NULL;

    if (!g_file_test(icons_dir->str, G_FILE_TEST_IS_DIR)) {
        return;
    }

    GDir* dir = g_dir_open(icons_dir->str, 0, &err);
    if (dir) {
        GString* name = g_string_new(g_dir_read_name(dir));
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
        log_error("Unable to open dir: %s", PROF_GERROR_MESSAGE(err));
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
    if (shutting_down || !gtk_ready) {
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

static void
_tray_shutdown(void)
{
    tray_disable();
    if (icon_filename) {
        g_string_free(icon_filename, TRUE);
        icon_filename = NULL;
    }
    if (icon_msg_filename) {
        g_string_free(icon_msg_filename, TRUE);
        icon_msg_filename = NULL;
    }
}

void
tray_init(void)
{
    prof_add_shutdown_routine(_tray_shutdown);
    _get_icons();
    gtk_ready = gtk_init_check(0, NULL);
    log_debug("Env is GTK-ready: %s", gtk_ready ? "true" : "false");
    if (!gtk_ready) {
        return;
    }

#if defined(GDK_WINDOWING_X11) && defined(HAVE_XEXITHANDLER)
    GdkDisplay* display = gdk_display_get_default();
    if (GDK_IS_X11_DISPLAY(display)) {
        XSetIOErrorExitHandler(GDK_DISPLAY_XDISPLAY(display), _x_io_error_handler, NULL);
        XSetIOErrorHandler(_x_io_handler);
    }
#endif

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

gboolean
tray_gtk_ready(void)
{
    return gtk_ready;
}

void
tray_set_timer(int interval)
{
    if (!gtk_ready) {
        return;
    }
    if (timer) {
        g_source_remove(timer);
    }
    _tray_change_icon(NULL);
    timer = g_timeout_add(interval * 1000, _tray_change_icon, NULL);
}

/*
 * Create tray icon
 *
 * This will initialize the timer that will be called in order to change the icons
 * and will search the icons in the defaults paths
 */
void
tray_enable(void)
{
    if (!gtk_ready) {
        return;
    }
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
    if (timer) {
        g_source_remove(timer);
        timer = 0;
    }
    if (prof_tray) {
        g_clear_object(&prof_tray);
        prof_tray = NULL;
    }
}

#endif
