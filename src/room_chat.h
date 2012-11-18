/*
 * room_chat.h
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

#ifndef ROOM_CHAT_H
#define ROOM_CHAT_H

#include <glib.h>

void room_join(const char * const room, const char * const nick);
void room_change_nick(const char * const room, const char * const nick);
void room_leave(const char * const room);
gboolean room_is_active(const char * const full_room_jid);
char * room_get_nick_for_room(const char * const room);
char * room_get_room_from_full_jid(const char * const full_room_jid);
char * room_get_nick_from_full_jid(const char * const full_room_jid);
gboolean room_parse_room_jid(const char * const full_room_jid, char **room,
    char **nick);
void room_add_to_roster(const char * const room, const char * const nick);
GList * room_get_roster(const char * const room);
void room_set_roster_received(const char * const room);
gboolean room_get_roster_received(const char * const room);
void room_remove_from_roster(const char * const room, const char * const nick);
char * room_create_full_room_jid(const char * const room,
    const char * const nick);

#endif
