/*
 * scripts.c
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
    char *scriptsdir = files_get_data_path(DIR_SCRIPTS);

    // mkdir if doesn't exist
    errno = 0;
    int res = g_mkdir_with_parents(scriptsdir, S_IRWXU);
    if (res == -1) {
        char *errmsg = strerror(errno);
        if (errmsg) {
            log_error("Error creating directory: %s, %s", scriptsdir, errmsg);
        } else {
            log_error("Error creating directory: %s", scriptsdir);
        }
    }

    free(scriptsdir);
}

GSList*
scripts_list(void)
{
    char *scriptsdir = files_get_data_path(DIR_SCRIPTS);

    GSList *result = NULL;
    GDir *scripts = g_dir_open(scriptsdir, 0, NULL);
    free(scriptsdir);

    if (scripts) {
        const gchar *script = g_dir_read_name(scripts);
        while (script) {
            result = g_slist_append(result, strdup(script));
            script = g_dir_read_name(scripts);
        }
        g_dir_close(scripts);
    }

    return result;
}

GSList*
scripts_read(const char *const script)
{
    char *scriptsdir = files_get_data_path(DIR_SCRIPTS);
    GString *scriptpath = g_string_new(scriptsdir);
    free(scriptsdir);
    g_string_append(scriptpath, "/");
    g_string_append(scriptpath, script);

    FILE *scriptfile = g_fopen(scriptpath->str, "r");
    if (!scriptfile) {
        log_info("Script not found: %s", scriptpath->str);
        g_string_free(scriptpath, TRUE);
        return NULL;
    }

    g_string_free(scriptpath, TRUE);

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    GSList *result = NULL;

    while ((read = getline(&line, &len, scriptfile)) != -1) {
        if (g_str_has_suffix(line, "\n")) {
            result = g_slist_append(result, g_strndup(line, strlen(line) -1));
        } else {
            result = g_slist_append(result, strdup(line));
        }
    }

    fclose(scriptfile);
    if (line) free(line);

    return result;
}

gboolean
scripts_exec(const char *const script)
{
    char *scriptsdir = files_get_data_path(DIR_SCRIPTS);
    GString *scriptpath = g_string_new(scriptsdir);
    free(scriptsdir);
    g_string_append(scriptpath, "/");
    g_string_append(scriptpath, script);

    FILE *scriptfile = g_fopen(scriptpath->str, "r");
    if (!scriptfile) {
        log_info("Script not found: %s", scriptpath->str);
        g_string_free(scriptpath, TRUE);
        return FALSE;
    }

    g_string_free(scriptpath, TRUE);

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, scriptfile)) != -1) {
        ProfWin *win = wins_get_current();
        cmd_process_input(win, line);
        session_process_events();
        ui_update();
    }

    fclose(scriptfile);
    if (line) free(line);

    return TRUE;
}

