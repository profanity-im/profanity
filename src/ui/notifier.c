/*
 * notifier.c
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
#include "muc.h"
#include "ui/ui.h"

static void _notify(const char * const message, int timeout,
    const char * const category);

static void
_notifier_init(void)
{
#ifdef HAVE_LIBNOTIFY
    notify_init("Profanity");
#endif
}

static void
_notifier_uninit(void)
{
#ifdef HAVE_LIBNOTIFY
    if (notify_is_initted()) {
        notify_uninit();
    }
#endif
}

static void
_notify_typing(const char * const handle)
{
    char message[strlen(handle) + 1 + 11];
    sprintf(message, "%s: typing...", handle);

    _notify(message, 10000, "Incoming message");
}

static void
_notify_invite(const char * const from, const char * const room,
    const char * const reason)
{
    GString *message = g_string_new("Room invite\nfrom: ");
    g_string_append(message, from);
    g_string_append(message, "\nto: ");
    g_string_append(message, room);
    if (reason != NULL) {
        g_string_append_printf(message, "\n\"%s\"", reason);
    }

    _notify(message->str, 10000, "Incoming message");

    g_string_free(message, TRUE);
}

static void
_notify_message(const char * const handle, int win)
{
    char message[strlen(handle) + 1 + 14];
    sprintf(message, "%s: message (%d).", handle, win);

    _notify(message, 10000, "incoming message");
}

static void
_notify_room_message(const char * const handle, const char * const room, int win)
{
    GString *text = g_string_new("");

    g_string_append_printf(text, "Room: %s\n", room);
    g_string_append_printf(text, "%s: message (%d).", handle, win);

    _notify(text->str, 10000, "incoming message");

    g_string_free(text, TRUE);
}

static void
_notify_subscription(const char * const from)
{
    GString *message = g_string_new("Subscription request: \n");
    g_string_append(message, from);
    _notify(message->str, 10000, "Incomming message");
    g_string_free(message, TRUE);
}

static void
_notify_remind(void)
{
    gint unread = ui_unread();
    gint open = muc_invite_count();
    gint subs = presence_sub_request_count();

    GString *text = g_string_new("");

    if (unread > 0) {
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

    if ((unread > 0) || (open > 0) || (subs > 0)) {
        _notify(text->str, 5000, "Incoming message");
    }

    g_string_free(text, TRUE);
}

static void
_notify(const char * const message, int timeout,
    const char * const category)
{
#ifdef HAVE_LIBNOTIFY

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
        }
    } else {
        log_error("Libnotify initialisation error.");
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
    GString *notify_command = g_string_new("terminal-notifier -title 'Profanity' -message '");
    g_string_append(notify_command, message);
    g_string_append(notify_command, "'");

    char *term_name = getenv("TERM_PROGRAM");
    char *app_id = NULL;
    if (g_strcmp0(term_name, "Apple_Terminal") == 0) {
        app_id = "com.apple.Terminal";
    } else if (g_strcmp0(term_name, "iTerm.app") == 0) {
        app_id = "com.googlecode.iterm2";
    }

    if (app_id != NULL) {
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

void
notifier_init_module(void)
{
    notifier_init = _notifier_init;
    notifier_uninit = _notifier_uninit;
    notify_typing = _notify_typing;
    notify_invite = _notify_invite;
    notify_message = _notify_message;
    notify_room_message =  _notify_room_message;
    notify_subscription = _notify_subscription;
    notify_remind = _notify_remind;
}

