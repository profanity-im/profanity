/*
 * common.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
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
#include <fcntl.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include <glib.h>
#include <gio/gio.h>
#include <glib/gstdio.h>

#ifdef HAVE_NCURSESW_NCURSES_H
#include <ncursesw/ncurses.h>
#elif HAVE_NCURSES_H
#include <ncurses.h>
#elif HAVE_CURSES_H
#include <curses.h>
#endif

#include "log.h"
#include "common.h"
#include "config/files.h"
#include "ui/ui.h"

#ifdef HAVE_GIT_VERSION
#include "gitversion.h"
#endif

struct curl_data_t
{
    char* buffer;
    size_t size;
};

static size_t _data_callback(void* ptr, size_t size, size_t nmemb, void* data);
static gchar* _get_file_or_linked(gchar* loc);

/**
 * Frees the memory allocated for a gchar* string.
 *
 * @param str Pointer to the gchar* string to be freed. If NULL, no action is taken.
 */
void
auto_free_gchar(gchar** str)
{
    if (str == NULL)
        return;
    g_free(*str);
}

/**
 * Frees the memory allocated for a guchar* string.
 *
 * @param str Pointer to the guchar* string to be freed. If NULL, no action is taken.
 */
void
auto_free_guchar(guchar** ptr)
{
    if (ptr == NULL) {
        return;
    }
    g_free(*ptr);
}

/**
 * Frees the memory allocated for a gchar** string array.
 *
 * @param args Pointer to the gchar** string array to be freed. If NULL, no action is taken.
 */
void
auto_free_gcharv(gchar*** args)
{
    if (args == NULL)
        return;
    g_strfreev(*args);
}

/**
 * Frees the memory allocated for a char* string.
 *
 * @param str Pointer to the char* string to be freed. If NULL, no action is taken.
 */
void
auto_free_char(char** str)
{
    if (str == NULL)
        return;
    free(*str);
}

/**
 * Closes the file descriptor.
 *
 * @param fd file descriptor handle. If NULL, no action is taken.
 */
void
auto_close_gfd(gint* fd)
{
    if (fd == NULL)
        return;

    if (close(*fd) == EOF)
        log_error(g_strerror(errno));
}

/**
 * Closes file stream
 *
 * @param fd file descriptor handle opened with fopen. If NULL, no action is taken.
 */
void
auto_close_FILE(FILE** fd)
{
    if (fd == NULL)
        return;

    if (fclose(*fd) == EOF)
        log_error(g_strerror(errno));
}

void
auto_free_gerror(GError** err)
{
    if (err == NULL)
        return;

    PROF_GERROR_FREE(*err);
}

static gboolean
_load_keyfile(prof_keyfile_t* keyfile)
{
    auto_gerror GError* error = NULL;
    keyfile->keyfile = g_key_file_new();

    if (g_key_file_load_from_file(keyfile->keyfile, keyfile->filename, G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, &error)) {
        return TRUE;
    } else if (error && error->code != G_FILE_ERROR_NOENT) {
        log_warning("[Keyfile] error loading %s: %s", keyfile->filename, error->message);
    } else {
        log_warning("[Keyfile] no such file: %s", keyfile->filename);
    }
    return FALSE;
}

gboolean
load_data_keyfile(prof_keyfile_t* keyfile, const char* filename)
{
    auto_gchar gchar* loc = files_get_data_path(filename);
    return load_custom_keyfile(keyfile, _get_file_or_linked(loc));
}

gboolean
load_config_keyfile(prof_keyfile_t* keyfile, const char* filename)
{
    auto_gchar gchar* loc = files_get_config_path(filename);
    return load_custom_keyfile(keyfile, _get_file_or_linked(loc));
}

