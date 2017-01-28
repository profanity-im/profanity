/*
 * lua_plugins.c
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

#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "config/preferences.h"
#include "plugins/api.h"
#include "plugins/callbacks.h"
#include "plugins/plugins.h"
#include "plugins/lua_api.h"
#include "plugins/lua_plugins.h"
#include "ui/ui.h"

static lua_State *L;

lua_State *
lua_get_state(void)
{
    return L;
}

void
lua_env_init(void)
{
    L = luaL_newstate();
    luaL_openlibs(L);
    lua_api_init(L);
}

ProfPlugin *
lua_plugin_create(const char * const filename)
{
    gchar *plugins_dir = plugins_get_dir();
    GString *abs_path = g_string_new(plugins_dir);
    g_string_append(abs_path, "/");
    g_string_append(abs_path, filename);

    int load_result = luaL_loadfile(L, abs_path->str);
    lua_check_error(load_result);

    int call_result = lua_pcall(L, 0, 1, 0);
    lua_check_error(call_result);

    int *module_p = malloc(sizeof(int));
    *module_p = luaL_ref(L, LUA_REGISTRYINDEX);

    gchar *module_name = g_strndup(filename, strlen(filename) - 4);

    if (load_result == 0 && call_result == 0) {
        ProfPlugin *plugin = malloc(sizeof(ProfPlugin));
        plugin->name = strdup(module_name);
        plugin->lang = LANG_LUA;
        plugin->module = module_p;
        plugin->init_func = lua_init_hook;
        plugin->on_start_func = lua_on_start_hook;
        plugin->on_shutdown_func = lua_on_shutdown_hook;
        plugin->on_connect_func = lua_on_connect_hook;
        plugin->on_disconnect_func = lua_on_disconnect_hook;
        plugin->pre_chat_message_display = lua_pre_chat_message_display_hook;
        plugin->post_chat_message_display = lua_post_chat_message_display_hook;
        plugin->pre_chat_message_send = lua_pre_chat_message_send_hook;
        plugin->post_chat_message_send = lua_post_chat_message_send_hook;
        plugin->pre_room_message_display = lua_pre_room_message_display_hook;
        plugin->post_room_message_display = lua_post_room_message_display_hook;
        plugin->pre_room_message_send = lua_pre_room_message_send_hook;
        plugin->post_room_message_send = lua_post_room_message_send_hook;
        plugin->pre_priv_message_display = lua_pre_priv_message_display_hook;
        plugin->post_priv_message_display = lua_post_priv_message_display_hook;
        plugin->pre_priv_message_send = lua_pre_priv_message_send_hook;
        plugin->post_priv_message_send = lua_post_priv_message_send_hook;
        g_free(module_name);
        g_free(plugins_dir);
        g_string_free(abs_path, TRUE);
        return plugin;
    } else {
        g_free(plugins_dir);
        g_string_free(abs_path, TRUE);
        g_free(module_name);
        return NULL;
    }
}

void
lua_init_hook(ProfPlugin *plugin, const char * const version, const char * const status)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_init");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, version);
        lua_pushstring(L, status);
        int res2 = lua_pcall(L, 2, 0, 0);
        lua_check_error(res2);
        lua_pop(L, 1);
    } else {
        lua_pop(L, 2);
    }
}

void
lua_on_start_hook(ProfPlugin *plugin)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_on_start");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        int res2 = lua_pcall(L, 0, 0, 0);
        lua_check_error(res2);
        lua_pop(L, 1);
    } else {
        lua_pop(L, 2);
    }
}

void
lua_on_shutdown_hook(ProfPlugin *plugin)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_on_shutdown");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        int res2 = lua_pcall(L, 0, 0, 0);
        lua_check_error(res2);
        lua_pop(L, 1);
    } else {
        lua_pop(L, 2);
    }
}

void
lua_on_connect_hook(ProfPlugin *plugin, const char * const account_name,
    const char * const fulljid)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_on_connect");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, account_name);
        lua_pushstring(L, fulljid);
        int res2 = lua_pcall(L, 2, 0, 0);
        lua_check_error(res2);
        lua_pop(L, 1);
    } else {
        lua_pop(L, 2);
    }
}

void
lua_on_disconnect_hook(ProfPlugin *plugin, const char * const account_name,
    const char * const fulljid)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_on_disconnect");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, account_name);
        lua_pushstring(L, fulljid);
        int res2 = lua_pcall(L, 2, 0, 0);
        lua_check_error(res2);
        lua_pop(L, 1);
    } else {
        lua_pop(L, 2);
    }
}

char*
lua_pre_chat_message_display_hook(ProfPlugin *plugin, const char * const jid, const char *message)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_pre_chat_message_display");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, jid);
        lua_pushstring(L, message);
        int res2 = lua_pcall(L, 2, 1, 0);
        lua_check_error(res2);

        char *result = NULL;
        if (lua_isstring(L, -1)) {
            result = strdup(lua_tostring(L, -1));
        }

        lua_pop(L, 2);

        return result;
    } else {
        lua_pop(L, 2);
        return NULL;
    }
}

void
lua_post_chat_message_display_hook(ProfPlugin *plugin, const char * const jid, const char *message)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_post_chat_message_display");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, jid);
        lua_pushstring(L, message);
        int res2 = lua_pcall(L, 2, 0, 0);
        lua_check_error(res2);
        lua_pop(L, 1);
    } else {
        lua_pop(L, 2);
    }
}

char*
lua_pre_chat_message_send_hook(ProfPlugin *plugin, const char * const jid, const char *message)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_pre_chat_message_send");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, jid);
        lua_pushstring(L, message);
        int res2 = lua_pcall(L, 2, 1, 0);
        lua_check_error(res2);

        char *result = NULL;
        if (lua_isstring(L, -1)) {
            result = strdup(lua_tostring(L, -1));
        }

        lua_pop(L, 2);

        return result;
    } else {
        lua_pop(L, 2);
        return NULL;
    }

    return NULL;
}

void
lua_post_chat_message_send_hook(ProfPlugin *plugin, const char * const jid, const char *message)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_pre_chat_message_send");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, jid);
        lua_pushstring(L, message);
        int res2 = lua_pcall(L, 2, 0, 0);
        lua_check_error(res2);
        lua_pop(L, 1);
    } else {
        lua_pop(L, 2);
    }
}

char*
lua_pre_room_message_display_hook(ProfPlugin *plugin, const char * const room, const char * const nick, const char *message)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_pre_room_message_display");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, room);
        lua_pushstring(L, nick);
        lua_pushstring(L, message);
        int res2 = lua_pcall(L, 3, 1, 0);
        lua_check_error(res2);

        char *result = NULL;
        if (lua_isstring(L, -1)) {
            result = strdup(lua_tostring(L, -1));
        }

        lua_pop(L, 2);

        return result;
    } else {
        lua_pop(L, 2);
        return NULL;
    }
}

void
lua_post_room_message_display_hook(ProfPlugin *plugin, const char * const room, const char * const nick, const char *message)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_post_room_message_display");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, room);
        lua_pushstring(L, nick);
        lua_pushstring(L, message);
        int res2 = lua_pcall(L, 3, 1, 0);
        lua_check_error(res2);
        lua_pop(L, 1);
    } else {
        lua_pop(L, 2);
    }
}

char*
lua_pre_room_message_send_hook(ProfPlugin *plugin, const char * const room, const char *message)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_pre_room_message_send");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, room);
        lua_pushstring(L, message);
        int res2 = lua_pcall(L, 2, 1, 0);
        lua_check_error(res2);

        char *result = NULL;
        if (lua_isstring(L, -1)) {
            result = strdup(lua_tostring(L, -1));
        }

        lua_pop(L, 2);

        return result;
    } else {
        lua_pop(L, 2);
        return NULL;
    }
}

void
lua_post_room_message_send_hook(ProfPlugin *plugin, const char * const room, const char *message)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_post_room_message_send");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, room);
        lua_pushstring(L, message);
        int res2 = lua_pcall(L, 2, 1, 0);
        lua_check_error(res2);
        lua_pop(L, 1);
    } else {
        lua_pop(L, 2);
    }
}

char*
lua_pre_priv_message_display_hook(ProfPlugin *plugin, const char * const room, const char * const nick, const char *message)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_pre_priv_message_display");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, room);
        lua_pushstring(L, nick);
        lua_pushstring(L, message);
        int res2 = lua_pcall(L, 3, 1, 0);
        lua_check_error(res2);

        char *result = NULL;
        if (lua_isstring(L, -1)) {
            result = strdup(lua_tostring(L, -1));
        }

        lua_pop(L, 2);

        return result;
    } else {
        lua_pop(L, 2);
        return NULL;
    }
}

void
lua_post_priv_message_display_hook(ProfPlugin *plugin, const char * const room, const char * const nick, const char *message)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_post_priv_message_display");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, room);
        lua_pushstring(L, nick);
        lua_pushstring(L, message);
        int res2 = lua_pcall(L, 3, 1, 0);
        lua_check_error(res2);
        lua_pop(L, 1);
    } else {
        lua_pop(L, 2);
    }
}

char*
lua_pre_priv_message_send_hook(ProfPlugin *plugin, const char * const room, const char * const nick, const char * const message)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_pre_priv_message_send");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, room);
        lua_pushstring(L, nick);
        lua_pushstring(L, message);
        int res2 = lua_pcall(L, 3, 1, 0);
        lua_check_error(res2);

        char *result = NULL;
        if (lua_isstring(L, -1)) {
            result = strdup(lua_tostring(L, -1));
        }

        lua_pop(L, 2);

        return result;
    } else {
        lua_pop(L, 2);
        return NULL;
    }
}

void
lua_post_priv_message_send_hook(ProfPlugin *plugin, const char * const room, const char * const nick, const char * const message)
{
    int *p_ref = (int *)plugin->module;
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, "prof_pre_priv_message_send");
    lua_gettable(L, -2);
    if (!lua_isnil(L, -1)) {
        lua_pushstring(L, room);
        lua_pushstring(L, nick);
        lua_pushstring(L, message);
        int res2 = lua_pcall(L, 3, 1, 0);
        lua_check_error(res2);
        lua_pop(L, 1);
    } else {
        lua_pop(L, 2);
    }
}

void
lua_check_error(int value)
{
    if (value) {
        cons_debug("%s", lua_tostring(L, -1));
        lua_pop(L, 1);
        l_stackdump(L);
    }
}

void
lua_plugin_destroy(ProfPlugin *plugin)
{
    free(plugin->name);
    free(plugin);
}

void
lua_shutdown(void)
{
    lua_close(L);
}

void
l_stackdump(lua_State *L)
{
    cons_debug("Lua stack:");
    int i;
    int top = lua_gettop(L);
    for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(L, i);
        switch (t) {

        case LUA_TSTRING:  /* strings */
            cons_debug("  \"%s\"", lua_tostring(L, i));
            break;

        case LUA_TBOOLEAN:  /* booleans */
            cons_debug(lua_toboolean(L, i) ? "  true" : "  false");
            break;

        case LUA_TNUMBER:  /* numbers */
            cons_debug("  %g", lua_tonumber(L, i));
            break;

        default:  /* other values */
            cons_debug("  %s", lua_typename(L, t));
            break;

        }
    }
    cons_debug("End stack");
    cons_debug("");
}
