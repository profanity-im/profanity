/*
 * http_common.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 William Wennerström <william@wstrm.dev>
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

#define _GNU_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gio/gio.h>

#include "tools/http_common.h"

#define FALLBACK_MSG ""

char*
http_basename_from_url(const char* url)
{
    const char* default_name = "index.html";

    GFile* file = g_file_new_for_uri(url);
    char* filename = g_file_get_basename(file);
    g_object_unref(file);

    if (g_strcmp0(filename, ".") == 0
        || g_strcmp0(filename, "..") == 0
        || g_strcmp0(filename, G_DIR_SEPARATOR_S) == 0) {
        g_free(filename);
        return strdup(default_name);
    }

    return filename;
}

void
http_print_transfer_update(ProfWin* window, char* url, const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    GString* msg = g_string_new(FALLBACK_MSG);
    g_string_vprintf(msg, fmt, args);
    va_end(args);

    win_update_entry_message(window, url, msg->str);

    g_string_free(msg, TRUE);
}

void
http_print_transfer(ProfWin* window, char* url, const char* fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    GString* msg = g_string_new(FALLBACK_MSG);
    g_string_vprintf(msg, fmt, args);
    va_end(args);

    win_print_http_transfer(window, msg->str, url);

    g_string_free(msg, TRUE);
}
