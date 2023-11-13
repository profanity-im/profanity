/*
 * conflists.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
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
#include "common.h"
#include "log.h"

#include <string.h>
#include <glib.h>

gboolean
conf_string_list_add(GKeyFile* keyfile, const char* const group, const char* const key, const char* const item)
{
    if (!item) {
        log_warning("Invalid parameter in `conf_string_list_add`: item is NULL. group=%s, key=%s", group, key);
        return FALSE;
    }

    gsize length;
    auto_gcharv gchar** list = g_key_file_get_string_list(keyfile, group, key, &length, NULL);

    if (!list) {
        const gchar* new_list[2] = { item, NULL };
        g_key_file_set_string_list(keyfile, group, key, new_list, 1);
        return TRUE;
    }

    // Check if item is already in the list
    for (gsize i = 0; i < length; ++i) {
        if (strcmp(list[i], item) == 0) {
            return FALSE;
        }
    }

    // Add item to the existing list
    gchar** new_list = g_new(gchar*, length + 2);
    for (gsize i = 0; i < length; ++i) {
        new_list[i] = g_strdup(list[i]);
    }
    new_list[length] = g_strdup(item);
    new_list[length + 1] = NULL;

    g_key_file_set_string_list(keyfile, group, key, (const gchar* const*)new_list, length + 1);

    return TRUE;
}

gboolean
conf_string_list_remove(GKeyFile* keyfile, const char* const group, const char* const key, const char* const item)
{
    gsize length;
    auto_gcharv gchar** list = g_key_file_get_string_list(keyfile, group, key, &length, NULL);

    if (!list) {
        return FALSE;
    }

    GList* glist = NULL;
    gboolean deleted = FALSE;

    for (int i = 0; i < length; i++) {
        if (strcmp(list[i], item) == 0) {
            deleted = TRUE;
            continue;
        }
        glist = g_list_append(glist, strdup(list[i]));
    }

    if (deleted) {
        if (g_list_length(glist) == 0) {
            g_key_file_remove_key(keyfile, group, key, NULL);
        } else {
            const gchar* new_list[g_list_length(glist) + 1];
            int i = 0;

            for (GList* curr = glist; curr; curr = g_list_next(curr)) {
                new_list[i++] = curr->data;
            }

            new_list[i] = NULL;
            g_key_file_set_string_list(keyfile, group, key, new_list, g_list_length(glist));
        }
    }

    g_list_free_full(glist, g_free);
    return deleted;
}
