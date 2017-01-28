/*
 * files.c
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
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>

#include "common.h"
#include "log.h"
#include "config/files.h"
#include "config/preferences.h"

static char* _files_get_xdg_config_home(void);
static char* _files_get_xdg_data_home(void);

void
files_create_directories(void)
{
    gchar *xdg_config = _files_get_xdg_config_home();
    gchar *xdg_data = _files_get_xdg_data_home();

    GString *themes_dir = g_string_new(xdg_config);
    g_string_append(themes_dir, "/profanity/themes");
    GString *icons_dir = g_string_new(xdg_config);
    g_string_append(icons_dir, "/profanity/icons");
    GString *chatlogs_dir = g_string_new(xdg_data);
    g_string_append(chatlogs_dir, "/profanity/chatlogs");
    GString *logs_dir = g_string_new(xdg_data);
    g_string_append(logs_dir, "/profanity/logs");
    GString *plugins_dir = g_string_new(xdg_data);
    g_string_append(plugins_dir, "/profanity/plugins");

    if (!mkdir_recursive(themes_dir->str)) {
        log_error("Error while creating directory %s", themes_dir->str);
    }
    if (!mkdir_recursive(icons_dir->str)) {
        log_error("Error while creating directory %s", icons_dir->str);
    }
    if (!mkdir_recursive(chatlogs_dir->str)) {
        log_error("Error while creating directory %s", chatlogs_dir->str);
    }
    if (!mkdir_recursive(logs_dir->str)) {
        log_error("Error while creating directory %s", logs_dir->str);
    }
    if (!mkdir_recursive(plugins_dir->str)) {
        log_error("Error while creating directory %s", plugins_dir->str);
    }

    g_string_free(themes_dir, TRUE);
    g_string_free(icons_dir, TRUE);
    g_string_free(chatlogs_dir, TRUE);
    g_string_free(logs_dir, TRUE);
    g_string_free(plugins_dir, TRUE);

    g_free(xdg_config);
    g_free(xdg_data);
}

char*
files_get_inputrc_file(void)
{
    gchar *xdg_config = _files_get_xdg_config_home();
    GString *inputrc_file = g_string_new(xdg_config);
    g_free(xdg_config);

    g_string_append(inputrc_file, "/profanity/inputrc");

    if (g_file_test(inputrc_file->str, G_FILE_TEST_IS_REGULAR)) {
        char *result = strdup(inputrc_file->str);
        g_string_free(inputrc_file, TRUE);

        return result;
    }

    g_string_free(inputrc_file, TRUE);

    return NULL;
}

char*
files_get_log_file(void)
{
    gchar *xdg_data = _files_get_xdg_data_home();
    GString *logfile = g_string_new(xdg_data);
    g_string_append(logfile, "/profanity/logs/profanity");
    if (!prefs_get_boolean(PREF_LOG_SHARED)) {
        g_string_append_printf(logfile, "%d", getpid());
    }
    g_string_append(logfile, ".log");
    char *result = strdup(logfile->str);
    free(xdg_data);
    g_string_free(logfile, TRUE);

    return result;
}

char*
files_get_config_path(char *config_base)
{
    gchar *xdg_config = _files_get_xdg_config_home();
    GString *file_str = g_string_new(xdg_config);
    g_string_append(file_str, "/profanity/");
    g_string_append(file_str, config_base);
    char *result = strdup(file_str->str);
    g_free(xdg_config);
    g_string_free(file_str, TRUE);

    return result;
}

char*
files_get_data_path(char *data_base)
{
    gchar *xdg_data = _files_get_xdg_data_home();
    GString *file_str = g_string_new(xdg_data);
    g_string_append(file_str, "/profanity/");
    g_string_append(file_str, data_base);
    char *result = strdup(file_str->str);
    g_free(xdg_data);
    g_string_free(file_str, TRUE);

    return result;
}

static char*
_files_get_xdg_config_home(void)
{
    gchar *xdg_config_home = getenv("XDG_CONFIG_HOME");
    if (xdg_config_home)
        g_strstrip(xdg_config_home);

    if (xdg_config_home && (strcmp(xdg_config_home, "") != 0)) {
        return strdup(xdg_config_home);
    } else {
        GString *default_path = g_string_new(getenv("HOME"));
        g_string_append(default_path, "/.config");
        char *result = strdup(default_path->str);
        g_string_free(default_path, TRUE);

        return result;
    }
}

static char*
_files_get_xdg_data_home(void)
{
    gchar *xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home)
        g_strstrip(xdg_data_home);

    if (xdg_data_home && (strcmp(xdg_data_home, "") != 0)) {
        return strdup(xdg_data_home);
    } else {
        GString *default_path = g_string_new(getenv("HOME"));
        g_string_append(default_path, "/.local/share");
        gchar *result = strdup(default_path->str);
        g_string_free(default_path, TRUE);

        return result;
    }
}
