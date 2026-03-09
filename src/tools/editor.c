/*
 * editor.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2022 - 2026 Michael Vetter <jubalh@iodoru.org>
 * Copyright (C) 2022 MarcoPolo PasTonMolo <marcopolopastonmolo@protonmail.com>
 * Copyright (C) 2022 Steffen Jaeckel <jaeckel-floss@eyet-services.de>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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

    auto_gchar gchar* filename = NULL;
    auto_gerror GError* glib_error = NULL;
    const char* jid = connection_get_barejid();
    if (jid) {
        filename = files_file_in_account_data_path(DIR_EDITOR, jid, "compose.md");
    } else {
        log_debug("[Editor] could not get JID");
        auto_gchar gchar* data_dir = files_get_data_path(DIR_EDITOR);
        if (!create_dir(data_dir)) {
            return TRUE;
        }
        filename = g_strdup_printf("%s/compose.md", data_dir);
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
        log_error("[Editor] could not write to %s: %s", filename, PROF_GERROR_MESSAGE(glib_error));
        return TRUE;
    }

    auto_gchar gchar* editor = prefs_get_string(PREF_COMPOSE_EDITOR);
    auto_gchar gchar* editor_with_filename = g_strdup_printf("%s %s", editor, filename);
    auto_gcharv gchar** editor_argv = g_strsplit(editor_with_filename, " ", 0);

    if (!editor_argv || !editor_argv[0]) {
        log_error("[Editor] Failed to parse editor command: %s", editor);
        return TRUE;
    }

    // Fork / exec
    pid_t pid = fork();
    if (pid == 0) {
        if (editor_argv && editor_argv[0]) {
            int x = execvp(editor_argv[0], editor_argv);
            if (x == -1)
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
            log_error("[Editor] could not read from %s: %s", filename, PROF_GERROR_MESSAGE(glib_error));
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
    }

    return FALSE;
}
