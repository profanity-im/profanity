/*
 * disco.c
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
#include <stdlib.h>

#include <glib.h>

// features to reference count map
static GHashTable *features = NULL;

// plugin to feature map
static GHashTable *plugin_to_features = NULL;

static void
_free_features(GHashTable *features)
{
    g_hash_table_destroy(features);
}

void
disco_add_feature(const char *plugin_name, char *feature)
{
    if (feature == NULL || plugin_name == NULL) {
        return;
    }

    if (!features) {
        features = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
    }
    if (!plugin_to_features) {
        plugin_to_features = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)_free_features);
    }

    GHashTable *plugin_features = g_hash_table_lookup(plugin_to_features, plugin_name);
    gboolean added = FALSE;
    if (plugin_features == NULL) {
        plugin_features = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
        g_hash_table_add(plugin_features, strdup(feature));
        g_hash_table_insert(plugin_to_features, strdup(plugin_name), plugin_features);
        added = TRUE;
    } else if (!g_hash_table_contains(plugin_features, feature)) {
        g_hash_table_add(plugin_features, strdup(feature));
        added = TRUE;
    }

    if (added == FALSE) {
        return;
    }

    if (!g_hash_table_contains(features, feature)) {
        g_hash_table_insert(features, strdup(feature), GINT_TO_POINTER(1));
    } else {
        void *refcountp = g_hash_table_lookup(features, feature);
        int refcount = GPOINTER_TO_INT(refcountp);
        refcount++;
        g_hash_table_replace(features, strdup(feature), GINT_TO_POINTER(refcount));
    }
}

void
disco_remove_features(const char *plugin_name)
{
    if (!features) {
        return;
    }
    if (!plugin_to_features) {
        return;
    }

    GHashTable *plugin_features_set = g_hash_table_lookup(plugin_to_features, plugin_name);
    if (!plugin_features_set) {
        return;
    }

    GList *plugin_feature_list = g_hash_table_get_keys(plugin_features_set);
    GList *curr = plugin_feature_list;
    while (curr) {
        char *feature = curr->data;
        if (g_hash_table_contains(features, feature)) {
            void *refcountp = g_hash_table_lookup(features, feature);
            int refcount = GPOINTER_TO_INT(refcountp);
            if (refcount == 1) {
                g_hash_table_remove(features, feature);
            } else {
                refcount--;
                g_hash_table_replace(features, strdup(feature), GINT_TO_POINTER(refcount));
            }
        }

        curr = g_list_next(curr);
    }
    g_list_free(plugin_feature_list);

}

GList*
disco_get_features(void)
{
    if (features == NULL) {
        return NULL;
    }

    return g_hash_table_get_keys(features);
}

void
disco_close(void)
{
    if (features) {
        g_hash_table_destroy(features);
        features = NULL;
    }

    if (plugin_to_features) {
        g_hash_table_destroy(plugin_to_features);
        plugin_to_features = NULL;
    }
}
