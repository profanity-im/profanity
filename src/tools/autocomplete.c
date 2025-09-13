/*
 * autocomplete.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "common.h"
#include "tools/autocomplete.h"
#include "tools/parser.h"
#include "ui/ui.h"

typedef enum {
    PREVIOUS,
    NEXT
} search_direction;

struct autocomplete_t
{
    GList* items;
    GList* last_found;
    gchar* search_str;
};

static gchar* _search(Autocomplete ac, GList* curr, gboolean quote, search_direction direction, gboolean substring);

Autocomplete
autocomplete_new(void)
{
    return calloc(1, sizeof(struct autocomplete_t));
}

void
autocomplete_clear(Autocomplete ac)
{
    if (ac) {
        if (ac->items) {
            g_list_free_full(ac->items, free);
            ac->items = NULL;
        }

        autocomplete_reset(ac);
    }
}

void
autocomplete_reset(Autocomplete ac)
{
    ac->last_found = NULL;
    FREE_SET_NULL(ac->search_str);
}

void
autocomplete_free(Autocomplete ac)
{
    if (ac) {
        autocomplete_clear(ac);
        free(ac);
    }
}

gint
autocomplete_length(Autocomplete ac)
{
    if (!ac) {
        return 0;
    } else if (!ac->items) {
        return 0;
    } else {
        return g_list_length(ac->items);
    }
}

void
autocomplete_update(Autocomplete ac, char** items)
{
    auto_gchar gchar* last_found = NULL;
    auto_gchar gchar* search_str = NULL;

    if (ac->last_found) {
        last_found = strdup(ac->last_found->data);
    }

    if (ac->search_str) {
        search_str = strdup(ac->search_str);
    }

    autocomplete_clear(ac);
    autocomplete_add_all(ac, items);

    if (last_found) {
        // NULL if last_found was removed on update.
        ac->last_found = g_list_find_custom(ac->items, last_found, (GCompareFunc)strcmp);
    }

    if (search_str) {
        ac->search_str = strdup(search_str);
    }
}

void
autocomplete_add_unsorted(Autocomplete ac, const char* item, const gboolean is_reversed)
{
    if (ac) {
        char* item_cpy;
        GList* curr = g_list_find_custom(ac->items, item, (GCompareFunc)strcmp);

        // if item already exists
        if (curr) {
            return;
        }

        item_cpy = strdup(item);

        if (is_reversed) {
            ac->items = g_list_prepend(ac->items, item_cpy);
        } else {
            ac->items = g_list_append(ac->items, item_cpy);
        }
    }
}

void
autocomplete_add(Autocomplete ac, const char* item)
{
    if (ac) {
        char* item_cpy;
        GList* curr = g_list_find_custom(ac->items, item, (GCompareFunc)strcmp);

        // if item already exists
        if (curr) {
            return;
        }

        item_cpy = strdup(item);
        ac->items = g_list_insert_sorted(ac->items, item_cpy, (GCompareFunc)strcmp);
    }
}

void
autocomplete_add_all(Autocomplete ac, char** items)
{
    for (int i = 0; i < g_strv_length(items); i++) {
        autocomplete_add(ac, items[i]);
    }
}

void
autocomplete_remove(Autocomplete ac, const char* const item)
{
    if (ac) {
        GList* curr = g_list_find_custom(ac->items, item, (GCompareFunc)strcmp);

        if (!curr) {
            return;
        }

        // reset last found if it points to the item to be removed
        if (ac->last_found == curr) {
            ac->last_found = NULL;
        }

        free(curr->data);
        ac->items = g_list_delete_link(ac->items, curr);
    }

    return;
}

void
autocomplete_remove_all(Autocomplete ac, char** items)
{
    for (int i = 0; i < g_strv_length(items); i++) {
        autocomplete_remove(ac, items[i]);
    }
}

GList*
autocomplete_create_list(Autocomplete ac)
{
    GList* copy = NULL;
    GList* curr = ac->items;

    while (curr) {
        copy = g_list_append(copy, strdup(curr->data));
        curr = g_list_next(curr);
    }

    return copy;
}

gboolean
autocomplete_contains(Autocomplete ac, const char* value)
{
    GList* curr = ac->items;

    while (curr) {
        if (strcmp(curr->data, value) == 0) {
            return TRUE;
        }
        curr = g_list_next(curr);
    }

    return FALSE;
}

gchar*
_autocomplete_complete_internal(Autocomplete ac, const gchar* search_str, gboolean quote, gboolean previous, gboolean substring)
{
    gchar* found = NULL;

    // no autocomplete to search
    if (!ac) {
        return NULL;
    }

    // no items to search
    if (!ac->items) {
        return NULL;
    }

    // first search attempt
    if (!ac->last_found) {
        if (ac->search_str) {
            FREE_SET_NULL(ac->search_str);
        }

        ac->search_str = strdup(search_str);
        found = _search(ac, ac->items, quote, NEXT, substring);

        return found;

        // subsequent search attempt
    } else {
        if (previous) {
            // search from here-1 to beginning
            found = _search(ac, g_list_previous(ac->last_found), quote, PREVIOUS, substring);
            if (found) {
                return found;
            }
        } else {
            // search from here+1 to end
            found = _search(ac, g_list_next(ac->last_found), quote, NEXT, substring);
            if (found) {
                return found;
            }
        }

        if (previous) {
            // search from end
            found = _search(ac, g_list_last(ac->items), quote, PREVIOUS, substring);
            if (found) {
                return found;
            }
        } else {
            // search from beginning
            found = _search(ac, ac->items, quote, NEXT, substring);
            if (found) {
                return found;
            }
        }

        // we found nothing, reset search
        autocomplete_reset(ac);

        return NULL;
    }
}

gchar*
autocomplete_complete(Autocomplete ac, const gchar* search_str, gboolean quote, gboolean previous)
{
    return _autocomplete_complete_internal(ac, search_str, quote, previous, FALSE);
}

gchar*
autocomplete_complete_substring(Autocomplete ac, const gchar* search_str, gboolean quote, gboolean previous)
{
    return _autocomplete_complete_internal(ac, search_str, quote, previous, TRUE);
}

// autocomplete_func func is used -> autocomplete_param_with_func
// Autocomplete ac, gboolean quote are used -> autocomplete_param_with_ac
static char*
_autocomplete_param_common(const char* const input, char* command, autocomplete_func func, Autocomplete ac, gboolean quote, gboolean previous, void* context)
{
    int len;
    auto_gchar gchar* command_cpy = g_strdup_printf("%s ", command);
    if (!command_cpy) {
        return NULL;
    }

    len = strlen(command_cpy);

    if (strncmp(input, command_cpy, len) == 0) {
        int inp_len = strlen(input);
        auto_char char* found = NULL;
        auto_char char* prefix = malloc(inp_len + 1);
        if (!prefix) {
            return NULL;
        }

        for (int i = len; i < inp_len; i++) {
            prefix[i - len] = input[i];
        }

        prefix[inp_len - len] = '\0';

        if (func) {
            found = func(prefix, previous, context);
        } else {
            found = autocomplete_complete(ac, prefix, quote, previous);
        }

        if (found) {
            return g_strdup_printf("%s%s", command_cpy, found);
        }
    }

    return NULL;
}

char*
autocomplete_param_with_func(const char* const input, char* command, autocomplete_func func, gboolean previous, void* context)
{
    return _autocomplete_param_common(input, command, func, NULL, FALSE, previous, context);
}

char*
autocomplete_param_with_ac(const char* const input, char* command, Autocomplete ac, gboolean quote, gboolean previous)
{
    return _autocomplete_param_common(input, command, NULL, ac, quote, previous, NULL);
}

char*
autocomplete_param_no_with_func(const char* const input, char* command, int arg_number, autocomplete_func func, gboolean previous, void* context)
{
    if (strncmp(input, command, strlen(command)) == 0) {
        // count tokens properly
        int num_tokens = count_tokens(input);

        // if correct number of tokens, then candidate for autocompletion of last param
        if (num_tokens == arg_number) {
            auto_gchar gchar* start_str = get_start(input, arg_number);
            auto_gchar gchar* comp_str = g_strdup(&input[strlen(start_str)]);

            // autocomplete param
            if (comp_str) {
                auto_gchar gchar* found = func(comp_str, previous, context);
                if (found) {
                    return g_strdup_printf("%s%s", start_str, found);
                }
            }
        }
    }

    return NULL;
}

/* remove the last message if we have more than max */
void
autocomplete_remove_older_than_max_reverse(Autocomplete ac, int maxsize)
{
    if (autocomplete_length(ac) > maxsize) {
        GList* last = g_list_last(ac->items);
        if (last) {
            free(last->data);
            ac->items = g_list_delete_link(ac->items, last);
        }
    }
}

