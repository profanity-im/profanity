/*
 * common.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2023 Michael Vetter <jubalh@iodoru.org>
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
#elif HAVE_CURSES_H
#include <curses.h>
#endif

#include "log.h"
#include "common.h"

struct curl_data_t
{
    char* buffer;
    size_t size;
};

static size_t _data_callback(void* ptr, size_t size, size_t nmemb, void* data);

void
auto_free_gchar(gchar** str)
{
    if (str == NULL)
        return;
    g_free(*str);
}

void
auto_free_gcharv(gchar*** args)
{
    if (args == NULL)
        return;
    g_strfreev(*args);
}

void
auto_free_char(char** str)
{
    if (str == NULL)
        return;
    free(*str);
}

gboolean
string_to_verbosity(const char* cmd, int* verbosity, gchar** err_msg)
{
    return strtoi_range(cmd, verbosity, 0, 3, err_msg);
}

gboolean
create_dir(const char* name)
{
    if (g_mkdir_with_parents(name, S_IRWXU) != 0) {
        log_error("Failed to create directory at '%s' with error '%s'", name, strerror(errno));
        return FALSE;
    }
    return TRUE;
}

gboolean
copy_file(const char* const sourcepath, const char* const targetpath, const gboolean overwrite_existing)
{
    GFile* source = g_file_new_for_path(sourcepath);
    GFile* dest = g_file_new_for_path(targetpath);
    GError* error = NULL;
    GFileCopyFlags flags = overwrite_existing ? G_FILE_COPY_OVERWRITE : G_FILE_COPY_NONE;
    gboolean success = g_file_copy(source, dest, flags, NULL, NULL, NULL, &error);
    if (error != NULL)
        g_error_free(error);
    g_object_unref(source);
    g_object_unref(dest);
    return success;
}

char*
str_replace(const char* string, const char* substr,
            const char* replacement)
{
    char* tok = NULL;
    char* newstr = NULL;
    char* head = NULL;

    if (string == NULL)
        return NULL;

    if (substr == NULL || replacement == NULL || (strcmp(substr, "") == 0))
        return strdup(string);

    newstr = strdup(string);
    head = newstr;

    while ((tok = strstr(head, substr))) {
        char* oldstr = newstr;
        newstr = malloc(strlen(oldstr) - strlen(substr) + strlen(replacement) + 1);

        if (newstr == NULL) {
            free(oldstr);
            return NULL;
        }

        memcpy(newstr, oldstr, tok - oldstr);
        memcpy(newstr + (tok - oldstr), replacement, strlen(replacement));
        memcpy(newstr + (tok - oldstr) + strlen(replacement),
               tok + strlen(substr),
               strlen(oldstr) - strlen(substr) - (tok - oldstr));
        memset(newstr + strlen(oldstr) - strlen(substr) + strlen(replacement), 0, 1);

        head = newstr + (tok - oldstr) + strlen(replacement);
        free(oldstr);
    }

    return newstr;
}

gboolean
strtoi_range(const char* str, int* saveptr, int min, int max, gchar** err_msg)
{
    char* ptr;
    int val;
    if (str == NULL) {
        if (err_msg)
            *err_msg = g_strdup_printf("'str' input pointer can not be NULL");
        return FALSE;
    }
    errno = 0;
    val = (int)strtol(str, &ptr, 0);
    if (errno != 0 || *str == '\0' || *ptr != '\0') {
        if (err_msg)
            *err_msg = g_strdup_printf("Could not convert \"%s\" to a number.", str);
        return FALSE;
    } else if (val < min || val > max) {
        if (err_msg)
            *err_msg = g_strdup_printf("Value %s out of range. Must be in %d..%d.", str, min, max);
        return FALSE;
    }

    *saveptr = val;

    return TRUE;
}

int
utf8_display_len(const char* const str)
{
    if (!str) {
        return 0;
    }

    int len = 0;
    gchar* curr = g_utf8_offset_to_pointer(str, 0);
    while (*curr != '\0') {
        gunichar curru = g_utf8_get_char(curr);
        if (g_unichar_iswide(curru)) {
            len += 2;
        } else {
            len++;
        }
        curr = g_utf8_next_char(curr);
    }

    return len;
}

char*
release_get_latest(void)
{
    char* url = "https://profanity-im.github.io/profanity_version.txt";

    CURL* handle = curl_easy_init();
    struct curl_data_t output;
    output.buffer = NULL;
    output.size = 0;

    curl_easy_setopt(handle, CURLOPT_URL, url);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, _data_callback);
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, 2);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)&output);

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
release_is_new(char* found_version)
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
_data_callback(void* ptr, size_t size, size_t nmemb, void* data)
{
    size_t realsize = size * nmemb;
    struct curl_data_t* mem = (struct curl_data_t*)data;
    mem->buffer = realloc(mem->buffer, mem->size + realsize + 1);

    if (mem->buffer) {
        memcpy(&(mem->buffer[mem->size]), ptr, realsize);
        mem->size += realsize;
        mem->buffer[mem->size] = 0;
    }

    return realsize;
}

char*
get_file_or_linked(char* loc, char* basedir)
{
    char* true_loc = NULL;

    // check for symlink
    if (g_file_test(loc, G_FILE_TEST_IS_SYMLINK)) {
        true_loc = g_file_read_link(loc, NULL);

        // if relative, add basedir
        if (!g_str_has_prefix(true_loc, "/") && !g_str_has_prefix(true_loc, "~")) {
            GString* base_str = g_string_new(basedir);
            g_string_append(base_str, "/");
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
strip_arg_quotes(const char* const input)
{
    char* unquoted = strdup(input);

    // Remove starting quote if it exists
    if (strchr(unquoted, '"')) {
        if (strchr(unquoted, ' ') + 1 == strchr(unquoted, '"')) {
            memmove(strchr(unquoted, '"'), strchr(unquoted, '"') + 1, strchr(unquoted, '\0') - strchr(unquoted, '"'));
        }
    }

    // Remove ending quote if it exists
    if (strchr(unquoted, '"')) {
        if (strchr(unquoted, '\0') - 1 == strchr(unquoted, '"')) {
            memmove(strchr(unquoted, '"'), strchr(unquoted, '"') + 1, strchr(unquoted, '\0') - strchr(unquoted, '"'));
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
prof_occurrences(const char* const needle, const char* const haystack, int offset, gboolean whole_word, GSList** result)
{
    if (needle == NULL || haystack == NULL) {
        return *result;
    }

    gchar* haystack_curr = g_utf8_offset_to_pointer(haystack, offset);
    if (g_str_has_prefix(haystack_curr, needle)) {
        if (whole_word) {
            gunichar before = 0;
            gchar* haystack_before_ch = g_utf8_find_prev_char(haystack, haystack_curr);
            if (haystack_before_ch) {
                before = g_utf8_get_char(haystack_before_ch);
            }

            gunichar after = 0;
            gchar* haystack_after_ch = haystack_curr + strlen(needle);
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
is_regular_file(const char* path)
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
is_dir(const char* path)
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
get_file_paths_recursive(const char* path, GSList** contents)
{
    if (!is_dir(path)) {
        return;
    }

    GDir* directory = g_dir_open(path, 0, NULL);
    const gchar* entry = g_dir_read_name(directory);
    while (entry) {
        GString* full = g_string_new(path);
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
    GRand* prng;
    char* rand;
    char alphabet[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int endrange = sizeof(alphabet) - 1;

    rand = calloc(length + 1, sizeof(char));

    prng = g_rand_new();

    for (int i = 0; i < length; i++) {
        rand[i] = alphabet[g_rand_int_range(prng, 0, endrange)];
    }
    g_rand_free(prng);

    return rand;
}

GSList*
get_mentions(gboolean whole_word, gboolean case_sensitive, const char* const message, const char* const nick)
{
    GSList* mentions = NULL;
    gchar* message_search = case_sensitive ? g_strdup(message) : g_utf8_strdown(message, -1);
    gchar* mynick_search = case_sensitive ? g_strdup(nick) : g_utf8_strdown(nick, -1);

    mentions = prof_occurrences(mynick_search, message_search, 0, whole_word, &mentions);

    g_free(message_search);
    g_free(mynick_search);

    return mentions;
}

gboolean
call_external(gchar** argv)
{
    GError* spawn_error;
    gboolean is_successful;

    GSpawnFlags flags = G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL;

    is_successful = g_spawn_async(NULL, // Inherit the parent PWD
                                  argv,
                                  NULL, // Inherit the parent environment
                                  flags,
                                  NULL, NULL, NULL,
                                  &spawn_error);
    if (!is_successful) {
        gchar* cmd = g_strjoinv(" ", argv);
        log_error("Spawning '%s' failed with error '%s'", cmd, spawn_error ? spawn_error->message : "Unknown, spawn_error is NULL");

        g_error_free(spawn_error);
        g_free(cmd);
    }

    return is_successful;
}

gchar**
format_call_external_argv(const char* template, const char* url, const char* filename)
{
    gchar** argv = g_strsplit(template, " ", 0);

    guint num_args = 0;
    while (argv[num_args]) {
        if (0 == g_strcmp0(argv[num_args], "%u") && url != NULL) {
            g_free(argv[num_args]);
            argv[num_args] = g_strdup(url);
        } else if (0 == g_strcmp0(argv[num_args], "%p") && filename != NULL) {
            g_free(argv[num_args]);
            argv[num_args] = strdup(filename);
        }
        num_args++;
    }

    return argv;
}

gchar*
_unique_filename(const char* filename)
{
    gchar* unique = g_strdup(filename);

    unsigned int i = 0;
    while (g_file_test(unique, G_FILE_TEST_EXISTS)) {
        g_free(unique);

        if (i > 1000) { // Give up after 1000 attempts.
            return NULL;
        }

        unique = g_strdup_printf("%s.%u", filename, i);
        if (!unique) {
            return NULL;
        }

        i++;
    }

    return unique;
}

bool
_has_directory_suffix(const char* path)
{
    return (g_str_has_suffix(path, ".")
            || g_str_has_suffix(path, "..")
            || g_str_has_suffix(path, G_DIR_SEPARATOR_S));
}

char*
_basename_from_url(const char* url)
{
    const char* default_name = "index";

    GFile* file = g_file_new_for_commandline_arg(url);
    char* basename = g_file_get_basename(file);

    if (_has_directory_suffix(basename)) {
        g_free(basename);
        basename = strdup(default_name);
    }

    g_object_unref(file);

    return basename;
}

gchar*
get_expanded_path(const char* path)
{
    GString* exp_path = g_string_new("");
    gchar* result;

    if (g_str_has_prefix(path, "file://")) {
        path += strlen("file://");
    }
    if (strlen(path) >= 2 && path[0] == '~' && path[1] == '/') {
        g_string_printf(exp_path, "%s/%s", getenv("HOME"), path + 2);
    } else {
        g_string_printf(exp_path, "%s", path);
    }

    result = exp_path->str;
    g_string_free(exp_path, FALSE);

    return result;
}

gchar*
unique_filename_from_url(const char* url, const char* path)
{
    gchar* realpath;

    // Default to './' as path when none has been provided.
    if (path == NULL) {
        realpath = g_strdup("./");
    } else {
        realpath = get_expanded_path(path);
    }

    // Resolves paths such as './../.' for path.
    GFile* target = g_file_new_for_commandline_arg(realpath);
    gchar* filename = NULL;

    if (_has_directory_suffix(realpath) || g_file_test(realpath, G_FILE_TEST_IS_DIR)) {
        // The target should be used as a directory. Assume that the basename
        // should be derived from the URL.
        char* basename = _basename_from_url(url);
        filename = g_build_filename(g_file_peek_path(target), basename, NULL);
        g_free(basename);
    } else {
        // Just use the target as filename.
        filename = g_build_filename(g_file_peek_path(target), NULL);
    }

    gchar* unique_filename = _unique_filename(filename);
    if (unique_filename == NULL) {
        g_free(filename);
        g_free(realpath);
        return NULL;
    }

    g_object_unref(target);
    g_free(filename);
    g_free(realpath);

    return unique_filename;
}

void
glib_hash_table_free(GHashTable* hash_table)
{
    g_hash_table_remove_all(hash_table);
    g_hash_table_unref(hash_table);
}
