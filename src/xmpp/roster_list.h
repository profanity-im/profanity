/*
 * roster_list.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_ROSTER_LIST_H
#define XMPP_ROSTER_LIST_H

#include <glib.h>

#include "xmpp/resource.h"
#include "xmpp/contact.h"

typedef enum {
    ROSTER_ORD_NAME,
    ROSTER_ORD_PRESENCE
} roster_ord_t;

void roster_clear(void);
gboolean roster_update_presence(const gchar* const barejid, Resource* resource, GDateTime* last_activity);
PContact roster_get_contact(const gchar* const barejid);
gboolean roster_contact_offline(const gchar* const barejid, const gchar* const resource, const gchar* const status);
void roster_reset_search_attempts(void);
void roster_create(void);
void roster_destroy(void);
void roster_change_name(PContact contact, const gchar* const new_name);
void roster_remove(const gchar* const name, const gchar* const barejid);
void roster_update(const gchar* const barejid, const gchar* const name, GSList* groups, const gchar* const subscription,
                   gboolean pending_out);
gboolean roster_add(const gchar* const barejid, const gchar* const name, GSList* groups, const gchar* const subscription,
                    gboolean pending_out);
gchar* roster_barejid_from_name(const gchar* const name);
GSList* roster_get_contacts(roster_ord_t order);
GSList* roster_get_contacts_online(void);
gboolean roster_has_pending_subscriptions(void);
gchar* roster_contact_autocomplete(const gchar* const search_str, gboolean previous, void* context);
gchar* roster_fulljid_autocomplete(const gchar* const search_str, gboolean previous, void* context);
GSList* roster_get_group(const gchar* const group, roster_ord_t order);
GList* roster_get_groups(void);
gchar* roster_group_autocomplete(const gchar* const search_str, gboolean previous, void* context);
gchar* roster_barejid_autocomplete(const gchar* const search_str, gboolean previous, void* context);
GSList* roster_get_contacts_by_presence(const gchar* const presence);
const gchar* roster_get_display_name(const gchar* const barejid);
gchar* roster_get_msg_display_name(const gchar* const barejid, const gchar* const resource);
gint roster_compare_name(PContact a, PContact b);
gint roster_compare_presence(PContact a, PContact b);
void roster_process_pending_presence(void);
gboolean roster_exists(void);

#endif
