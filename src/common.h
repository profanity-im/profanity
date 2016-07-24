/*
 * common.h
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <wchar.h>

#include <glib.h>

#if !GLIB_CHECK_VERSION(2,28,0)
#define g_slist_free_full(items, free_func)         p_slist_free_full(items, free_func)
#define g_list_free_full(items, free_func)          p_list_free_full(items, free_func)
#endif

#if !GLIB_CHECK_VERSION(2,30,0)
#define g_utf8_substring(str, start_pos, end_pos)   p_utf8_substring(str, start_pos, end_pos)
#endif

#if !GLIB_CHECK_VERSION(2,32,0)
#define g_hash_table_add(hash_table, key)           p_hash_table_add(hash_table, key)
#define g_hash_table_contains(hash_table, key)      p_hash_table_contains(hash_table, key)
#endif

#ifndef NOTIFY_CHECK_VERSION
#define notify_notification_new(summary, body, icon) notify_notification_new(summary, body, icon, NULL)
#endif

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

// assume malloc stores at most 8 bytes for size of allocated memory
// and page size is at least 4KB
#define READ_BUF_SIZE 4088


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
void p_list_free_full(GList *items, GDestroyNotify free_func);
gboolean p_hash_table_add(GHashTable *hash_table, gpointer key);
gboolean p_hash_table_contains(GHashTable *hash_table, gconstpointer key);

gboolean create_dir(char *name);
gboolean mkdir_recursive(const char *dir);
gboolean copy_file(const char *const src, const char *const target);
char* str_replace(const char *string, const char *substr, const char *replacement);
int str_contains(const char str[], int size, char ch);
gboolean strtoi_range(char *str, int *saveptr, int min, int max, char **err_msg);
int utf8_display_len(const char *const str);
char* prof_getline(FILE *stream);
char* release_get_latest(void);
gboolean release_is_new(char *found_version);

gboolean valid_resource_presence_string(const char *const str);
const char* string_from_resource_presence(resource_presence_t presence);
resource_presence_t resource_presence_from_string(const char *const str);
contact_presence_t contact_presence_from_resource_presence(resource_presence_t resource_presence);

char* p_sha1_hash(char *str);
char* create_unique_id(char *prefix);
void reset_unique_id(void);

int cmp_win_num(gconstpointer a, gconstpointer b);
int get_next_available_win_num(GList *used);

char* get_file_or_linked(char *loc, char *basedir);
char* strip_arg_quotes(const char *const input);
gboolean is_notify_enabled(void);

GSList* prof_occurrences(const char *const needle, const char *const haystack, int offset, gboolean whole_word,
    GSList **result);

#endif
