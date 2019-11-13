/*
 * clipboard.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 Michael Vetter <jubalh@iodoru.org>
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

#ifdef HAVE_GTK
#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>

#include "log.h"

/*
For now we rely on tray_init(void)

void clipboard_init(int argc, char **argv) {
    gtk_init(&argc, &argv);
}
*/

char *clipboard_get(void) {
    gchar *clip;

    GtkClipboard *cl = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_clear(cl);

    if(cl==NULL) {
        log_error("Could not get clipboard");
        return NULL;
    }

    //while(!gtk_clipboard_wait_is_text_available(cld)) {}

    clip = gtk_clipboard_wait_for_text(cl);
    return clip;
}

#endif
