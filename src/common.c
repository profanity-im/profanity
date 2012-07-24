/* 
 * common.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include <glib.h>

#include "common.h"

void
p_slist_free_full(GSList *items, GDestroyNotify free_func)
{
    g_slist_foreach (items, (GFunc) free_func, NULL);
    g_slist_free (items);
}

void
create_config_directory()
{
    GString *dir = g_string_new(getenv("HOME"));
    g_string_append(dir, "/.profanity");
    create_dir(dir->str);
    g_string_free(dir, TRUE);
}

void
create_dir(char *name)
{
    int e;
    struct stat sb;

    e = stat(name, &sb);
    if (e != 0)
        if (errno == ENOENT)
            e = mkdir(name, S_IRWXU);
}
