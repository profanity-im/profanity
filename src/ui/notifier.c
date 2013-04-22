/*
 * notifier.c
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

#include <stdio.h>
#include <string.h>

#include <glib.h>
#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif
#ifdef PLATFORM_CYGWIN
#include <windows.h>
#endif

#include "log.h"
#include "ui/ui.h"

static void _notify(const char * const message, int timeout,
    const char * const category);

void
notifier_init(void)
{
#ifdef HAVE_LIBNOTIFY
    notify_init("Profanity");
#endif
}

void
notifier_uninit(void)
{
#ifdef HAVE_LIBNOTIFY
    if (notify_is_initted()) {
        notify_uninit();
    }
#endif
}

void
notify_typing(const char * const from)
{
    char message[strlen(from) + 1 + 11];
    sprintf(message, "%s: typing...", from);

    _notify(message, 10000, "Incoming message");
}

void
notify_invite(const char * const from, const char * const room)
{
    GString *message = g_string_new("Room invite\nfrom: ");
    g_string_append(message, from);
    g_string_append(message, "\nto: ");
    g_string_append(message, room);

    _notify(message->str, 10000, "Incoming message");

    g_string_free(message, FALSE);
}

void
notify_message(const char * const short_from)
{
    char message[strlen(short_from) + 1 + 10];
    sprintf(message, "%s: message.", short_from);

    _notify(message, 10000, "Incoming message");
}

void
notify_remind(void)
{
    gint unread = ui_unread();
    //gint open = jabber_open_invites();
    gint open = 0;

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
            g_string_append(text, "1 open invite");
        } else {
            g_string_append_printf(text, "%d open invites", open);
        }
    }

    if ((unread > 0) || (open > 0)) {
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
    strcpy(nid.szTip, "Tray Icon");
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    Shell_NotifyIcon(NIM_ADD, &nid);

    // For a Ballon Tip
    nid.uFlags = NIF_INFO;
    strcpy(nid.szInfoTitle, "Profanity"); // Title
    strcpy(nid.szInfo, message); // Copy Tip
    nid.uTimeout = timeout;  // 3 Seconds
    nid.dwInfoFlags = NIIF_INFO;

    Shell_NotifyIcon(NIM_MODIFY, &nid);
#endif
}
