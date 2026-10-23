/*
 * notifier.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#ifdef HAVE_NOTIFICATIONS
#include "tools/dbus.h"
#include <gio/gio.h>
#endif

#ifdef PLATFORM_CYGWIN
#include <windows.h>
#endif

#include "common.h"
#include "config/files.h"
#include "config/preferences.h"
#include "log.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/avatar.h"
#include "xmpp/muc.h"
#include "xmpp/xmpp.h"

static GTimer* remind_timer;

static void
_notifier_uninit(void)
{
}

#ifdef HAVE_NOTIFICATIONS
static void
_on_notify_reply(GObject* source_object, GAsyncResult* res, gpointer user_data)
{
    auto_gerror GError* error = NULL;
    GVariant* result = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source_object), res, &error);
    if (!result) {
        log_debug("Failed to send desktop notification: %s", PROF_GERROR_MESSAGE(error));
        return;
    }

    guint32 id = 0;
    g_variant_get(result, "(u)", &id);
    log_debug("Desktop notification sent (id: %u)", id);
    g_variant_unref(result);
}

static void
_notify(const gchar* const message, int timeout, const gchar* const category, const gchar* const avatar_path)
{
    GDBusConnection* conn = dbus_get_session_bus();
    if (!conn) {
        log_debug("D-Bus session bus not available, cannot send notification.");
        return;
    }

    auto_gchar gchar* summary = NULL;
    const gchar* body = NULL;
    const gchar* newline = strchr(message, '\n');
    if (newline) {
        summary = g_strndup(message, newline - message);
        body = newline + 1;
    } else {
        summary = g_strdup("Profanity");
        body = message;
    }

    auto_gchar gchar* app_icon_path = files_get_icon_path("proIcon.png");
    const gchar* app_icon = app_icon_path ? app_icon_path : "proIcon";

    GVariantBuilder hints_builder;
    g_variant_builder_init(&hints_builder, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&hints_builder, "{sv}", "urgency", g_variant_new_byte(1));
    g_variant_builder_add(&hints_builder, "{sv}", "category", g_variant_new_string(category ? category : "im.received"));
    g_variant_builder_add(&hints_builder, "{sv}", "desktop-entry", g_variant_new_string("profanity"));

    if (avatar_path && g_file_test(avatar_path, G_FILE_TEST_EXISTS)) {
        g_variant_builder_add(&hints_builder, "{sv}", "image-path", g_variant_new_string(avatar_path));
        g_variant_builder_add(&hints_builder, "{sv}", "image_path", g_variant_new_string(avatar_path));
    }

    GVariantBuilder actions_builder;
    g_variant_builder_init(&actions_builder, G_VARIANT_TYPE("as"));

    GVariant* parameters = g_variant_new(
        "(susssasa{sv}i)",
        "Profanity",
        0,
        app_icon,
        summary,
        body,
        &actions_builder,
        &hints_builder,
        timeout);

    g_dbus_connection_call(
        conn,
        "org.freedesktop.Notifications",
        "/org/freedesktop/Notifications",
        "org.freedesktop.Notifications",
        "Notify",
        parameters,
        G_VARIANT_TYPE("(u)"),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        _on_notify_reply,
        NULL);
}
#elif defined(PLATFORM_CYGWIN)
static void
_notify(const gchar* const message, int timeout, const gchar* const category, const gchar* const avatar_path)
{
    NOTIFYICONDATA nid;
    memset(&nid, 0, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    // nid.hWnd = hWnd;
    nid.uID = 100;
    nid.uVersion = NOTIFYICON_VERSION;
    // nid.uCallbackMessage = WM_MYMESSAGE;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    strcpy(nid.szTip, "Tray Icon");
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    Shell_NotifyIcon(NIM_ADD, &nid);

    // For a Ballon Tip
    nid.uFlags = NIF_INFO;
    strcpy(nid.szInfoTitle, "Profanity");                 // Title
    strncpy(nid.szInfo, message, sizeof(nid.szInfo) - 1); // Copy Tip
    nid.uTimeout = timeout;                               // 3 Seconds
    nid.dwInfoFlags = NIIF_INFO;

    Shell_NotifyIcon(NIM_MODIFY, &nid);
}
#elif defined(HAVE_OSXNOTIFY)
static void
_notify(const gchar* const message, int timeout, const gchar* const category, const gchar* const avatar_path)
{
    auto_gchar gchar* msg = NULL;
    if (message && (message[0] == '<' || message[0] == '[' || message[0] == '(' || message[0] == '{')) {
        msg = g_strdup_printf("\\%s", message);
    }
    auto_gchar gchar* quoted = g_shell_quote(msg ? msg : message);

    GString* notify_command = g_string_new("terminal-notifier -title \"Profanity\" -message ");
    g_string_append(notify_command, quoted);

    char* term_name = getenv("TERM_PROGRAM");
    char* app_id = NULL;
    if (g_strcmp0(term_name, "Apple_Terminal") == 0) {
        app_id = "com.apple.Terminal";
    } else if (g_strcmp0(term_name, "iTerm.app") == 0) {
        app_id = "com.googlecode.iterm2";
    }

    if (app_id) {
        g_string_append(notify_command, " -sender ");
        g_string_append(notify_command, app_id);
    }

    if (avatar_path && g_file_test(avatar_path, G_FILE_TEST_EXISTS)) {
        auto_gchar gchar* quoted_avatar = g_shell_quote(avatar_path);
        g_string_append_printf(notify_command, " -contentImage %s", quoted_avatar);
    }

    int res = system(notify_command->str);
    if (res == -1) {
        log_error("Could not send desktop notification.");
    }

    g_string_free(notify_command, TRUE);
}
#else
static void
_notify(const gchar* const message, int timeout, const gchar* const category, const gchar* const avatar_path)
{
    log_error_once("Notification backend missing");
}
#endif

void
notifier_initialise(void)
{
    remind_timer = g_timer_new();
}

void
notifier_uninit(void)
{
    _notifier_uninit();
    g_timer_destroy(remind_timer);
}

void
notify_typing(const gchar* const name, const gchar* const barejid)
{
    auto_gchar gchar* message = g_strdup_printf("%s: typing…", name);
    auto_gchar gchar* avatar_path = avatar_get_path(barejid);
    log_debug("Attempting notification: %s", message);
    _notify(message, 10000, "Incoming message", avatar_path);
}

void
notify_invite(const gchar* const from, const gchar* const room, const gchar* const reason)
{
    GString* message = g_string_new("Room invite\nfrom: ");
    g_string_append(message, from);
    g_string_append(message, "\nto: ");
    g_string_append(message, room);
    if (reason) {
        g_string_append_printf(message, "\n\"%s\"", reason);
    }

    auto_gchar gchar* avatar_path = avatar_get_path(from);
    log_debug("Attempting notification: %s", message->str);
    _notify(message->str, 10000, "Incoming message", avatar_path);

    g_string_free(message, TRUE);
}

void
notify_message(const gchar* const name, const gchar* const barejid, int num, const gchar* const text)
{
    int ui_index = num;
    if (ui_index == 10) {
        ui_index = 0;
    }

    GString* message = g_string_new("");
    g_string_append_printf(message, "%s (win %d)", name, ui_index);
    if (text && prefs_get_boolean(PREF_NOTIFY_CHAT_TEXT)) {
        g_string_append_printf(message, "\n%s", text);
    }

    auto_gchar gchar* avatar_path = avatar_get_path(barejid);
    log_debug("Attempting notification: %s", message->str);
    _notify(message->str, 10000, "incoming message", avatar_path);
    g_string_free(message, TRUE);
}

void
notify_room_message(const gchar* const nick, const gchar* const room, int num, const gchar* const text)
{
    int ui_index = num;
    if (ui_index == 10) {
        ui_index = 0;
    }

    GString* message = g_string_new("");
    g_string_append_printf(message, "%s in %s (win %d)", nick, room, ui_index);
    if (text && prefs_get_boolean(PREF_NOTIFY_ROOM_TEXT)) {
        g_string_append_printf(message, "\n%s", text);
    }

    auto_gchar gchar* avatar_path = avatar_get_path(room);
    log_debug("Attempting notification: %s", message->str);
    _notify(message->str, 10000, "incoming message", avatar_path);

    g_string_free(message, TRUE);
}

void
notify_subscription(const gchar* const from)
{
    GString* message = g_string_new("Subscription request: \n");
    g_string_append(message, from);
    auto_gchar gchar* avatar_path = avatar_get_path(from);
    log_debug("Attempting notification: %s", message->str);
    _notify(message->str, 10000, "Incoming message", avatar_path);
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

        GString* text = g_string_new("");

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
            log_debug("Attempting notification: %s", text->str);
            _notify(text->str, 5000, "Incoming message", NULL);
        }

        g_string_free(text, TRUE);

        g_timer_start(remind_timer);
    }
}

void
notify(const gchar* const message, int timeout, const gchar* const category)
{
    log_debug("Attempting notification: %s", message);
    _notify(message, timeout, category, NULL);
}
