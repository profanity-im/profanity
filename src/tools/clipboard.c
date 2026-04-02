/*
 * clipboard.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#ifdef HAVE_GTK
#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>

#include "log.h"
#include "ui/tray.h"

/*
For now we rely on tray_init(void)

void clipboard_init(int argc, char **argv) {
    gtk_init(&argc, &argv);
}
*/

char*
clipboard_get(void)
{
    if (!tray_gtk_ready()) {
        return NULL;
    }
    gchar* clip;

    GtkClipboard* cl = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_clear(cl);

    if (cl == NULL) {
        log_error("Could not get clipboard");
        return NULL;
    }

    // while(!gtk_clipboard_wait_is_text_available(cld)) {}

    clip = gtk_clipboard_wait_for_text(cl);
    return clip;
}

#endif
