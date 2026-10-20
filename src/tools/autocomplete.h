/*
 * autocomplete.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef TOOLS_AUTOCOMPLETE_H
#define TOOLS_AUTOCOMPLETE_H

#include <glib.h>

typedef gchar* (*autocomplete_func)(const gchar* const, gboolean, void*);
typedef struct autocomplete_t* Autocomplete;

// allocate new autocompleter with no items
Autocomplete autocomplete_new(void);

// Remove all items from the autocompleter
void autocomplete_clear(Autocomplete ac);

// free all memory used by the autocompleter
void autocomplete_free(Autocomplete ac);

void autocomplete_add(Autocomplete ac, const gchar* item);
void autocomplete_add_all(Autocomplete ac, gchar** items);
void autocomplete_update(Autocomplete ac, gchar** items);
void autocomplete_remove(Autocomplete ac, const gchar* const item);
void autocomplete_remove_all(Autocomplete ac, gchar** items);
void autocomplete_add_unsorted(Autocomplete ac, const gchar* item, const gboolean is_reversed);

// find the next item prefixed with search string
gchar* autocomplete_complete(Autocomplete ac, const gchar* search_str, gboolean quote, gboolean previous);

GList* autocomplete_create_list(Autocomplete ac);
gint autocomplete_length(Autocomplete ac);

gchar* autocomplete_param_with_func(const gchar* const input, const gchar* command,
                                    autocomplete_func func, gboolean previous, void* context);

gchar* autocomplete_param_with_ac(const gchar* const input, const gchar* command,
                                  Autocomplete ac, gboolean quote, gboolean previous);

gchar* autocomplete_param_no_with_func(const gchar* const input, const gchar* command,
                                       int arg_number, autocomplete_func func, gboolean previous, void* context);

void autocomplete_reset(Autocomplete ac);

gboolean autocomplete_contains(Autocomplete ac, const gchar* value);

void autocomplete_remove_older_than_max_reverse(Autocomplete ac, int maxsize);
#endif
