/*
 * common.c
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
#include "config.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include <glib.h>

#include "log.h"
#include "common.h"

// assume malloc stores at most 8 bytes for size of allocated memory
// and page size is at least 4KB
#define READ_BUF_SIZE 4088

struct curl_data_t
{
    char *buffer;
    size_t size;
};

static size_t _data_callback(void *ptr, size_t size, size_t nmemb, void *data);

// taken from glib 2.30.3
gchar *
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

// backwards compatibility for GLib version < 2.28
void
p_slist_free_full(GSList *items, GDestroyNotify free_func)
{
    g_slist_foreach (items, (GFunc) free_func, NULL);
    g_slist_free (items);
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

char *
str_replace(const char *string, const char *substr,
    const char *replacement)
{
    char *tok = NULL;
    char *newstr = NULL;
    char *oldstr = NULL;
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
        oldstr = newstr;
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
str_contains(char str[], int size, char ch)
{
    int i;
    for (i = 0; i < size; i++) {
        if (str[i] == ch)
            return 1;
    }

    return 0;
}

char *
prof_getline(FILE *stream)
{
    char *buf;
    size_t buf_size;
    char *result;
    char *s = NULL;
    size_t s_size = 1;
    int need_exit = 0;

    buf = (char *)malloc(READ_BUF_SIZE);

    while (TRUE) {
        result = fgets(buf, READ_BUF_SIZE, stream);
        if (result == NULL)
            break;
        buf_size = strlen(buf);
        if (buf[buf_size - 1] == '\n') {
            buf_size--;
            buf[buf_size] = '\0';
            need_exit = 1;
        }

        result = (char *)realloc(s, s_size + buf_size);
        if (result == NULL) {
            if (s != NULL) {
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

char *
release_get_latest()
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

    if (output.buffer != NULL) {
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
valid_resource_presence_string(const char * const str)
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

const char *
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
resource_presence_from_string(const char * const str)
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

gchar *
xdg_get_config_home(void)
{
    gchar *xdg_config_home = getenv("XDG_CONFIG_HOME");
    if (xdg_config_home != NULL)
        g_strstrip(xdg_config_home);

    if ((xdg_config_home != NULL) && (strcmp(xdg_config_home, "") != 0)) {
        return strdup(xdg_config_home);
    } else {
        GString *default_path = g_string_new(getenv("HOME"));
        g_string_append(default_path, "/.config");
        gchar *result = strdup(default_path->str);
        g_string_free(default_path, TRUE);

        return result;
    }
}

gchar *
xdg_get_data_home(void)
{
    gchar *xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home != NULL)
        g_strstrip(xdg_data_home);

    if ((xdg_data_home != NULL) && (strcmp(xdg_data_home, "") != 0)) {
        return strdup(xdg_data_home);
    } else {
        GString *default_path = g_string_new(getenv("HOME"));
        g_string_append(default_path, "/.local/share");
        gchar *result = strdup(default_path->str);
        g_string_free(default_path, TRUE);

        return result;
    }
}

char *
generate_unique_id(char *prefix)
{
    static unsigned long unique_id;
    char *result = NULL;
    GString *result_str = g_string_new("");

    unique_id++;
    if (prefix != NULL) {
        g_string_printf(result_str, "prof_%s_%lu", prefix, unique_id);
    } else {
        g_string_printf(result_str, "prof_%lu", unique_id);
    }
    result = result_str->str;
    g_string_free(result_str, FALSE);

    return result;
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
    int result = 0;
    used = g_list_sort(used, cmp_win_num);
    // only console used
    if (g_list_length(used) == 1) {
        return 2;
    } else {
        int last_num = 1;
        GList *curr = used;
        // skip console
        curr = g_list_next(curr);
        while (curr != NULL) {
            int curr_num = GPOINTER_TO_INT(curr->data);
            if (((last_num != 9) && ((last_num + 1) != curr_num)) ||
                    ((last_num == 9) && (curr_num != 0))) {
                result = last_num + 1;
                if (result == 10) {
                    result = 0;
                }
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
