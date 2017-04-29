/*
 * common.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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
#include "config.h"

#include <errno.h>
#include <sys/select.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include <glib.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "log.h"
#include "common.h"
#include "tools/p_sha1.h"

struct curl_data_t
{
    char *buffer;
    size_t size;
};

static unsigned long unique_id = 0;

static size_t _data_callback(void *ptr, size_t size, size_t nmemb, void *data);

gboolean
create_dir(char *name)
{
    struct stat sb;

    if (stat(name, &sb) != 0) {
        if (errno != ENOENT || mkdir(name, S_IRWXU) != 0) {
            return FALSE;
        }
    } else {
        if ((sb.st_mode & S_IFDIR) != S_IFDIR) {
            log_debug("create_dir: %s exists and is not a directory!", name);
            return FALSE;
        }
    }

    return TRUE;
}

gboolean
mkdir_recursive(const char *dir)
{
    int i;
    gboolean result = TRUE;

    for (i = 1; i <= strlen(dir); i++) {
        if (dir[i] == '/' || dir[i] == '\0') {
            gchar *next_dir = g_strndup(dir, i);
            result = create_dir(next_dir);
            g_free(next_dir);
            if (!result) {
                break;
            }
        }
    }

    return result;
}

gboolean
copy_file(const char *const sourcepath, const char *const targetpath)
{
    int ch;
    FILE *source = fopen(sourcepath, "rb");
    if (source == NULL) {
        return FALSE;
    }

    FILE *target = fopen(targetpath, "wb");
    if (target == NULL) {
        fclose(source);
        return FALSE;
    }

    while((ch = fgetc(source)) != EOF) {
        fputc(ch, target);
    }

    fclose(source);
    fclose(target);

    return TRUE;
}

char*
str_replace(const char *string, const char *substr,
    const char *replacement)
{
    char *tok = NULL;
    char *newstr = NULL;
    char *head = NULL;

    if (string == NULL)
        return NULL;

    if ( substr == NULL ||
         replacement == NULL ||
         (strcmp(substr, "") == 0))
        return strdup (string);

    newstr = strdup (string);
    head = newstr;

    while ( (tok = strstr ( head, substr ))) {
        char *oldstr = newstr;
        newstr = malloc ( strlen ( oldstr ) - strlen ( substr ) +
            strlen ( replacement ) + 1 );

        if ( newstr == NULL ) {
            free (oldstr);
            return NULL;
        }

        memcpy ( newstr, oldstr, tok - oldstr );
        memcpy ( newstr + (tok - oldstr), replacement, strlen ( replacement ) );
        memcpy ( newstr + (tok - oldstr) + strlen( replacement ),
            tok + strlen ( substr ),
            strlen ( oldstr ) - strlen ( substr ) - ( tok - oldstr ) );
        memset ( newstr + strlen ( oldstr ) - strlen ( substr ) +
            strlen ( replacement ) , 0, 1 );

        head = newstr + (tok - oldstr) + strlen( replacement );
        free (oldstr);
    }

    return newstr;
}

int
str_contains(const char str[], int size, char ch)
{
    int i;
    for (i = 0; i < size; i++) {
        if (str[i] == ch)
            return 1;
    }

    return 0;
}

gboolean
strtoi_range(char *str, int *saveptr, int min, int max, char **err_msg)
{
    char *ptr;
    int val;

    errno = 0;
    val = (int)strtol(str, &ptr, 0);
    if (errno != 0 || *str == '\0' || *ptr != '\0') {
        GString *err_str = g_string_new("");
        g_string_printf(err_str, "Could not convert \"%s\" to a number.", str);
        *err_msg = err_str->str;
        g_string_free(err_str, FALSE);
        return FALSE;
    } else if (val < min || val > max) {
        GString *err_str = g_string_new("");
        g_string_printf(err_str, "Value %s out of range. Must be in %d..%d.", str, min, max);
        *err_msg = err_str->str;
        g_string_free(err_str, FALSE);
        return FALSE;
    }

    *saveptr = val;

    return TRUE;
}

int
utf8_display_len(const char *const str)
{
    if (!str) {
        return 0;
    }

    int len = 0;
    gchar *curr = g_utf8_offset_to_pointer(str, 0);
    while (*curr != '\0') {
        gunichar curru = g_utf8_get_char(curr);
        if (g_unichar_iswide(curru)) {
            len += 2;
        } else {
            len ++;
        }
        curr = g_utf8_next_char(curr);
    }

    return len;
}

char*
file_getline(FILE *stream)
{
    char *buf;
    char *result;
    char *s = NULL;
    size_t s_size = 1;
    int need_exit = 0;

    buf = (char *)malloc(READ_BUF_SIZE);

    while (TRUE) {
        result = fgets(buf, READ_BUF_SIZE, stream);
        if (result == NULL)
            break;
        size_t buf_size = strlen(buf);
        if (buf[buf_size - 1] == '\n') {
            buf_size--;
            buf[buf_size] = '\0';
            need_exit = 1;
        }

        result = (char *)realloc(s, s_size + buf_size);
        if (result == NULL) {
            if (s) {
                free(s);
                s = NULL;
            }
            break;
        }
        s = result;

        memcpy(s + s_size - 1, buf, buf_size);
        s_size += buf_size;
        s[s_size - 1] = '\0';

        if (need_exit != 0 || feof(stream) != 0)
            break;
    }

    free(buf);
    return s;
}

char*
release_get_latest(void)
{
    char *url = "http://www.profanity.im/profanity_version.txt";

    CURL *handle = curl_easy_init();
    struct curl_data_t output;
    output.buffer = NULL;
    output.size = 0;

    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, _data_callback);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 2);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&output);

    curl_easy_perform(handle);
    curl_easy_cleanup(handle);

    if (output.buffer) {
        output.buffer[output.size++] = '\0';
        return output.buffer;
    } else {
        return NULL;
    }
}

gboolean
release_is_new(char *found_version)
{
    int curr_maj, curr_min, curr_patch, found_maj, found_min, found_patch;

    int parse_curr = sscanf(PACKAGE_VERSION, "%d.%d.%d", &curr_maj, &curr_min,
        &curr_patch);
    int parse_found = sscanf(found_version, "%d.%d.%d", &found_maj, &found_min,
        &found_patch);

    if (parse_found == 3 && parse_curr == 3) {
        if (found_maj > curr_maj) {
            return TRUE;
        } else if (found_maj == curr_maj && found_min > curr_min) {
            return TRUE;
        } else if (found_maj == curr_maj && found_min == curr_min
                                        && found_patch > curr_patch) {
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

char*
create_unique_id(char *prefix)
{
    char *result = NULL;
    GString *result_str = g_string_new("");

    unique_id++;
    if (prefix) {
        g_string_printf(result_str, "prof_%s_%lu", prefix, unique_id);
    } else {
        g_string_printf(result_str, "prof_%lu", unique_id);
    }
    result = result_str->str;
    g_string_free(result_str, FALSE);

    return result;
}

void
reset_unique_id(void)
{
    unique_id = 0;
}

char*
p_sha1_hash(char *str)
{
    P_SHA1_CTX ctx;
    uint8_t digest[20];
    uint8_t *input = (uint8_t*)malloc(strlen(str) + 1);
    memcpy(input, str, strlen(str) + 1);

    P_SHA1_Init(&ctx);
    P_SHA1_Update(&ctx, input, strlen(str));
    P_SHA1_Final(&ctx, digest);

    free(input);
    return g_base64_encode(digest, sizeof(digest));
}

static size_t
_data_callback(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    struct curl_data_t *mem = (struct curl_data_t *) data;
    mem->buffer = realloc(mem->buffer, mem->size + realsize + 1);

    if ( mem->buffer )
    {
        memcpy( &( mem->buffer[ mem->size ] ), ptr, realsize );
        mem->size += realsize;
        mem->buffer[ mem->size ] = 0;
    }

    return realsize;
}

char*
get_file_or_linked(char *loc, char *basedir)
{
    char *true_loc = NULL;

    // check for symlink
    if (g_file_test(loc, G_FILE_TEST_IS_SYMLINK)) {
        true_loc = g_file_read_link(loc, NULL);

        // if relative, add basedir
        if (!g_str_has_prefix(true_loc, "/") && !g_str_has_prefix(true_loc, "~")) {
            GString *base_str = g_string_new(basedir);
            g_string_append(base_str, true_loc);
            free(true_loc);
            true_loc = base_str->str;
            g_string_free(base_str, FALSE);
        }
    // use given location
    } else {
        true_loc = strdup(loc);
    }

    return true_loc;
}

char*
strip_arg_quotes(const char *const input)
{
    char *unquoted = strdup(input);

    // Remove starting quote if it exists
    if(strchr(unquoted, '"')) {
        if(strchr(unquoted, ' ') + 1 == strchr(unquoted, '"')) {
            memmove(strchr(unquoted, '"'), strchr(unquoted, '"')+1, strchr(unquoted, '\0') - strchr(unquoted, '"'));
        }
    }

    // Remove ending quote if it exists
    if(strchr(unquoted, '"')) {
        if(strchr(unquoted, '\0') - 1 == strchr(unquoted, '"')) {
            memmove(strchr(unquoted, '"'), strchr(unquoted, '"')+1, strchr(unquoted, '\0') - strchr(unquoted, '"'));
        }
    }

    return unquoted;
}

gboolean
is_notify_enabled(void)
{
    gboolean notify_enabled = FALSE;

#ifdef HAVE_OSXNOTIFY
    notify_enabled = TRUE;
#endif
#ifdef HAVE_LIBNOTIFY
    notify_enabled = TRUE;
#endif
#ifdef PLATFORM_CYGWIN
    notify_enabled = TRUE;
#endif

    return notify_enabled;
}

GSList*
prof_occurrences(const char *const needle, const char *const haystack, int offset, gboolean whole_word, GSList **result)
{
    if (needle == NULL || haystack == NULL) {
        return *result;
    }

    gchar *haystack_curr = g_utf8_offset_to_pointer(haystack, offset);
    if (g_str_has_prefix(haystack_curr, needle)) {
        if (whole_word) {
            gchar *needle_last_ch = g_utf8_offset_to_pointer(needle, g_utf8_strlen(needle, -1)- 1);
            int needle_last_ch_len = mblen(needle_last_ch, MB_CUR_MAX);

            gunichar before = 0;
            gchar *haystack_before_ch = g_utf8_find_prev_char(haystack, haystack_curr);
            if (haystack_before_ch) {
                before = g_utf8_get_char(haystack_before_ch);
            }

            gunichar after = 0;
            gchar *haystack_after_ch = g_utf8_find_next_char(haystack_curr + strlen(needle) - needle_last_ch_len, NULL);
            if (haystack_after_ch) {
                after = g_utf8_get_char(haystack_after_ch);
            }

            if (!g_unichar_isalnum(before) && !g_unichar_isalnum(after)) {
                *result = g_slist_append(*result, GINT_TO_POINTER(offset));
            }
        } else {
            *result = g_slist_append(*result, GINT_TO_POINTER(offset));
        }
    }

    offset++;
    if (g_strcmp0(g_utf8_offset_to_pointer(haystack, offset), "\0") != 0) {
        *result = prof_occurrences(needle, haystack, offset, whole_word, result);
    }

    return *result;
}

int
is_regular_file(const char *path)
{
    struct stat st;
    stat(path, &st);
    return S_ISREG(st.st_mode);
}

int
is_dir(const char *path)
{
    struct stat st;
    stat(path, &st);
    return S_ISDIR(st.st_mode);
}

void
get_file_paths_recursive(const char *path, GSList **contents)
{
    if (!is_dir(path)) {
        return;
    }

    GDir* directory = g_dir_open(path, 0, NULL);
    const gchar *entry = g_dir_read_name(directory);
    while (entry) {
        GString *full = g_string_new(path);
        if (!g_str_has_suffix(full->str, "/")) {
            g_string_append(full, "/");
        }
        g_string_append(full, entry);

        if (is_dir(full->str)) {
            get_file_paths_recursive(full->str, contents);
        } else if (is_regular_file(full->str)) {
            *contents = g_slist_append(*contents, full->str);
        }

        g_string_free(full, FALSE);
        entry = g_dir_read_name(directory);
    }
}
