/*
 * muc.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
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

typedef enum {
    MUC_ANONYMITY_TYPE_UNKNOWN,
    MUC_ANONYMITY_TYPE_NONANONYMOUS,
    MUC_ANONYMITY_TYPE_SEMIANONYMOUS
} muc_anonymity_type_t;

typedef struct _muc_occupant_t
{
    char* nick;
    gchar* nick_collate_key;
    char* jid;
    muc_role_t role;
    muc_affiliation_t affiliation;
    resource_presence_t presence;
    char* status;
} Occupant;

void muc_init(void);

void muc_join(const char* const room, const char* const nick, const char* const password, gboolean autojoin);
void muc_leave(const char* const room);

gboolean muc_active(const char* const room);
gboolean muc_autojoin(const char* const room);

GList* muc_rooms(void);

void muc_set_features(const char* const room, GSList* features);

const char* muc_nick(const char* const room);
char* muc_password(const char* const room);

void muc_nick_change_start(const char* const room, const char* const new_nick);
void muc_nick_change_complete(const char* const room, const char* const nick);
gboolean muc_nick_change_pending(const char* const room);
char* muc_old_nick(const char* const room, const char* const new_nick);

gboolean muc_roster_contains_nick(const char* const room, const char* const nick);
gboolean muc_roster_complete(const char* const room);
gboolean muc_roster_add(const char* const room, const char* const nick, const char* const jid, const char* const role,
                        const char* const affiliation, const char* const show, const char* const status);
void muc_roster_remove(const char* const room, const char* const nick);
void muc_roster_set_complete(const char* const room);
GList* muc_roster(const char* const room);
Autocomplete muc_roster_ac(const char* const room);
Autocomplete muc_roster_jid_ac(const char* const room);
void muc_jid_autocomplete_reset(const char* const room);
void muc_jid_autocomplete_add_all(const char* const room, GSList* jids);

Occupant* muc_roster_item(const char* const room, const char* const nick);

gboolean muc_occupant_available(Occupant* occupant);
const char* muc_occupant_affiliation_str(Occupant* occupant);
const char* muc_occupant_role_str(Occupant* occupant);
GSList* muc_occupants_by_role(const char* const room, muc_role_t role);
GSList* muc_occupants_by_affiliation(const char* const room, muc_affiliation_t affiliation);

void muc_occupant_nick_change_start(const char* const room, const char* const new_nick, const char* const old_nick);
char* muc_roster_nick_change_complete(const char* const room, const char* const nick);

void muc_confserver_add(const char* const server);
void muc_confserver_reset_ac(void);
char* muc_confserver_find(const char* const search_str, gboolean previous, void* context);
void muc_confserver_clear(void);

void muc_invites_add(const char* const room, const char* const password);
void muc_invites_remove(const char* const room);
gint muc_invites_count(void);
GList* muc_invites(void);
gboolean muc_invites_contain(const char* const room);
void muc_invites_reset_ac(void);
char* muc_invites_find(const char* const search_str, gboolean previous, void* context);
void muc_invites_clear(void);
char* muc_invite_password(const char* const room);

void muc_set_subject(const char* const room, const char* const subject);
char* muc_subject(const char* const room);

void muc_pending_broadcasts_add(const char* const room, const char* const message);
GList* muc_pending_broadcasts(const char* const room);

char* muc_autocomplete(ProfWin* window, const char* const input, gboolean previous);
void muc_autocomplete_reset(const char* const room);

gboolean muc_requires_config(const char* const room);
void muc_set_requires_config(const char* const room, gboolean val);

void muc_set_role(const char* const room, const char* const role);
void muc_set_affiliation(const char* const room, const char* const affiliation);
char* muc_role_str(const char* const room);
char* muc_affiliation_str(const char* const room);

muc_member_type_t muc_member_type(const char* const room);
muc_anonymity_type_t muc_anonymity_type(const char* const room);

GList* muc_members(const char* const room);
void muc_members_add(const char* const room, const char* const jid);
void muc_members_remove(const char* const room, const char* const jid);
void muc_members_update(const char* const room, const char* const jid, const char* const affiliation);

#endif
