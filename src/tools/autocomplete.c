/*
 * autocomplete.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "tools/autocomplete.h"
#include "tools/parser.h"

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
    FREE_SET_NULL(ac->search_str);
}

void
autocomplete_free(Autocomplete ac)
{
    autocomplete_clear(ac);
    free(ac);
}

gint
autocomplete_length(Autocomplete ac)
{
    if (ac == NULL) {
        return 0;
    } else if (ac->items == NULL) {
        return 0;
    } else {
        return g_slist_length(ac->items);
    }
}

void
autocomplete_add(Autocomplete ac, const char *item)
{
    char *item_cpy;
    GSList *curr = g_slist_find_custom(ac->items, item, (GCompareFunc)strcmp);

    // if item already exists
    if (curr != NULL) {
        return;
    }

    item_cpy = strdup(item);
    ac->items = g_slist_insert_sorted(ac->items, item_cpy, (GCompareFunc)strcmp);
    return;
}

void
autocomplete_remove(Autocomplete ac, const char * const item)
{
    GSList *curr = g_slist_find_custom(ac->items, item, (GCompareFunc)strcmp);

    if (!curr) {
        return;
    }

    // reset last found if it points to the item to be removed
    if (ac->last_found == curr) {
        ac->last_found = NULL;
    }

    free(curr->data);
    ac->items = g_slist_delete_link(ac->items, curr);

    return;
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

gboolean
autocomplete_contains(Autocomplete ac, const char *value)
{
    GSList *curr = ac->items;

    while(curr) {
        if (strcmp(curr->data, value) == 0) {
            return TRUE;
        }
        curr = g_slist_next(curr);
    }

    return FALSE;
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
        ac->search_str = strdup(search_str);
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

char *
autocomplete_param_with_func(char *input, int *size, char *command,
    autocomplete_func func)
{
    char *found = NULL;
    GString *auto_msg = NULL;
    char *result = NULL;
    char inp_cpy[*size];
    int i;
    char command_cpy[strlen(command) + 2];
    sprintf(command_cpy, "%s ", command);
    int len = strlen(command_cpy);

    if ((strncmp(input, command_cpy, len) == 0) && (*size > len)) {
        for(i = len; i < *size; i++) {
            inp_cpy[i-len] = input[i];
        }
        inp_cpy[(*size) - len] = '\0';
        found = func(inp_cpy);
        if (found != NULL) {
            auto_msg = g_string_new(command_cpy);
            g_string_append(auto_msg, found);
            free(found);
            result = auto_msg->str;
            g_string_free(auto_msg, FALSE);
        }
    }

    return result;
}

char *
autocomplete_param_with_ac(char *input, int *size, char *command,
    Autocomplete ac)
{
    char *found = NULL;
    GString *auto_msg = NULL;
    char *result = NULL;
    char inp_cpy[*size];
    int i;
    char *command_cpy = malloc(strlen(command) + 2);
    sprintf(command_cpy, "%s ", command);
    int len = strlen(command_cpy);
    if ((strncmp(input, command_cpy, len) == 0) && (*size > len)) {
        for(i = len; i < *size; i++) {
            inp_cpy[i-len] = input[i];
        }
        inp_cpy[(*size) - len] = '\0';
        found = autocomplete_complete(ac, inp_cpy);
        if (found != NULL) {
            auto_msg = g_string_new(command_cpy);
            g_string_append(auto_msg, found);
            free(found);
            result = auto_msg->str;
            g_string_free(auto_msg, FALSE);
        }
    }
    free(command_cpy);

    return result;
}

char *
autocomplete_param_no_with_func(char *input, int *size, char *command,
    int arg_number, autocomplete_func func)
{
    char *result = NULL;
    if (strncmp(input, command, strlen(command)) == 0 && (*size > strlen(command))) {
        int i = 0;
        char *found = NULL;
        GString *result_str = NULL;

        // copy and null terminate input
        gchar inp_cpy[*size];
        for (i = 0; i < *size; i++) {
            inp_cpy[i] = input[i];
        }
        inp_cpy[i] = '\0';
        g_strstrip(inp_cpy);

        // count tokens properly
        int num_tokens = count_tokens(inp_cpy);

        // if correct number of tokens, then candidate for autocompletion of last param
        if (num_tokens == arg_number) {
            gchar *start_str = get_start(inp_cpy, arg_number);
            gchar *comp_str = g_strdup(&inp_cpy[strlen(start_str)]);

            // autocomplete param
            if (comp_str != NULL) {
                found = func(comp_str);
                if (found != NULL) {
                    result_str = g_string_new("");
                    g_string_append(result_str, start_str);
                    g_string_append(result_str, found);
                    result = result_str->str;
                    g_string_free(result_str, FALSE);
                    return result;
                }
            }
        }
    }

    return NULL;
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
