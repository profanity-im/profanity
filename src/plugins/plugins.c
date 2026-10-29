/*
 * plugins.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#ifdef HAVE_PLUGINS

#include <string.h>
#include <stdlib.h>
#include <gio/gio.h>
#include <glib.h>

#include "log.h"
#include "config/files.h"
#include "config/preferences.h"
#include "event/client_events.h"
#include "plugins/callbacks.h"
#include "plugins/autocompleters.h"
#include "plugins/api.h"
#include "plugins/plugins.h"
#include "plugins/themes.h"
#include "plugins/settings.h"
#include "plugins/disco.h"
#include "ui/ui.h"
#include "xmpp/xmpp.h"

static GHashTable* plugins = NULL;
static GList* plugins_ordered = NULL;

static gint
_plugin_compare(gconstpointer a, gconstpointer b)
{
    const ProfPlugin* p1 = a;
    const ProfPlugin* p2 = b;
    if (p1->priority != p2->priority) {
        return p1->priority - p2->priority;
    }
    return g_strcmp0(p1->name, p2->name);
}

const char*
plugins_get_lua_version(void)
{
#ifdef HAVE_PLUGINS
#ifdef LUA_RELEASE
    return LUA_RELEASE;
#else
    return "5.4";
#endif
#else
    return "none";
#endif
}

#ifdef HAVE_PLUGINS
static void
_lua_call_void_0arg(ProfPlugin* plugin, const char* hook)
{
    lua_State* L = plugin->L;
    lua_getglobal(L, hook);
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log_error("Plugin %s error in %s: %s", plugin->name, hook, err);
        cons_show("Plugin %s error in %s: %s", plugin->name, hook, err);
        lua_pop(L, 1);
    }
}

static void
_lua_call_void_1str(ProfPlugin* plugin, const char* hook, const char* s1)
{
    lua_State* L = plugin->L;
    lua_getglobal(L, hook);
    lua_pushstring(L, s1 ? s1 : "");
    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log_error("Plugin %s error in %s: %s", plugin->name, hook, err);
        cons_show("Plugin %s error in %s: %s", plugin->name, hook, err);
        lua_pop(L, 1);
    }
}

static void
_lua_call_void_2str(ProfPlugin* plugin, const char* hook, const char* s1, const char* s2)
{
    lua_State* L = plugin->L;
    lua_getglobal(L, hook);
    lua_pushstring(L, s1 ? s1 : "");
    lua_pushstring(L, s2 ? s2 : "");
    if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log_error("Plugin %s error in %s: %s", plugin->name, hook, err);
        cons_show("Plugin %s error in %s: %s", plugin->name, hook, err);
        lua_pop(L, 1);
    }
}

static void
_lua_call_void_3str(ProfPlugin* plugin, const char* hook, const char* s1, const char* s2, const char* s3)
{
    lua_State* L = plugin->L;
    lua_getglobal(L, hook);
    lua_pushstring(L, s1 ? s1 : "");
    if (s2) {
        lua_pushstring(L, s2);
    } else {
        lua_pushnil(L);
    }
    lua_pushstring(L, s3 ? s3 : "");
    if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log_error("Plugin %s error in %s: %s", plugin->name, hook, err);
        cons_show("Plugin %s error in %s: %s", plugin->name, hook, err);
        lua_pop(L, 1);
    }
}

static void
_lua_call_void_4str(ProfPlugin* plugin, const char* hook, const char* s1, const char* s2, const char* s3, const char* s4)
{
    lua_State* L = plugin->L;
    lua_getglobal(L, hook);
    lua_pushstring(L, s1 ? s1 : "");
    if (s2) {
        lua_pushstring(L, s2);
    } else {
        lua_pushnil(L);
    }
    lua_pushstring(L, s3 ? s3 : "");
    if (s4) {
        lua_pushstring(L, s4);
    } else {
        lua_pushnil(L);
    }
    if (lua_pcall(L, 4, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log_error("Plugin %s error in %s: %s", plugin->name, hook, err);
        cons_show("Plugin %s error in %s: %s", plugin->name, hook, err);
        lua_pop(L, 1);
    }
}

static void
_lua_call_void_presence(ProfPlugin* plugin, const char* barejid, const char* resource,
                        const char* presence, const char* status, int priority)
{
    lua_State* L = plugin->L;
    lua_getglobal(L, "prof_on_contact_presence");
    lua_pushstring(L, barejid ? barejid : "");
    if (resource) {
        lua_pushstring(L, resource);
    } else {
        lua_pushnil(L);
    }
    if (presence) {
        lua_pushstring(L, presence);
    } else {
        lua_pushnil(L);
    }
    if (status) {
        lua_pushstring(L, status);
    } else {
        lua_pushnil(L);
    }
    lua_pushinteger(L, priority);
    if (lua_pcall(L, 5, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log_error("Plugin %s error in prof_on_contact_presence: %s", plugin->name, err);
        cons_show("Plugin %s error in prof_on_contact_presence: %s", plugin->name, err);
        lua_pop(L, 1);
    }
}

static char*
_lua_call_filter_2str(ProfPlugin* plugin, const char* hook, const char* s1, const char* s2)
{
    lua_State* L = plugin->L;
    lua_getglobal(L, hook);
    lua_pushstring(L, s1 ? s1 : "");
    lua_pushstring(L, s2 ? s2 : "");
    if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log_error("Plugin %s error in %s: %s", plugin->name, hook, err);
        cons_show("Plugin %s error in %s: %s", plugin->name, hook, err);
        lua_pop(L, 1);
        return NULL;
    }
    char* result = NULL;
    if (lua_isstring(L, -1)) {
        result = g_strdup(lua_tostring(L, -1));
    }
    lua_pop(L, 1);
    return result;
}

static char*
_lua_call_filter_3str(ProfPlugin* plugin, const char* hook, const char* s1, const char* s2, const char* s3)
{
    lua_State* L = plugin->L;
    lua_getglobal(L, hook);
    lua_pushstring(L, s1 ? s1 : "");
    if (s2) {
        lua_pushstring(L, s2);
    } else {
        lua_pushnil(L);
    }
    lua_pushstring(L, s3 ? s3 : "");
    if (lua_pcall(L, 3, 1, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log_error("Plugin %s error in %s: %s", plugin->name, hook, err);
        cons_show("Plugin %s error in %s: %s", plugin->name, hook, err);
        lua_pop(L, 1);
        return NULL;
    }
    char* result = NULL;
    if (lua_isstring(L, -1)) {
        result = g_strdup(lua_tostring(L, -1));
    }
    lua_pop(L, 1);
    return result;
}

static gboolean
_lua_call_bool_1str(ProfPlugin* plugin, const char* hook, const char* s1)
{
    lua_State* L = plugin->L;
    lua_getglobal(L, hook);
    lua_pushstring(L, s1 ? s1 : "");
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log_error("Plugin %s error in %s: %s", plugin->name, hook, err);
        cons_show("Plugin %s error in %s: %s", plugin->name, hook, err);
        lua_pop(L, 1);
        return TRUE;
    }
    gboolean result = TRUE;
    if (lua_isboolean(L, -1)) {
        result = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);
    return result;
}

static void
_plugin_destroy(ProfPlugin* plugin)
{
    if (!plugin) {
        return;
    }
    if (plugin->L) {
        lua_close(plugin->L);
        plugin->L = NULL;
    }
    g_free(plugin->name);
    g_free(plugin);
}

static ProfPlugin*
_lua_plugin_create(const char* const filename)
{
    auto_gchar gchar* plugins_dir = files_get_data_path(DIR_PLUGINS);
    auto_gchar gchar* full_path = g_build_filename(plugins_dir, filename, NULL);

    if (!g_file_test(full_path, G_FILE_TEST_IS_REGULAR)) {
        log_error("Plugin file not found: %s", full_path);
        return NULL;
    }

    lua_State* L = luaL_newstate();
    if (!L) {
        log_error("Failed to allocate Lua state for plugin %s", filename);
        return NULL;
    }
    luaL_openlibs(L);
    api_init_lua(filename, L);

    lua_getglobal(L, "package");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "path");
        const char* current_path = lua_tostring(L, -1);
        auto_gchar gchar* new_path = g_strdup_printf("%s;%s/?.lua;%s/?/init.lua", current_path, plugins_dir, plugins_dir);
        lua_pushstring(L, new_path);
        lua_setfield(L, -3, "path");
        lua_pop(L, 1);

        lua_getfield(L, -1, "cpath");
        const char* current_cpath = lua_tostring(L, -1);
        auto_gchar gchar* new_cpath = g_strdup_printf("%s;%s/?.so", current_cpath, plugins_dir);
        lua_pushstring(L, new_cpath);
        lua_setfield(L, -3, "cpath");
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    if (luaL_dofile(L, full_path) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log_error("Error loading Lua plugin %s: %s", filename, err ? err : "unknown error");
        cons_show("Error loading Lua plugin %s: %s", filename, err ? err : "unknown error");
        lua_close(L);
        return NULL;
    }

    ProfPlugin* plugin = g_new0(ProfPlugin, 1);
    plugin->name = g_strdup(filename);
    plugin->L = L;
    plugin->hook_mask = 0;

    static const char* const hooks[] = {
        "prof_init", "prof_on_start", "prof_on_shutdown", "prof_on_unload", "prof_on_connect", "prof_on_disconnect",
        "prof_pre_chat_message_display", "prof_post_chat_message_display", "prof_pre_chat_message_send", "prof_post_chat_message_send",
        "prof_pre_room_message_display", "prof_post_room_message_display", "prof_pre_room_message_send", "prof_post_room_message_send",
        "prof_on_room_history_message",
        "prof_pre_priv_message_display", "prof_post_priv_message_display", "prof_pre_priv_message_send", "prof_post_priv_message_send",
        "prof_on_message_stanza_send", "prof_on_message_stanza_receive",
        "prof_on_presence_stanza_send", "prof_on_presence_stanza_receive",
        "prof_on_iq_stanza_send", "prof_on_iq_stanza_receive",
        "prof_on_contact_offline", "prof_on_contact_presence",
        "prof_on_chat_win_focus", "prof_on_room_win_focus",
        "prof_on_room_occupant_join", "prof_on_room_occupant_leave",
        "prof_on_chat_state", "prof_on_message_correction", "prof_on_win_open", "prof_on_win_close"
    };

    G_STATIC_ASSERT(G_N_ELEMENTS(hooks) <= 64);
    for (size_t i = 0; i < G_N_ELEMENTS(hooks); i++) {
        lua_getglobal(L, hooks[i]);
        if (lua_isfunction(L, -1)) {
            plugin->hook_mask |= (1ULL << i);
        }
        lua_pop(L, 1);
    }

    lua_getglobal(L, "prof_priority");
    if (lua_isnumber(L, -1)) {
        plugin->priority = lua_tointeger(L, -1);
    } else {
        plugin->priority = PROF_PLUGIN_PRIORITY_NORMAL;
    }
    lua_pop(L, 1);

    return plugin;
}

static void
_plugin_call_init(ProfPlugin* plugin, const char* version, const char* status,
                  const char* account_name, const char* fulljid)
{
    lua_State* L = plugin->L;
    lua_getglobal(L, "prof_init");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    lua_pushstring(L, version ? version : "");
    lua_pushstring(L, status ? status : "");
    lua_pushstring(L, account_name ? account_name : "");
    lua_pushstring(L, fulljid ? fulljid : "");
    if (lua_pcall(L, 4, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        log_error("Plugin %s error in prof_init: %s", plugin->name, err);
        cons_show("Plugin %s error in prof_init: %s", plugin->name, err);
        lua_pop(L, 1);
    }
}
#endif

static void
_plugins_shutdown(void)
{
    GList* curr = plugins_ordered;

    while (curr) {
        ProfPlugin* plugin = curr->data;
#ifdef HAVE_PLUGINS
        if (plugin->L) {
            if (plugin->hook_mask & HOOK_ON_SHUTDOWN)
                _lua_call_void_0arg(plugin, "prof_on_shutdown");
        }
        _plugin_destroy(plugin);
#endif
        curr = g_list_next(curr);
    }

    plugin_settings_close();
    plugin_themes_close();
    autocompleters_destroy();

    callbacks_close();
    disco_close();
    g_clear_pointer(&plugins, g_hash_table_destroy);
    g_clear_pointer(&plugins_ordered, g_list_free);
}

void
plugins_init(void)
{
    plugins = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    callbacks_init();
    autocompleters_init();
    plugin_themes_init();
    plugin_settings_init();

    // load plugins
    auto_gcharv gchar** plugins_pref = prefs_get_plugins();
    if (!plugins_pref) {
        return;
    }
    for (guint i = 0; i < g_strv_length(plugins_pref); i++) {
        gboolean loaded = FALSE;
        gchar* filename = plugins_pref[i];
#ifdef HAVE_PLUGINS
        if (g_str_has_suffix(filename, ".lua")) {
            ProfPlugin* plugin = _lua_plugin_create(filename);
            if (plugin) {
                g_hash_table_insert(plugins, g_strdup(filename), plugin);
                plugins_ordered = g_list_insert_sorted(plugins_ordered, plugin, _plugin_compare);
                loaded = TRUE;
            }
        }
#endif
        if (loaded) {
            log_info("Loaded plugin: %s", filename);
        } else {
            log_info("Failed to load plugin: %s", filename);
        }
    }

    // initialise plugins
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_INIT) {
            _plugin_call_init(plugin, PACKAGE_VERSION, PACKAGE_STATUS, NULL, NULL);
        }
        curr = g_list_next(curr);
    }
#endif
}

void
plugins_free_install_result(PluginsInstallResult* result)
{
    if (!result) {
        return;
    }
    g_slist_free_full(result->installed, g_free);
    g_slist_free_full(result->failed, g_free);
    g_free(result);
}

static void
_plugins_install_dir_recursive(const char* const path, PluginsInstallResult* result)
{
    GDir* directory = g_dir_open(path, 0, NULL);
    if (!directory) {
        log_error("Could not open directory %s", path);
        return;
    }

    GPathBuf pbuf;
    g_path_buf_init_from_path(&pbuf, path);

    const gchar* entry = g_dir_read_name(directory);
    while (entry) {
        g_path_buf_push(&pbuf, entry);
        const gchar* full = g_path_buf_to_path(&pbuf);
        if (g_file_test(full, G_FILE_TEST_IS_DIR)) {
            _plugins_install_dir_recursive(full, result);
        } else if (g_file_test(full, G_FILE_TEST_IS_REGULAR)) {
            if (g_str_has_suffix(entry, ".lua")) {
                GString* error_message = g_string_new(NULL);
                if (plugins_install(entry, full, error_message)) {
                    result->installed = g_slist_append(result->installed, g_strdup(full));
                } else {
                    result->failed = g_slist_append(result->failed, g_strdup(full));
                }
                g_string_free(error_message, TRUE);
            }
        }
        g_path_buf_pop(&pbuf);
        entry = g_dir_read_name(directory);
    }
    g_dir_close(directory);
}

PluginsInstallResult*
plugins_install_all(const char* const path)
{
    PluginsInstallResult* result = g_new0(PluginsInstallResult, 1);
    result->installed = NULL;
    result->failed = NULL;

    _plugins_install_dir_recursive(path, result);

    return result;
}

gboolean
plugins_uninstall(const char* const plugin_name)
{
    plugins_unload(plugin_name);
    prefs_remove_plugin(plugin_name);
    auto_gchar gchar* plugins_dir = files_get_data_path(DIR_PLUGINS);
    auto_gchar gchar* full_path = g_build_filename(plugins_dir, plugin_name, NULL);
    GFile* file = g_file_new_for_path(full_path);
    gboolean result = g_file_delete(file, NULL, NULL);
    g_object_unref(file);

    return result;
}

gboolean
plugins_install(const char* const plugin_name, const char* const filename, GString* error_message)
{
    GFile* source = g_file_new_for_path(filename);
    auto_gchar gchar* plugins_dir = files_get_data_path(DIR_PLUGINS);
    auto_gchar gchar* full_path = g_build_filename(plugins_dir, plugin_name, NULL);
    GFile* destination = g_file_new_for_path(full_path);

    GError* err = NULL;
    gboolean res = g_file_copy(source, destination, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, &err);
    if (!res) {
        log_error("Could not install plugin %s: %s", filename, err->message);
        g_string_assign(error_message, err->message);
        g_error_free(err);
        g_object_unref(source);
        g_object_unref(destination);
        return FALSE;
    }
    g_object_unref(source);
    g_object_unref(destination);

    return TRUE;
}

gboolean
plugins_load(const char* const name, GString* error_message)
{
    ProfPlugin* plugin = g_hash_table_lookup(plugins, name);
    if (plugin) {
        log_info("Failed to load plugin: %s, plugin already loaded", name);
        return FALSE;
    }

#ifdef HAVE_PLUGINS
    if (g_str_has_suffix(name, ".lua")) {
        plugin = _lua_plugin_create(name);
        if (!plugin) {
            g_string_assign(error_message, "Failed to load Lua plugin, check error log.");
        }
    } else {
        g_string_assign(error_message, "Plugin must have .lua extension.");
    }
#else
    g_string_assign(error_message, "Lua plugin support is disabled.");
#endif

    if (plugin) {

        g_hash_table_insert(plugins, g_strdup(name), plugin);
        plugins_ordered = g_list_insert_sorted(plugins_ordered, plugin, _plugin_compare);
#ifdef HAVE_PLUGINS
        if (plugin->hook_mask & HOOK_INIT) {
            if (connection_get_status() == JABBER_CONNECTED) {
                _plugin_call_init(plugin, PACKAGE_VERSION, PACKAGE_STATUS, session_get_account_name(), connection_get_fulljid());
            } else {
                _plugin_call_init(plugin, PACKAGE_VERSION, PACKAGE_STATUS, NULL, NULL);
            }
        }
#endif
        log_info("Loaded plugin: %s", name);
        return TRUE;
    } else {
        log_info("Failed to load plugin: %s", name);
        return FALSE;
    }
}

GSList*
plugins_load_all(void)
{
    GSList* result = NULL;
    GSList* unloaded = plugins_unloaded_list();
    GSList* curr = unloaded;
    while (curr) {
        GString* error_message = g_string_new(NULL);
        if (plugins_load(curr->data, error_message)) {
            result = g_slist_append(result, g_strdup(curr->data));
        }
        g_string_free(error_message, TRUE);
        curr = g_slist_next(curr);
    }
    g_slist_free_full(unloaded, g_free);

    return result;
}

gboolean
plugins_unload_all(void)
{
    gboolean result = TRUE;
    GList* plugin_names = g_hash_table_get_keys(plugins);
    GList* plugin_names_dup = NULL;
    GList* curr = plugin_names;
    while (curr) {
        plugin_names_dup = g_list_append(plugin_names_dup, g_strdup(curr->data));
        curr = g_list_next(curr);
    }
    g_list_free(plugin_names);

    curr = plugin_names_dup;
    while (curr) {
        if (!plugins_unload(curr->data)) {
            result = FALSE;
        }
        curr = g_list_next(curr);
    }

    g_list_free_full(plugin_names_dup, g_free);

    return result;
}

gboolean
plugins_unload(const char* const name)
{
    ProfPlugin* plugin = g_hash_table_lookup(plugins, name);
    if (plugin) {
#ifdef HAVE_PLUGINS
        if (plugin->L) {
            if (plugin->hook_mask & HOOK_ON_UNLOAD)
                _lua_call_void_0arg(plugin, "prof_on_unload");
        }
#endif
        callbacks_remove(name);

        plugins_ordered = g_list_remove(plugins_ordered, plugin);
        g_hash_table_remove(plugins, name);
#ifdef HAVE_PLUGINS
        _plugin_destroy(plugin);
#endif

        caps_reset_ver();
        if (connection_get_status() == JABBER_CONNECTED) {
            resource_presence_t last_presence = accounts_get_last_presence(session_get_account_name());
            cl_ev_presence_send(last_presence, 0);
        }
        return TRUE;
    }
    return FALSE;
}

static void
_plugins_unloaded_list_dir(const gchar* const dir, GSList** result)
{
    GDir* plugins_dir = g_dir_open(dir, 0, NULL);
    if (!plugins_dir) {
        return;
    }

    const gchar* plugin = g_dir_read_name(plugins_dir);
    while (plugin) {
        ProfPlugin* found = g_hash_table_lookup(plugins, plugin);
        if (g_str_has_suffix(plugin, ".lua") && !found) {
            *result = g_slist_append(*result, g_strdup(plugin));
        }
        plugin = g_dir_read_name(plugins_dir);
    }
    g_dir_close(plugins_dir);
}

GSList*
plugins_installed_list(void)
{
    GSList* result = NULL;
    auto_gchar gchar* local_dir = files_get_data_path(DIR_PLUGINS);
    GDir* plugins_dir = g_dir_open(local_dir, 0, NULL);
    if (!plugins_dir) {
        return NULL;
    }

    const gchar* filename = g_dir_read_name(plugins_dir);
    while (filename) {
        if (g_str_has_suffix(filename, ".lua")) {
            result = g_slist_append(result, g_strdup(filename));
        }
        filename = g_dir_read_name(plugins_dir);
    }
    g_dir_close(plugins_dir);

    return result;
}

GSList*
plugins_unloaded_list(void)
{
    GSList* result = NULL;
    auto_gchar gchar* local_dir = files_get_data_path(DIR_PLUGINS);
    _plugins_unloaded_list_dir(local_dir, &result);

    return result;
}

GList*
plugins_loaded_list(void)
{
    return g_hash_table_get_keys(plugins);
}

gboolean
plugins_is_loaded(const char* const name)
{
    if (!name || !plugins) {
        return FALSE;
    }

    return g_hash_table_contains(plugins, name);
}

char*
plugins_autocomplete(const char* const input, gboolean previous)
{
    return autocompleters_complete(input, previous);
}

void
plugins_reset_autocomplete(void)
{
    autocompleters_reset();
}

void
plugins_win_process_line(char* win, const char* const line)
{
    PluginWindowCallback* window = callbacks_get_window_handler(win);
    if (window) {
#ifdef HAVE_PLUGINS
        if (window->L && window->lua_ref > 0) {
            lua_rawgeti(window->L, LUA_REGISTRYINDEX, window->lua_ref);
            lua_pushstring(window->L, win ? win : "");
            lua_pushstring(window->L, line ? line : "");
            if (lua_pcall(window->L, 2, 0, 0) != LUA_OK) {
                const char* err = lua_tostring(window->L, -1);
                log_error("Lua error in window %s callback: %s", win, err);
                cons_show("Lua error in window %s callback: %s", win, err);
                lua_pop(window->L, 1);
            }
        }
#endif
    }
}

void
plugins_close_win(const char* const plugin_name, const char* const tag)
{
    callbacks_remove_win(plugin_name, tag);
}

void
plugins_on_start(void)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_START)
            _lua_call_void_0arg(plugin, "prof_on_start");
        curr = g_list_next(curr);
    }
#endif
}

void
plugins_on_shutdown(void)
{
    _plugins_shutdown();
}

void
plugins_close(void)
{
    _plugins_shutdown();
}

void
plugins_on_connect(const char* const account_name, const char* const fulljid)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_CONNECT)
            _lua_call_void_2str(plugin, "prof_on_connect", account_name, fulljid);
        curr = g_list_next(curr);
    }
#endif
}

void
plugins_on_disconnect(const char* const account_name, const char* const fulljid)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_DISCONNECT)
            _lua_call_void_2str(plugin, "prof_on_disconnect", account_name, fulljid);
        curr = g_list_next(curr);
    }
#endif
}

char*
plugins_pre_chat_message_display(const char* const barejid, const char* const resource, const char* message)
{
#ifdef HAVE_PLUGINS
    char* result = NULL;
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;

        if (plugin->hook_mask & HOOK_PRE_CHAT_MESSAGE_DISPLAY) {
            char* new_msg = _lua_call_filter_3str(plugin, "prof_pre_chat_message_display", barejid, resource, result ? result : message);
            if (new_msg) {
                g_free(result);
                result = new_msg;
            }
        }
        curr = g_list_next(curr);
    }
    return result;
#else
    return NULL;
#endif
}

void
plugins_post_chat_message_display(const char* const barejid, const char* const resource, const char* message)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_POST_CHAT_MESSAGE_DISPLAY)
            _lua_call_void_3str(plugin, "prof_post_chat_message_display", barejid, resource, message);
        curr = g_list_next(curr);
    }
#endif
}

char*
plugins_pre_chat_message_send(const char* const barejid, const char* message)
{
#ifdef HAVE_PLUGINS
    char* result = NULL;
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;

        if (plugin->hook_mask & HOOK_PRE_CHAT_MESSAGE_SEND) {
            char* new_msg = _lua_call_filter_2str(plugin, "prof_pre_chat_message_send", barejid, result ? result : message);
            if (new_msg) {
                g_free(result);
                result = new_msg;
            }
        }
        curr = g_list_next(curr);
    }
    return result;
#else
    return NULL;
#endif
}

void
plugins_post_chat_message_send(const char* const barejid, const char* message)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_POST_CHAT_MESSAGE_SEND)
            _lua_call_void_2str(plugin, "prof_post_chat_message_send", barejid, message);
        curr = g_list_next(curr);
    }
#endif
}

char*
plugins_pre_room_message_display(const char* const barejid, const char* const nick, const char* message)
{
#ifdef HAVE_PLUGINS
    char* result = NULL;
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;

        if (plugin->hook_mask & HOOK_PRE_ROOM_MESSAGE_DISPLAY) {
            char* new_msg = _lua_call_filter_3str(plugin, "prof_pre_room_message_display", barejid, nick, result ? result : message);
            if (new_msg) {
                g_free(result);
                result = new_msg;
            }
        }
        curr = g_list_next(curr);
    }
    return result;
#else
    return NULL;
#endif
}

void
plugins_post_room_message_display(const char* const barejid, const char* const nick, const char* message)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_POST_ROOM_MESSAGE_DISPLAY)
            _lua_call_void_3str(plugin, "prof_post_room_message_display", barejid, nick, message);
        curr = g_list_next(curr);
    }
#endif
}

char*
plugins_pre_room_message_send(const char* const barejid, const char* message)
{
#ifdef HAVE_PLUGINS
    char* result = NULL;
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;

        if (plugin->hook_mask & HOOK_PRE_ROOM_MESSAGE_SEND) {
            char* new_msg = _lua_call_filter_2str(plugin, "prof_pre_room_message_send", barejid, result ? result : message);
            if (new_msg) {
                g_free(result);
                result = new_msg;
            }
        }
        curr = g_list_next(curr);
    }
    return result;
#else
    return NULL;
#endif
}

void
plugins_post_room_message_send(const char* const barejid, const char* message)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_POST_ROOM_MESSAGE_SEND)
            _lua_call_void_2str(plugin, "prof_post_room_message_send", barejid, message);
        curr = g_list_next(curr);
    }
#endif
}

void
plugins_on_room_history_message(const char* const barejid, const char* const nick, const char* const message,
                                GDateTime* timestamp)
{
#ifdef HAVE_PLUGINS
    auto_gchar gchar* time_str = NULL;
    if (timestamp) {
        time_str = g_date_time_format_iso8601(timestamp);
    }
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_ROOM_HISTORY_MESSAGE)
            _lua_call_void_4str(plugin, "prof_on_room_history_message", barejid, nick, message, time_str);
        curr = g_list_next(curr);
    }
#endif
}

char*
plugins_pre_priv_message_display(const char* const fulljid, const char* message)
{
#ifdef HAVE_PLUGINS
    char* result = NULL;
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;

        if (plugin->hook_mask & HOOK_PRE_PRIV_MESSAGE_DISPLAY) {
            char* new_msg = _lua_call_filter_2str(plugin, "prof_pre_priv_message_display", fulljid, result ? result : message);
            if (new_msg) {
                g_free(result);
                result = new_msg;
            }
        }
        curr = g_list_next(curr);
    }
    return result;
#else
    return NULL;
#endif
}

void
plugins_post_priv_message_display(const char* const fulljid, const char* message)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_POST_PRIV_MESSAGE_DISPLAY)
            _lua_call_void_2str(plugin, "prof_post_priv_message_display", fulljid, message);
        curr = g_list_next(curr);
    }
#endif
}

char*
plugins_pre_priv_message_send(const char* const fulljid, const char* const message)
{
#ifdef HAVE_PLUGINS
    char* result = NULL;
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;

        if (plugin->hook_mask & HOOK_PRE_PRIV_MESSAGE_SEND) {
            char* new_msg = _lua_call_filter_2str(plugin, "prof_pre_priv_message_send", fulljid, result ? result : message);
            if (new_msg) {
                g_free(result);
                result = new_msg;
            }
        }
        curr = g_list_next(curr);
    }
    return result;
#else
    return NULL;
#endif
}

void
plugins_post_priv_message_send(const char* const fulljid, const char* const message)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_POST_PRIV_MESSAGE_SEND)
            _lua_call_void_2str(plugin, "prof_post_priv_message_send", fulljid, message);
        curr = g_list_next(curr);
    }
#endif
}

char*
plugins_on_message_stanza_send(const char* const text)
{
#ifdef HAVE_PLUGINS
    char* result = NULL;
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;

        if (plugin->hook_mask & HOOK_ON_MESSAGE_STANZA_SEND) {
            char* new_stanza = _lua_call_filter_2str(plugin, "prof_on_message_stanza_send", result ? result : text, "");
            if (new_stanza) {
                g_free(result);
                result = new_stanza;
            }
        }
        curr = g_list_next(curr);
    }
    return result;
#else
    return NULL;
#endif
}

gboolean
plugins_on_message_stanza_receive(const char* const text)
{
#ifdef HAVE_PLUGINS
    gboolean result = TRUE;
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_MESSAGE_STANZA_RECEIVE) {
            if (!_lua_call_bool_1str(plugin, "prof_on_message_stanza_receive", text)) {
                result = FALSE;
            }
        }
        curr = g_list_next(curr);
    }
    return result;
#else
    return TRUE;
#endif
}

char*
plugins_on_presence_stanza_send(const char* const text)
{
#ifdef HAVE_PLUGINS
    char* result = NULL;
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;

        if (plugin->hook_mask & HOOK_ON_PRESENCE_STANZA_SEND) {
            char* new_stanza = _lua_call_filter_2str(plugin, "prof_on_presence_stanza_send", result ? result : text, "");
            if (new_stanza) {
                g_free(result);
                result = new_stanza;
            }
        }
        curr = g_list_next(curr);
    }
    return result;
#else
    return NULL;
#endif
}

gboolean
plugins_on_presence_stanza_receive(const char* const text)
{
#ifdef HAVE_PLUGINS
    gboolean result = TRUE;
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_PRESENCE_STANZA_RECEIVE) {
            if (!_lua_call_bool_1str(plugin, "prof_on_presence_stanza_receive", text)) {
                result = FALSE;
            }
        }
        curr = g_list_next(curr);
    }
    return result;
#else
    return TRUE;
#endif
}

char*
plugins_on_iq_stanza_send(const char* const text)
{
#ifdef HAVE_PLUGINS
    char* result = NULL;
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;

        if (plugin->hook_mask & HOOK_ON_IQ_STANZA_SEND) {
            char* new_stanza = _lua_call_filter_2str(plugin, "prof_on_iq_stanza_send", result ? result : text, "");
            if (new_stanza) {
                g_free(result);
                result = new_stanza;
            }
        }
        curr = g_list_next(curr);
    }
    return result;
#else
    return NULL;
#endif
}

gboolean
plugins_on_iq_stanza_receive(const char* const text)
{
#ifdef HAVE_PLUGINS
    gboolean result = TRUE;
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_IQ_STANZA_RECEIVE) {
            if (!_lua_call_bool_1str(plugin, "prof_on_iq_stanza_receive", text)) {
                result = FALSE;
            }
        }
        curr = g_list_next(curr);
    }
    return result;
#else
    return TRUE;
#endif
}

void
plugins_on_contact_offline(const char* const barejid, const char* const resource, const char* const status)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_CONTACT_OFFLINE)
            _lua_call_void_3str(plugin, "prof_on_contact_offline", barejid, resource, status);
        curr = g_list_next(curr);
    }
#endif
}

void
plugins_on_contact_presence(const char* const barejid, const char* const resource, const char* const presence,
                            const char* const status, const int priority)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_CONTACT_PRESENCE)
            _lua_call_void_presence(plugin, barejid, resource, presence, status, priority);
        curr = g_list_next(curr);
    }
#endif
}

void
plugins_on_chat_win_focus(const char* const barejid)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_CHAT_WIN_FOCUS)
            _lua_call_void_1str(plugin, "prof_on_chat_win_focus", barejid);
        curr = g_list_next(curr);
    }
#endif
}

void
plugins_on_room_win_focus(const char* const barejid)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_ROOM_WIN_FOCUS)
            _lua_call_void_1str(plugin, "prof_on_room_win_focus", barejid);
        curr = g_list_next(curr);
    }
#endif
}

void
plugins_on_room_occupant_join(const char* const roomjid, const char* const nick)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_ROOM_OCCUPANT_JOIN)
            _lua_call_void_2str(plugin, "prof_on_room_occupant_join", roomjid, nick);
        curr = g_list_next(curr);
    }
#endif
}

void
plugins_on_room_occupant_leave(const char* const roomjid, const char* const nick, const char* const reason)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_ROOM_OCCUPANT_LEAVE)
            _lua_call_void_3str(plugin, "prof_on_room_occupant_leave", roomjid, nick, reason);
        curr = g_list_next(curr);
    }
#endif
}

void
plugins_on_chat_state(const char* const barejid, const char* const state)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_CHAT_STATE)
            _lua_call_void_2str(plugin, "prof_on_chat_state", barejid, state);
        curr = g_list_next(curr);
    }
#endif
}

void
plugins_on_message_correction(const char* const barejid, const char* const id, const char* const new_message)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_MESSAGE_CORRECTION)
            _lua_call_void_3str(plugin, "prof_on_message_correction", barejid, id, new_message);
        curr = g_list_next(curr);
    }
#endif
}

void
plugins_on_win_open(const char* const title, const char* const type)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_WIN_OPEN)
            _lua_call_void_2str(plugin, "prof_on_win_open", title, type);
        curr = g_list_next(curr);
    }
#endif
}

void
plugins_on_win_close(const char* const title, const char* const type)
{
#ifdef HAVE_PLUGINS
    GList* curr = plugins_ordered;
    while (curr) {
        ProfPlugin* plugin = curr->data;
        if (plugin->hook_mask & HOOK_ON_WIN_CLOSE)
            _lua_call_void_2str(plugin, "prof_on_win_close", title, type);
        curr = g_list_next(curr);
    }
#endif
}

GList*
plugins_get_disco_features(void)
{
    return disco_get_features();
}

#else // !HAVE_PLUGINS

#include <glib.h>
#include "plugins/plugins.h"

const char*
plugins_get_lua_version(void)
{
    return "none";
}

void
plugins_init(void)
{
}

GSList*
plugins_installed_list(void)
{
    return NULL;
}

GSList*
plugins_unloaded_list(void)
{
    return NULL;
}

GList*
plugins_loaded_list(void)
{
    return NULL;
}

gboolean
plugins_is_loaded(const char* const name)
{
    return FALSE;
}

char*
plugins_autocomplete(const char* const input, gboolean previous)
{
    return NULL;
}

void
plugins_reset_autocomplete(void)
{
}

void
plugins_free_install_result(PluginsInstallResult* result)
{
    if (result) {
        g_free(result);
    }
}

gboolean
plugins_install(const char* const plugin_name, const char* const filename, GString* error_message)
{
    if (error_message) {
        g_string_assign(error_message, "Plugin support is disabled.");
    }
    return FALSE;
}

PluginsInstallResult*
plugins_install_all(const char* const path)
{
    return NULL;
}

gboolean
plugins_uninstall(const char* const plugin_name)
{
    return FALSE;
}

gboolean
plugins_load(const char* const name, GString* error_message)
{
    if (error_message) {
        g_string_assign(error_message, "Plugin support is disabled.");
    }
    return FALSE;
}

GSList*
plugins_load_all(void)
{
    return NULL;
}

gboolean
plugins_unload(const char* const name)
{
    return FALSE;
}

gboolean
plugins_unload_all(void)
{
    return TRUE;
}

void
plugins_on_start(void)
{
}

void
plugins_on_shutdown(void)
{
}

void
plugins_close(void)
{
}

void
plugins_on_connect(const char* const account_name, const char* const fulljid)
{
}

void
plugins_on_disconnect(const char* const account_name, const char* const fulljid)
{
}

char*
plugins_pre_chat_message_display(const char* const barejid, const char* const resource, const char* message)
{
    return NULL;
}

void
plugins_post_chat_message_display(const char* const barejid, const char* const resource, const char* message)
{
}

char*
plugins_pre_chat_message_send(const char* const barejid, const char* message)
{
    return NULL;
}

void
plugins_post_chat_message_send(const char* const barejid, const char* message)
{
}

char*
plugins_pre_room_message_display(const char* const barejid, const char* const nick, const char* message)
{
    return NULL;
}

void
plugins_post_room_message_display(const char* const barejid, const char* const nick, const char* message)
{
}

char*
plugins_pre_room_message_send(const char* const barejid, const char* message)
{
    return NULL;
}

void
plugins_post_room_message_send(const char* const barejid, const char* message)
{
}

void
plugins_on_room_history_message(const char* const barejid, const char* const nick, const char* const message,
                                GDateTime* timestamp)
{
}

char*
plugins_pre_priv_message_display(const char* const fulljid, const char* message)
{
    return NULL;
}

void
plugins_post_priv_message_display(const char* const fulljid, const char* message)
{
}

char*
plugins_pre_priv_message_send(const char* const fulljid, const char* const message)
{
    return NULL;
}

void
plugins_post_priv_message_send(const char* const fulljid, const char* const message)
{
}

void
plugins_win_process_line(char* win, const char* const line)
{
}

void
plugins_close_win(const char* const plugin_name, const char* const tag)
{
}

char*
plugins_on_message_stanza_send(const char* const text)
{
    return NULL;
}

gboolean
plugins_on_message_stanza_receive(const char* const text)
{
    return TRUE;
}

char*
plugins_on_presence_stanza_send(const char* const text)
{
    return NULL;
}

gboolean
plugins_on_presence_stanza_receive(const char* const text)
{
    return TRUE;
}

char*
plugins_on_iq_stanza_send(const char* const text)
{
    return NULL;
}

gboolean
plugins_on_iq_stanza_receive(const char* const text)
{
    return TRUE;
}

void
plugins_on_contact_offline(const char* const barejid, const char* const resource, const char* const status)
{
}

void
plugins_on_contact_presence(const char* const barejid, const char* const resource, const char* const presence,
                            const char* const status, const int priority)
{
}

void
plugins_on_chat_win_focus(const char* const barejid)
{
}

void
plugins_on_room_win_focus(const char* const barejid)
{
}

void
plugins_on_room_occupant_join(const char* const roomjid, const char* const nick)
{
}

void
plugins_on_room_occupant_leave(const char* const roomjid, const char* const nick, const char* const reason)
{
}

void
plugins_on_chat_state(const char* const barejid, const char* const state)
{
}

void
plugins_on_message_correction(const char* const barejid, const char* const id, const char* const new_message)
{
}

void
plugins_on_win_open(const char* const title, const char* const type)
{
}

void
plugins_on_win_close(const char* const title, const char* const type)
{
}

gboolean
plugins_run_command(const char* const cmd)
{
    return FALSE;
}

void
plugins_run_timed(void)
{
}

GList*
plugins_get_command_names(void)
{
    return NULL;
}

CommandHelp*
plugins_get_help(const char* const cmd)
{
    return NULL;
}

GList*
plugins_get_disco_features(void)
{
    return NULL;
}

#endif
