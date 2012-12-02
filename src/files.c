/*
 * files.c
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "xdg_base.h"

#include <glib.h>

static void _files_create_config_directory(void);
static void _files_create_data_directory(void);
static void _files_create_chatlog_directory(void);
static void _files_create_log_directory(void);
static void _files_create_themes_directory(void);
static void _create_dir(char *name);
static void _mkdir_recursive(const char *dir);

void
files_create_directories(void)
{
    _files_create_config_directory();
    _files_create_data_directory();
    _files_create_chatlog_directory();
    _files_create_log_directory();
    _files_create_themes_directory();
}

gchar *
files_get_chatlog_dir(void)
{
    gchar *xdg_data = xdg_get_data_home();
    GString *chatlogs_dir = g_string_new(xdg_data);
    g_string_append(chatlogs_dir, "/profanity/chatlogs");
    gchar *result = strdup(chatlogs_dir->str);
    g_free(xdg_data);
    g_string_free(chatlogs_dir, TRUE);

    return result;
}

gchar *
files_get_preferences_file(void)
{
    gchar *xdg_config = xdg_get_config_home();
    GString *prefs_file = g_string_new(xdg_config);
    g_string_append(prefs_file, "/profanity/profrc");
    gchar *result = strdup(prefs_file->str);
    g_free(xdg_config);
    g_string_free(prefs_file, TRUE);

    return result;
}

gchar *
files_get_log_file(void)
{
    gchar *xdg_data = xdg_get_data_home();
    GString *logfile = g_string_new(xdg_data);
    g_string_append(logfile, "/profanity/logs/profanity.log");
    gchar *result = strdup(logfile->str);
    g_free(xdg_data);
    g_string_free(logfile, TRUE);

    return result;
}

gchar *
files_get_themes_dir(void)
{
    gchar *xdg_config = xdg_get_config_home();
    GString *themes_dir = g_string_new(xdg_config);
    g_string_append(themes_dir, "/profanity/themes");
    gchar *result = strdup(themes_dir->str);
    g_free(xdg_config);
    g_string_free(themes_dir, TRUE);

    return result;
}

static void
_files_create_config_directory(void)
{
    gchar *xdg_config = xdg_get_config_home();
    GString *prof_conf_dir = g_string_new(xdg_config);
    g_string_append(prof_conf_dir, "/profanity");
    _mkdir_recursive(prof_conf_dir->str);
    g_free(xdg_config);
    g_string_free(prof_conf_dir, TRUE);
}

static void
_files_create_data_directory(void)
{
    gchar *xdg_data = xdg_get_data_home();
    GString *prof_data_dir = g_string_new(xdg_data);
    g_string_append(prof_data_dir, "/profanity");
    _mkdir_recursive(prof_data_dir->str);
    g_free(xdg_data);
    g_string_free(prof_data_dir, TRUE);
}

static void
_files_create_chatlog_directory(void)
{
    gchar *xdg_data = xdg_get_data_home();
    GString *chatlogs_dir = g_string_new(xdg_data);
    g_string_append(chatlogs_dir, "/profanity/chatlogs");
    _mkdir_recursive(chatlogs_dir->str);
    g_free(xdg_data);
    g_string_free(chatlogs_dir, TRUE);
}

static void
_files_create_log_directory(void)
{
    gchar *xdg_data = xdg_get_data_home();
    GString *chatlogs_dir = g_string_new(xdg_data);
    g_string_append(chatlogs_dir, "/profanity/logs");
    _mkdir_recursive(chatlogs_dir->str);
    g_free(xdg_data);
    g_string_free(chatlogs_dir, TRUE);
}

static void
_files_create_themes_directory(void)
{
    gchar *xdg_config = xdg_get_config_home();
    GString *themes_dir = g_string_new(xdg_config);
    g_string_append(themes_dir, "/profanity/themes");
    _mkdir_recursive(themes_dir->str);
    g_free(xdg_config);
    g_string_free(themes_dir, TRUE);
}

static void
_create_dir(char *name)
{
    int e;
    struct stat sb;

    e = stat(name, &sb);
    if (e != 0)
        if (errno == ENOENT)
            e = mkdir(name, S_IRWXU);
}

static void
_mkdir_recursive(const char *dir)
{
    int i;
    for (i = 1; i <= strlen(dir); i++) {
        if (dir[i] == '/' || dir[i] == '\0') {
            gchar *next_dir = g_strndup(dir, i);
            _create_dir(next_dir);
            g_free(next_dir);
        }
    }
}

