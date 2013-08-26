/*
 * autocomplete.h
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

#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

#include <glib.h>

typedef char*(*autocomplete_func)(char *);
typedef struct autocomplete_t *Autocomplete;
typedef const char * (*PStrFunc)(const void *obj);
typedef void * (*PCopyFunc)(const void *obj);
typedef int (*PEqualFunc)(const void *o1, const void *o2);
typedef int (*PEqualDeepFunc)(const void *o1, const void *o2);

Autocomplete autocomplete_new(void);
Autocomplete obj_autocomplete_new(PStrFunc str_func, PCopyFunc copy_func,
    PEqualDeepFunc equal_deep_func, GDestroyNotify free_func);
void autocomplete_clear(Autocomplete ac);
void autocomplete_reset(Autocomplete ac);
void autocomplete_free(Autocomplete ac);
gboolean autocomplete_add(Autocomplete ac, const char *item);
gboolean autocomplete_remove(Autocomplete ac, const char * const item);
GSList * autocomplete_get_list(Autocomplete ac);
gchar * autocomplete_complete(Autocomplete ac, gchar *search_str);
gint autocomplete_length(Autocomplete ac);
char * autocomplete_param_with_func(char *input, int *size, char *command,
    autocomplete_func func);
char * autocomplete_param_with_ac(char *input, int *size, char *command,
    Autocomplete ac);
char * autocomplete_param_no_with_func(char *input, int *size, char *command,
    int arg_number, autocomplete_func func);

#endif
