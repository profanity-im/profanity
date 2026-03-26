/*
 * http_common.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 William Wennerström <william@wstrm.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gio/gio.h>

#include "tools/http_common.h"

#define FALLBACK_MSG ""

void
http_print_transfer_update(ProfWin* window, char* id, theme_item_t theme_item, int flags, const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    GString* msg = g_string_new(FALLBACK_MSG);
    g_string_vprintf(msg, fmt, args);
    va_end(args);

    if (window->type != WIN_CONSOLE) {
        win_update_entry(window, id, msg->str, theme_item, flags);
    } else {
        cons_show("%s", msg->str);
    }

    g_string_free(msg, TRUE);
}

void
http_print_transfer(ProfWin* window, char* id, theme_item_t theme_item, const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    GString* msg = g_string_new(FALLBACK_MSG);
    g_string_vprintf(msg, fmt, args);
    va_end(args);

    win_print_status_with_id(window, msg->str, id, theme_item, 0);

    g_string_free(msg, TRUE);
}
