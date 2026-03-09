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

typedef char* (*autocomplete_func)(const char* const, gboolean, void*);
typedef struct autocomplete_t* Autocomplete;

// allocate new autocompleter with no items
Autocomplete autocomplete_new(void);

// Remove all items from the autocompleter
void autocomplete_clear(Autocomplete ac);

// free all memory used by the autocompleter
void autocomplete_free(Autocomplete ac);

void autocomplete_add(Autocomplete ac, const char* item);
void autocomplete_add_all(Autocomplete ac, char** items);
void autocomplete_update(Autocomplete ac, char** items);
void autocomplete_remove(Autocomplete ac, const char* const item);
void autocomplete_remove_all(Autocomplete ac, char** items);
void autocomplete_add_unsorted(Autocomplete ac, const char* item, const gboolean is_reversed);

// find the next item prefixed with search string
gchar* autocomplete_complete(Autocomplete ac, const gchar* search_str, gboolean quote, gboolean previous);

GList* autocomplete_create_list(Autocomplete ac);
gint autocomplete_length(Autocomplete ac);

char* autocomplete_param_with_func(const char* const input, char* command,
                                   autocomplete_func func, gboolean previous, void* context);

char* autocomplete_param_with_ac(const char* const input, char* command,
                                 Autocomplete ac, gboolean quote, gboolean previous);

char* autocomplete_param_no_with_func(const char* const input, char* command,
                                      int arg_number, autocomplete_func func, gboolean previous, void* context);

void autocomplete_reset(Autocomplete ac);

gboolean autocomplete_contains(Autocomplete ac, const char* value);

void autocomplete_remove_older_than_max_reverse(Autocomplete ac, int maxsize);
#endif
