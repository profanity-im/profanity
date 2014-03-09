/*
 * lua_api.h
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

#ifndef LUA_API_H
#define LUA_API_H

#include <lua.h>

lua_State * lua_get_state(void);
void l_stackdump(lua_State *L);

void lua_env_init(void);
void lua_api_init(lua_State *L);
void lua_shutdown(void);

void lua_command_callback(PluginCommand *command, gchar **args);
void lua_timed_callback(PluginTimedFunction *timed_function);
void lua_window_callback(PluginWindowCallback *window_callback, char *tag, char *line);

#endif
