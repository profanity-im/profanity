/*
 * jid.h
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

#ifndef JID_H
#define JID_H

#include <glib.h>

struct jid_t {
    char *str;
    char *localpart;
    char *domainpart;
    char *resourcepart;
    char *barejid;
    char *fulljid;
};

typedef struct jid_t Jid;

Jid * jid_create(const gchar * const str);
Jid * jid_create_from_bare_and_resource(const char * const room, const char * const nick);
void jid_destroy(Jid *jid);

gboolean jid_is_valid_room_form(Jid *jid);
char * create_fulljid(const char * const barejid, const char * const resource);
char * get_room_from_full_jid(const char * const full_room_jid);
char * get_nick_from_full_jid(const char * const full_room_jid);
gboolean parse_room_jid(const char * const full_room_jid, char **room,
    char **nick);

#endif
