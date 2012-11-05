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

GSList *rooms;

void
room_join(const char * const jid, const char * const nick)
{
    muc_room *new_room = malloc(sizeof(muc_room));
    new_room->jid = strdup(jid);
    new_room->nick = strdup(nick);

    rooms = g_slist_append(rooms, new_room);
}

gboolean
room_jid_is_room_chat(const char * const jid)
{
    GSList *current = rooms;
    while (current != NULL) {
        muc_room *room = current->data;
        if (g_str_has_prefix(jid, room->jid)) {
            return TRUE;
        }
        current = g_slist_next(current);
    }

    return FALSE;

}

char *
room_get_nick_for_room(const char * const jid)
{
    GSList *current = rooms;
    while (current != NULL) {
        muc_room *room = current->data;
        if (strcmp(jid, room->jid) == 0) {
            return room->nick;
        }
        current = g_slist_next(current);
    }

    return NULL;
}