gboolean
load_custom_keyfile(prof_keyfile_t* keyfile, gchar* filename)
{
    keyfile->filename = filename;

    if (g_file_test(keyfile->filename, G_FILE_TEST_EXISTS)) {
        g_chmod(keyfile->filename, S_IRUSR | S_IWUSR);
    } else {
        int fno = g_creat(keyfile->filename, S_IRUSR | S_IWUSR);
        if (fno != -1)
            g_close(fno, NULL);
    }

    return _load_keyfile(keyfile);
}

gboolean
save_keyfile(prof_keyfile_t* keyfile)
{
    auto_gerror GError* error = NULL;
    if (!g_key_file_save_to_file(keyfile->keyfile, keyfile->filename, &error)) {
        log_error("[Keyfile]: saving file %s failed! %s", STR_MAYBE_NULL(keyfile->filename), PROF_GERROR_MESSAGE(error));
        return FALSE;
    }
    g_chmod(keyfile->filename, S_IRUSR | S_IWUSR);
    return TRUE;
}

void
free_keyfile(prof_keyfile_t* keyfile)
{
    log_debug("[Keyfile]: free %s", STR_MAYBE_NULL(keyfile->filename));
    if (keyfile->keyfile)
        g_key_file_free(keyfile->keyfile);
    keyfile->keyfile = NULL;
    g_free(keyfile->filename);
    keyfile->filename = NULL;
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
    GFileCopyFlags flags = overwrite_existing ? G_FILE_COPY_OVERWRITE : G_FILE_COPY_NONE;
    gboolean success = g_file_copy(source, dest, flags, NULL, NULL, NULL, NULL);
    g_object_unref(source);
    g_object_unref(dest);
    return success;
}

/**
 * @brief Replaces all occurrences of a substring with a replacement in a string.
 *
 * @param string      The input string to perform replacement on.
 * @param substr      The substring to be replaced.
 * @param replacement The replacement string.
 * @return            The modified string with replacements, or NULL on failure.
 *
 * @note Remember to free returned value using `auto_char` or `free()` when it is no longer needed.
 * @note If 'string' is NULL, the function returns NULL.
 * @note If 'substr' or 'replacement' is NULL or an empty string, a duplicate of 'string' is returned.
 */
