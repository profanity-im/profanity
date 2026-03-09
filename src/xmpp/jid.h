/*
 * jid.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_JID_H
#define XMPP_JID_H

#include <glib.h>

struct jid_t
{
    unsigned int refcnt;
    gchar* str;
    gchar* localpart;
    gchar* domainpart;
    gchar* resourcepart;
    gchar* barejid;
    gchar* fulljid;
};

typedef struct jid_t Jid;

Jid* jid_create(const gchar* const str);
gboolean jid_is_valid(const gchar* const str);
gboolean jid_is_valid_user_jid(const gchar* const str);
Jid* jid_create_from_bare_and_resource(const gchar* const barejid, const gchar* const resource);
void jid_destroy(Jid* jid);
void jid_ref(Jid* jid);

void jid_auto_destroy(Jid** str);
#define auto_jid __attribute__((__cleanup__(jid_auto_destroy)))

gboolean jid_is_valid_room_form(Jid* jid);
gchar* create_fulljid(const gchar* const barejid, const gchar* const resource);
gchar* get_nick_from_full_jid(const gchar* const full_room_jid);

const gchar* jid_fulljid_or_barejid(Jid* jid);
gchar* jid_random_resource(void);

#endif
