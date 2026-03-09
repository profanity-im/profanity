/*
 * python_api.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PLUGINS_PYTHON_API_H
#define PLUGINS_PYTHON_API_H

void python_env_init(void);
void python_init_prof(void);
void python_shutdown(void);

void python_command_callback(PluginCommand* command, gchar** args);
void python_timed_callback(PluginTimedFunction* timed_function);
void python_window_callback(PluginWindowCallback* window_callback, char* tag, char* line);

char* python_str_or_unicode_to_string(void* obj);

#endif
