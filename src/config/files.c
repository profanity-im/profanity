/*
 * files.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2020 - 2025 Michael Vetter <jubalh@iodoru.org>
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
#include <fcntl.h>
#include <errno.h>
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
    auto_gchar gchar* xdg_config = _files_get_xdg_config_home();
    auto_gchar gchar* xdg_data = _files_get_xdg_data_home();

    GString* themes_dir = g_string_new(xdg_config);
    g_string_append(themes_dir, "/profanity/themes");
    GString* icons_dir = g_string_new(xdg_config);
    g_string_append(icons_dir, "/profanity/icons");
    GString* chatlogs_dir = g_string_new(xdg_data);
    g_string_append(chatlogs_dir, "/profanity/chatlogs");
    GString* logs_dir = g_string_new(xdg_data);
    g_string_append(logs_dir, "/profanity/logs");
    GString* plugins_dir = g_string_new(xdg_data);
    g_string_append(plugins_dir, "/profanity/plugins");

    if (!create_dir(themes_dir->str)) {
        log_error("Error while creating directory %s", themes_dir->str);
    }
    if (!create_dir(icons_dir->str)) {
        log_error("Error while creating directory %s", icons_dir->str);
    }
    if (!create_dir(chatlogs_dir->str)) {
        log_error("Error while creating directory %s", chatlogs_dir->str);
    }
    if (!create_dir(logs_dir->str)) {
        log_error("Error while creating directory %s", logs_dir->str);
    }
    if (!create_dir(plugins_dir->str)) {
        log_error("Error while creating directory %s", plugins_dir->str);
    }

    g_string_free(themes_dir, TRUE);
    g_string_free(icons_dir, TRUE);
    g_string_free(chatlogs_dir, TRUE);
    g_string_free(logs_dir, TRUE);
    g_string_free(plugins_dir, TRUE);
}

gchar*
files_get_inputrc_file(void)
{
    auto_gchar gchar* xdg_config = _files_get_xdg_config_home();
    GString* inputrc_file = g_string_new(xdg_config);

    g_string_append(inputrc_file, "/profanity/inputrc");

    if (g_file_test(inputrc_file->str, G_FILE_TEST_IS_REGULAR)) {
        gchar* result = g_strdup(inputrc_file->str);
        g_string_free(inputrc_file, TRUE);

        return result;
    }

    g_string_free(inputrc_file, TRUE);

    return NULL;
}

gchar*
files_get_log_file(const char* const log_file)
{
    auto_gchar gchar* xdg_data = _files_get_xdg_data_home();
    GString* logfile;

    if (log_file) {
        auto_gchar gchar* log_path = g_path_get_dirname(log_file);
        if (!create_dir(log_path)) {
            log_error("Error while creating directory %s", log_path);
        }

        logfile = g_string_new(log_file);
    } else {
        logfile = g_string_new(xdg_data);
        g_string_append(logfile, "/profanity/logs/profanity");

        if (!prefs_get_boolean(PREF_LOG_SHARED)) {
            g_string_append_printf(logfile, "%d", getpid());
        }

        g_string_append(logfile, ".log");
    }

    gchar* result = g_strdup(logfile->str);

    g_string_free(logfile, TRUE);

    return result;
}

gchar*
files_get_config_path(const char* const config_base)
{
    auto_gchar gchar* xdg_config = _files_get_xdg_config_home();
    gchar* result = g_strdup_printf("%s/profanity/%s", xdg_config, config_base);
    return result;
}

/**
 * Get the full path by appending the given location to the project base path.
 *
 * @param location The location (directory or file) to append to the project base path.
 * @return The full path obtained by appending the location to the project base path.
 *
 * @note Remember to free returned string using `auto_gchar` or `g_free()` when it is no longer needed to prevent memory leaks.
 */
gchar*
files_get_data_path(const char* const location)
{
    auto_gchar gchar* xdg_data = _files_get_xdg_data_home();
    return g_strdup_printf("%s/profanity/%s", xdg_data, location);
}

gchar*
files_get_download_path(const char* const jid)
{
    auto_gchar gchar* xdg_data = _files_get_xdg_data_home();

    if (jid) {
        auto_char char* account_dir = str_replace(jid, "@", "_at_");
        GDateTime* now = g_date_time_new_now_local();
        auto_gchar gchar* date = g_date_time_format(now, "%Y_%m_%d");
        g_date_time_unref(now);
        return g_strdup_printf("%s/profanity/%s/%s/%s", xdg_data, DIR_DOWNLOADS, account_dir, date);
    } else {
        return g_strdup_printf("%s/profanity/%s", xdg_data, DIR_DOWNLOADS);
    }
}

gchar*
files_get_account_data_path(const char* const specific_dir, const char* const jid)
{
    auto_gchar gchar* data_dir = files_get_data_path(specific_dir);
    auto_char char* account_dir = str_replace(jid, "@", "_at_");

    return g_strdup_printf("%s/%s", data_dir, account_dir);
}

gchar*
files_file_in_account_data_path(const char* const specific_dir, const char* const jid, const char* const file_name)
{
    gchar* data_path = files_get_account_data_path(specific_dir, jid);

    if (!create_dir(data_path)) {
        g_free(data_path);
        return NULL;
    }

    if (!file_name) {
        return data_path;
    }
    gchar* filename = g_strdup_printf("%s/%s", data_path, file_name);
    g_free(data_path);
    return filename;
}

static char*
_files_get_xdg_config_home(void)
{
    gchar* xdg_config_home_env = getenv("XDG_CONFIG_HOME");
    gchar* xdg_config_home = NULL;

    if (xdg_config_home_env) {
        xdg_config_home = strdup(xdg_config_home_env);
        g_strstrip(xdg_config_home);
    }

    if (xdg_config_home && (strcmp(xdg_config_home, "") != 0)) {
        return xdg_config_home;
    } else {
        GString* default_path = g_string_new(getenv("HOME"));
        g_string_append(default_path, "/.config");
        char* result = strdup(default_path->str);
        g_string_free(default_path, TRUE);

        return result;
    }
}

static char*
_files_get_xdg_data_home(void)
{
    gchar* xdg_data_home_env = getenv("XDG_DATA_HOME");
    gchar* xdg_data_home = NULL;

    if (xdg_data_home_env) {
        xdg_data_home = strdup(xdg_data_home_env);
        g_strstrip(xdg_data_home);
    }

    if (xdg_data_home && (strcmp(xdg_data_home, "") != 0)) {
        return xdg_data_home;
    } else {
        GString* default_path = g_string_new(getenv("HOME"));
        g_string_append(default_path, "/.local/share");
        gchar* result = strdup(default_path->str);
        g_string_free(default_path, TRUE);

        return result;
    }
}
