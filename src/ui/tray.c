/*
 * tray.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#ifdef HAVE_TRAY

#include <gio/gio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "log.h"
#include "config/files.h"
#include "config/preferences.h"
#include "tools/dbus.h"
#include "ui/tray.h"
#include "ui/window_list.h"

static const gchar sni_introspection_xml[] = "<node>"
                                             "  <interface name=\"org.kde.StatusNotifierItem\">"
                                             "    <property name=\"Category\" type=\"s\" access=\"read\"/>"
                                             "    <property name=\"Id\" type=\"s\" access=\"read\"/>"
                                             "    <property name=\"Title\" type=\"s\" access=\"read\"/>"
                                             "    <property name=\"Status\" type=\"s\" access=\"read\"/>"
                                             "    <property name=\"WindowId\" type=\"i\" access=\"read\"/>"
                                             "    <property name=\"IconThemePath\" type=\"s\" access=\"read\"/>"
                                             "    <property name=\"IconName\" type=\"s\" access=\"read\"/>"
                                             "    <property name=\"IconPixmap\" type=\"a(iiay)\" access=\"read\"/>"
                                             "    <property name=\"OverlayIconName\" type=\"s\" access=\"read\"/>"
                                             "    <property name=\"OverlayIconPixmap\" type=\"a(iiay)\" access=\"read\"/>"
                                             "    <property name=\"AttentionIconName\" type=\"s\" access=\"read\"/>"
                                             "    <property name=\"AttentionIconPixmap\" type=\"a(iiay)\" access=\"read\"/>"
                                             "    <property name=\"AttentionMovieName\" type=\"s\" access=\"read\"/>"
                                             "    <property name=\"ToolTip\" type=\"(sa(iiay)ss)\" access=\"read\"/>"
                                             "    <property name=\"ItemIsMenu\" type=\"b\" access=\"read\"/>"
                                             "    <property name=\"Menu\" type=\"o\" access=\"read\"/>"
                                             "    <method name=\"ContextMenu\">"
                                             "      <arg name=\"x\" type=\"i\" direction=\"in\"/>"
                                             "      <arg name=\"y\" type=\"i\" direction=\"in\"/>"
                                             "    </method>"
                                             "    <method name=\"Activate\">"
                                             "      <arg name=\"x\" type=\"i\" direction=\"in\"/>"
                                             "      <arg name=\"y\" type=\"i\" direction=\"in\"/>"
                                             "    </method>"
                                             "    <method name=\"SecondaryActivate\">"
                                             "      <arg name=\"x\" type=\"i\" direction=\"in\"/>"
                                             "      <arg name=\"y\" type=\"i\" direction=\"in\"/>"
                                             "    </method>"
                                             "    <method name=\"Scroll\">"
                                             "      <arg name=\"delta\" type=\"i\" direction=\"in\"/>"
                                             "      <arg name=\"orientation\" type=\"s\" direction=\"in\"/>"
                                             "    </method>"
                                             "    <signal name=\"NewTitle\"/>"
                                             "    <signal name=\"NewIcon\"/>"
                                             "    <signal name=\"NewAttentionIcon\"/>"
                                             "    <signal name=\"NewOverlayIcon\"/>"
                                             "    <signal name=\"NewToolTip\"/>"
                                             "    <signal name=\"NewStatus\">"
                                             "      <arg name=\"status\" type=\"s\"/>"
                                             "    </signal>"
                                             "    <signal name=\"NewIconThemePath\">"
                                             "      <arg name=\"icon_theme_path\" type=\"s\"/>"
                                             "    </signal>"
                                             "  </interface>"
                                             "</node>";

static GDBusConnection* session_bus = NULL;
static GDBusNodeInfo* introspection_data = NULL;
static guint registration_id = 0;
static guint watcher_id = 0;
static guint own_name_id = 0;
static gchar* service_name = NULL;
static gchar* icon_theme_path = NULL;
static guint timer = 0;
static gint unread_messages = 0;
static gboolean tray_enabled = FALSE;
static gboolean shutting_down = FALSE;

static const gchar*
_get_status_str(void)
{
    if (!tray_enabled) {
        return "Passive";
    }
    if (unread_messages > 0) {
        return "NeedsAttention";
    }
    if (prefs_get_boolean(PREF_TRAY_READ)) {
        return "Active";
    }
    return "Passive";
}

static const gchar*
_get_icon_name(void)
{
    if (unread_messages > 0) {
        return "profanity-new-message";
    }
    return "profanity-msg-bubble";
}

static GVariant*
_sni_get_property(GDBusConnection* connection,
                  const gchar* sender,
                  const gchar* object_path,
                  const gchar* interface_name,
                  const gchar* property_name,
                  GError** error,
                  gpointer user_data)
{
    if (g_strcmp0(property_name, "Category") == 0) {
        return g_variant_new_string("Communications");
    } else if (g_strcmp0(property_name, "Id") == 0) {
        return g_variant_new_string("profanity");
    } else if (g_strcmp0(property_name, "Title") == 0) {
        return g_variant_new_string("Profanity");
    } else if (g_strcmp0(property_name, "Status") == 0) {
        return g_variant_new_string(_get_status_str());
    } else if (g_strcmp0(property_name, "WindowId") == 0) {
        return g_variant_new_int32(0);
    } else if (g_strcmp0(property_name, "IconThemePath") == 0) {
        return g_variant_new_string(icon_theme_path ? icon_theme_path : "");
    } else if (g_strcmp0(property_name, "IconName") == 0) {
        return g_variant_new_string(_get_icon_name());
    } else if (g_strcmp0(property_name, "IconPixmap") == 0) {
        return g_variant_new_array(G_VARIANT_TYPE("(iiay)"), NULL, 0);
    } else if (g_strcmp0(property_name, "OverlayIconName") == 0) {
        return g_variant_new_string("");
    } else if (g_strcmp0(property_name, "OverlayIconPixmap") == 0) {
        return g_variant_new_array(G_VARIANT_TYPE("(iiay)"), NULL, 0);
    } else if (g_strcmp0(property_name, "AttentionIconName") == 0) {
        return g_variant_new_string("profanity-new-message");
    } else if (g_strcmp0(property_name, "AttentionIconPixmap") == 0) {
        return g_variant_new_array(G_VARIANT_TYPE("(iiay)"), NULL, 0);
    } else if (g_strcmp0(property_name, "AttentionMovieName") == 0) {
        return g_variant_new_string("");
    } else if (g_strcmp0(property_name, "ItemIsMenu") == 0) {
        return g_variant_new_boolean(FALSE);
    } else if (g_strcmp0(property_name, "Menu") == 0) {
        return g_variant_new_object_path("/NO_DBUSMENU");
    } else if (g_strcmp0(property_name, "ToolTip") == 0) {
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("(sa(iiay)ss)"));
        g_variant_builder_add(&builder, "s", _get_icon_name());
        g_variant_builder_open(&builder, G_VARIANT_TYPE("a(iiay)"));
        g_variant_builder_close(&builder);
        g_variant_builder_add(&builder, "s", "Profanity");
        if (unread_messages > 0) {
            auto_gchar gchar* msg = g_strdup_printf("Unread messages: %d", unread_messages);
            g_variant_builder_add(&builder, "s", msg);
        } else {
            g_variant_builder_add(&builder, "s", "No unread messages");
        }
        return g_variant_builder_end(&builder);
    }

    g_set_error(error, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_PROPERTY,
                "Unknown property: %s", property_name);
    return NULL;
}

static void
_sni_method_call(GDBusConnection* connection,
                 const gchar* sender,
                 const gchar* object_path,
                 const gchar* interface_name,
                 const gchar* method_name,
                 GVariant* parameters,
                 GDBusMethodInvocation* invocation,
                 gpointer user_data)
{
    if (g_strcmp0(method_name, "ContextMenu") == 0 || g_strcmp0(method_name, "Activate") == 0 || g_strcmp0(method_name, "SecondaryActivate") == 0 || g_strcmp0(method_name, "Scroll") == 0) {
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else {
        g_dbus_method_invocation_return_error(invocation,
                                              G_DBUS_ERROR,
                                              G_DBUS_ERROR_UNKNOWN_METHOD,
                                              "Unknown method: %s",
                                              method_name);
    }
}

static const GDBusInterfaceVTable sni_vtable = {
    _sni_method_call,
    _sni_get_property,
    NULL,
    { 0 }
};

static void
_tray_emit_changed(void)
{
    if (!session_bus || registration_id == 0) {
        return;
    }

    const gchar* status = _get_status_str();

    g_dbus_connection_emit_signal(
        session_bus,
        NULL,
        "/StatusNotifierItem",
        "org.kde.StatusNotifierItem",
        "NewStatus",
        g_variant_new("(s)", status),
        NULL);

    g_dbus_connection_emit_signal(
        session_bus,
        NULL,
        "/StatusNotifierItem",
        "org.kde.StatusNotifierItem",
        "NewIcon",
        NULL,
        NULL);

    g_dbus_connection_emit_signal(
        session_bus,
        NULL,
        "/StatusNotifierItem",
        "org.kde.StatusNotifierItem",
        "NewAttentionIcon",
        NULL,
        NULL);

    g_dbus_connection_emit_signal(
        session_bus,
        NULL,
        "/StatusNotifierItem",
        "org.kde.StatusNotifierItem",
        "NewToolTip",
        NULL,
        NULL);
}

static gboolean
_tray_change_icon(gpointer data)
{
    if (shutting_down || !tray_enabled || !session_bus) {
        return FALSE;
    }

    gint new_unread = wins_get_total_unread();
    if (new_unread != unread_messages) {
        unread_messages = new_unread;
        _tray_emit_changed();
    }

    return TRUE;
}

static void
_on_register_reply(GObject* source_object, GAsyncResult* res, gpointer user_data)
{
    auto_gerror GError* error = NULL;
    GVariant* result = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source_object), res, &error);
    if (!result) {
        log_debug("Could not register with StatusNotifierWatcher: %s", PROF_GERROR_MESSAGE(error));
    } else {
        log_debug("Successfully registered with StatusNotifierWatcher");
        g_variant_unref(result);
    }
}

static void
_register_with_watcher(GDBusConnection* conn)
{
    if (!conn || !tray_enabled || !service_name) {
        return;
    }

    log_debug("Registering with StatusNotifierWatcher: %s", service_name);
    g_dbus_connection_call(
        conn,
        "org.kde.StatusNotifierWatcher",
        "/StatusNotifierWatcher",
        "org.kde.StatusNotifierWatcher",
        "RegisterStatusNotifierItem",
        g_variant_new("(s)", service_name),
        NULL,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        NULL,
        _on_register_reply,
        NULL);
}

static void
_on_name_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data)
{
    log_debug("Acquired bus name %s", name);
    if (tray_enabled) {
        _register_with_watcher(connection);
    }
}

static void
_on_name_lost(GDBusConnection* connection, const gchar* name, gpointer user_data)
{
    log_debug("Lost bus name %s", name);
}

static void
_on_watcher_appeared(GDBusConnection* connection, const gchar* name, const gchar* name_owner, gpointer user_data)
{
    log_debug("StatusNotifierWatcher appeared: %s (%s)", name, name_owner);
    if (tray_enabled) {
        _register_with_watcher(connection);
    }
}

static void
_on_watcher_vanished(GDBusConnection* connection, const gchar* name, gpointer user_data)
{
    log_debug("StatusNotifierWatcher vanished: %s", name);
}

static void
_find_icons(void)
{
    auto_gchar gchar* icon_file = files_get_icon_path("profanity-msg-bubble.png");
    if (icon_file) {
        icon_theme_path = g_path_get_dirname(icon_file);
        return;
    }

#ifdef ICONS_PATH
    icon_theme_path = g_strdup(ICONS_PATH);
#else
    icon_theme_path = g_strdup("");
#endif
}

static void
_tray_shutdown(void)
{
    shutting_down = TRUE;
    tray_disable();

    if (own_name_id > 0) {
        g_bus_unown_name(own_name_id);
        own_name_id = 0;
    }

    if (watcher_id > 0) {
        g_bus_unwatch_name(watcher_id);
        watcher_id = 0;
    }

    if (registration_id > 0 && session_bus) {
        g_dbus_connection_unregister_object(session_bus, registration_id);
        registration_id = 0;
    }

    if (introspection_data) {
        g_dbus_node_info_unref(introspection_data);
        introspection_data = NULL;
    }

    if (service_name) {
        g_free(service_name);
        service_name = NULL;
    }

    if (icon_theme_path) {
        g_free(icon_theme_path);
        icon_theme_path = NULL;
    }
}

void
tray_init(void)
{
    prof_add_shutdown_routine(_tray_shutdown);
    _find_icons();

    session_bus = dbus_get_session_bus();
    if (!session_bus) {
        log_debug("D-Bus session bus not available, tray disabled.");
        return;
    }

    auto_gerror GError* error = NULL;
    introspection_data = g_dbus_node_info_new_for_xml(sni_introspection_xml, &error);
    if (!introspection_data) {
        log_error("Failed to parse SNI introspection XML: %s", PROF_GERROR_MESSAGE(error));
        return;
    }

    registration_id = g_dbus_connection_register_object(
        session_bus,
        "/StatusNotifierItem",
        introspection_data->interfaces[0],
        &sni_vtable,
        NULL,
        NULL,
        &error);

    if (registration_id == 0) {
        log_error("Failed to register SNI object: %s", PROF_GERROR_MESSAGE(error));
        return;
    }

    service_name = g_strdup_printf("org.kde.StatusNotifierItem-%d-1", (int)getpid());

    own_name_id = g_bus_own_name_on_connection(
        session_bus,
        service_name,
        G_BUS_NAME_OWNER_FLAGS_NONE,
        _on_name_acquired,
        _on_name_lost,
        NULL,
        NULL);

    watcher_id = g_bus_watch_name_on_connection(
        session_bus,
        "org.kde.StatusNotifierWatcher",
        G_BUS_NAME_WATCHER_FLAGS_NONE,
        _on_watcher_appeared,
        _on_watcher_vanished,
        NULL,
        NULL);

    if (prefs_get_boolean(PREF_TRAY)) {
        tray_enable();
    }
}

void
tray_update(void)
{
    if (tray_enabled && session_bus) {
        _tray_change_icon(NULL);
    }
}

gboolean
tray_is_ready(void)
{
    return dbus_is_available();
}

void
tray_set_timer(int interval)
{
    if (!dbus_is_available()) {
        return;
    }
    if (timer) {
        g_source_remove(timer);
        timer = 0;
    }
    if (interval < 1) {
        interval = 1;
    }
    _tray_change_icon(NULL);
    if (tray_enabled) {
        timer = g_timeout_add_seconds(interval, _tray_change_icon, NULL);
    }
}

void
tray_enable(void)
{
    if (!dbus_is_available()) {
        return;
    }
    tray_enabled = TRUE;
    shutting_down = FALSE;

    _register_with_watcher(session_bus);

    _tray_change_icon(NULL);

    if (timer) {
        g_source_remove(timer);
        timer = 0;
    }
    int interval = prefs_get_tray_timer();
    if (interval < 1) {
        interval = 1;
    }
    timer = g_timeout_add_seconds(interval, _tray_change_icon, NULL);
}

void
tray_disable(void)
{
    tray_enabled = FALSE;
    if (timer) {
        g_source_remove(timer);
        timer = 0;
    }
    if (session_bus && registration_id > 0) {
        _tray_emit_changed();
    }
}

#endif
