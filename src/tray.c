/*
 * tray.c
 *
 * Copyright (C) 2012 - 2016 David Petroni <petrodavi@gmail.com>
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

#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>

#include "tray.h"
#include "window_list.h"

static GtkStatusIcon *prof_tray = NULL;
static GString *icon_filename = NULL;
static GString *icon_msg_filename = NULL;
static int unread_messages;
static bool shutting_down;
static guint timer;

static gchar*
_get_icons_dir(void)
{
    GString *icons_dir =  NULL;

#ifdef ICONS_PATH

    icons_dir = g_string_new(ICONS_PATH);

#else

    gchar *xdg_config = xdg_get_config_home();
    icons_dir = g_string_new(xdg_config);
    g_free(xdg_config);
    g_string_append(icons_dir, "/profanity/icons");

#endif /* ICONS_PATH */

    return g_string_free(icons_dir, false);
}

gboolean tray_change_icon(gpointer data)
{
    if (shutting_down) {
        return false;
    }

    unread_messages = wins_get_total_unread();

    if (unread_messages) {
        gtk_status_icon_set_from_file(prof_tray, icon_msg_filename->str);
    } else {
        gtk_status_icon_set_from_file(prof_tray, icon_filename->str);
    }

    return true;
}

void create_tray(void)
{
    gchar *icons_dir_ret = _get_icons_dir();
    icon_filename = g_string_new(icons_dir_ret);
    icon_msg_filename = g_string_new(icons_dir_ret);
    g_string_append(icon_filename, "/proIcon.png");
    g_string_append(icon_msg_filename, "/proIconMsg.png");
    prof_tray = gtk_status_icon_new_from_file(icon_filename->str);
    shutting_down = false;
    timer = g_timeout_add(5000, tray_change_icon, NULL);
    g_free(icons_dir_ret);
}

void destroy_tray(void)
{
    shutting_down = true;
    g_source_remove(timer);
    if (prof_tray) {
        gtk_widget_destroy(GTK_WIDGET(prof_tray));
        prof_tray = NULL;
    }
    g_string_free(icon_filename, false);
    g_string_free(icon_msg_filename, false);
}
