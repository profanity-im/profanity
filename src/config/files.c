/*
 * files.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2020 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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

void
files_create_directories(void)
{
    const gchar* const config_dirs[] = { DIR_THEMES, DIR_ICONS, NULL };
    for (int i = 0; config_dirs[i]; i++) {
        auto_gchar gchar* dir = files_get_config_path(config_dirs[i]);
        if (!create_dir(dir)) {
            log_error("Error while creating directory %s", dir);
        }
    }

    const gchar* const data_dirs[] = { DIR_CHATLOGS, DIR_LOGS, DIR_PLUGINS, NULL };
    for (int i = 0; data_dirs[i]; i++) {
        auto_gchar gchar* dir = files_get_data_path(data_dirs[i]);
        if (!create_dir(dir)) {
            log_error("Error while creating directory %s", dir);
        }
    }
}

gchar*
files_get_inputrc_file(void)
{
    gchar* inputrc_file = files_get_config_path("inputrc");

    if (g_file_test(inputrc_file, G_FILE_TEST_IS_REGULAR)) {
        return inputrc_file;
    }

    g_free(inputrc_file);
    return NULL;
}

gchar*
files_get_log_file(const gchar* const log_file)
{
    if (log_file) {
        auto_gchar gchar* log_path = g_path_get_dirname(log_file);
        if (!create_dir(log_path)) {
            log_error("Error while creating directory %s", log_path);
        }

        return g_strdup(log_file);
    }

    auto_gchar gchar* base = files_get_data_path(DIR_LOGS "/profanity");
    if (!prefs_get_boolean(PREF_LOG_SHARED)) {
        return g_strdup_printf("%s%d.log", base, getpid());
    }

    return g_strdup_printf("%s.log", base);
}

gchar*
files_get_config_path(const gchar* const config_base)
{
    return g_build_filename(g_get_user_config_dir(), "profanity", config_base, NULL);
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
files_get_data_path(const gchar* const location)
{
    return g_build_filename(g_get_user_data_dir(), "profanity", location, NULL);
}

gchar*
files_sanitize_jid(const gchar* const jid)
{
    if (jid == NULL) {
        return NULL;
    }

    GString* s = g_string_new(jid);
    g_string_replace(s, "@", "_at_", 0);

    return g_string_free(s, FALSE);
}

gchar*
files_get_download_path(const gchar* const jid)
{
    if (jid) {
        auto_gchar gchar* account_dir = files_sanitize_jid(jid);
        g_autoptr(GDateTime) now = g_date_time_new_now_local();
        auto_gchar gchar* date = g_date_time_format(now, "%Y_%m_%d");
        return g_build_filename(g_get_user_data_dir(), "profanity", DIR_DOWNLOADS, account_dir, date, NULL);
    } else {
        return g_build_filename(g_get_user_data_dir(), "profanity", DIR_DOWNLOADS, NULL);
    }
}

gchar*
files_get_account_data_path(const gchar* const specific_dir, const gchar* const jid)
{
    auto_gchar gchar* data_dir = files_get_data_path(specific_dir);
    auto_gchar gchar* account_dir = files_sanitize_jid(jid);

    return g_build_filename(data_dir, account_dir, NULL);
}

gchar*
files_file_in_account_data_path(const gchar* const specific_dir, const gchar* const jid, const gchar* const file_name)
{
    gchar* data_path = files_get_account_data_path(specific_dir, jid);

    if (!create_dir(data_path)) {
        g_free(data_path);
        return NULL;
    }

    if (!file_name) {
        return data_path;
    }
    gchar* filename = g_build_filename(data_path, file_name, NULL);
    g_free(data_path);
    return filename;
}
