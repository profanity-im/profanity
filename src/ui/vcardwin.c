/*
 * vcardwin.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2022 Marouane L. <techmetx11@disroot.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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
