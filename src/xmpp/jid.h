/*
 * jid.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#ifndef XMPP_JID_H
#define XMPP_JID_H

#include <glib.h>

struct jid_t
{
    unsigned int refcnt;
    char* str;
    char* localpart;
    char* domainpart;
    char* resourcepart;
    char* barejid;
    char* fulljid;
};

typedef struct jid_t Jid;

Jid* jid_create(const gchar* const str);
Jid* jid_create_from_bare_and_resource(const char* const barejid, const char* const resource);
void jid_destroy(Jid* jid);
void jid_ref(Jid* jid);

void jid_auto_destroy(Jid** str);
#define auto_jid __attribute__((__cleanup__(jid_auto_destroy)))

gboolean jid_is_valid_room_form(Jid* jid);
char* create_fulljid(const char* const barejid, const char* const resource);
char* get_nick_from_full_jid(const char* const full_room_jid);

const char* jid_fulljid_or_barejid(Jid* jid);
gchar* jid_random_resource(void);

#endif
