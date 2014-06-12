/*
 * autocompleters.c
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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

#include <string.h>

#include <glib.h>

#include "tools/autocomplete.h"

static GHashTable *autocompleters;

void
autocompleters_init(void)
{
    autocompleters = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)autocomplete_free);
}

void
autocompleters_add(const char *key, char **items)
{
    Autocomplete new_ac = autocomplete_new();
    int i = 0;
    for (i = 0; i < g_strv_length(items); i++) {
        autocomplete_add(new_ac, items[i]);
    }
    g_hash_table_insert(autocompleters, strdup(key), new_ac);
}

char *
autocompleters_complete(char *input, int *size)
{
    char *result = NULL;

    GList *keys = g_hash_table_get_keys(autocompleters);
    GList *curr = keys;
    while (curr != NULL) {
        result = autocomplete_param_with_ac(input, size, curr->data, g_hash_table_lookup(autocompleters, curr->data));
        if (result != NULL) {
            return result;
        }
        curr = g_list_next(curr);
    }

    return NULL;
}

void
autocompleters_reset(void)
{
    GList *acs = g_hash_table_get_values(autocompleters);
    GList *curr = acs;
    while (curr != NULL) {
        autocomplete_reset(curr->data);
        curr = g_list_next(curr);
    }
}

void autocompleters_destroy(void)
{
    g_hash_table_destroy(autocompleters);
}