/*
 * jid.c
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "common.h"
#include "jid.h"

Jid *
jid_create(const gchar * const str)
{
    Jid *result = NULL;

    if (str == NULL) {
        return NULL;
    }
    if (strlen(str) == 0) {
        return NULL;
    }

    gchar *trimmed = g_strdup(str);

    if (g_str_has_prefix(trimmed, "/") || g_str_has_prefix(trimmed, "@")) {
        g_free(trimmed);
        return NULL;
    } else if (g_str_has_suffix(trimmed, "/") || g_str_has_suffix(trimmed, "@")) {
        g_free(trimmed);
        return NULL;
    }

    gchar *slashp = g_strrstr(trimmed, "/");

    // has resourcepart
    if (slashp != NULL) {
        result = malloc(sizeof(struct jid_t));
        result->str = strdup(trimmed);
        result->resourcepart = g_strdup(slashp + 1);
        result->barejid = g_strndup(trimmed, strlen(trimmed) - strlen(result->resourcepart) - 1);
        result->fulljid = g_strdup(trimmed);

        gchar *atp = g_strrstr(result->barejid, "@");

        // has domain and local parts
        if (atp != NULL) {
            result->domainpart = g_strndup(atp + 1, strlen(trimmed));
            result->localpart = g_strndup(trimmed, strlen(trimmed) - (strlen(result->resourcepart) + strlen(result->domainpart) + 2));

        // only domain part
        } else {
            result->domainpart = strdup(result->barejid);
            result->localpart = NULL;
        }

        g_free(trimmed);
        return result;

    // no resourcepart
    } else {
        result = malloc(sizeof(struct jid_t));
        result->str = strdup(trimmed);
        result->resourcepart = NULL;
        result->barejid = g_strdup(trimmed);
        result->fulljid = NULL;

        gchar *atp = g_strrstr(trimmed, "@");

        // has local and domain parts
        if (atp != NULL) {
            result->domainpart = g_strdup(atp + 1);
            result->localpart = g_strndup(trimmed, strlen(trimmed) - strlen(result->domainpart) - 1);

        // only domain part
        } else {
            result->domainpart = g_strdup(trimmed);
            result->localpart = NULL;
        }

        g_free(trimmed);
        return result;
    }
}

Jid *
jid_create_room_jid(const char * const room, const char * const nick)
{
    Jid *result;
    char *jid = create_full_room_jid(room, nick);
    result = jid_create(jid);
    free(jid);

    return result;
}

void
jid_destroy(Jid *jid)
{
    FREE_SET_NULL(jid->str);
    FREE_SET_NULL(jid->localpart);
    FREE_SET_NULL(jid->domainpart);
    FREE_SET_NULL(jid->resourcepart);
    FREE_SET_NULL(jid->barejid);
    FREE_SET_NULL(jid->fulljid);
    FREE_SET_NULL(jid);
}

gboolean
jid_is_valid_room_form(Jid *jid)
{
    return (jid->fulljid != NULL);
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
    gboolean result = FALSE;
    char **tokens = g_strsplit(full_room_jid, "/", 0);

    if (tokens == NULL)
        return FALSE;

    if (tokens[0] != NULL && tokens[1] != NULL) {
        *room = strdup(tokens[0]);
        *nick = strdup(tokens[1]);
        result = TRUE;
    }

    g_strfreev(tokens);

    return result;
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

