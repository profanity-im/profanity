/*
 * vcardwin.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2022 Marouane L. <techmetx11@disroot.org>
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

#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/vcard.h"

void
vcardwin_show_vcard_config(ProfVcardWin* vcardwin)
{
    ProfWin* window = &vcardwin->window;

    win_clear(window);
    win_show_vcard(window, vcardwin->vcard);

    win_println(window, THEME_DEFAULT, "-", "Use '/vcard save' to save changes.");
    win_println(window, THEME_DEFAULT, "-", "Use '/help vcard' for more information.");
}

gchar*
vcardwin_get_string(ProfVcardWin* vcardwin)
{
    GString* string = g_string_new("vCard: ");
    g_string_append(string, connection_get_barejid());

    if (vcardwin->vcard && vcardwin->vcard->modified) {
        g_string_append(string, " (modified)");
    }

    return g_string_free(string, FALSE);
}

void
vcardwin_update(void)
{
    ProfVcardWin* win = wins_get_vcard();

    if (win) {
        vcardwin_show_vcard_config(win);
    }
}
