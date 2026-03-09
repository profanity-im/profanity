/*
 * xmlwin.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <assert.h>
#include <string.h>

#include "ui/win_types.h"
#include "ui/window_list.h"

void
xmlwin_show(ProfXMLWin* xmlwin, const char* const msg)
{
    assert(xmlwin != NULL);

    ProfWin* window = (ProfWin*)xmlwin;
    if (g_str_has_prefix(msg, "SENT:")) {
        win_println(window, THEME_DEFAULT, "-", "SENT:");
        win_println(window, THEME_ONLINE, "-", "%s", &msg[6]);
        win_println(window, THEME_ONLINE, "-", "");
    } else if (g_str_has_prefix(msg, "RECV:")) {
        win_println(window, THEME_DEFAULT, "-", "RECV:");
        win_println(window, THEME_AWAY, "-", "%s", &msg[6]);
        win_println(window, THEME_AWAY, "-", "");
    }
}

gchar*
xmlwin_get_string(ProfXMLWin* xmlwin)
{
    assert(xmlwin != NULL);

    return g_strdup("XML console");
}
