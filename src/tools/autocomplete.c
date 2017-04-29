/*
 * autocomplete.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "common.h"
#include "tools/autocomplete.h"
#include "tools/parser.h"
#include "ui/ui.h"

struct autocomplete_t {
    GList *items;
    GList *last_found;
    gchar *search_str;
};

static gchar* _search_next(Autocomplete ac, GList *curr, gboolean quote);
static gchar* _search_prev(Autocomplete ac, GList *curr, gboolean quote);

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
    if (ac) {
        g_list_free_full(ac->items, free);
        ac->items = NULL;

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
autocomplete_add(Autocomplete ac, const char *item)
{
    if (ac) {
        char *item_cpy;
        GList *curr = g_list_find_custom(ac->items, item, (GCompareFunc)strcmp);

        // if item already exists
        if (curr) {
            return;
        }

        item_cpy = strdup(item);
        ac->items = g_list_insert_sorted(ac->items, item_cpy, (GCompareFunc)strcmp);
    }

    return;
}

void
autocomplete_add_all(Autocomplete ac, char **items)
{
    int i = 0;
    for (i = 0; i < g_strv_length(items); i++) {
        autocomplete_add(ac, items[i]);
    }
}

void
autocomplete_remove(Autocomplete ac, const char *const item)
{
    if (ac) {
        GList *curr = g_list_find_custom(ac->items, item, (GCompareFunc)strcmp);

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
autocomplete_remove_all(Autocomplete ac, char **items)
{
    int i = 0;
    for (i = 0; i < g_strv_length(items); i++) {
        autocomplete_remove(ac, items[i]);
    }
}

GList*
autocomplete_create_list(Autocomplete ac)
{
    GList *copy = NULL;
    GList *curr = ac->items;

    while(curr) {
        copy = g_list_append(copy, strdup(curr->data));
        curr = g_list_next(curr);
    }

    return copy;
}

gboolean
autocomplete_contains(Autocomplete ac, const char *value)
{
    GList *curr = ac->items;

    while(curr) {
        if (strcmp(curr->data, value) == 0) {
            return TRUE;
        }
        curr = g_list_next(curr);
    }

    return FALSE;
}

gchar*
autocomplete_complete(Autocomplete ac, const gchar *search_str, gboolean quote, gboolean previous)
{
    gchar *found = NULL;

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
        found = _search_next(ac, ac->items, quote);

        return found;

    // subsequent search attempt
    } else {
        if (previous) {
            // search from here-1 to beginning
            found = _search_prev(ac, g_list_previous(ac->last_found), quote);
            if (found) {
                return found;
            }
        } else {
            // search from here+1 to end
            found = _search_next(ac, g_list_next(ac->last_found), quote);
            if (found) {
                return found;
            }
        }

        if (previous) {
            // search from end
            found = _search_prev(ac, g_list_last(ac->items), quote);
            if (found) {
                return found;
            }
        } else {
            // search from beginning
            found = _search_next(ac, ac->items, quote);
            if (found) {
                return found;
            }
        }

        // we found nothing, reset search
        autocomplete_reset(ac);

        return NULL;
    }
}

char*
autocomplete_param_with_func(const char *const input, char *command, autocomplete_func func, gboolean previous)
{
    GString *auto_msg = NULL;
    char *result = NULL;
    char command_cpy[strlen(command) + 2];
    sprintf(command_cpy, "%s ", command);
    int len = strlen(command_cpy);

    if (strncmp(input, command_cpy, len) == 0) {
        int i;
        int inp_len = strlen(input);
        char prefix[inp_len];
        for(i = len; i < inp_len; i++) {
            prefix[i-len] = input[i];
        }
        prefix[inp_len - len] = '\0';

        char *found = func(prefix, previous);
        if (found) {
            auto_msg = g_string_new(command_cpy);
            g_string_append(auto_msg, found);
            free(found);
            result = auto_msg->str;
            g_string_free(auto_msg, FALSE);
        }
    }

    return result;
}

char*
autocomplete_param_with_ac(const char *const input, char *command, Autocomplete ac, gboolean quote, gboolean previous)
{
    GString *auto_msg = NULL;
    char *result = NULL;
    char *command_cpy = malloc(strlen(command) + 2);
    sprintf(command_cpy, "%s ", command);
    int len = strlen(command_cpy);
    int inp_len = strlen(input);
    if (strncmp(input, command_cpy, len) == 0) {
        int i;
        char prefix[inp_len];
        for(i = len; i < inp_len; i++) {
            prefix[i-len] = input[i];
        }
        prefix[inp_len - len] = '\0';

        char *found = autocomplete_complete(ac, prefix, quote, previous);
        if (found) {
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

char*
autocomplete_param_no_with_func(const char *const input, char *command, int arg_number, autocomplete_func func, gboolean previous)
{
    if (strncmp(input, command, strlen(command)) == 0) {
        GString *result_str = NULL;

        // count tokens properly
        int num_tokens = count_tokens(input);

        // if correct number of tokens, then candidate for autocompletion of last param
        if (num_tokens == arg_number) {
            gchar *start_str = get_start(input, arg_number);
            gchar *comp_str = g_strdup(&input[strlen(start_str)]);

            // autocomplete param
            if (comp_str) {
                char *found = func(comp_str, previous);
                if (found) {
                    result_str = g_string_new("");
                    g_string_append(result_str, start_str);
                    g_string_append(result_str, found);
                    char *result = result_str->str;
                    g_string_free(result_str, FALSE);
                    return result;
                }
            }
        }
    }

    return NULL;
}

static gchar*
_search_next(Autocomplete ac, GList *curr, gboolean quote)
{
    gchar *search_str_ascii = g_str_to_ascii(ac->search_str, NULL);
    gchar *search_str_lower = g_ascii_strdown(search_str_ascii, -1);
    g_free(search_str_ascii);

    while(curr) {
        gchar *curr_ascii = g_str_to_ascii(curr->data, NULL);
        gchar *curr_lower = g_ascii_strdown(curr_ascii, -1);
        g_free(curr_ascii);

        // match found
        if (strncmp(curr_lower, search_str_lower, strlen(search_str_lower)) == 0) {

            // set pointer to last found
            ac->last_found = curr;

            // if contains space, quote before returning
            if (quote && g_strrstr(curr->data, " ")) {
                GString *quoted = g_string_new("\"");
                g_string_append(quoted, curr->data);
                g_string_append(quoted, "\"");

                gchar *result = quoted->str;
                g_string_free(quoted, FALSE);

                g_free(search_str_lower);
                g_free(curr_lower);
                return result;

            // otherwise just return the string
            } else {
                g_free(search_str_lower);
                g_free(curr_lower);
                return strdup(curr->data);
            }
        }

        g_free(curr_lower);
        curr = g_list_next(curr);
    }

    g_free(search_str_lower);
    return NULL;
}

static gchar*
_search_prev(Autocomplete ac, GList *curr, gboolean quote)
{
    gchar *search_str_ascii = g_str_to_ascii(ac->search_str, NULL);
    gchar *search_str_lower = g_ascii_strdown(search_str_ascii, -1);
    g_free(search_str_ascii);

    while(curr) {
        gchar *curr_ascii = g_str_to_ascii(curr->data, NULL);
        gchar *curr_lower = g_ascii_strdown(curr_ascii, -1);
        g_free(curr_ascii);

        // match found
        if (strncmp(curr_lower, search_str_lower, strlen(search_str_lower)) == 0) {

            // set pointer to last found
            ac->last_found = curr;

            // if contains space, quote before returning
            if (quote && g_strrstr(curr->data, " ")) {
                GString *quoted = g_string_new("\"");
                g_string_append(quoted, curr->data);
                g_string_append(quoted, "\"");

                gchar *result = quoted->str;
                g_string_free(quoted, FALSE);

                g_free(search_str_lower);
                g_free(curr_lower);
                return result;

            // otherwise just return the string
            } else {
                g_free(search_str_lower);
                g_free(curr_lower);
                return strdup(curr->data);
            }
        }

        g_free(curr_lower);
        curr = g_list_previous(curr);
    }

    g_free(search_str_lower);
    return NULL;
}
