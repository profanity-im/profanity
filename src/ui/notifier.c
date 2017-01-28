/*
 * notifier.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <glib.h>

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

#ifdef PLATFORM_CYGWIN
#include <windows.h>
#endif

#include "log.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/xmpp.h"
#include "xmpp/muc.h"

static GTimer *remind_timer;

void
notifier_initialise(void)
{
    remind_timer = g_timer_new();
}

void
notifier_uninit(void)
{
#ifdef HAVE_LIBNOTIFY
    if (notify_is_initted()) {
        notify_uninit();
    }
#endif
    g_timer_destroy(remind_timer);
}

void
notify_typing(const char *const name)
{
    char message[strlen(name) + 1 + 11];
    sprintf(message, "%s: typing...", name);

    notify(message, 10000, "Incoming message");
}

void
notify_invite(const char *const from, const char *const room, const char *const reason)
{
    GString *message = g_string_new("Room invite\nfrom: ");
    g_string_append(message, from);
    g_string_append(message, "\nto: ");
    g_string_append(message, room);
    if (reason) {
        g_string_append_printf(message, "\n\"%s\"", reason);
    }

    notify(message->str, 10000, "Incoming message");

    g_string_free(message, TRUE);
}

void
notify_message(const char *const name, int num, const char *const text)
{
    int ui_index = num;
    if (ui_index == 10) {
        ui_index = 0;
    }

    GString *message = g_string_new("");
    g_string_append_printf(message, "%s (win %d)", name, ui_index);
    if (text && prefs_get_boolean(PREF_NOTIFY_CHAT_TEXT)) {
        g_string_append_printf(message, "\n%s", text);
    }

    notify(message->str, 10000, "incoming message");
    g_string_free(message, TRUE);
}

void
notify_room_message(const char *const nick, const char *const room, int num, const char *const text)
{
    int ui_index = num;
    if (ui_index == 10) {
        ui_index = 0;
    }

    GString *message = g_string_new("");
    g_string_append_printf(message, "%s in %s (win %d)", nick, room, ui_index);
    if (text && prefs_get_boolean(PREF_NOTIFY_ROOM_TEXT)) {
        g_string_append_printf(message, "\n%s", text);
    }

    notify(message->str, 10000, "incoming message");

    g_string_free(message, TRUE);
}

void
notify_subscription(const char *const from)
{
    GString *message = g_string_new("Subscription request: \n");
    g_string_append(message, from);
    notify(message->str, 10000, "Incoming message");
    g_string_free(message, TRUE);
}

void
notify_remind(void)
{
    gdouble elapsed = g_timer_elapsed(remind_timer, NULL);
    gint remind_period = prefs_get_notify_remind();
    if (remind_period > 0 && elapsed >= remind_period) {
        gboolean donotify = wins_do_notify_remind();
        gint unread = wins_get_total_unread();
        gint open = muc_invites_count();
        gint subs = presence_sub_request_count();

        GString *text = g_string_new("");

        if (donotify && unread > 0) {
            if (unread == 1) {
                g_string_append(text, "1 unread message");
            } else {
                g_string_append_printf(text, "%d unread messages", unread);
            }

        }
        if (open > 0) {
            if (unread > 0) {
                g_string_append(text, "\n");
            }
            if (open == 1) {
                g_string_append(text, "1 room invite");
            } else {
                g_string_append_printf(text, "%d room invites", open);
            }
        }
        if (subs > 0) {
            if ((unread > 0) || (open > 0)) {
                g_string_append(text, "\n");
            }
            if (subs == 1) {
                g_string_append(text, "1 subscription request");
            } else {
                g_string_append_printf(text, "%d subscription requests", subs);
            }
        }

        if ((donotify && unread > 0) || (open > 0) || (subs > 0)) {
            notify(text->str, 5000, "Incoming message");
        }

        g_string_free(text, TRUE);

        g_timer_start(remind_timer);
    }
}

void
notify(const char *const message, int timeout, const char *const category)
{
#ifdef HAVE_LIBNOTIFY
    log_debug("Attempting notification: %s", message);
    if (notify_is_initted()) {
        log_debug("Reinitialising libnotify");
        notify_uninit();
        notify_init("Profanity");
    } else {
        log_debug("Initialising libnotify");
        notify_init("Profanity");
    }
    if (notify_is_initted()) {
        NotifyNotification *notification;
        notification = notify_notification_new("Profanity", message, NULL);
        notify_notification_set_timeout(notification, timeout);
        notify_notification_set_category(notification, category);
        notify_notification_set_urgency(notification, NOTIFY_URGENCY_NORMAL);

        GError *error = NULL;
        gboolean notify_success = notify_notification_show(notification, &error);

        if (!notify_success) {
            log_error("Error sending desktop notification:");
            log_error("  -> Message : %s", message);
            log_error("  -> Error   : %s", error->message);
        } else {
	    log_debug("Notification sent.");
	}
    } else {
        log_error("Libnotify not initialised.");
    }
#endif
#ifdef PLATFORM_CYGWIN
    NOTIFYICONDATA nid;
    nid.cbSize = sizeof(NOTIFYICONDATA);
    //nid.hWnd = hWnd;
    nid.uID = 100;
    nid.uVersion = NOTIFYICON_VERSION;
    //nid.uCallbackMessage = WM_MYMESSAGE;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    strncpy(nid.szTip, "Tray Icon", 10);
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    Shell_NotifyIcon(NIM_ADD, &nid);

    // For a Ballon Tip
    nid.uFlags = NIF_INFO;
    strncpy(nid.szInfoTitle, "Profanity", 10); // Title
    strncpy(nid.szInfo, message, 256); // Copy Tip
    nid.uTimeout = timeout;  // 3 Seconds
    nid.dwInfoFlags = NIIF_INFO;

    Shell_NotifyIcon(NIM_MODIFY, &nid);
#endif
#ifdef HAVE_OSXNOTIFY
    GString *notify_command = g_string_new("terminal-notifier -title \"Profanity\" -message '");

    char *escaped_single = str_replace(message, "'", "'\\''");

    if (escaped_single[0] == '<') {
        g_string_append(notify_command, "\\<");
        g_string_append(notify_command, &escaped_single[1]);
    } else if (escaped_single[0] == '[') {
        g_string_append(notify_command, "\\[");
        g_string_append(notify_command, &escaped_single[1]);
    } else if (escaped_single[0] == '(') {
        g_string_append(notify_command, "\\(");
        g_string_append(notify_command, &escaped_single[1]);
    } else if (escaped_single[0] == '{') {
        g_string_append(notify_command, "\\{");
        g_string_append(notify_command, &escaped_single[1]);
    } else {
        g_string_append(notify_command, escaped_single);
    }

    g_string_append(notify_command, "'");
    free(escaped_single);

    char *term_name = getenv("TERM_PROGRAM");
    char *app_id = NULL;
    if (g_strcmp0(term_name, "Apple_Terminal") == 0) {
        app_id = "com.apple.Terminal";
    } else if (g_strcmp0(term_name, "iTerm.app") == 0) {
        app_id = "com.googlecode.iterm2";
    }

    if (app_id) {
        g_string_append(notify_command, " -sender ");
        g_string_append(notify_command, app_id);
    }

    int res = system(notify_command->str);
    if (res == -1) {
        log_error("Could not send desktop notificaion.");
    }

    g_string_free(notify_command, TRUE);
#endif
}
