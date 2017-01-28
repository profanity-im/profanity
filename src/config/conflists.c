/*
 * conflists.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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

#include <string.h>
#include <glib.h>

gboolean
conf_string_list_add(GKeyFile *keyfile, const char *const group, const char *const key, const char *const item)
{
    gsize length;
    gchar **list = g_key_file_get_string_list(keyfile, group, key, &length, NULL);
    GList *glist = NULL;

    // list found
    if (list) {
        int i = 0;
        for (i = 0; i < length; i++) {
            // item already in list, exit function
            if (strcmp(list[i], item) == 0) {
                g_list_free_full(glist, g_free);
                g_strfreev(list);
                return FALSE;
            }
            // add item to our g_list
            glist = g_list_append(glist, strdup(list[i]));
        }

        // item not found, add to our g_list
        glist = g_list_append(glist, strdup(item));

        // create the new list entry
        const gchar* new_list[g_list_length(glist)+1];
        GList *curr = glist;
        i = 0;
        while (curr) {
            new_list[i++] = curr->data;
            curr = g_list_next(curr);
        }
        new_list[i] = NULL;
        g_key_file_set_string_list(keyfile, group, key, new_list, g_list_length(glist));

    // list not found
    } else {
        const gchar* new_list[2];
        new_list[0] = item;
        new_list[1] = NULL;
        g_key_file_set_string_list(keyfile, group, key, new_list, 1);
    }

    g_strfreev(list);
    g_list_free_full(glist, g_free);

    return TRUE;
}

gboolean
conf_string_list_remove(GKeyFile *keyfile, const char *const group, const char *const key, const char *const item)
{
    gsize length;
    gchar **list = g_key_file_get_string_list(keyfile, group, key, &length, NULL);

    gboolean deleted = FALSE;
    if (list) {
        int i = 0;
        GList *glist = NULL;

        for (i = 0; i < length; i++) {
            // item found, mark as deleted
            if (strcmp(list[i], item) == 0) {
                deleted = TRUE;
            } else {
                // add item to our g_list
                glist = g_list_append(glist, strdup(list[i]));
            }
        }

        if (deleted) {
            if (g_list_length(glist) == 0) {
                g_key_file_remove_key(keyfile, group, key, NULL);
            } else {
                // create the new list entry
                const gchar* new_list[g_list_length(glist)+1];
                GList *curr = glist;
                i = 0;
                while (curr) {
                    new_list[i++] = curr->data;
                    curr = g_list_next(curr);
                }
                new_list[i] = NULL;
                g_key_file_set_string_list(keyfile, group, key, new_list, g_list_length(glist));
            }
        }

        g_list_free_full(glist, g_free);
    }

    g_strfreev(list);

    return deleted;
}
