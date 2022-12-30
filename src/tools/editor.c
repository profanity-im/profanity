/*
 * editor.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2022 - 2023 Michael Vetter <jubalh@iodoru.org>
 * Copyright (C) 2022 MarcoPolo PasTonMolo <marcopolopastonmolo@protonmail.com>
 * Copyright (C) 2022 Steffen Jaeckel <jaeckel-floss@eyet-services.de>
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
#include <glib.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

#include "config/files.h"
#include "config/preferences.h"
#include "log.h"
#include "common.h"
#include "xmpp/xmpp.h"

// Returns true if an error occurred
gboolean
get_message_from_editor(gchar* message, gchar** returned_message)
{
    /* Make sure that there's no junk in the return-pointer in error cases */
    *returned_message = NULL;

    gchar* filename = NULL;
    GError* glib_error = NULL;
    auto_char char* jid = connection_get_barejid();
    if (jid) {
        filename = files_file_in_account_data_path(DIR_EDITOR, jid, "compose.md");
    } else {
        log_debug("[Editor] could not get JID");
        gchar* data_dir = files_get_data_path(DIR_EDITOR);
        if (!create_dir(data_dir)) {
            return TRUE;
        }
        filename = g_strdup_printf("%s/compose.md", data_dir);
        g_free(data_dir);
    }
    if (!filename) {
        log_error("[Editor] something went wrong while creating compose file");
        return TRUE;
    }

    gsize messagelen = 0;
    if (message != NULL) {
        messagelen = strlen(message);
    }

    if (!g_file_set_contents(filename, message, messagelen, &glib_error)) {
        log_error("[Editor] could not write to %s: %s", filename, glib_error ? glib_error->message : "No GLib error given");
        if (glib_error) {
            g_error_free(glib_error);
        }
        g_free(filename);
        return TRUE;
    }

    char* editor = prefs_get_string(PREF_COMPOSE_EDITOR);

    // Fork / exec
    pid_t pid = fork();
    if (pid == 0) {
        int x = execlp(editor, editor, filename, (char*)NULL);
        if (x == -1) {
            log_error("[Editor] Failed to exec %s", editor);
        }
        _exit(EXIT_FAILURE);
    } else {
        if (pid == -1) {
            return TRUE;
        }
        waitpid(pid, NULL, 0);

        gchar* contents;
        gsize length;
        if (!g_file_get_contents(filename, &contents, &length, &glib_error)) {
            log_error("[Editor] could not read from %s: %s", filename, glib_error ? glib_error->message : "No GLib error given");
            if (glib_error) {
                g_error_free(glib_error);
            }
            g_free(filename);
            g_free(editor);
            return TRUE;
        }
        /* Remove all trailing new-line characters */
        g_strchomp(contents);
        *returned_message = contents;
        if (remove(filename) != 0) {
            log_error("[Editor] error during file deletion of %s", filename);
        } else {
            log_debug("[Editor] deleted file: %s", filename);
        }
        g_free(filename);
    }

    g_free(editor);

    return FALSE;
}
