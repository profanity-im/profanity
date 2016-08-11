/*
 * disco.c
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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
#include <stdlib.h>

#include <glib.h>

static GHashTable *plugin_to_features = NULL;

static void
_free_features(GList *features)
{
    g_list_free_full(features, free);
}

void
disco_add_feature(const char *plugin_name, char* feature)
{
    if (feature == NULL || plugin_name == NULL) {
        return;
    }

    if (!plugin_to_features) {
        plugin_to_features = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)_free_features);
    }

    GList *features = g_hash_table_lookup(plugin_to_features, plugin_name);
    if (!features) {
        features = g_list_append(features, strdup(feature));
        g_hash_table_insert(plugin_to_features, strdup(plugin_name), features);
    } else {
        features = g_list_append(features, strdup(feature));
    }
}

void
disco_remove_features(const char *plugin_name)
{
    if (!plugin_to_features) {
        return;
    }

    if (!g_hash_table_contains(plugin_to_features, plugin_name)) {
        return;
    }

    g_hash_table_remove(plugin_to_features, plugin_name);
}

GList*
disco_get_features(void)
{
    GList *result = NULL;
    if (!plugin_to_features) {
        return result;
    }

    GList *lists = g_hash_table_get_values(plugin_to_features);
    GList *curr_list = lists;
    while (curr_list) {
        GList *features = curr_list->data;
        GList *curr = features;
        while (curr) {
            result = g_list_append(result, curr->data);
            curr = g_list_next(curr);
        }
        curr_list = g_list_next(curr_list);
    }

    g_list_free(lists);

    return result;
}

void
disco_close(void)
{
    if (plugin_to_features) {
        g_hash_table_destroy(plugin_to_features);
        plugin_to_features = NULL;
    }
}
