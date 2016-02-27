/*
 * autocompleters.c
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
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
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
autocompleters_complete(const char * const input)
{
    char *result = NULL;

    GList *keys = g_hash_table_get_keys(autocompleters);
    GList *curr = keys;
    while (curr) {
        result = autocomplete_param_with_ac(input, curr->data, g_hash_table_lookup(autocompleters, curr->data), TRUE);
        if (result) {
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
    while (curr) {
        autocomplete_reset(curr->data);
        curr = g_list_next(curr);
    }
}

void autocompleters_destroy(void)
{
    g_hash_table_destroy(autocompleters);
}
