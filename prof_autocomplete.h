/* 
 * prof_autocomplete.h
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
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

#ifndef PROF_AUTOCOMPLETE_H
#define PROF_AUTOCOMPLETE_H

#include <glib.h>

typedef struct p_autocomplete_t *PAutocomplete;

PAutocomplete p_autocomplete_new(void);
void p_autocomplete_clear(PAutocomplete ac, GDestroyNotify free_func);
void p_autocomplete_reset(PAutocomplete ac);
void p_autocomplete_add(PAutocomplete ac, void *item, char * (*str_func)(void *), 
    GDestroyNotify free_func);
void p_autocomplete_remove(PAutocomplete ac, char *item, char * (*str_func)(void *),   GDestroyNotify free_func);
GSList * p_autocomplete_get_list(PAutocomplete ac, void * (*copy_func)(void *));
gchar * p_autocomplete_complete(PAutocomplete ac, gchar *search_str, 
    char * (*str_func)(void *));

#endif
