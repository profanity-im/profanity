/*
 * callbacks.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PLUGINS_CALLBACKS_H
#define PLUGINS_CALLBACKS_H

#include <glib.h>

#include "command/cmd_defs.h"

#ifdef HAVE_PLUGINS
#include <lua.h>
#endif

typedef struct p_command
{
    char* command_name;
    int min_args;
    int max_args;
    CommandHelp* help;
#ifdef HAVE_PLUGINS
    lua_State* L;
    int lua_ref;
#endif
} PluginCommand;

typedef struct p_timed_function
{
    int interval_seconds;
    GTimer* timer;
#ifdef HAVE_PLUGINS
    lua_State* L;
    int lua_ref;
#endif
} PluginTimedFunction;

typedef struct p_window_input_callback
{
#ifdef HAVE_PLUGINS
    lua_State* L;
    int lua_ref;
#endif
} PluginWindowCallback;

void callbacks_init(void);
void callbacks_remove(const char* const plugin_name);
void callbacks_close(void);

void callbacks_add_command(const char* const plugin_name, PluginCommand* command);
void callbacks_add_timed(const char* const plugin_name, PluginTimedFunction* timed_function);
gboolean callbacks_win_exists(const char* const plugin_name, const char* tag);
void callbacks_add_window_handler(const char* const plugin_name, const char* tag, PluginWindowCallback* window_callback);
void* callbacks_get_window_handler(const char* tag);
void callbacks_remove_win(const char* const plugin_name, const char* const tag);

#endif
