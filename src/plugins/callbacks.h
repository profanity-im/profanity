/*
 * callbacks.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PLUGINS_CALLBACKS_H
#define PLUGINS_CALLBACKS_H

#include <glib.h>

#include "command/cmd_defs.h"

typedef struct p_command
{
    char* command_name;
    int min_args;
    int max_args;
    CommandHelp* help;
    void* callback;
    void (*callback_exec)(struct p_command* command, gchar** args);
    void (*callback_destroy)(void* callback);
} PluginCommand;

typedef struct p_timed_function
{
    void* callback;
    void (*callback_exec)(struct p_timed_function* timed_function);
    void (*callback_destroy)(void* callback);
    int interval_seconds;
    GTimer* timer;
} PluginTimedFunction;

typedef struct p_window_input_callback
{
    void* callback;
    void (*callback_exec)(struct p_window_input_callback* window_callback, const char* tag, const char* const line);
    void (*callback_destroy)(void* callback);
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
