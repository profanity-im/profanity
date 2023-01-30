/*
 * common.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
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

#ifndef NOTIFY_CHECK_VERSION
#define notify_notification_new(summary, body, icon) notify_notification_new(summary, body, icon, NULL)
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

void auto_free_gchar(gchar** str);
#define auto_gchar __attribute__((__cleanup__(auto_free_gchar)))
void auto_free_gcharv(gchar*** args);
#define auto_gcharv __attribute__((__cleanup__(auto_free_gcharv)))
void auto_free_char(char** str);
#define auto_char __attribute__((__cleanup__(auto_free_char)))

#if defined(__OpenBSD__)
#define STR_MAYBE_NULL(p) (p) ?: "(null)"
#else
#define STR_MAYBE_NULL(p) (p)
#endif

// assume malloc stores at most 8 bytes for size of allocated memory
// and page size is at least 4KB
#define READ_BUF_SIZE 4088

#define FREE_SET_NULL(resource) \
    do {                        \
        free(resource);         \
        resource = NULL;        \
    } while (0)

#define GFREE_SET_NULL(resource) \
    do {                         \
        g_free(resource);        \
        resource = NULL;         \
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

gboolean string_to_verbosity(const char* cmd, int* verbosity, gchar** err_msg);

gboolean create_dir(const char* name);
gboolean copy_file(const char* const src, const char* const target, const gboolean overwrite_existing);
char* str_replace(const char* string, const char* substr, const char* replacement);
gboolean strtoi_range(const char* str, int* saveptr, int min, int max, char** err_msg);
int utf8_display_len(const char* const str);

char* release_get_latest(void);
gboolean release_is_new(char* found_version);

char* get_file_or_linked(char* loc, char* basedir);
char* strip_arg_quotes(const char* const input);
gboolean is_notify_enabled(void);

GSList* prof_occurrences(const char* const needle, const char* const haystack, int offset, gboolean whole_word,
                         GSList** result);
GSList* get_mentions(gboolean whole_word, gboolean case_sensitive, const char* const message, const char* const nick);

int is_regular_file(const char* path);
int is_dir(const char* path);
void get_file_paths_recursive(const char* directory, GSList** contents);

char* get_random_string(int length);

gboolean call_external(gchar** argv);
gchar** format_call_external_argv(const char* template, const char* url, const char* filename);

gchar* unique_filename_from_url(const char* url, const char* path);
gchar* get_expanded_path(const char* path);

void glib_hash_table_free(GHashTable* hash_table);

#endif
