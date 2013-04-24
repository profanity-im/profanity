/*
 * autocomplete.c
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

#include "autocomplete.h"

struct autocomplete_t {
    GSList *items;
    GSList *last_found;
    gchar *search_str;
};

static gchar * _search_from(Autocomplete ac, GSList *curr);

Autocomplete
autocomplete_new(void)
{
    Autocomplete new = malloc(sizeof(struct autocomplete_t));
    new->items = NULL;
    new->last_found = NULL;
    new->search_str = NULL;

    return new;
}

void
autocomplete_clear(Autocomplete ac)
{
    g_slist_free_full(ac->items, free);
    ac->items = NULL;

    autocomplete_reset(ac);
}

void
autocomplete_reset(Autocomplete ac)
{
    ac->last_found = NULL;
    if (ac->search_str != NULL) {
        free(ac->search_str);
        ac->search_str = NULL;
    }
}

void
autocomplete_free(Autocomplete ac)
{
    autocomplete_clear(ac);
    g_free(ac);
    ac = NULL;
}

gint
autocomplete_length(Autocomplete ac)
{
    return g_slist_length(ac->items);
}

gboolean
autocomplete_add(Autocomplete ac, void *item)
{
    if (ac->items == NULL) {
        ac->items = g_slist_append(ac->items, item);
        return TRUE;
    } else {
        GSList *curr = ac->items;

        while(curr) {

            // insert
            if (g_strcmp0(curr->data, item) > 0) {
                ac->items = g_slist_insert_before(ac->items,
                    curr, item);
                return TRUE;

            // update
            } else if (g_strcmp0(curr->data, item) == 0) {
                // only update if data different
                if (strcmp(curr->data, item) != 0) {
                    free(curr->data);
                    curr->data = item;
                    return TRUE;
                } else {
                    return FALSE;
                }
            }

            curr = g_slist_next(curr);
        }

        // hit end, append
        ac->items = g_slist_append(ac->items, item);

        return TRUE;
    }
}

gboolean
autocomplete_remove(Autocomplete ac, const char * const item)
{
    // reset last found if it points to the item to be removed
    if (ac->last_found != NULL)
        if (g_strcmp0(ac->last_found->data, item) == 0)
            ac->last_found = NULL;

    if (!ac->items) {
        return FALSE;
    } else {
        GSList *curr = ac->items;

        while(curr) {
            if (g_strcmp0(curr->data, item) == 0) {
                void *current_item = curr->data;
                ac->items = g_slist_remove(ac->items, curr->data);
                free(current_item);

                return TRUE;
            }

            curr = g_slist_next(curr);
        }

        return FALSE;
    }
}

GSList *
autocomplete_get_list(Autocomplete ac)
{
    GSList *copy = NULL;
    GSList *curr = ac->items;

    while(curr) {
        copy = g_slist_append(copy, strdup(curr->data));
        curr = g_slist_next(curr);
    }

    return copy;
}

gchar *
autocomplete_complete(Autocomplete ac, gchar *search_str)
{
    gchar *found = NULL;

    // no items to search
    if (!ac->items)
        return NULL;

    // first search attempt
    if (ac->last_found == NULL) {
        ac->search_str =
            (gchar *) malloc((strlen(search_str) + 1) * sizeof(gchar));
        strcpy(ac->search_str, search_str);

        found = _search_from(ac, ac->items);
        return found;

    // subsequent search attempt
    } else {
        // search from here+1 tp end
        found = _search_from(ac, g_slist_next(ac->last_found));
        if (found != NULL)
            return found;

        // search from beginning
        found = _search_from(ac, ac->items);
        if (found != NULL)
            return found;

        // we found nothing, reset search
        autocomplete_reset(ac);
        return NULL;
    }
}

static gchar *
_search_from(Autocomplete ac, GSList *curr)
{
    while(curr) {

        // match found
        if (strncmp(curr->data, ac->search_str, strlen(ac->search_str)) == 0) {

            // set pointer to last found
            ac->last_found = curr;

            // if contains space, quote before returning
            if (g_strrstr(curr->data, " ")) {
                GString *quoted = g_string_new("\"");
                g_string_append(quoted, curr->data);
                g_string_append(quoted, "\"");

                gchar *result = quoted->str;
                g_string_free(quoted, FALSE);

                return result;

            // otherwise just return the string
            } else {
                return strdup(curr->data);
            }
        }

        curr = g_slist_next(curr);
    }

    return NULL;
}
