/*
 * common.h
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

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

#include <glib.h>

#if !GLIB_CHECK_VERSION(2,28,0)
#define g_slist_free_full(items, free_func)         p_slist_free_full(items, free_func)
#endif

#if !GLIB_CHECK_VERSION(2,30,0)
#define g_utf8_substring(str, start_pos, end_pos)   p_utf8_substring(str, start_pos, end_pos)
#endif

#ifndef NOTIFY_CHECK_VERSION
#define notify_notification_new(summary, body, icon) notify_notification_new(summary, body, icon, NULL)
#endif

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define FREE_SET_NULL(resource) \
do { \
    free(resource); \
    resource = NULL; \
} while (0)

#define GFREE_SET_NULL(resource) \
do { \
    g_free(resource); \
    resource = NULL; \
} while (0)

typedef enum {
    CONTACT_OFFLINE,
    CONTACT_ONLINE,
    CONTACT_AWAY,
    CONTACT_DND,
    CONTACT_CHAT,
    CONTACT_XA
} contact_presence_t;

typedef enum {
    RESOURCE_ONLINE,
    RESOURCE_AWAY,
    RESOURCE_DND,
    RESOURCE_CHAT,
    RESOURCE_XA
} resource_presence_t;

gchar* p_utf8_substring(const gchar *str, glong start_pos, glong end_pos);
void p_slist_free_full(GSList *items, GDestroyNotify free_func);
gboolean create_dir(char *name);
gboolean mkdir_recursive(const char *dir);
char * str_replace(const char *string, const char *substr,
    const char *replacement);
int str_contains(char str[], int size, char ch);
char * prof_getline(FILE *stream);
char* release_get_latest(void);
gboolean release_is_new(char *found_version);
gchar * xdg_get_config_home(void);
gchar * xdg_get_data_home(void);

gboolean valid_resource_presence_string(const char * const str);
const char * string_from_resource_presence(resource_presence_t presence);
resource_presence_t resource_presence_from_string(const char * const str);
contact_presence_t contact_presence_from_resource_presence(resource_presence_t resource_presence);

char * generate_unique_id(char *prefix);

int cmp_win_num(gconstpointer a, gconstpointer b);
int get_next_available_win_num(GList *used);

#endif
