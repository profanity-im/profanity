/*
 * jid.c
 *
 * Copyright (C) 2012 -2014 James Booth <boothj5@gmail.com>
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

#include "jid.h"

#include "common.h"

Jid *
jid_create(const gchar * const str)
{
    Jid *result = NULL;

    /* if str is NULL g_strdup returns NULL */
    gchar *trimmed = g_strdup(str);
    if (trimmed == NULL) {
        return NULL;
    }

    if (strlen(trimmed) == 0) {
        g_free(trimmed);
        return NULL;
    }

    if (g_str_has_prefix(trimmed, "/") || g_str_has_prefix(trimmed, "@")) {
        g_free(trimmed);
        return NULL;
    }

    if (!g_utf8_validate(trimmed, -1, NULL)) {
        g_free(trimmed);
        return NULL;
    }

    result = malloc(sizeof(struct jid_t));
    result->str = NULL;
    result->localpart = NULL;
    result->domainpart = NULL;
    result->resourcepart = NULL;
    result->barejid = NULL;
    result->fulljid = NULL;

    gchar *atp = g_utf8_strchr(trimmed, -1, '@');
    gchar *slashp = g_utf8_strchr(trimmed, -1, '/');
    gchar *domain_start = trimmed;


    if (atp != NULL) {
        result->localpart = g_utf8_substring(trimmed, 0, g_utf8_pointer_to_offset(trimmed, atp));
        domain_start = atp + 1;
    }

    if (slashp != NULL) {
        result->resourcepart = g_strdup(slashp + 1);
        result->domainpart = g_utf8_substring(domain_start, 0, g_utf8_pointer_to_offset(domain_start, slashp));
        result->barejid = g_utf8_substring(trimmed, 0, g_utf8_pointer_to_offset(trimmed, slashp));
        result->fulljid = g_strdup(trimmed);
    } else {
        result->domainpart = g_strdup(domain_start);
        result->barejid = g_strdup(trimmed);
    }

    if (result->domainpart == NULL) {
        jid_destroy(result);
        return NULL;
    }

    result->str = trimmed;

    return result;
}

Jid *
jid_create_from_bare_and_resource(const char * const room, const char * const nick)
{
    Jid *result;
    char *jid = create_fulljid(room, nick);
    result = jid_create(jid);
    free(jid);

    return result;
}

void
jid_destroy(Jid *jid)
{
    if (jid != NULL) {
        g_free(jid->str);
        g_free(jid->localpart);
        g_free(jid->domainpart);
        g_free(jid->resourcepart);
        g_free(jid->barejid);
        g_free(jid->fulljid);
        free(jid);
    }
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
    Jid *jid = jid_create(full_room_jid);

    if (jid == NULL) {
        return FALSE;
    }

    if (jid->resourcepart == NULL) {
        jid_destroy(jid);
        return FALSE;
    }

    *room = strdup(jid->barejid);
    *nick = strdup(jid->resourcepart);

    jid_destroy(jid);

    return TRUE;
}

/*
 * Given a barejid, and resourcepart, create and return a full JID of the form
 * barejid/resourcepart
 * Will return a newly created string that must be freed by the caller
 */
char *
create_fulljid(const char * const barejid, const char * const resource)
{
    GString *full_jid = g_string_new(barejid);
    g_string_append(full_jid, "/");
    g_string_append(full_jid, resource);

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
    char *room_part = NULL;

    if (tokens != NULL) {
        if (tokens[0] != NULL) {
            room_part = strdup(tokens[0]);
        }

        g_strfreev(tokens);
    }

    return room_part;
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
    char *nick_part = NULL;

    if (tokens != NULL) {
        if (tokens[0] != NULL && tokens[1] != NULL) {
            nick_part = strdup(tokens[1]);
        }

        g_strfreev(tokens);
    }

    return nick_part;
}
