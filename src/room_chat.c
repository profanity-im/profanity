/*
 * room_chat.c
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
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

typedef struct _muc_room_t {
    char *jid;
    char *nick;
} muc_room;

GHashTable *rooms = NULL;

static void _room_free(muc_room *room);

void
room_join(const char * const jid, const char * const nick)
{
    if (rooms == NULL) {
        rooms = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
            (GDestroyNotify)_room_free);
    }

    muc_room *new_room = malloc(sizeof(muc_room));
    new_room->jid = strdup(jid);
    new_room->nick = strdup(nick);

    g_hash_table_insert(rooms, strdup(jid), new_room);
}

void
room_leave(const char * const jid)
{
    g_hash_table_remove(rooms, jid);
}

gboolean
room_jid_is_room_chat(const char * const jid)
{
    char **tokens = g_strsplit(jid, "/", 0);
    char *jid_part = tokens[0];

    if (rooms != NULL) {
        muc_room *room = g_hash_table_lookup(rooms, jid_part);

        if (room != NULL) {
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return FALSE;
    }
}

char *
room_get_nick_for_room(const char * const jid)
{
    if (rooms != NULL) {
        muc_room *room = g_hash_table_lookup(rooms, jid);

        if (room != NULL) {
            return room->nick;
        } else {
            return NULL;
        }
    } else {
        return NULL;
    }
}

static void
_room_free(muc_room *room)
{
    if (room != NULL) {
        if (room->jid != NULL) {
            g_free(room->jid);
            room->jid = NULL;
        }
        if (room->nick != NULL) {
            g_free(room->nick);
            room->nick = NULL;
        }
        g_free(room);
    }
    room = NULL;
}
