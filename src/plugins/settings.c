/*
 * settings.c
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

#include <string.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "common.h"
#include "config/theme.h"
#include "config/files.h"
#include "config/conflists.h"

static GKeyFile *settings;

static void _save_settings(void);

void
plugin_settings_init(void)
{
    char *settings_file = files_get_data_path(FILE_PLUGIN_SETTINGS);

    if (g_file_test(settings_file, G_FILE_TEST_EXISTS)) {
        g_chmod(settings_file, S_IRUSR | S_IWUSR);
    }

    settings = g_key_file_new();
    g_key_file_load_from_file(settings, settings_file, G_KEY_FILE_KEEP_COMMENTS, NULL);

    gsize g_data_size;
    gchar *g_data = g_key_file_to_data(settings, &g_data_size, NULL);
    g_file_set_contents(settings_file, g_data, g_data_size, NULL);
    g_chmod(settings_file, S_IRUSR | S_IWUSR);
    g_free(g_data);
    free(settings_file);
}

void
plugin_settings_close(void)
{
    g_key_file_free(settings);
    settings = NULL;
}

gboolean
plugin_settings_boolean_get(const char *const group, const char *const key, gboolean def)
{
    if (group && key && g_key_file_has_key(settings, group, key, NULL)) {
        return g_key_file_get_boolean(settings, group, key, NULL);
    } else {
        return def;
    }
}

void
plugin_settings_boolean_set(const char *const group, const char *const key, gboolean value)
{
    g_key_file_set_boolean(settings, group, key, value);
    _save_settings();
}

char*
plugin_settings_string_get(const char *const group, const char *const key, const char *const def)
{
    if (group && key && g_key_file_has_key(settings, group, key, NULL)) {
        return g_key_file_get_string(settings, group, key, NULL);
    } else if (def) {
        return strdup(def);
    } else {
        return NULL;
    }
}

void
plugin_settings_string_set(const char *const group, const char *const key, const char *const value)
{
    g_key_file_set_string(settings, group, key, value);
    _save_settings();
}

int
plugin_settings_int_get(const char *const group, const char *const key, int def)
{
    if (group && key && g_key_file_has_key(settings, group, key, NULL)) {
        return g_key_file_get_integer(settings, group, key, NULL);
    } else {
        return def;
    }
}

void
plugin_settings_int_set(const char *const group, const char *const key, int value)
{
    g_key_file_set_integer(settings, group, key, value);
    _save_settings();
}

gchar**
plugin_settings_string_list_get(const char *const group, const char *const key)
{
    if (!g_key_file_has_key(settings, group, key, NULL)) {
        return NULL;
    }

    return g_key_file_get_string_list(settings, group, key, NULL, NULL);
}

int
plugin_settings_string_list_add(const char *const group, const char *const key, const char *const value)
{
    int res = conf_string_list_add(settings, group, key, value);
    _save_settings();

    return res;
}

int
plugin_settings_string_list_remove(const char *const group, const char *const key, const char *const value)
{
    int res = conf_string_list_remove(settings, group, key, value);
    _save_settings();

    return res;
}

int
plugin_settings_string_list_clear(const char *const group, const char *const key)
{
    if (!g_key_file_has_key(settings, group, key, NULL)) {
        return 0;
    }

    g_key_file_remove_key(settings, group, key, NULL);
    _save_settings();

    return 1;
}

static void
_save_settings(void)
{
    gsize g_data_size;
    gchar *g_data = g_key_file_to_data(settings, &g_data_size, NULL);

    char *fileloc = files_get_data_path(FILE_PLUGIN_SETTINGS);
    gchar *base = g_path_get_basename(fileloc);
    gchar *true_loc = get_file_or_linked(fileloc, base);
    g_free(base);
    g_file_set_contents(true_loc, g_data, g_data_size, NULL);
    free(true_loc);
    g_free(g_data);
    g_chmod(fileloc, S_IRUSR | S_IWUSR);
    free(fileloc);
}
