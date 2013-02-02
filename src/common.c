/*
 * common.c
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include <glib.h>

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

void
create_dir(char *name)
{
    int e;
    struct stat sb;

    e = stat(name, &sb);
    if (e != 0)
        if (errno == ENOENT)
            e = mkdir(name, S_IRWXU);
}

void
mkdir_recursive(const char *dir)
{
    int i;
    for (i = 1; i <= strlen(dir); i++) {
        if (dir[i] == '/' || dir[i] == '\0') {
            gchar *next_dir = g_strndup(dir, i);
            create_dir(next_dir);
            g_free(next_dir);
        }
    }
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
encode_xml(const char * const xml)
{
    char *coded_msg = str_replace(xml, "&", "&amp;");
    char *coded_msg2 = str_replace(coded_msg, "<", "&lt;");
    char *coded_msg3 = str_replace(coded_msg2, ">", "&gt;");

    free(coded_msg);
    free(coded_msg2);

    return coded_msg3;
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

int
octet_compare(unsigned char *str1, unsigned char *str2)
{
    if ((strcmp((char *)str1, "") == 0) && (strcmp((char *)str2, "") == 0)) {
        return 0;
    }

    if ((strcmp((char *)str1, "") == 0) && (strcmp((char *)str2, "") != 0)) {
        return -1;
    }

    if ((strcmp((char *)str1, "") != 0) && (strcmp((char *)str2, "") == 0)) {
        return 1;
    }

    if (str1[0] == str2[0]) {
        return octet_compare(&str1[1], &str2[1]);
    }

    if (str1[0] < str2[0]) {
        return -1;
    }

    return 1;
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
presence_valid_string(const char * const str)
{
    if (str == NULL) {
        return FALSE;
    } else if ((strcmp(str, "online") == 0) ||
                (strcmp(str, "chat") == 0) ||
                (strcmp(str, "away") == 0) ||
                (strcmp(str, "xa") == 0) ||
                (strcmp(str, "dnd") == 0)) {
        return TRUE;
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
