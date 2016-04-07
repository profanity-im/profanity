/*
 * common.c
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
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
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

#include <sys/select.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
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

#include "tools/p_sha1.h"

#include "log.h"
#include "common.h"


struct curl_data_t
{
    char *buffer;
    size_t size;
};

static unsigned long unique_id = 0;

static size_t _data_callback(void *ptr, size_t size, size_t nmemb, void *data);

// taken from glib 2.30.3
gchar*
p_utf8_substring(const gchar *str, glong start_pos, glong end_pos)
{
    gchar *start, *end, *out;

    start = g_utf8_offset_to_pointer (str, start_pos);
    end = g_utf8_offset_to_pointer (start, end_pos - start_pos);

    out = g_malloc (end - start + 1);
    memcpy (out, start, end - start);
    out[end - start] = 0;

    return out;
}

void
p_slist_free_full(GSList *items, GDestroyNotify free_func)
{
    g_slist_foreach (items, (GFunc) free_func, NULL);
    g_slist_free (items);
}

void
p_list_free_full(GList *items, GDestroyNotify free_func)
{
    g_list_foreach (items, (GFunc) free_func, NULL);
    g_list_free (items);
}

gboolean
p_hash_table_add(GHashTable *hash_table, gpointer key)
{
    // doesn't handle when key exists, but value == NULL
    gpointer found = g_hash_table_lookup(hash_table, key);
    g_hash_table_replace(hash_table, key, key);

    return (found == NULL);
}

gboolean
p_hash_table_contains(GHashTable *hash_table, gconstpointer key)
{
    // doesn't handle when key exists, but value == NULL
    gpointer found = g_hash_table_lookup(hash_table, key);
    return (found != NULL);
}

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
prof_getline(FILE *stream)
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

gboolean
valid_resource_presence_string(const char *const str)
{
    assert(str != NULL);
    if ((strcmp(str, "online") == 0) || (strcmp(str, "chat") == 0) ||
            (strcmp(str, "away") == 0) || (strcmp(str, "xa") == 0) ||
            (strcmp(str, "dnd") == 0)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

const char*
string_from_resource_presence(resource_presence_t presence)
{
    switch(presence)
    {
        case RESOURCE_CHAT:
            return "chat";
        case RESOURCE_AWAY:
            return "away";
        case RESOURCE_XA:
            return "xa";
        case RESOURCE_DND:
            return "dnd";
        default:
            return "online";
    }
}

resource_presence_t
resource_presence_from_string(const char *const str)
{
    if (str == NULL) {
        return RESOURCE_ONLINE;
    } else if (strcmp(str, "online") == 0) {
        return RESOURCE_ONLINE;
    } else if (strcmp(str, "chat") == 0) {
        return RESOURCE_CHAT;
    } else if (strcmp(str, "away") == 0) {
        return RESOURCE_AWAY;
    } else if (strcmp(str, "xa") == 0) {
        return RESOURCE_XA;
    } else if (strcmp(str, "dnd") == 0) {
        return RESOURCE_DND;
    } else {
        return RESOURCE_ONLINE;
    }
}

contact_presence_t
contact_presence_from_resource_presence(resource_presence_t resource_presence)
{
    switch(resource_presence)
    {
        case RESOURCE_CHAT:
            return CONTACT_CHAT;
        case RESOURCE_AWAY:
            return CONTACT_AWAY;
        case RESOURCE_XA:
            return CONTACT_XA;
        case RESOURCE_DND:
            return CONTACT_DND;
        default:
            return CONTACT_ONLINE;
    }
}

gchar*
xdg_get_config_home(void)
{
    gchar *xdg_config_home = getenv("XDG_CONFIG_HOME");
    if (xdg_config_home)
        g_strstrip(xdg_config_home);

    if (xdg_config_home && (strcmp(xdg_config_home, "") != 0)) {
        return strdup(xdg_config_home);
    } else {
        GString *default_path = g_string_new(getenv("HOME"));
        g_string_append(default_path, "/.config");
        gchar *result = strdup(default_path->str);
        g_string_free(default_path, TRUE);

        return result;
    }
}

gchar*
xdg_get_data_home(void)
{
    gchar *xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home)
        g_strstrip(xdg_data_home);

    if (xdg_data_home && (strcmp(xdg_data_home, "") != 0)) {
        return strdup(xdg_data_home);
    } else {
        GString *default_path = g_string_new(getenv("HOME"));
        g_string_append(default_path, "/.local/share");
        gchar *result = strdup(default_path->str);
        g_string_free(default_path, TRUE);

        return result;
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

int
cmp_win_num(gconstpointer a, gconstpointer b)
{
    int real_a = GPOINTER_TO_INT(a);
    int real_b = GPOINTER_TO_INT(b);

    if (real_a == 0) {
        real_a = 10;
    }

    if (real_b == 0) {
        real_b = 10;
    }

    if (real_a < real_b) {
        return -1;
    } else if (real_a == real_b) {
        return 0;
    } else {
        return 1;
    }
}

int
get_next_available_win_num(GList *used)
{
    // only console used
    if (g_list_length(used) == 1) {
        return 2;
    } else {
        GList *sorted = NULL;
        GList *curr = used;
        while (curr) {
            sorted = g_list_insert_sorted(sorted, curr->data, cmp_win_num);
            curr = g_list_next(curr);
        }

        int result = 0;
        int last_num = 1;
        curr = sorted;
        // skip console
        curr = g_list_next(curr);
        while (curr) {
            int curr_num = GPOINTER_TO_INT(curr->data);

            if (((last_num != 9) && ((last_num + 1) != curr_num)) ||
                    ((last_num == 9) && (curr_num != 0))) {
                result = last_num + 1;
                if (result == 10) {
                    result = 0;
                }
                g_list_free(sorted);
                return (result);

            } else {
                last_num = curr_num;
                if (last_num == 0) {
                    last_num = 10;
                }
            }
            curr = g_list_next(curr);
        }
        result = last_num + 1;
        if (result == 10) {
            result = 0;
        }

        g_list_free(sorted);
        return result;
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

    if (g_str_has_prefix(&haystack[offset], needle)) {
        if (whole_word) {
            char *prev = g_utf8_prev_char(&haystack[offset]);
            char *next = g_utf8_next_char(&haystack[offset] + strlen(needle) - 1);
            gunichar prevu = g_utf8_get_char(prev);
            gunichar nextu = g_utf8_get_char(next);
            if (!g_unichar_isalnum(prevu) && !g_unichar_isalnum(nextu)) {
                *result = g_slist_append(*result, GINT_TO_POINTER(offset));
            }
        } else {
            *result = g_slist_append(*result, GINT_TO_POINTER(offset));
        }
    }

    if (haystack[offset+1] != '\0') {
        *result = prof_occurrences(needle, haystack, offset+1, whole_word, result);
    }

    return *result;
}

