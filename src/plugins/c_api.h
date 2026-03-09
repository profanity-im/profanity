/*
 * c_api.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PLUGINS_C_API_H
#define PLUGINS_C_API_H

#include <glib.h>

void c_api_init(void);

void c_command_callback(PluginCommand* command, gchar** args);
void c_timed_callback(PluginTimedFunction* timed_function);
void c_window_callback(PluginWindowCallback* window_callback, char* tag, char* line);

#endif
