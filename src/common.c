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

#include <glib.h>

#include "common.h"

// assume malloc stores at most 8 bytes for size of allocated memory
// and page size is at least 4KB
#define READ_BUF_SIZE 4088

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

/*
 * Given a full room JID of the form
 * room@server/nick
 * Will create two new strings and point room and nick to them e.g.
 * *room = "room@server", *nick = "nick"
 * The strings must be freed by the caller
 * Returns TRUE if the JID was parsed successfully, FALSE otherwise
 */
gboolean
parse_room_jid(const char * const full_room_jid, char **room, char **nick)
{
    char **tokens = g_strsplit(full_room_jid, "/", 0);

    if (tokens == NULL || tokens[0] == NULL || tokens[1] == NULL) {
        return FALSE;
    } else {
        *room = strdup(tokens[0]);
        *nick = strdup(tokens[1]);

        g_strfreev(tokens);

        return TRUE;
    }
}

/*
 * Given a room name, and a nick name create and return a full JID of the form
 * room@server/nick
 * Will return a newly created string that must be freed by the caller
 */
char *
create_full_room_jid(const char * const room, const char * const nick)
{
    GString *full_jid = g_string_new(room);
    g_string_append(full_jid, "/");
    g_string_append(full_jid, nick);

    char *result = strdup(full_jid->str);

    g_string_free(full_jid, TRUE);

    return result;
}

/*
 * Returns TRUE if the JID is a room JID
 * The test is that the passed JID does not contain a "/"
 */
gboolean
jid_is_room(const char * const room_jid)
{
    gchar *result = g_strrstr(room_jid, "/");
    return (result == NULL);
}

/*
 * Get the room name part of the full JID, e.g.
 * Full JID = "test@conference.server/person"
 * returns "test@conference.server"
 */
char *
get_room_from_full_jid(const char * const full_room_jid)
{
    char **tokens = g_strsplit(full_room_jid, "/", 0);
    char *room_part;

    if (tokens == NULL || tokens[0] == NULL) {
        return NULL;
    } else {
        room_part = strdup(tokens[0]);

        g_strfreev(tokens);

        return room_part;
    }
}

/*
 * Get the nickname part of the full JID, e.g.
 * Full JID = "test@conference.server/person"
 * returns "person"
 */
char *
get_nick_from_full_jid(const char * const full_room_jid)
{
    char **tokens = g_strsplit(full_room_jid, "/", 0);
    char *nick_part;

    if (tokens == NULL || tokens[1] == NULL) {
        return NULL;
    } else {
        nick_part = strdup(tokens[1]);

        g_strfreev(tokens);

        return nick_part;
    }
}

