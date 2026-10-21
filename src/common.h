/*
 * common.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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

#define PROF_STRINGIFY_(n) #n
#define PROF_STRINGIFY(n)  PROF_STRINGIFY_(n)

void auto_free_gchar(gchar** str);

/**
 * Macro for automatically freeing a gchar* string when it goes out of scope.
 *
 * Example usage:
 * ```
 * auto_gchar gchar* myString = g_strdup("Hello, world!");
 * ```
 */
#define auto_gchar __attribute__((__cleanup__(auto_free_gchar)))

void auto_free_gcharv(gchar*** args);

/**
 * Macro for automatically freeing a gchar** string array when it goes out of scope.
 *
 * Example usage:
 * ```
 * auto_gcharv gchar** stringArray = g_strsplit("Hello, world!", " ", -1);
 * ```
 */
#define auto_gcharv __attribute__((__cleanup__(auto_free_gcharv)))

void auto_free_char(char** str);

/**
 * Macro for automatically freeing a char* string when it goes out of scope.
 *
 * Example usage:
 * ```
 * auto_char char* myString = strdup("Hello, world!");
 * ```
 */
#define auto_char __attribute__((__cleanup__(auto_free_char)))

void auto_free_guchar(guchar** str);

/**
 * @brief Macro for automatically freeing a guchar* string when it goes out of scope.
 *
 * This macro is used to automatically free a guchar* string when it goes out of scope.
 * It utilizes the `auto_free_guchar` function and should be placed before the variable declaration.
 *
 * Example usage:
 * ```
 * auto_guchar guchar* myString = g_base64_decode("SGVsbG8sIHdvcmxkIQ==", NULL);
 * ```
 */
#define auto_guchar __attribute__((__cleanup__(auto_free_guchar)))

#define auto_gfd __attribute__((__cleanup__(auto_close_gfd)))

void auto_close_gfd(gint* fd);

#define auto_FILE __attribute__((__cleanup__(auto_close_FILE)))

void auto_close_FILE(FILE** fd);

void auto_free_gerror(GError** err);
#define auto_gerror __attribute__((__cleanup__(auto_free_gerror)))

#if defined(__OpenBSD__)
#define STR_MAYBE_NULL(p) (p) ?: "(null)"
#else
#define STR_MAYBE_NULL(p) (p)
#endif

#define PROF_GERROR_MESSAGE(err) (err) ? (err)->message : "error message missing"
#define PROF_GERROR_FREE(err)  \
    do {                       \
        if (err)               \
            g_error_free(err); \
    } while (0)

typedef struct prof_keyfile_t
{
    gchar* filename;
    GKeyFile* keyfile;
} prof_keyfile_t;

gboolean
load_data_keyfile(prof_keyfile_t* keyfile, const gchar* filename);
gboolean
load_config_keyfile(prof_keyfile_t* keyfile, const gchar* filename);
gboolean
load_custom_keyfile(prof_keyfile_t* keyfile, gchar* filename);
gboolean
save_keyfile(prof_keyfile_t* keyfile);
void
free_keyfile(prof_keyfile_t* keyfile);

/* Our own define of MB_CUR_MAX but this time at compile time */
#define PROF_MB_CUR_MAX 8

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

void prof_wipe_string(gchar* str);

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

gboolean string_to_verbosity(const gchar* cmd, int* verbosity, gchar** err_msg);

gboolean create_dir(const gchar* name);
gboolean copy_file(const gchar* const src, const gchar* const target, const gboolean overwrite_existing);
gboolean strtoi_range(const gchar* str, int* saveptr, int min, int max, gchar** err_msg);
int utf8_display_len(const gchar* const str);
gchar* str_xml_sanitize(const gchar* const str);
void str_clean_directional_chars(gchar* str);
void str_clean_terminal_control_chars(gchar* str);
gint str_ascii_casecmp0(const gchar* s1, const gchar* s2);

gboolean string_matches_one_of(const gchar* what, const gchar* is, gboolean is_can_be_null, const gchar* first, ...) __attribute__((sentinel));
gboolean valid_tls_policy_option(const gchar* is);

gchar* release_get_latest(void);
gboolean release_is_new(const gchar* const curr_version, const gchar* const found_version);

gchar* strip_arg_quotes(const gchar* const input);
gboolean is_notify_enabled(void);

GSList* prof_occurrences(const gchar* const needle, const gchar* const haystack, int offset, gboolean whole_word,
                         GSList** result);
GSList* get_mentions(gboolean whole_word, gboolean case_sensitive, const gchar* const message, const gchar* const nick);

void get_file_paths_recursive(const gchar* directory, GSList** contents);

gchar* get_random_string(int length);

gboolean call_external(gchar** argv);
gchar** format_call_external_argv(const gchar* template, const gchar* url, const gchar* filename);

gchar* unique_filename_from_url(const gchar* url, const gchar* path);
gchar* get_expanded_path(const gchar* path);

gchar* basename_from_url(const gchar* url);

gchar* prof_date_time_format_iso8601(GDateTime* dt);

gchar* prof_get_version(void);
void prof_add_shutdown_routine(void (*routine)(void));

#endif
