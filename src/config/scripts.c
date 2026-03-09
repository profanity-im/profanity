/*
 * scripts.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "common.h"
#include "log.h"
#include "config/files.h"
#include "command/cmd_defs.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/xmpp.h"

void
scripts_init(void)
{
    auto_gchar gchar* scriptsdir = files_get_data_path(DIR_SCRIPTS);

    // mkdir if doesn't exist
    errno = 0;
    int res = g_mkdir_with_parents(scriptsdir, S_IRWXU);
    if (res == -1) {
        const char* errmsg = strerror(errno);
        if (errmsg) {
            log_error("Error creating directory: %s, %s", scriptsdir, errmsg);
        } else {
            log_error("Error creating directory: %s", scriptsdir);
        }
    }
}

GSList*
scripts_list(void)
{
    auto_gchar gchar* scriptsdir = files_get_data_path(DIR_SCRIPTS);

    GSList* result = NULL;
    GDir* scripts = g_dir_open(scriptsdir, 0, NULL);

    if (scripts) {
        const gchar* script = g_dir_read_name(scripts);
        while (script) {
            result = g_slist_append(result, strdup(script));
            script = g_dir_read_name(scripts);
        }
        g_dir_close(scripts);
    }

    return result;
}

GSList*
scripts_read(const char* const script)
{
    auto_gchar gchar* scriptsdir = files_get_data_path(DIR_SCRIPTS);
    GString* scriptpath = g_string_new(scriptsdir);
    g_string_append(scriptpath, "/");
    g_string_append(scriptpath, script);

    FILE* scriptfile = g_fopen(scriptpath->str, "r");
    if (!scriptfile) {
        log_info("Script not found: %s", scriptpath->str);
        g_string_free(scriptpath, TRUE);
        return NULL;
    }

    g_string_free(scriptpath, TRUE);

    auto_char char* line = NULL;
    size_t len = 0;
    GSList* result = NULL;

    while (getline(&line, &len, scriptfile) != -1) {
        if (g_str_has_suffix(line, "\n")) {
            result = g_slist_append(result, g_strndup(line, strlen(line) - 1));
        } else {
            result = g_slist_append(result, strdup(line));
        }
    }

    fclose(scriptfile);

    return result;
}

gboolean
scripts_exec(const char* const script)
{
    auto_gchar gchar* scriptsdir = files_get_data_path(DIR_SCRIPTS);
    GString* scriptpath = g_string_new(scriptsdir);
    g_string_append(scriptpath, "/");
    g_string_append(scriptpath, script);

    FILE* scriptfile = g_fopen(scriptpath->str, "r");
    if (!scriptfile) {
        log_info("Script not found: %s", scriptpath->str);
        g_string_free(scriptpath, TRUE);
        return FALSE;
    }

    g_string_free(scriptpath, TRUE);

    auto_char char* line = NULL;
    size_t len = 0;

    while (getline(&line, &len, scriptfile) != -1) {
        ProfWin* win = wins_get_current();
        cmd_process_input(win, line);
        session_process_events();
        ui_update();
    }

    fclose(scriptfile);

    return TRUE;
}