char*
str_replace(const char* string, const char* substr,
            const char* replacement)
{
    const char* head = NULL;
    const char* tok = NULL;
    char* newstr = NULL;
    char* wr = NULL;
    size_t num_substr = 0;
    size_t len_string, len_substr, len_replacement, len_string_remains, len_newstr;

    if (string == NULL)
        return NULL;

    if (substr == NULL || replacement == NULL || (strcmp(substr, "") == 0))
        return strdup(string);

    tok = string;
    while ((tok = strstr(tok, substr))) {
        num_substr++;
        tok++;
    }

    if (num_substr == 0)
        return strdup(string);

    len_string = strlen(string);
    len_substr = strlen(substr);
    len_replacement = strlen(replacement);
    len_newstr = len_string - (num_substr * len_substr) + (num_substr * len_replacement);
    newstr = malloc(len_newstr + 1);
    if (newstr == NULL) {
        return NULL;
    }
    len_string_remains = len_string;

    head = string;
    wr = newstr;

    while ((tok = strstr(head, substr))) {
        size_t l = tok - head;
        memcpy(wr, head, l);
        wr += l;
        memcpy(wr, replacement, len_replacement);
        wr += len_replacement;
        len_string_remains -= len_substr + l;

        head = tok + len_substr;
    }
    memcpy(wr, head, len_string_remains);
    newstr[len_newstr] = '\0';

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

gboolean
string_matches_one_of(const char* what, const char* is, gboolean is_can_be_null, const char* first, ...)
{
    gboolean ret = FALSE;
    va_list ap;
    const char* cur = first;
    if (!is)
        return is_can_be_null;

    va_start(ap, first);
    while (cur != NULL) {
        if (g_strcmp0(is, cur) == 0) {
            ret = TRUE;
            break;
        }
        cur = va_arg(ap, const char*);
    }
    va_end(ap);
    if (!ret && what) {
        cons_show("Invalid %s: '%s'", what, is);
        char errmsg[256] = { 0 };
        size_t sz = 0;
        int s = snprintf(errmsg, sizeof(errmsg) - sz, "%s must be one of:", what);
        if (s < 0 || s + sz >= sizeof(errmsg))
            return ret;
        sz += s;

        cur = first;
        va_start(ap, first);
        while (cur != NULL) {
            const char* next = va_arg(ap, const char*);
            if (next) {
                s = snprintf(errmsg + sz, sizeof(errmsg) - sz, " '%s',", cur);
            } else {
                /* remove last ',' */
                sz--;
                errmsg[sz] = '\0';
                s = snprintf(errmsg + sz, sizeof(errmsg) - sz, " or '%s'.", cur);
            }
            if (s < 0 || s + sz >= sizeof(errmsg)) {
                log_debug("Error message too long or some other error occurred (%d).", s);
                s = -1;
                break;
            }
            sz += s;
            cur = next;
        }
        va_end(ap);
        if (s > 0)
            cons_show(errmsg);
    }
    return ret;
}

gboolean
valid_tls_policy_option(const char* is)
{
    return string_matches_one_of("TLS policy", is, TRUE, "force", "allow", "trust", "disable", "legacy", "direct", NULL);
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
    curl_easy_setopt(handle, CURLOPT_TIMEOUT, (long)2);
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

static gchar*
_get_file_or_linked(gchar* loc)
{
    auto_gchar gchar* basedir = g_path_get_dirname(loc);
    gchar* true_loc = g_strdup(loc);

    // check for symlink
    while (g_file_test(true_loc, G_FILE_TEST_IS_SYMLINK)) {
        gchar* tmp = g_file_read_link(true_loc, NULL);
        g_free(true_loc);
        true_loc = tmp;

        // if relative, add basedir
        if (!g_str_has_prefix(true_loc, "/") && !g_str_has_prefix(true_loc, "~")) {
            tmp = g_strdup_printf("%s/%s", basedir, true_loc);
            g_free(true_loc);
            true_loc = tmp;
        }
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

    do {
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
    } while (g_strcmp0(g_utf8_offset_to_pointer(haystack, offset), "\0") != 0);

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
    gchar* full;
    while (entry) {
        if (g_str_has_suffix(path, "/")) {
            full = g_strdup_printf("%s%s", path, entry);
        } else {
            full = g_strdup_printf("%s/%s", path, entry);
        }

        if (is_dir(full)) {
            get_file_paths_recursive(full, contents);
        } else if (is_regular_file(full)) {
            *contents = g_slist_append(*contents, full);
        }

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
    auto_gchar gchar* message_search = case_sensitive ? g_strdup(message) : g_utf8_strdown(message, -1);
    auto_gchar gchar* mynick_search = case_sensitive ? g_strdup(nick) : g_utf8_strdown(nick, -1);

    mentions = prof_occurrences(mynick_search, message_search, 0, whole_word, &mentions);

    return mentions;
}

gboolean
call_external(gchar** argv)
{
    auto_gerror GError* spawn_error = NULL;
    gboolean is_successful;

    GSpawnFlags flags = G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL;

    is_successful = g_spawn_async(NULL, // Inherit the parent PWD
                                  argv,
                                  NULL, // Inherit the parent environment
                                  flags,
                                  NULL, NULL, NULL,
                                  &spawn_error);
    if (!is_successful) {
        auto_gchar gchar* cmd = g_strjoinv(" ", argv);
        log_error("Spawning '%s' failed with error '%s'", cmd, PROF_GERROR_MESSAGE(spawn_error));
    }

    return is_successful;
}

/**
 * @brief Formats an argument vector for calling an external command with placeholders.
 *
 * This function constructs an argument vector (argv) based on the provided template string, replacing placeholders ("%u" and "%p") with the provided URL and filename, respectively.
 *
 * @param template  The template string with placeholders.
 * @param url       The URL to replace "%u" (or NULL to skip).
 * @param filename  The filename to replace "%p" (or NULL to skip).
 * @return          The constructed argument vector (argv) as a null-terminated array of strings.
 *
 * @note Remember to free the returned argument vector using `auto_gcharv` or `g_strfreev()`.
 */
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

static gchar*
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

static gboolean
_has_directory_suffix(const char* path)
{
    return (g_str_has_suffix(path, ".")
            || g_str_has_suffix(path, "..")
            || g_str_has_suffix(path, G_DIR_SEPARATOR_S));
}

/**
 * @brief Extracts the basename from a given URL.
 *
 * @param url The URL to extract the basename from.
 * @return The extracted basename or a default name ("index") if the basename has a directory suffix.
 *
 * @note The returned string must be freed by the caller using `auto_free` or `free()`.
 */
char*
basename_from_url(const char* url)
{
    const char* default_name = "index";

    GFile* file = g_file_new_for_commandline_arg(url);
    auto_gchar gchar* basename = g_file_get_basename(file);
    g_object_unref(file);

    if (_has_directory_suffix(basename)) {
        return strdup(default_name);
    }

    return strdup(basename);
}

/**
 * Expands the provided path by resolving special characters like '~' and 'file://'.
 *
 * @param path The path to expand.
 * @return The expanded path.
 *
 * @note Remember to free the returned value using `auto_gchar` or `g_free()`.
 */
gchar*
get_expanded_path(const char* path)
{
    if (g_str_has_prefix(path, "file://")) {
        path += strlen("file://");
    }
    if (strlen(path) >= 2 && path[0] == '~' && path[1] == '/') {
        return g_strdup_printf("%s/%s", getenv("HOME"), path + 2);
    } else {
        return g_strdup_printf("%s", path);
    }
}

/**
 * Generates a unique filename based on the provided URL and path.
 *
 * @param url The URL to derive the basename from.
 * @param path The path to prepend to the generated filename. If NULL, the current directory is used.
 * @return A unique filename combining the path and derived basename from the URL.
 *
 * @note Remember to free the returned value using `auto_gchar` or `g_free()`.
 */
gchar*
unique_filename_from_url(const char* url, const char* path)
{
    auto_gchar gchar* realpath = NULL;

    // Default to './' as path when none has been provided.
    if (path == NULL) {
        realpath = g_strdup("./");
    } else {
        realpath = get_expanded_path(path);
    }

    // Resolves paths such as './../.' for path.
    GFile* target = g_file_new_for_commandline_arg(realpath);
    auto_gchar gchar* filename = NULL;

    if (_has_directory_suffix(realpath) || g_file_test(realpath, G_FILE_TEST_IS_DIR)) {
        // The target should be used as a directory. Assume that the basename
        // should be derived from the URL.
        auto_char char* basename = basename_from_url(url);
        filename = g_build_filename(g_file_peek_path(target), basename, NULL);
    } else {
        // Just use the target as filename.
        filename = g_build_filename(g_file_peek_path(target), NULL);
    }

    gchar* unique_filename = _unique_filename(filename);
    if (unique_filename == NULL) {
        return NULL;
    }

    g_object_unref(target);

    return unique_filename;
}

/* build profanity version string.
 * example: 0.13.1dev.master.69d8c1f9
 */
gchar*
prof_get_version(void)
{
    if (strcmp(PACKAGE_STATUS, "development") == 0) {
#ifdef HAVE_GIT_VERSION
        return g_strdup_printf("%sdev.%s.%s", PACKAGE_VERSION, PROF_GIT_BRANCH, PROF_GIT_REVISION);
#else
        return g_strdup_printf("%sdev", PACKAGE_VERSION);
#endif
    } else {
        return g_strdup_printf("%s", PACKAGE_VERSION);
    }
}
