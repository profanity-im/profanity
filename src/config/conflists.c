/*
 * conflists.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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
    const gchar** new_list = g_new(const gchar*, length + 2);
    memcpy(new_list, list, sizeof(list[0]) * length);
    new_list[length] = item;
    new_list[length + 1] = NULL;

    g_key_file_set_string_list(keyfile, group, key, new_list, length + 1);

    g_free(new_list);

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

    gsize new_length = 0;
    const gchar** new_list = g_new(const gchar*, length + 1);
    gboolean deleted = FALSE;

    for (gsize i = 0; i < length; i++) {
        if (strcmp(list[i], item) == 0) {
            deleted = TRUE;
            continue;
        }
        new_list[new_length++] = list[i];
    }
    new_list[new_length] = NULL;

    if (deleted) {
        if (new_length == 0) {
            g_key_file_remove_key(keyfile, group, key, NULL);
        } else {
            g_key_file_set_string_list(keyfile, group, key, new_list, new_length);
        }
    }

    g_free(new_list);

    return deleted;
}
