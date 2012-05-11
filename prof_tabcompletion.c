/* 
 * prof_tabcompletion.c
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

#include <stdlib.h>

#include <glib.h>

#include "prof_tabcompletion.h"

struct p_tabcompletion_t {
    GList *list;
    GList *last_found;
    gchar *search_str;
};

PTabCompletion p_tabcompletion_new(void)
{
    PTabCompletion new = malloc(sizeof(struct p_tabcompletion_t));
    new->list = NULL;
    new->last_found = NULL;
    new->search_str = NULL;

    return new;
}
