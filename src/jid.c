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

    gchar *trimmed = g_strdup(str);

    if (strlen(trimmed) == 0) {
        return NULL;
    }

    if (g_str_has_prefix(trimmed, "/") || g_str_has_prefix(trimmed, "@")) {
        g_free(trimmed);
        return NULL;
    }

    if (!g_utf8_validate(trimmed, -1, NULL)) {
        return NULL;
    }

    result = malloc(sizeof(struct jid_t));
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
        free(trimmed);
        return NULL;
    }

    result->str = g_strdup(trimmed);

    free(trimmed);

    return result;
}

Jid *
jid_create_from_bare_and_resource(const char * const room, const char * const nick)
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
    if (jid != NULL) {
        FREE_SET_NULL(jid->str);
        FREE_SET_NULL(jid->localpart);
        FREE_SET_NULL(jid->domainpart);
        FREE_SET_NULL(jid->resourcepart);
        FREE_SET_NULL(jid->barejid);
        FREE_SET_NULL(jid->fulljid);
        FREE_SET_NULL(jid);
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

