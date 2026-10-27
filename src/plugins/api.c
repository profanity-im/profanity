/*
 * api.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#ifdef HAVE_PLUGINS

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "profanity.h"
#include "log.h"
#include "common.h"
#include "config/theme.h"
#include "command/cmd_funcs.h"
#include "event/server_events.h"
#include "event/client_events.h"
#include "plugins/callbacks.h"
#include "plugins/autocompleters.h"
#include "plugins/themes.h"
#include "plugins/settings.h"
#include "plugins/disco.h"
#include "plugins/api.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/roster_list.h"
#include "xmpp/contact.h"
#include "xmpp/session.h"
#include "xmpp/connection.h"
#include "xmpp/capabilities.h"
#include "xmpp/message.h"
#include "xmpp/jid.h"
#include "xmpp/xmpp.h"
#include "xmpp/muc.h"

#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif

static const gchar*
_normalize_crlf(const gchar* const message, gchar** to_free)
{
    *to_free = NULL;
    if (message == NULL || strstr(message, "\r\n") == NULL) {
        return message;
    }

    auto_gcharv gchar** parts = g_strsplit(message, "\r\n", -1);
    *to_free = g_strjoinv("\n", parts);
    return *to_free;
}

static const char*
_lua_get_plugin_name(lua_State* L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "prof_plugin_name");
    const char* name = lua_tostring(L, -1);
    lua_pop(L, 1);
    return name;
}

static int
_lua_prof_cons_alert(lua_State* L)
{
    cons_alert(NULL);
    return 0;
}

static int
_lua_prof_cons_show(lua_State* L)
{
    const char* message = luaL_checkstring(L, 1);
    auto_gchar gchar* to_free = NULL;
    const gchar* parsed = _normalize_crlf(message, &to_free);
    cons_show("%s", parsed);
    return 0;
}

static int
_lua_prof_cons_show_themed(lua_State* L)
{
    const char* group = luaL_checkstring(L, 1);
    const char* key = luaL_checkstring(L, 2);
    const char* def = luaL_checkstring(L, 3);
    const char* message = luaL_checkstring(L, 4);

    auto_gchar gchar* to_free = NULL;
    const gchar* parsed = _normalize_crlf(message, &to_free);
    theme_item_t themeitem = plugin_themes_get(group, key, def);
    ProfWin* console = wins_get_console();
    win_println(console, themeitem, "-", "%s", parsed);
    return 0;
}

static int
_lua_prof_cons_bad_cmd_usage(lua_State* L)
{
    const char* cmd = luaL_checkstring(L, 1);
    cons_bad_cmd_usage(cmd);
    return 0;
}

static int
_lua_prof_notify(lua_State* L)
{
    const char* message = luaL_checkstring(L, 1);
    int timeout_ms = luaL_optinteger(L, 2, 5000);
    const char* category = luaL_optstring(L, 3, "message");
    notify(message, timeout_ms, category);
    return 0;
}

static int
_lua_prof_send_line(lua_State* L)
{
    const char* line = luaL_checkstring(L, 1);
    ProfWin* current = wins_get_current();
    if (!current) {
        return 0;
    }
    if (current->type == WIN_CHAT) {
        ProfChatWin* chatwin = (ProfChatWin*)current;
        cl_ev_send_msg(chatwin, line, NULL);
    } else if (current->type == WIN_MUC) {
        ProfMucWin* mucwin = (ProfMucWin*)current;
        cl_ev_send_muc_msg(mucwin, line, NULL);
    } else {
        cmd_process_input(current, (char*)line);
    }
    return 0;
}

static int
_lua_prof_send_stanza(lua_State* L)
{
    const char* stanza = luaL_checkstring(L, 1);
    connection_send_stanza(stanza);
    return 0;
}

static int
_lua_prof_is_connected(lua_State* L)
{
    lua_pushboolean(L, connection_get_status() == JABBER_CONNECTED);
    return 1;
}

static int
_lua_prof_get_account_name(lua_State* L)
{
    const char* name = session_get_account_name();
    if (name) {
        lua_pushstring(L, name);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int
_lua_prof_get_my_jid(lua_State* L)
{
    const char* jid = connection_get_fulljid();
    if (jid) {
        lua_pushstring(L, jid);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int
_lua_prof_get_current_recipient(lua_State* L)
{
    ProfWin* current = wins_get_current();
    if (current && current->type == WIN_CHAT) {
        ProfChatWin* chatwin = (ProfChatWin*)current;
        lua_pushstring(L, chatwin->barejid);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int
_lua_prof_get_current_muc(lua_State* L)
{
    ProfWin* current = wins_get_current();
    if (current && current->type == WIN_MUC) {
        ProfMucWin* mucwin = (ProfMucWin*)current;
        lua_pushstring(L, mucwin->roomjid);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int
_lua_prof_current_win_is_console(lua_State* L)
{
    ProfWin* current = wins_get_current();
    lua_pushboolean(L, current && current->type == WIN_CONSOLE);
    return 1;
}

static int
_lua_prof_get_current_nick(lua_State* L)
{
    ProfWin* current = wins_get_current();
    if (current && current->type == WIN_MUC) {
        ProfMucWin* mucwin = (ProfMucWin*)current;
        const char* nick = muc_nick(mucwin->roomjid);
        if (nick) {
            lua_pushstring(L, nick);
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}

static int
_lua_prof_get_name_from_roster(lua_State* L)
{
    const char* barejid = luaL_checkstring(L, 1);
    const char* name = roster_get_display_name(barejid);
    if (name) {
        lua_pushstring(L, name);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int
_lua_prof_get_barejid_from_roster(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    char* barejid = roster_barejid_from_name(name);
    if (barejid) {
        lua_pushstring(L, barejid);
        free(barejid);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int
_lua_prof_get_roster(lua_State* L)
{
    GSList* contacts = roster_get_contacts(ROSTER_ORD_NAME);
    lua_newtable(L);

    int idx = 1;
    GSList* curr = contacts;
    while (curr) {
        PContact contact = curr->data;
        if (contact) {
            lua_newtable(L);

            const char* barejid = p_contact_barejid(contact);
            lua_pushstring(L, barejid ? barejid : "");
            lua_setfield(L, -2, "barejid");

            const char* name = p_contact_name(contact);
            if (name) {
                lua_pushstring(L, name);
            } else {
                lua_pushnil(L);
            }
            lua_setfield(L, -2, "name");

            const char* sub = p_contact_subscription(contact);
            if (sub) {
                lua_pushstring(L, sub);
            } else {
                lua_pushnil(L);
            }
            lua_setfield(L, -2, "subscription");

            const char* presence = p_contact_presence(contact);
            if (presence) {
                lua_pushstring(L, presence);
            } else {
                lua_pushnil(L);
            }
            lua_setfield(L, -2, "presence");

            const char* status = p_contact_status(contact);
            if (status) {
                lua_pushstring(L, status);
            } else {
                lua_pushnil(L);
            }
            lua_setfield(L, -2, "status");

            GSList* groups = p_contact_groups(contact);
            lua_newtable(L);
            int g_idx = 1;
            while (groups) {
                lua_pushstring(L, (const char*)groups->data);
                lua_rawseti(L, -2, g_idx++);
                groups = g_slist_next(groups);
            }
            lua_setfield(L, -2, "groups");

            lua_rawseti(L, -2, idx++);
        }
        curr = g_slist_next(curr);
    }
    g_slist_free(contacts);

    return 1;
}

static int
_lua_prof_get_current_occupants(lua_State* L)
{
    ProfWin* current = wins_get_current();
    lua_newtable(L);
    if (current && current->type == WIN_MUC) {
        ProfMucWin* mucwin = (ProfMucWin*)current;
        GList* occupants_list = muc_roster(mucwin->roomjid);
        GList* curr = occupants_list;
        int idx = 1;
        while (curr) {
            Occupant* occupant = curr->data;
            if (occupant && occupant->nick) {
                lua_pushstring(L, occupant->nick);
                lua_rawseti(L, -2, idx++);
            }
            curr = g_list_next(curr);
        }
        g_list_free(occupants_list);
    }
    return 1;
}

static int
_lua_prof_get_room_nick(lua_State* L)
{
    const char* barejid = luaL_checkstring(L, 1);
    const char* nick = muc_nick(barejid);
    if (nick) {
        lua_pushstring(L, nick);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int
_lua_prof_log_debug(lua_State* L)
{
    const char* message = luaL_checkstring(L, 1);
    log_debug("%s", message);
    return 0;
}

static int
_lua_prof_log_info(lua_State* L)
{
    const char* message = luaL_checkstring(L, 1);
    log_info("%s", message);
    return 0;
}

static int
_lua_prof_log_warning(lua_State* L)
{
    const char* message = luaL_checkstring(L, 1);
    log_warning("%s", message);
    return 0;
}

static int
_lua_prof_log_error(lua_State* L)
{
    const char* message = luaL_checkstring(L, 1);
    log_error("%s", message);
    return 0;
}

static int
_lua_prof_win_exists(lua_State* L)
{
    const char* tag = luaL_checkstring(L, 1);
    lua_pushboolean(L, wins_get_plugin(tag) != NULL);
    return 1;
}

static int
_lua_prof_win_create(lua_State* L)
{
    const char* tag = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    const char* plugin_name = _lua_get_plugin_name(L);
    lua_pushvalue(L, 2);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    ProfWin* newwin = wins_new_plugin(plugin_name, tag);
    if (newwin) {
        PluginWindowCallback* window_callback = g_new0(PluginWindowCallback, 1);
        window_callback->L = L;
        window_callback->lua_ref = ref;
        callbacks_add_window_handler(plugin_name, tag, window_callback);
    } else {
        luaL_unref(L, LUA_REGISTRYINDEX, ref);
    }
    return 0;
}

static int
_lua_prof_win_close(lua_State* L)
{
    const char* tag = luaL_checkstring(L, 1);
    wins_close_plugin(tag);
    return 0;
}

static const char*
_win_type_str(win_type_t type)
{
    switch (type) {
    case WIN_CONSOLE:
        return "console";
    case WIN_CHAT:
        return "chat";
    case WIN_MUC:
        return "muc";
    case WIN_PRIVATE:
        return "private";
    case WIN_XML:
        return "xmlconsole";
    case WIN_PLUGIN:
        return "plugin";
    case WIN_CONFIG:
        return "config";
    case WIN_VCARD:
        return "vcard";
    default:
        return "unknown";
    }
}

static int
_lua_prof_get_open_windows(lua_State* L)
{
    GList* nums = wins_get_nums();
    lua_newtable(L);

    int idx = 1;
    GList* curr = nums;
    while (curr) {
        int num = GPOINTER_TO_INT(curr->data);
        ProfWin* window = wins_get_by_num(num);
        if (window) {
            lua_newtable(L);

            lua_pushinteger(L, num);
            lua_setfield(L, -2, "num");

            lua_pushstring(L, _win_type_str(window->type));
            lua_setfield(L, -2, "type");

            auto_gchar gchar* title = win_to_string(window);
            lua_pushstring(L, title ? title : "");
            lua_setfield(L, -2, "title");

            lua_pushinteger(L, win_unread(window));
            lua_setfield(L, -2, "unread");

            lua_pushboolean(L, wins_is_current(window));
            lua_setfield(L, -2, "is_current");

            lua_rawseti(L, -2, idx++);
        }
        curr = g_list_next(curr);
    }
    g_list_free(nums);

    return 1;
}

static int
_lua_prof_win_focus(lua_State* L)
{
    const char* tag = luaL_checkstring(L, 1);
    ProfWin* window = (ProfWin*)wins_get_plugin(tag);
    if (window) {
        ui_focus_win(window);
        lua_pushboolean(L, TRUE);
    } else {
        lua_pushboolean(L, FALSE);
    }
    return 1;
}

static int
_lua_prof_win_show(lua_State* L)
{
    const char* tag = luaL_checkstring(L, 1);
    const char* line = luaL_checkstring(L, 2);
    ProfPluginWin* pluginwin = wins_get_plugin(tag);
    if (pluginwin) {
        auto_gchar gchar* to_free = NULL;
        const gchar* parsed = _normalize_crlf(line, &to_free);
        win_println((ProfWin*)pluginwin, THEME_DEFAULT, "-", "%s", parsed);
    }
    return 0;
}

static int
_lua_prof_win_show_themed(lua_State* L)
{
    const char* tag = luaL_checkstring(L, 1);
    const char* group = luaL_checkstring(L, 2);
    const char* key = luaL_checkstring(L, 3);
    const char* def = luaL_checkstring(L, 4);
    const char* line = luaL_checkstring(L, 5);

    ProfPluginWin* pluginwin = wins_get_plugin(tag);
    if (pluginwin) {
        auto_gchar gchar* to_free = NULL;
        const gchar* parsed = _normalize_crlf(line, &to_free);
        theme_item_t themeitem = plugin_themes_get(group, key, def);
        win_println((ProfWin*)pluginwin, themeitem, "-", "%s", parsed);
    }
    return 0;
}

static int
_lua_prof_chat_show(lua_State* L)
{
    const char* barejid = luaL_checkstring(L, 1);
    const char* message = luaL_checkstring(L, 2);

    ProfChatWin* chatwin = wins_get_chat(barejid);
    if (chatwin) {
        auto_gchar gchar* to_free = NULL;
        const gchar* parsed = _normalize_crlf(message, &to_free);
        win_println((ProfWin*)chatwin, THEME_DEFAULT, "-", "%s", parsed);
    }
    return 0;
}

static int
_lua_prof_chat_show_themed(lua_State* L)
{
    const char* barejid = luaL_checkstring(L, 1);
    const char* group = luaL_checkstring(L, 2);
    const char* key = luaL_checkstring(L, 3);
    const char* def = luaL_checkstring(L, 4);
    const char* ch = luaL_optstring(L, 5, "-");
    const char* message = luaL_checkstring(L, 6);

    ProfChatWin* chatwin = wins_get_chat(barejid);
    if (chatwin) {
        auto_gchar gchar* to_free = NULL;
        const gchar* parsed = _normalize_crlf(message, &to_free);
        theme_item_t themeitem = plugin_themes_get(group, key, def);
        win_println((ProfWin*)chatwin, themeitem, ch, "%s", parsed);
    }
    return 0;
}

static int
_lua_prof_room_show(lua_State* L)
{
    const char* roomjid = luaL_checkstring(L, 1);
    const char* message = luaL_checkstring(L, 2);

    ProfMucWin* mucwin = wins_get_muc(roomjid);
    if (mucwin) {
        auto_gchar gchar* to_free = NULL;
        const gchar* parsed = _normalize_crlf(message, &to_free);
        win_println((ProfWin*)mucwin, THEME_DEFAULT, "-", "%s", parsed);
    }
    return 0;
}

static int
_lua_prof_room_show_themed(lua_State* L)
{
    const char* roomjid = luaL_checkstring(L, 1);
    const char* group = luaL_checkstring(L, 2);
    const char* key = luaL_checkstring(L, 3);
    const char* def = luaL_checkstring(L, 4);
    const char* ch = luaL_optstring(L, 5, "-");
    const char* message = luaL_checkstring(L, 6);

    ProfMucWin* mucwin = wins_get_muc(roomjid);
    if (mucwin) {
        auto_gchar gchar* to_free = NULL;
        const gchar* parsed = _normalize_crlf(message, &to_free);
        theme_item_t themeitem = plugin_themes_get(group, key, def);
        win_println((ProfWin*)mucwin, themeitem, ch, "%s", parsed);
    }
    return 0;
}

static int
_lua_prof_room_join(lua_State* L)
{
    const char* roomjid = luaL_checkstring(L, 1);
    const char* nick = luaL_optstring(L, 2, NULL);
    const char* password = luaL_optstring(L, 3, NULL);

    GString* cmd = g_string_new("/join ");
    g_string_append(cmd, roomjid);
    if (nick) {
        g_string_append_printf(cmd, " %s", nick);
    }
    if (password) {
        g_string_append_printf(cmd, " %s", password);
    }
    ProfWin* current = wins_get_current();
    if (current) {
        cmd_process_input(current, cmd->str);
    }
    g_string_free(cmd, TRUE);
    return 0;
}

static int
_lua_prof_room_leave(lua_State* L)
{
    const char* roomjid = luaL_checkstring(L, 1);
    if (muc_active(roomjid)) {
        presence_leave_chat_room(roomjid);
        muc_leave(roomjid);
        ui_leave_room(roomjid);
    }
    return 0;
}

static int
_lua_prof_chat_set_titlebar_enctext(lua_State* L)
{
    const char* barejid = luaL_checkstring(L, 1);
    const char* enctext = luaL_checkstring(L, 2);
    ProfChatWin* chatwin = wins_get_chat(barejid);
    if (chatwin) {
        chatwin_set_enctext(chatwin, enctext);
    }
    return 0;
}

static int
_lua_prof_chat_unset_titlebar_enctext(lua_State* L)
{
    const char* barejid = luaL_checkstring(L, 1);
    ProfChatWin* chatwin = wins_get_chat(barejid);
    if (chatwin) {
        chatwin_unset_enctext(chatwin);
    }
    return 0;
}

static int
_lua_prof_chat_set_incoming_char(lua_State* L)
{
    const char* barejid = luaL_checkstring(L, 1);
    const char* ch = luaL_checkstring(L, 2);
    ProfChatWin* chatwin = wins_get_chat(barejid);
    if (chatwin && strlen(ch) == 1) {
        chatwin_set_incoming_char(chatwin, ch);
    }
    return 0;
}

static int
_lua_prof_chat_unset_incoming_char(lua_State* L)
{
    const char* barejid = luaL_checkstring(L, 1);
    ProfChatWin* chatwin = wins_get_chat(barejid);
    if (chatwin) {
        chatwin_unset_incoming_char(chatwin);
    }
    return 0;
}

static int
_lua_prof_chat_set_outgoing_char(lua_State* L)
{
    const char* barejid = luaL_checkstring(L, 1);
    const char* ch = luaL_checkstring(L, 2);
    ProfChatWin* chatwin = wins_get_chat(barejid);
    if (chatwin && strlen(ch) == 1) {
        chatwin_set_outgoing_char(chatwin, ch);
    }
    return 0;
}

static int
_lua_prof_chat_unset_outgoing_char(lua_State* L)
{
    const char* barejid = luaL_checkstring(L, 1);
    ProfChatWin* chatwin = wins_get_chat(barejid);
    if (chatwin) {
        chatwin_unset_outgoing_char(chatwin);
    }
    return 0;
}

static int
_lua_prof_room_set_titlebar_enctext(lua_State* L)
{
    const char* roomjid = luaL_checkstring(L, 1);
    const char* enctext = luaL_checkstring(L, 2);
    ProfMucWin* mucwin = wins_get_muc(roomjid);
    if (mucwin) {
        mucwin_set_enctext(mucwin, enctext);
    }
    return 0;
}

static int
_lua_prof_room_unset_titlebar_enctext(lua_State* L)
{
    const char* roomjid = luaL_checkstring(L, 1);
    ProfMucWin* mucwin = wins_get_muc(roomjid);
    if (mucwin) {
        mucwin_unset_enctext(mucwin);
    }
    return 0;
}

static int
_lua_prof_room_set_message_char(lua_State* L)
{
    const char* roomjid = luaL_checkstring(L, 1);
    const char* ch = luaL_checkstring(L, 2);
    ProfMucWin* mucwin = wins_get_muc(roomjid);
    if (mucwin && strlen(ch) == 1) {
        mucwin_set_message_char(mucwin, ch);
    }
    return 0;
}

static int
_lua_prof_room_unset_message_char(lua_State* L)
{
    const char* roomjid = luaL_checkstring(L, 1);
    ProfMucWin* mucwin = wins_get_muc(roomjid);
    if (mucwin) {
        mucwin_unset_message_char(mucwin);
    }
    return 0;
}

static int
_lua_prof_incoming_message(lua_State* L)
{
    const char* barejid = luaL_checkstring(L, 1);
    const char* resource = luaL_optstring(L, 2, NULL);
    const char* plain = luaL_checkstring(L, 3);

    ProfMessage* message = message_init();
    message->from_jid = jid_create_from_bare_and_resource(barejid, resource);
    message->plain = g_strdup(plain);

    sv_ev_incoming_message(message);
    sv_ev_activity((char*)barejid, (char*)resource, FALSE);

    message_free(message);
    return 0;
}

static int
_lua_prof_disco_add_feature(lua_State* L)
{
    const char* feature = luaL_checkstring(L, 1);
    const char* plugin_name = _lua_get_plugin_name(L);
    disco_add_feature((char*)plugin_name, (char*)feature);
    caps_reset_ver();
    return 0;
}

static int
_lua_prof_encryption_reset(lua_State* L)
{
    const char* barejid = luaL_checkstring(L, 1);
    ProfChatWin* chatwin = wins_get_chat(barejid);
    if (chatwin) {
#ifdef HAVE_LIBGPGME
        if (chatwin->pgp_send) {
            chatwin->pgp_send = FALSE;
            win_println((ProfWin*)chatwin, THEME_DEFAULT, "!", "PGP encryption disabled.");
        }
#endif
#ifdef HAVE_LIBOTR
        if (chatwin->is_otr) {
            chatwin_otr_unsecured(chatwin, TRUE);
            otr_end_session(chatwin->barejid);
        }
#endif
    }
    return 0;
}

static int
_lua_prof_settings_get(lua_State* L)
{
    const char* group = luaL_checkstring(L, 1);
    const char* key = luaL_checkstring(L, 2);

    // If arg 3 is missing or a string, default to string
    if (lua_isnoneornil(L, 3) || lua_type(L, 3) == LUA_TSTRING) {
        const char* def = luaL_optstring(L, 3, NULL);
        char* val = plugin_settings_string_get(group, key, def);
        if (val) {
            lua_pushstring(L, val);
            free(val);
        } else {
            lua_pushnil(L);
        }
        return 1;
    }

    // If arg 3 is boolean, we get a boolean
    if (lua_isboolean(L, 3)) {
        gboolean def = lua_toboolean(L, 3);
        lua_pushboolean(L, plugin_settings_boolean_get(group, key, def));
        return 1;
    }

    // If arg 3 is number, we get an int
    if (lua_isnumber(L, 3)) {
        int def = lua_tointeger(L, 3);
        lua_pushinteger(L, plugin_settings_int_get(group, key, def));
        return 1;
    }

    luaL_error(L, "prof.settings.get: arg 3 must be string, boolean, or number default value");
    return 0;
}

static int
_lua_prof_settings_set(lua_State* L)
{
    const char* group = luaL_checkstring(L, 1);
    const char* key = luaL_checkstring(L, 2);

    if (lua_isboolean(L, 3)) {
        gboolean val = lua_toboolean(L, 3);
        plugin_settings_boolean_set(group, key, val);
    } else if (lua_isnumber(L, 3)) {
        int val = lua_tointeger(L, 3);
        plugin_settings_int_set(group, key, val);
    } else {
        const char* val = luaL_checkstring(L, 3);
        plugin_settings_string_set(group, key, val);
    }
    return 0;
}

static int
_lua_prof_settings_string_list_get(lua_State* L)
{
    const char* group = luaL_checkstring(L, 1);
    const char* key = luaL_checkstring(L, 2);
    char** list = plugin_settings_string_list_get(group, key);
    lua_newtable(L);
    if (list) {
        for (int i = 0; list[i]; i++) {
            lua_pushstring(L, list[i]);
            lua_rawseti(L, -2, i + 1);
        }
        g_strfreev(list);
    }
    return 1;
}

static int
_lua_prof_settings_string_list_add(lua_State* L)
{
    const char* group = luaL_checkstring(L, 1);
    const char* key = luaL_checkstring(L, 2);
    const char* val = luaL_checkstring(L, 3);
    plugin_settings_string_list_add(group, key, val);
    return 0;
}

static int
_lua_prof_settings_string_list_remove(lua_State* L)
{
    const char* group = luaL_checkstring(L, 1);
    const char* key = luaL_checkstring(L, 2);
    const char* val = luaL_checkstring(L, 3);
    lua_pushboolean(L, plugin_settings_string_list_remove(group, key, val));
    return 1;
}

static int
_lua_prof_settings_string_list_clear(lua_State* L)
{
    const char* group = luaL_checkstring(L, 1);
    const char* key = luaL_checkstring(L, 2);
    lua_pushboolean(L, plugin_settings_string_list_clear(group, key));
    return 1;
}

static int
_lua_prof_register_command(lua_State* L)
{
    const char* command_name = luaL_checkstring(L, 1);
    int min_args = luaL_checkinteger(L, 2);
    int max_args = luaL_checkinteger(L, 3);
    luaL_checktype(L, 4, LUA_TFUNCTION);
    const char* synopsis = luaL_optstring(L, 5, NULL);
    const char* description = luaL_optstring(L, 6, NULL);

    const char* plugin_name = _lua_get_plugin_name(L);

    lua_pushvalue(L, 4);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    PluginCommand* command = g_new0(PluginCommand, 1);
    command->command_name = g_strdup(command_name);
    command->min_args = min_args;
    command->max_args = max_args;
    command->L = L;
    command->lua_ref = ref;

    CommandHelp* help = g_new0(CommandHelp, 1);
    if (synopsis) {
        help->synopsis[0] = g_strdup(synopsis);
    }
    if (description) {
        help->desc = g_strdup(description);
    }
    command->help = help;

    callbacks_add_command(plugin_name, command);
    return 0;
}

static int
_lua_prof_register_timed(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    int interval_seconds = luaL_checkinteger(L, 2);

    const char* plugin_name = _lua_get_plugin_name(L);

    lua_pushvalue(L, 1);
    int ref = luaL_ref(L, LUA_REGISTRYINDEX);

    PluginTimedFunction* timed_function = g_new0(PluginTimedFunction, 1);
    timed_function->interval_seconds = interval_seconds;
    timed_function->timer = g_timer_new();
    timed_function->L = L;
    timed_function->lua_ref = ref;

    callbacks_add_timed(plugin_name, timed_function);
    return 0;
}

static int
_lua_prof_completer_add(lua_State* L)
{
    const char* key = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    const char* plugin_name = _lua_get_plugin_name(L);
    int len = luaL_len(L, 2);
    char** items = g_new0(char*, len + 1);
    for (int i = 1; i <= len; i++) {
        lua_rawgeti(L, 2, i);
        items[i - 1] = g_strdup(luaL_checkstring(L, -1));
        lua_pop(L, 1);
    }

    autocompleters_add(plugin_name, key, items);
    g_strfreev(items);
    return 0;
}

static int
_lua_prof_completer_remove(lua_State* L)
{
    const char* key = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    const char* plugin_name = _lua_get_plugin_name(L);
    int len = luaL_len(L, 2);
    char** items = g_new0(char*, len + 1);
    for (int i = 1; i <= len; i++) {
        lua_rawgeti(L, 2, i);
        items[i - 1] = g_strdup(luaL_checkstring(L, -1));
        lua_pop(L, 1);
    }

    autocompleters_remove(plugin_name, key, items);
    g_strfreev(items);
    return 0;
}

static int
_lua_prof_completer_clear(lua_State* L)
{
    const char* key = luaL_checkstring(L, 1);
    const char* plugin_name = _lua_get_plugin_name(L);
    autocompleters_clear(plugin_name, key);
    return 0;
}

static int
_lua_prof_filepath_completer_add(lua_State* L)
{
    const char* prefix = luaL_checkstring(L, 1);
    const char* plugin_name = _lua_get_plugin_name(L);
    autocompleters_filepath_add(plugin_name, prefix);
    return 0;
}

static const struct luaL_Reg log_lib[] = {
    { "debug", _lua_prof_log_debug },
    { "info", _lua_prof_log_info },
    { "warning", _lua_prof_log_warning },
    { "error", _lua_prof_log_error },
    { NULL, NULL }
};

static const struct luaL_Reg chat_lib[] = {
    { "show", _lua_prof_chat_show },
    { "show_themed", _lua_prof_chat_show_themed },
    { "set_titlebar_enctext", _lua_prof_chat_set_titlebar_enctext },
    { "unset_titlebar_enctext", _lua_prof_chat_unset_titlebar_enctext },
    { "set_incoming_char", _lua_prof_chat_set_incoming_char },
    { "unset_incoming_char", _lua_prof_chat_unset_incoming_char },
    { "set_outgoing_char", _lua_prof_chat_set_outgoing_char },
    { "unset_outgoing_char", _lua_prof_chat_unset_outgoing_char },
    { NULL, NULL }
};

static const struct luaL_Reg muc_lib[] = {
    { "show", _lua_prof_room_show },
    { "show_themed", _lua_prof_room_show_themed },
    { "join", _lua_prof_room_join },
    { "leave", _lua_prof_room_leave },
    { "set_titlebar_enctext", _lua_prof_room_set_titlebar_enctext },
    { "unset_titlebar_enctext", _lua_prof_room_unset_titlebar_enctext },
    { "set_message_char", _lua_prof_room_set_message_char },
    { "unset_message_char", _lua_prof_room_unset_message_char },
    { NULL, NULL }
};

static const struct luaL_Reg win_lib[] = {
    { "exists", _lua_prof_win_exists },
    { "create", _lua_prof_win_create },
    { "close", _lua_prof_win_close },
    { "focus", _lua_prof_win_focus },
    { "show", _lua_prof_win_show },
    { "show_themed", _lua_prof_win_show_themed },
    { NULL, NULL }
};

static const struct luaL_Reg cons_lib[] = {
    { "alert", _lua_prof_cons_alert },
    { "show", _lua_prof_cons_show },
    { "show_themed", _lua_prof_cons_show_themed },
    { "bad_cmd_usage", _lua_prof_cons_bad_cmd_usage },
    { NULL, NULL }
};

static const struct luaL_Reg disco_lib[] = {
    { "add_feature", _lua_prof_disco_add_feature },
    { NULL, NULL }
};

static const struct luaL_Reg settings_lib[] = {
    { "get", _lua_prof_settings_get },
    { "set", _lua_prof_settings_set },
    { "list_get", _lua_prof_settings_string_list_get },
    { "list_add", _lua_prof_settings_string_list_add },
    { "list_remove", _lua_prof_settings_string_list_remove },
    { "list_clear", _lua_prof_settings_string_list_clear },
    { NULL, NULL }
};

static const struct luaL_Reg completer_lib[] = {
    { "add", _lua_prof_completer_add },
    { "remove", _lua_prof_completer_remove },
    { "clear", _lua_prof_completer_clear },
    { "filepath_add", _lua_prof_filepath_completer_add },
    { NULL, NULL }
};

static const struct luaL_Reg prof_lib[] = {
    { "notify", _lua_prof_notify },
    { "is_connected", _lua_prof_is_connected },
    { "get_account_name", _lua_prof_get_account_name },
    { "get_my_jid", _lua_prof_get_my_jid },
    { "send_line", _lua_prof_send_line },
    { "send_stanza", _lua_prof_send_stanza },
    { "get_current_recipient", _lua_prof_get_current_recipient },
    { "get_current_muc", _lua_prof_get_current_muc },
    { "current_win_is_console", _lua_prof_current_win_is_console },
    { "get_current_nick", _lua_prof_get_current_nick },
    { "get_name_from_roster", _lua_prof_get_name_from_roster },
    { "get_barejid_from_roster", _lua_prof_get_barejid_from_roster },
    { "get_roster", _lua_prof_get_roster },
    { "get_current_occupants", _lua_prof_get_current_occupants },
    { "get_room_nick", _lua_prof_get_room_nick },
    { "get_open_windows", _lua_prof_get_open_windows },
    { "incoming_message", _lua_prof_incoming_message },
    { "encryption_reset", _lua_prof_encryption_reset },
    { "register_command", _lua_prof_register_command },
    { "register_timed", _lua_prof_register_timed },
    { NULL, NULL }
};

void
api_init_lua(const char* const plugin_name, lua_State* L)
{
    lua_pushstring(L, plugin_name);
    lua_setfield(L, LUA_REGISTRYINDEX, "prof_plugin_name");

    luaL_newlib(L, prof_lib);

    luaL_newlib(L, log_lib);
    lua_setfield(L, -2, "log");

    luaL_newlib(L, chat_lib);
    lua_setfield(L, -2, "chat");

    luaL_newlib(L, muc_lib);
    lua_setfield(L, -2, "muc");

    luaL_newlib(L, win_lib);
    lua_setfield(L, -2, "win");

    luaL_newlib(L, cons_lib);
    lua_setfield(L, -2, "cons");

    luaL_newlib(L, disco_lib);
    lua_setfield(L, -2, "disco");

    luaL_newlib(L, settings_lib);
    lua_setfield(L, -2, "settings");

    luaL_newlib(L, completer_lib);
    lua_setfield(L, -2, "completer");

    lua_setglobal(L, "prof");
}

#endif
