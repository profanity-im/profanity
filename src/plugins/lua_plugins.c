/*
 * lua_plugins.c
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
    
    // first call returns function
    int call_result = lua_pcall(L, 0, 0, 0);
    lua_check_error(call_result);
    
    // store reference to function table
    int r = luaL_ref(L, LUA_REGISTRYINDEX);

    gchar *module_name = g_strndup(filename, strlen(filename) - 4);

    if (load_result == 0 && call_result == 0) {
        cons_debug("LOADING PLUGIN");

        ProfPlugin *plugin = malloc(sizeof(ProfPlugin));
        plugin->name = strdup(module_name);
        plugin->lang = LANG_LUA;
        plugin->module = &r;
        plugin->init_func = lua_init_hook;
        plugin->on_start_func = lua_on_start_hook;
        plugin->on_connect_func = lua_on_connect_hook;
        plugin->on_disconnect_func = lua_on_disconnect_hook;
        plugin->on_message_received_func = lua_on_message_received_hook;
        plugin->on_room_message_received_func = lua_on_room_message_received_hook;
        plugin->on_private_message_received_func = lua_on_private_message_received_hook;
        plugin->on_message_send_func = lua_on_message_send_hook;
        plugin->on_private_message_send_func = lua_on_private_message_send_hook;
        plugin->on_room_message_send_func = lua_on_room_message_send_hook;
        plugin->on_shutdown_func = lua_on_shutdown_hook;
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
/*
    int *p_ref = (int *)plugin->module;
    cons_debug("OK 1");

    // push function table
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    cons_debug("OK 2");

    int res1 = lua_pcall(L, 0, 0, 0);
    lua_check_error(res1);
    cons_debug("OK 2.1");

    // push key
    lua_pushstring(L, "prof_init");
    cons_debug("OK 3");

    // push args
    lua_pushstring(L, version);
    lua_pushstring(L, status);
    cons_debug("OK 4");

    // push function
    lua_gettable(L, -2);
    cons_debug("OK 5");

    int res2 = lua_pcall(L, 0, 0, 0);
    lua_check_error(res2);
    cons_debug("OK 6");
*/
}

void
lua_on_start_hook(ProfPlugin *plugin)
{
}

void
lua_on_connect_hook(ProfPlugin *plugin, const char * const account_name,
    const char * const fulljid)
{
}

void
lua_on_disconnect_hook(ProfPlugin *plugin, const char * const account_name,
    const char * const fulljid)
{
}

char *
lua_on_message_received_hook(ProfPlugin *plugin, const char * const jid,
    const char *message)
{
    return NULL;
}

char *
lua_on_private_message_received_hook(ProfPlugin *plugin, const char * const room,
    const char * const nick, const char *message)
{
    return NULL;
}

char *
lua_on_room_message_received_hook(ProfPlugin *plugin, const char * const room,
    const char * const nick, const char *message)
{
    return NULL;
}

char *
lua_on_message_send_hook(ProfPlugin *plugin, const char * const jid,
    const char *message)
{
    return NULL;
}

char *
lua_on_private_message_send_hook(ProfPlugin *plugin, const char * const room,
    const char * const nick, const char *message)
{
    return NULL;
}

char *
lua_on_room_message_send_hook(ProfPlugin *plugin, const char * const room,
    const char *message)
{
    return NULL;
}

void
lua_on_shutdown_hook(ProfPlugin *plugin)
{
}

void
lua_check_error(int value)
{
    if (value) {
        cons_debug("%s", lua_tostring(L, -1));
        lua_pop(L, 1);
    } else {
        cons_debug("Success");
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
