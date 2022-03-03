/*
 * editor.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2022 Michael Vetter <jubalh@iodoru.org>
 * Copyright (C) 2022 MarcoPolo PasTonMolo  <marcopolopastonmolo@protonmail.com>
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

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <glib.h>
#include <gio/gio.h>

#include "config/files.h"
#include "config/preferences.h"
#include "ui/ui.h"

// Returns true if an error occurred
gboolean
get_message_from_editor(gchar* message, gchar** returned_message)
{
    // create editor dir if not present
    char* jid = connection_get_barejid();
    if (!jid) {
        return TRUE;
    }
    gchar* path = files_get_account_data_path(DIR_EDITOR, jid);
    free(jid);
    if (g_mkdir_with_parents(path, S_IRWXU) != 0) {
        cons_show_error("Failed to create directory at '%s' with error '%s'", path, strerror(errno));
        g_free(path);
        return TRUE;
    }

    // build temp file name. Example: /home/user/.local/share/profanity/editor/jid/compose.md
    char* filename = g_strdup_printf("%s/compose.md", path);
    g_free(path);

    GError* creation_error = NULL;
    GFile* file = g_file_new_for_path(filename);
    GFileOutputStream* fos = g_file_create(file, G_FILE_CREATE_PRIVATE, NULL, &creation_error);

    free(filename);

    if (message != NULL && strlen(message) > 0) {
        int fd_output_file = open(g_file_get_path(file), O_WRONLY);
        if (fd_output_file < 0) {
            cons_show_error("Editor: Could not open file '%s': %s", file, strerror(errno));
            return TRUE;
        }
        if (-1 == write(fd_output_file, message, strlen(message))) {
            cons_show_error("Editor: failed to write '%s' to file: %s", message, strerror(errno));
            return TRUE;
        }
        close(fd_output_file);
    }

    if (creation_error) {
        cons_show_error("Editor: could not create temp file");
        return TRUE;
    }
    g_object_unref(fos);

    char* editor = prefs_get_string(PREF_COMPOSE_EDITOR);

    // Fork / exec
    pid_t pid = fork();
    if (pid == 0) {
        int x = execlp(editor, editor, g_file_get_path(file), (char*)NULL);
        if (x == -1) {
            cons_show_error("Editor:Failed to exec %s", editor);
        }
        _exit(EXIT_FAILURE);
    } else {
        if (pid == -1) {
            return TRUE;
        }
        int status = 0;
        waitpid(pid, &status, 0);
        int fd_input_file = open(g_file_get_path(file), O_RDONLY);
        const size_t COUNT = 8192;
        char buf[COUNT];
        ssize_t size_read = read(fd_input_file, buf, COUNT);
        if (size_read > 0 && size_read <= COUNT) {
            buf[size_read - 1] = '\0';
            GString* text = g_string_new(buf);
            *returned_message = g_strdup(text->str);
            g_string_free(text, TRUE);
        } else {
            *returned_message = g_strdup("");
        }
        close(fd_input_file);

        GError* deletion_error = NULL;
        g_file_delete(file, NULL, &deletion_error);
        if (deletion_error) {
            cons_show("Editor: error during file deletion");
            g_free(*returned_message);
            return TRUE;
        }
        g_object_unref(file);
    }

    g_free(editor);

    return FALSE;
}
