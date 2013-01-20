/*
 * capabilities.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "common.h"
#include "capabilities.h"

static GHashTable *capabilities;

static void _caps_destroy(Capabilities *caps);

void
caps_init(void)
{
    capabilities = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
        (GDestroyNotify)_caps_destroy);
}

void
caps_add(const char * const caps_str, const char * const client)
{
    Capabilities *new_caps = malloc(sizeof(struct capabilities_t));

    if (client != NULL) {
        new_caps->client = strdup(client);
    } else {
        new_caps->client = NULL;
    }

    g_hash_table_insert(capabilities, strdup(caps_str), new_caps);
}

gboolean
caps_contains(const char * const caps_str)
{
    return (g_hash_table_lookup(capabilities, caps_str) != NULL);
}

Capabilities *
caps_get(const char * const caps_str)
{
    return g_hash_table_lookup(capabilities, caps_str);
}

void
caps_close(void)
{
    g_hash_table_destroy(capabilities);
}

static void
_caps_destroy(Capabilities *caps)
{
    if (caps != NULL) {
        FREE_SET_NULL(caps->client);
        FREE_SET_NULL(caps);
    }
}