static gchar*
_search(Autocomplete ac, GList* curr, gboolean quote, search_direction direction, gboolean substring)
{
    auto_gchar gchar* search_str_ascii = g_str_to_ascii(ac->search_str, NULL);
    auto_gchar gchar* search_str_lower = g_ascii_strdown(search_str_ascii, -1);

    while (curr) {
        auto_gchar gchar* curr_ascii = g_str_to_ascii(curr->data, NULL);
        auto_gchar gchar* curr_lower = g_ascii_strdown(curr_ascii, -1);

        gboolean found = FALSE;

        if (!substring && (strncmp(curr_lower, search_str_lower, strlen(search_str_lower)) == 0)) {
            // if we want to start from beginning (prefix)
            found = TRUE;
        } else if (substring && (strstr(curr_lower, search_str_lower) != 0)) {
            // if we only are looking for a substring
            found = TRUE;
        }

        // match found
        if (found) {
            // set pointer to last found
            ac->last_found = curr;

            // if contains space, quote before returning
            if (quote && g_strrstr(curr->data, " ")) {
                return g_strdup_printf("\"%s\"", (gchar*)curr->data);
                // otherwise just return the string
            } else {
                return strdup(curr->data);
            }
        }

        if (direction == PREVIOUS) {
            curr = g_list_previous(curr);
        } else {
            curr = g_list_next(curr);
        }
    }

    return NULL;
}
