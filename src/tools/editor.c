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
#include "ui/ui.h"

typedef struct EditorContext
{
    gchar* filename;
    void (*callback)(gchar* content, void* user_data);
    void* user_data;
} EditorContext;

static void
_editor_exit_cb(GPid pid, gint status, gpointer data)
{
    EditorContext* ctx = data;
    gchar* contents = NULL;
    GError* error = NULL;

    ui_resume();
    ui_resize();

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        if (g_file_get_contents(ctx->filename, &contents, NULL, &error)) {
            g_strchomp(contents);
            ctx->callback(contents, ctx->user_data);
            g_free(contents);
        } else {
            log_error("[Editor] could not read from %s: %s", ctx->filename, error->message);
            cons_show_error("Could not read edited content: %s", error->message);
            g_error_free(error);
        }
    } else {
        cons_show_error("Editor exited with error status %d", WEXITSTATUS(status));
    }

    if (remove(ctx->filename) != 0) {
        log_error("[Editor] error during file deletion of %s", ctx->filename);
    }

    g_free(ctx->filename);
    g_free(ctx);
    g_spawn_close_pid(pid);
}

// Returns true if an error occurred
gboolean
launch_editor(gchar* initial_content, void (*callback)(gchar* content, void* data), void* user_data)
{
    auto_gchar gchar* filename = NULL;
    auto_gerror GError* glib_error = NULL;
    const char* jid = connection_get_barejid();
    if (jid) {
        filename = files_file_in_account_data_path(DIR_EDITOR, jid, "compose.md");
    } else {
        log_debug("[Editor] could not get JID");
        auto_gchar gchar* data_dir = files_get_data_path(DIR_EDITOR);
        if (!create_dir(data_dir)) {
            cons_show_error("Could not create editor directory.");
            return TRUE;
        }
        filename = g_strdup_printf("%s/compose.md", data_dir);
    }
    if (!filename) {
        log_error("[Editor] something went wrong while creating compose file");
        cons_show_error("Could not create compose file.");
        return TRUE;
    }

    gsize messagelen = 0;
    if (initial_content != NULL) {
        messagelen = strlen(initial_content);
    }

    if (!g_file_set_contents(filename, initial_content, messagelen, &glib_error)) {
        log_error("[Editor] could not write to %s: %s", filename, PROF_GERROR_MESSAGE(glib_error));
        cons_show_error("Could not write to compose file: %s", PROF_GERROR_MESSAGE(glib_error));
        return TRUE;
    }

    auto_gchar gchar* editor = prefs_get_string(PREF_COMPOSE_EDITOR);
    gchar** editor_argv = NULL;
    GError* error = NULL;

    auto_gchar gchar* full_cmd = g_strdup_printf("%s %s", editor, filename);
    if (!g_shell_parse_argv(full_cmd, NULL, &editor_argv, &error)) {
        log_error("[Editor] Failed to parse editor command: %s", error->message);
        cons_show_error("Failed to parse editor command: %s", error->message);
        g_error_free(error);
        return TRUE;
    }

    EditorContext* ctx = g_new0(EditorContext, 1);
    ctx->filename = g_steal_pointer(&filename);
    ctx->callback = callback;
    ctx->user_data = user_data;

    ui_suspend();

    pid_t pid = fork();
    if (pid == -1) {
        log_error("[Editor] Failed to fork: %s", strerror(errno));
        ui_resume();
        ui_resize();
        cons_show_error("Failed to start editor: %s", strerror(errno));
        g_strfreev(editor_argv);
        g_free(ctx->filename);
        g_free(ctx);
        return TRUE;
    } else if (pid == 0) {
        // Child process: Inherits TTY from parent
        execvp(editor_argv[0], editor_argv);
        _exit(EXIT_FAILURE);
    }

    // Parent process: Watch the child asynchronously
    g_child_watch_add((GPid)pid, _editor_exit_cb, ctx);
    g_strfreev(editor_argv);
    return FALSE;
}
