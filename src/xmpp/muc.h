/*
 * muc.h
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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

#ifndef XMPP_MUC_H
#define XMPP_MUC_H

#include <glib.h>

#include "tools/autocomplete.h"
#include "ui/win_types.h"
#include "xmpp/contact.h"
#include "xmpp/jid.h"

typedef enum {
    MUC_ROLE_NONE,
    MUC_ROLE_VISITOR,
    MUC_ROLE_PARTICIPANT,
    MUC_ROLE_MODERATOR
} muc_role_t;

typedef enum {
    MUC_AFFILIATION_NONE,
    MUC_AFFILIATION_OUTCAST,
    MUC_AFFILIATION_MEMBER,
    MUC_AFFILIATION_ADMIN,
    MUC_AFFILIATION_OWNER
} muc_affiliation_t;

typedef enum {
    MUC_MEMBER_TYPE_UNKNOWN,
    MUC_MEMBER_TYPE_PUBLIC,
    MUC_MEMBER_TYPE_MEMBERS_ONLY
} muc_member_type_t;

typedef struct _muc_occupant_t {
    char *nick;
    gchar *nick_collate_key;
    char *jid;
    muc_role_t role;
    muc_affiliation_t affiliation;
    resource_presence_t presence;
    char *status;
} Occupant;

void muc_init(void);
void muc_close(void);

void muc_join(const char *const room, const char *const nick, const char *const password, gboolean autojoin);
void muc_leave(const char *const room);

gboolean muc_active(const char *const room);
gboolean muc_autojoin(const char *const room);

GList* muc_rooms(void);

void muc_set_features(const char *const room, GSList *features);

char* muc_nick(const char *const room);
char* muc_password(const char *const room);

void muc_nick_change_start(const char *const room, const char *const new_nick);
void muc_nick_change_complete(const char *const room, const char *const nick);
gboolean muc_nick_change_pending(const char *const room);
char* muc_old_nick(const char *const room, const char *const new_nick);

gboolean muc_roster_contains_nick(const char *const room, const char *const nick);
gboolean muc_roster_complete(const char *const room);
gboolean muc_roster_add(const char *const room, const char *const nick, const char *const jid, const char *const role,
    const char *const affiliation, const char *const show, const char *const status);
void muc_roster_remove(const char *const room, const char *const nick);
void muc_roster_set_complete(const char *const room);
GList* muc_roster(const char *const room);
Autocomplete muc_roster_ac(const char *const room);
Autocomplete muc_roster_jid_ac(const char *const room);
void muc_jid_autocomplete_reset(const char *const room);
void muc_jid_autocomplete_add_all(const char *const room, GSList *jids);

Occupant* muc_roster_item(const char *const room, const char *const nick);

gboolean muc_occupant_available(Occupant *occupant);
const char* muc_occupant_affiliation_str(Occupant *occupant);
const char* muc_occupant_role_str(Occupant *occupant);
GSList* muc_occupants_by_role(const char *const room, muc_role_t role);
GSList* muc_occupants_by_affiliation(const char *const room, muc_affiliation_t affiliation);

void muc_occupant_nick_change_start(const char *const room, const char *const new_nick, const char *const old_nick);
char* muc_roster_nick_change_complete(const char *const room, const char *const nick);

void muc_invites_add(const char *const room, const char *const password);
void muc_invites_remove(const char *const room);
gint muc_invites_count(void);
GList* muc_invites(void);
gboolean muc_invites_contain(const char *const room);
void muc_invites_reset_ac(void);
char* muc_invites_find(const char *const search_str, gboolean previous);
void muc_invites_clear(void);
char* muc_invite_password(const char *const room);

void muc_set_subject(const char *const room, const char *const subject);
char* muc_subject(const char *const room);

void muc_pending_broadcasts_add(const char *const room, const char *const message);
GList* muc_pending_broadcasts(const char *const room);

char* muc_autocomplete(ProfWin *window, const char *const input, gboolean previous);
void muc_autocomplete_reset(const char *const room);

gboolean muc_requires_config(const char *const room);
void muc_set_requires_config(const char *const room, gboolean val);

void muc_set_role(const char *const room, const char *const role);
void muc_set_affiliation(const char *const room, const char *const affiliation);
char* muc_role_str(const char *const room);
char* muc_affiliation_str(const char *const room);

muc_member_type_t muc_member_type(const char *const room);

#endif
