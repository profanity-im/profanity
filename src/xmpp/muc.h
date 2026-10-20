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
    gchar* nick;
    gchar* nick_collate_key;
    gchar* jid;
    muc_role_t role;
    muc_affiliation_t affiliation;
    resource_presence_t presence;
    gchar* status;
} Occupant;

void muc_init(void);

void muc_join(const gchar* const room, const gchar* const nick, const gchar* const password, gboolean autojoin);
void muc_leave(const gchar* const room);

void muc_update_activity(const gchar* const room);
gint64 muc_last_activity(const gchar* const room);
void muc_set_ping_sent_time(const gchar* const room, gint64 time);
gint64 muc_ping_sent_time(const gchar* const room);

gboolean muc_active(const gchar* const room);
gboolean muc_autojoin(const gchar* const room);

GList* muc_rooms(void);

void muc_set_features(const gchar* const room, GSList* features);
gboolean muc_supports_mam(const gchar* const room);

const gchar* muc_nick(const gchar* const room);
gchar* muc_password(const gchar* const room);

void muc_nick_change_start(const gchar* const room, const gchar* const new_nick);
void muc_nick_change_complete(const gchar* const room, const gchar* const nick);
gboolean muc_nick_change_pending(const gchar* const room);
gchar* muc_old_nick(const gchar* const room, const gchar* const new_nick);

gboolean muc_roster_contains_nick(const gchar* const room, const gchar* const nick);
gboolean muc_roster_complete(const gchar* const room);
gboolean muc_roster_add(const gchar* const room, const gchar* const nick, const gchar* const jid, const gchar* const role,
                        const gchar* const affiliation, const gchar* const show, const gchar* const status);
void muc_roster_remove(const gchar* const room, const gchar* const nick);
void muc_roster_set_complete(const gchar* const room);
GList* muc_roster(const gchar* const room);
Autocomplete muc_roster_ac(const gchar* const room);
Autocomplete muc_roster_jid_ac(const gchar* const room);
void muc_jid_autocomplete_reset(const gchar* const room);
void muc_jid_autocomplete_add_all(const gchar* const room, GSList* jids);

Occupant* muc_roster_item(const gchar* const room, const gchar* const nick);

gboolean muc_occupant_available(Occupant* occupant);
const gchar* muc_occupant_affiliation_str(Occupant* occupant);
const gchar* muc_occupant_role_str(Occupant* occupant);
GSList* muc_occupants_by_role(const gchar* const room, muc_role_t role);
GSList* muc_occupants_by_affiliation(const gchar* const room, muc_affiliation_t affiliation);

void muc_occupant_nick_change_start(const gchar* const room, const gchar* const new_nick, const gchar* const old_nick);
gchar* muc_roster_nick_change_complete(const gchar* const room, const gchar* const nick);

void muc_confserver_add(const gchar* const server);
void muc_confserver_reset_ac(void);
gchar* muc_confserver_find(const gchar* const search_str, gboolean previous, void* context);
void muc_confserver_clear(void);

void muc_invites_add(const gchar* const room, const gchar* const password);
void muc_invites_remove(const gchar* const room);
gint muc_invites_count(void);
GList* muc_invites(void);
gboolean muc_invites_contain(const gchar* const room);
void muc_invites_reset_ac(void);
gchar* muc_invites_find(const gchar* const search_str, gboolean previous, void* context);
void muc_invites_clear(void);
gchar* muc_invite_password(const gchar* const room);

void muc_set_subject(const gchar* const room, const gchar* const subject);
gchar* muc_subject(const gchar* const room);

void muc_pending_broadcasts_add(const gchar* const room, const gchar* const message);
GList* muc_pending_broadcasts(const gchar* const room);

gchar* muc_autocomplete(ProfWin* window, const gchar* const input, gboolean previous);
void muc_autocomplete_reset(const gchar* const room);

gboolean muc_requires_config(const gchar* const room);
void muc_set_requires_config(const gchar* const room, gboolean val);

void muc_set_role(const gchar* const room, const gchar* const role);
void muc_set_affiliation(const gchar* const room, const gchar* const affiliation);
gchar* muc_role_str(const gchar* const room);
gchar* muc_affiliation_str(const gchar* const room);

muc_member_type_t muc_member_type(const gchar* const room);
muc_anonymity_type_t muc_anonymity_type(const gchar* const room);

GList* muc_members(const gchar* const room);
void muc_members_add(const gchar* const room, const gchar* const jid);
void muc_members_remove(const gchar* const room, const gchar* const jid);
void muc_members_update(const gchar* const room, const gchar* const jid, const gchar* const affiliation);

#endif
