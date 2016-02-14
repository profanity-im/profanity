/*
 * lua_api.c
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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

#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <glib.h>

#include "plugins/api.h"
#include "plugins/lua_api.h"
#include "plugins/callbacks.h"
#include "plugins/autocompleters.h"

#include "ui/ui.h"

static int
lua_api_cons_alert(lua_State *L)
{
    api_cons_alert();
    return 0;
}

static int
lua_api_cons_show(lua_State *L)
{
    const char *message = lua_tostring(L, -1);
    api_cons_show(message);
    return 0;
}

static int
lua_api_register_command(lua_State *L)
{
    const char *command_name = lua_tostring(L, -7);
    int min_args = lua_tonumber(L, -6);
    int max_args = lua_tonumber(L, -5);
    const char *usage = lua_tostring(L, -4);
    const char *short_help = lua_tostring(L, -3);
    const char *long_help = lua_tostring(L, -2);

    int *p_callback_ref = malloc(sizeof(int));
    *p_callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    api_register_command(command_name, min_args, max_args, usage, short_help,
        long_help, p_callback_ref, lua_command_callback);

    return 0;
}

static int
lua_api_register_timed(lua_State *L)
{
    int interval_seconds = lua_tonumber(L, -1);
    lua_pop(L, 1);
    int *p_callback_ref = malloc(sizeof(int));
    *p_callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    api_register_timed(p_callback_ref, interval_seconds, lua_timed_callback);

    return 0;
}

static int
lua_api_register_ac(lua_State *L)
{
    const char *key = lua_tostring(L, -2);
    int len = lua_objlen(L, -1);
    char *c_items[len];

    int i;
    for (i = 0; i < len; i++) {
        lua_pushinteger(L, i+1);
        lua_gettable(L, -2);

        // create copy as cannot guarantee the
        // item does not get gc'd after pop
        const char *val = lua_tostring(L, -1);
        c_items[i] = strdup(val);
        lua_pop(L, 1);
    }
    c_items[len] = NULL;

    // makes own copy of items
    autocompleters_add(key, c_items);

    // free all items
    for (i = 0; i < len; i++) {
        free(c_items[i]);
    }

    return 0;
}

static int
lua_api_notify(lua_State *L)
{
    const char *message = lua_tostring(L, -3);
    int timeout_ms = lua_tonumber(L, -2);
    const char *category = lua_tostring(L, -1);

    api_notify(message, category, timeout_ms);

    return 0;
}

static int
lua_api_send_line(lua_State *L)
{
    const char *line = lua_tostring(L, -1);
    api_send_line(strdup(line));
    return 0;
}

static int
lua_api_get_current_recipient(lua_State *L)
{
    const char *recipient = api_get_current_recipient();

    if (recipient) {
        lua_pushstring(L, recipient);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

static int
lua_api_get_current_muc(lua_State *L)
{
    const char *room = api_get_current_muc();

    if (room) {
        lua_pushstring(L, room);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

static int
lua_api_log_debug(lua_State *L)
{
    const char *message = lua_tostring(L, -1);
    api_log_debug(message);
    return 0;
}

static int
lua_api_log_info(lua_State *L)
{
    const char *message = lua_tostring(L, -1);
    api_log_info(message);
    return 0;
}

static int
lua_api_log_warning(lua_State *L)
{
    const char *message = lua_tostring(L, -1);
    api_log_warning(message);
    return 0;
}

static int
lua_api_log_error(lua_State *L)
{
    const char *message = lua_tostring(L, -1);
    api_log_error(message);
    return 0;
}

static int
lua_api_win_exists(lua_State *L)
{
    const char *tag = lua_tostring(L, -1);

    if (api_win_exists(tag)) {
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

static int
lua_api_win_create(lua_State *L)
{
    const char *tag = lua_tostring(L, -2);

    int *p_callback_ref = malloc(sizeof(int));
    *p_callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    api_win_create(tag, p_callback_ref, lua_window_callback);

    return 0;
}

static int
lua_api_win_focus(lua_State *L)
{
    const char *tag = lua_tostring(L, -1);
    api_win_focus(tag);
    return 0;
}

static int
lua_api_win_show(lua_State *L)
{
    const char *tag = lua_tostring(L, -2);
    const char *line = lua_tostring(L, -1);
    api_win_show(tag, line);
    return 0;
}

static int
lua_api_win_show_green(lua_State *L)
{
    const char *tag = lua_tostring(L, -2);
    const char *line = lua_tostring(L, -1);
    api_win_show_green(tag, line);
    return 0;
}

static int
lua_api_win_show_red(lua_State *L)
{
    const char *tag = lua_tostring(L, -2);
    const char *line = lua_tostring(L, -1);
    api_win_show_red(tag, line);
    return 0;
}

static int
lua_api_win_show_cyan(lua_State *L)
{
    const char *tag = lua_tostring(L, -2);
    const char *line = lua_tostring(L, -1);
    api_win_show_cyan(tag, line);
    return 0;
}

static int
lua_api_win_show_yellow(lua_State *L)
{
    const char *tag = lua_tostring(L, -2);
    const char *line = lua_tostring(L, -1);
    api_win_show_yellow(tag, line);
    return 0;
}

void
lua_command_callback(PluginCommand *command, gchar **args)
{
    int *p_ref = (int *)command->callback;
    lua_State *L = lua_get_state();

    int num_args = g_strv_length(args);
    if (num_args == 0) {
        if (command->max_args == 1) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
            lua_pushnil(L);
            lua_pcall(L, 1, 0, 0);
        } else {
            lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
            lua_pcall(L, 0, 0, 0);
        }
    } else if (num_args == 1) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
        lua_pushstring(L, args[0]);
        lua_pcall(L, 1, 0, 0);
    } else if (num_args == 2) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
        lua_pushstring(L, args[0]);
        lua_pushstring(L, args[1]);
        lua_pcall(L, 2, 0, 0);
    } else if (num_args == 3) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
        lua_pushstring(L, args[0]);
        lua_pushstring(L, args[1]);
        lua_pushstring(L, args[2]);
        lua_pcall(L, 3, 0, 0);
    } else if (num_args == 4) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
        lua_pushstring(L, args[0]);
        lua_pushstring(L, args[1]);
        lua_pushstring(L, args[2]);
        lua_pushstring(L, args[3]);
        lua_pcall(L, 4, 0, 0);
    } else if (num_args == 5) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
        lua_pushstring(L, args[0]);
        lua_pushstring(L, args[1]);
        lua_pushstring(L, args[2]);
        lua_pushstring(L, args[3]);
        lua_pushstring(L, args[4]);
        lua_pcall(L, 5, 0, 0);
    }
}

void
lua_timed_callback(PluginTimedFunction *timed_function)
{
    int *p_ref = (int *)timed_function->callback;
    lua_State *L = lua_get_state();

    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pcall(L, 0, 0, 0);
}

void
lua_window_callback(PluginWindowCallback *window_callback, char *tag, char *line)
{
    int *p_ref = (int *)window_callback->callback;

    lua_State *L = lua_get_state();
    lua_rawgeti(L, LUA_REGISTRYINDEX, *p_ref);
    lua_pushstring(L, tag);
    lua_pushstring(L, line);
    lua_pcall(L, 2, 0, 0);
}
void
lua_api_init(lua_State *L)
{
    lua_pushcfunction(L, lua_api_cons_alert);
    lua_setglobal(L, "prof_cons_alert");
    lua_pushcfunction(L, lua_api_cons_show);
    lua_setglobal(L, "prof_cons_show");
    lua_pushcfunction(L, lua_api_register_command);
    lua_setglobal(L, "prof_register_command");
    lua_pushcfunction(L, lua_api_register_timed);
    lua_setglobal(L, "prof_register_timed");
    lua_pushcfunction(L, lua_api_register_ac);
    lua_setglobal(L, "prof_register_ac");
    lua_pushcfunction(L, lua_api_send_line);
    lua_setglobal(L, "prof_send_line");
    lua_pushcfunction(L, lua_api_notify);
    lua_setglobal(L, "prof_notify");
    lua_pushcfunction(L, lua_api_get_current_recipient);
    lua_setglobal(L, "prof_get_current_recipient");
    lua_pushcfunction(L, lua_api_get_current_muc);
    lua_setglobal(L, "prof_get_current_muc");
    lua_pushcfunction(L, lua_api_log_debug);
    lua_setglobal(L, "prof_log_debug");
    lua_pushcfunction(L, lua_api_log_info);
    lua_setglobal(L, "prof_log_info");
    lua_pushcfunction(L, lua_api_log_warning);
    lua_setglobal(L, "prof_log_warning");
    lua_pushcfunction(L, lua_api_log_error);
    lua_setglobal(L, "prof_log_error");
    lua_pushcfunction(L, lua_api_win_exists);
    lua_setglobal(L, "prof_win_exists");
    lua_pushcfunction(L, lua_api_win_create);
    lua_setglobal(L, "prof_win_create");
    lua_pushcfunction(L, lua_api_win_focus);
    lua_setglobal(L, "prof_win_focus");
    lua_pushcfunction(L, lua_api_win_show);
    lua_setglobal(L, "prof_win_show");
    lua_pushcfunction(L, lua_api_win_show_green);
    lua_setglobal(L, "prof_win_show_green");
    lua_pushcfunction(L, lua_api_win_show_red);
    lua_setglobal(L, "prof_win_show_red");
    lua_pushcfunction(L, lua_api_win_show_cyan);
    lua_setglobal(L, "prof_win_show_cyan");
    lua_pushcfunction(L, lua_api_win_show_yellow);
    lua_setglobal(L, "prof_win_show_yellow");
}
