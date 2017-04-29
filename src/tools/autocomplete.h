/*
 * autocomplete.h
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

#ifndef TOOLS_AUTOCOMPLETE_H
#define TOOLS_AUTOCOMPLETE_H

#include <glib.h>

typedef char* (*autocomplete_func)(const char *const, gboolean);
typedef struct autocomplete_t *Autocomplete;

// allocate new autocompleter with no items
Autocomplete autocomplete_new(void);

// Remove all items from the autocompleter
void autocomplete_clear(Autocomplete ac);

// free all memory used by the autocompleter
void autocomplete_free(Autocomplete ac);

void autocomplete_add(Autocomplete ac, const char *item);
void autocomplete_add_all(Autocomplete ac, char **items);
void autocomplete_remove(Autocomplete ac, const char *const item);
void autocomplete_remove_all(Autocomplete ac, char **items);

// find the next item prefixed with search string
gchar* autocomplete_complete(Autocomplete ac, const gchar *search_str, gboolean quote, gboolean previous);

GList* autocomplete_create_list(Autocomplete ac);
gint autocomplete_length(Autocomplete ac);

char* autocomplete_param_with_func(const char *const input, char *command,
    autocomplete_func func, gboolean previous);

char* autocomplete_param_with_ac(const char *const input, char *command,
    Autocomplete ac, gboolean quote, gboolean previous);

char* autocomplete_param_no_with_func(const char *const input, char *command,
    int arg_number, autocomplete_func func, gboolean previous);

void autocomplete_reset(Autocomplete ac);

gboolean autocomplete_contains(Autocomplete ac, const char *value);
#endif
