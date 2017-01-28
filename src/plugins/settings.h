/*
 * settings.h
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
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
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

#ifndef PLUGINS_SETTINGS_H
#define PLUGINS_SETTINGS_H

void plugin_settings_init(void);
void plugin_settings_close(void);

gboolean plugin_settings_boolean_get(const char *const group, const char *const key, gboolean def);
void plugin_settings_boolean_set(const char *const group, const char *const key, gboolean value);
char* plugin_settings_string_get(const char *const group, const char *const key, const char *const def);
void plugin_settings_string_set(const char *const group, const char *const key, const char *const value);
int plugin_settings_int_get(const char *const group, const char *const key, int def);
void plugin_settings_int_set(const char *const group, const char *const key, int value);
char** plugin_settings_string_list_get(const char *const group, const char *const key);
void plugin_settings_string_list_add(const char *const group, const char *const key, const char *const value);
int plugin_settings_string_list_remove(const char *const group, const char *const key, const char *const value);
int plugin_settings_string_list_clear(const char *const group, const char *const key);

#endif
