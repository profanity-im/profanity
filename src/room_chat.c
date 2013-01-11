/*
 * room_chat.c
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

#include "contact.h"
#include "prof_autocomplete.h"

typedef struct _muc_room_t {
    char *room;
    char *nick;
    gboolean pending_nick_change;
    GHashTable *roster;
    PAutocomplete nick_ac;
    GHashTable *nick_changes;
    gboolean roster_received;
} muc_room;

GHashTable *rooms = NULL;

static void _room_free(muc_room *room);

void
room_join(const char * const room, const char * const nick)
{
    if (rooms == NULL) {
        rooms = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
            (GDestroyNotify)_room_free);
    }

    muc_room *new_room = malloc(sizeof(muc_room));
    new_room->room = strdup(room);
    new_room->nick = strdup(nick);
    new_room->roster = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
        (GDestroyNotify)p_contact_free);
    new_room->nick_ac = p_autocomplete_new();
    new_room->nick_changes = g_hash_table_new_full(g_str_hash, g_str_equal,
        g_free, g_free);
    new_room->roster_received = FALSE;
    new_room->pending_nick_change = FALSE;

    g_hash_table_insert(rooms, strdup(room), new_room);
}

void
room_set_pending_nick_change(const char * const room)
{
    muc_room *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        chat_room->pending_nick_change = TRUE;
    }
}

gboolean
room_is_pending_nick_change(const char * const room)
{
    muc_room *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        return chat_room->pending_nick_change;
    } else {
        return FALSE;
    }
}

void
room_change_nick(const char * const room, const char * const nick)
{
    muc_room *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        free(chat_room->nick);
        chat_room->nick = strdup(nick);
        chat_room->pending_nick_change = FALSE;
    }
}

void
room_leave(const char * const room)
{
    g_hash_table_remove(rooms, room);
}

gboolean
room_is_active(const char * const full_room_jid)
{
    char **tokens = g_strsplit(full_room_jid, "/", 0);
    char *room_part = tokens[0];

    if (rooms != NULL) {
        muc_room *chat_room = g_hash_table_lookup(rooms, room_part);

        if (chat_room != NULL) {
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

GList *
room_get_rooms(void)
{
    if (rooms != NULL) {
        return g_hash_table_get_keys(rooms);
    } else {
        return NULL;
    }
}

char *
room_get_nick_for_room(const char * const room)
{
    if (rooms != NULL) {
        muc_room *chat_room = g_hash_table_lookup(rooms, room);

        if (chat_room != NULL) {
            return chat_room->nick;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

char *
room_get_room_from_full_jid(const char * const full_room_jid)
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

gboolean
room_from_jid_is_room(const char * const room_jid)
{
    gchar *result = g_strrstr(room_jid, "/");
    return (result == NULL);
}

char *
room_get_nick_from_full_jid(const char * const full_room_jid)
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

char *
room_create_full_room_jid(const char * const room, const char * const nick)
{
    GString *full_jid = g_string_new(room);
    g_string_append(full_jid, "/");
    g_string_append(full_jid, nick);

    char *result = strdup(full_jid->str);

    g_string_free(full_jid, TRUE);

    return result;
}

gboolean
room_parse_room_jid(const char * const full_room_jid, char **room, char **nick)
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

gboolean
room_nick_in_roster(const char * const room, const char * const nick)
{
    muc_room *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        PContact contact = g_hash_table_lookup(chat_room->roster, nick);
        if (contact != NULL) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    return FALSE;
}

gboolean
room_add_to_roster(const char * const room, const char * const nick,
    const char * const show, const char * const status)
{
    muc_room *chat_room = g_hash_table_lookup(rooms, room);
    gboolean updated = FALSE;

    if (chat_room != NULL) {
        PContact old = g_hash_table_lookup(chat_room->roster, nick);

        if (old == NULL) {
            updated = TRUE;
            p_autocomplete_add(chat_room->nick_ac, strdup(nick));
        } else if ((g_strcmp0(p_contact_presence(old), show) != 0) ||
                    (g_strcmp0(p_contact_status(old), status) != 0)) {
            updated = TRUE;
        }

        PContact contact = p_contact_new(nick, NULL, show, status, NULL, FALSE);
        g_hash_table_replace(chat_room->roster, strdup(nick), contact);
    }

    return updated;
}

void
room_remove_from_roster(const char * const room, const char * const nick)
{
    muc_room *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        g_hash_table_remove(chat_room->roster, nick);
        p_autocomplete_remove(chat_room->nick_ac, nick);
    }
}

GList *
room_get_roster(const char * const room)
{
    muc_room *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        return g_hash_table_get_values(chat_room->roster);
    } else {
        return NULL;
    }
}

PAutocomplete
room_get_nick_ac(const char * const room)
{
    muc_room *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        return chat_room->nick_ac;
    } else {
        return NULL;
    }
}

void
room_set_roster_received(const char * const room)
{
    muc_room *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        chat_room->roster_received = TRUE;
    }
}

gboolean
room_get_roster_received(const char * const room)
{
    muc_room *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        return chat_room->roster_received;
    } else {
        return FALSE;
    }
}

void
room_add_pending_nick_change(const char * const room,
    const char * const new_nick, const char * const old_nick)
{
    muc_room *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        g_hash_table_insert(chat_room->nick_changes, strdup(new_nick), strdup(old_nick));
        room_remove_from_roster(room, old_nick);
    }
}

char *
room_complete_pending_nick_change(const char * const room,
    const char * const nick)
{
    muc_room *chat_room = g_hash_table_lookup(rooms, room);

    if (chat_room != NULL) {
        char *old_nick = g_hash_table_lookup(chat_room->nick_changes, nick);
        char *old_nick_cpy;

        if (old_nick != NULL) {
            old_nick_cpy = strdup(old_nick);
            g_hash_table_remove(chat_room->nick_changes, nick);

            return old_nick_cpy;
        }
    }

    return NULL;
}

static void
_room_free(muc_room *room)
{
    if (room != NULL) {
        if (room->room != NULL) {
            g_free(room->room);
            room->room = NULL;
        }
        if (room->nick != NULL) {
            g_free(room->nick);
            room->nick = NULL;
        }
        if (room->roster != NULL) {
            g_hash_table_remove_all(room->roster);
            room->roster = NULL;
        }
        if (room->nick_ac != NULL) {
            p_autocomplete_free(room->nick_ac);
        }
        if (room->nick_changes != NULL) {
            g_hash_table_remove_all(room->nick_changes);
            room->nick_changes = NULL;
        }
        g_free(room);
    }
    room = NULL;
}
