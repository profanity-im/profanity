/*
 * disco.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include <glib.h>

// features to reference count map
static GHashTable* features = NULL;

// plugin to feature map
static GHashTable* plugin_to_features = NULL;

static void
_free_features(GHashTable* features)
{
    g_hash_table_destroy(features);
}

void
disco_add_feature(const char* plugin_name, char* feature)
{
    if (feature == NULL || plugin_name == NULL) {
        return;
    }

    if (!features) {
        features = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    }
    if (!plugin_to_features) {
        plugin_to_features = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)_free_features);
    }

    GHashTable* plugin_features = g_hash_table_lookup(plugin_to_features, plugin_name);
    gboolean added = FALSE;
    if (plugin_features == NULL) {
        plugin_features = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
        g_hash_table_add(plugin_features, g_strdup(feature));
        g_hash_table_insert(plugin_to_features, g_strdup(plugin_name), plugin_features);
        added = TRUE;
    } else if (!g_hash_table_contains(plugin_features, feature)) {
        g_hash_table_add(plugin_features, g_strdup(feature));
        added = TRUE;
    }

    if (added == FALSE) {
        return;
    }

    if (!g_hash_table_contains(features, feature)) {
        g_hash_table_insert(features, g_strdup(feature), GINT_TO_POINTER(1));
    } else {
        void* refcountp = g_hash_table_lookup(features, feature);
        int refcount = GPOINTER_TO_INT(refcountp);
        refcount++;
        g_hash_table_replace(features, g_strdup(feature), GINT_TO_POINTER(refcount));
    }
}

void
disco_remove_features(const char* plugin_name)
{
    if (!features) {
        return;
    }
    if (!plugin_to_features) {
        return;
    }

    GHashTable* plugin_features_set = g_hash_table_lookup(plugin_to_features, plugin_name);
    if (!plugin_features_set) {
        return;
    }

    GList* plugin_feature_list = g_hash_table_get_keys(plugin_features_set);
    GList* curr = plugin_feature_list;
    while (curr) {
        char* feature = curr->data;
        if (g_hash_table_contains(features, feature)) {
            void* refcountp = g_hash_table_lookup(features, feature);
            int refcount = GPOINTER_TO_INT(refcountp);
            if (refcount == 1) {
                g_hash_table_remove(features, feature);
            } else {
                refcount--;
                g_hash_table_replace(features, g_strdup(feature), GINT_TO_POINTER(refcount));
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
