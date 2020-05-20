/*
 * common.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2020 Michael Vetter <jubalh@iodoru.org>
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
#include <gio/gio.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#endif

#include "log.h"
#include "common.h"

struct curl_data_t
{
    char *buffer;
    size_t size;
};

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
copy_file(const char *const sourcepath, const char *const targetpath, const gboolean overwrite_existing)
{
    GFile *source = g_file_new_for_path(sourcepath);
    GFile *dest = g_file_new_for_path(targetpath);
    GError *error = NULL;
    GFileCopyFlags flags = overwrite_existing ? G_FILE_COPY_OVERWRITE : G_FILE_COPY_NONE;
    gboolean success = g_file_copy (source, dest, flags, NULL, NULL, NULL, &error);
    if (error != NULL)
        g_error_free(error);
    g_object_unref(source);
    g_object_unref(dest);
    return success;
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
release_get_latest(void)
{
    char *url = "https://profanity-im.github.io/profanity_version.txt";

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
            gunichar before = 0;
            gchar *haystack_before_ch = g_utf8_find_prev_char(haystack, haystack_curr);
            if (haystack_before_ch) {
                before = g_utf8_get_char(haystack_before_ch);
            }

            gunichar after = 0;
            gchar *haystack_after_ch = haystack_curr + strlen(needle);
            if (haystack_after_ch[0] != '\0') {
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
    int ret = stat(path, &st);
    if (ret != 0) {
        perror(NULL);
        return 0;
    }
    return S_ISREG(st.st_mode);
}

int
is_dir(const char *path)
{
    struct stat st;
    int ret = stat(path, &st);
    if (ret != 0) {
        perror(NULL);
        return 0;
    }
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

char*
get_random_string(int length)
{
    GRand *prng;
    char *rand;
    char alphabet[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

    rand = calloc(length+1, sizeof(char));

    prng = g_rand_new();

    int i;
    for (i = 0; i < length; i++) {
        rand[i] = alphabet[g_rand_int_range(prng, 0, sizeof(alphabet))];
    }
    g_rand_free(prng);

    return rand;
}

GSList*
get_mentions(gboolean whole_word, gboolean case_sensitive, const char *const message, const char *const nick)
{
    GSList *mentions = NULL;
    char *message_search = case_sensitive ? strdup(message) : g_utf8_strdown(message, -1);
    char *mynick_search = case_sensitive ? strdup(nick) : g_utf8_strdown(nick, -1);

    mentions = prof_occurrences(mynick_search, message_search, 0, whole_word, &mentions);

    g_free(message_search);
    g_free(mynick_search);

    return mentions;
}

void
call_external(const char *const exe, const char *const param)
{
    GString *cmd = g_string_new("");

    g_string_append_printf(cmd, "%s %s > /dev/null 2>&1", exe, param);
    log_debug("Calling external: %s", cmd->str);
    FILE *stream = popen(cmd->str, "r");

    pclose(stream);
    g_string_free(cmd, TRUE);
}
