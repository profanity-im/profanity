/*
 * common.h
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

#ifndef COMMON_H
#define COMMON_H

#include <glib.h>

#if !GLIB_CHECK_VERSION(2,28,0)
#define g_slist_free_full(items, free_func)      p_slist_free_full(items, free_func)
#endif

#ifndef NOTIFY_CHECK_VERSION
#define notify_notification_new(summary, body, icon) notify_notification_new(summary, body, icon, NULL)
#endif

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

void p_slist_free_full(GSList *items, GDestroyNotify free_func);
void create_dir(char *name);
char * str_replace(const char *string, const char *substr,
    const char *replacement);
int str_contains(char str[], int size, char ch);

#endif
