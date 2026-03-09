/*
 * settings.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef PLUGINS_SETTINGS_H
#define PLUGINS_SETTINGS_H

void plugin_settings_init(void);
void plugin_settings_close(void);

gboolean plugin_settings_boolean_get(const char* const group, const char* const key, gboolean def);
void plugin_settings_boolean_set(const char* const group, const char* const key, gboolean value);
char* plugin_settings_string_get(const char* const group, const char* const key, const char* const def);
void plugin_settings_string_set(const char* const group, const char* const key, const char* const value);
int plugin_settings_int_get(const char* const group, const char* const key, int def);
void plugin_settings_int_set(const char* const group, const char* const key, int value);
char** plugin_settings_string_list_get(const char* const group, const char* const key);
void plugin_settings_string_list_add(const char* const group, const char* const key, const char* const value);
int plugin_settings_string_list_remove(const char* const group, const char* const key, const char* const value);
int plugin_settings_string_list_clear(const char* const group, const char* const key);

#endif
