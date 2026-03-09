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
gboolean roster_update_presence(const char* const barejid, Resource* resource, GDateTime* last_activity);
PContact roster_get_contact(const char* const barejid);
gboolean roster_contact_offline(const char* const barejid, const char* const resource, const char* const status);
void roster_reset_search_attempts(void);
void roster_create(void);
void roster_destroy(void);
void roster_change_name(PContact contact, const char* const new_name);
void roster_remove(const char* const name, const char* const barejid);
void roster_update(const char* const barejid, const char* const name, GSList* groups, const char* const subscription,
                   gboolean pending_out);
gboolean roster_add(const char* const barejid, const char* const name, GSList* groups, const char* const subscription,
                    gboolean pending_out);
char* roster_barejid_from_name(const char* const name);
GSList* roster_get_contacts(roster_ord_t order);
GSList* roster_get_contacts_online(void);
gboolean roster_has_pending_subscriptions(void);
char* roster_contact_autocomplete(const char* const search_str, gboolean previous, void* context);
char* roster_fulljid_autocomplete(const char* const search_str, gboolean previous, void* context);
GSList* roster_get_group(const char* const group, roster_ord_t order);
GList* roster_get_groups(void);
char* roster_group_autocomplete(const char* const search_str, gboolean previous, void* context);
char* roster_barejid_autocomplete(const char* const search_str, gboolean previous, void* context);
GSList* roster_get_contacts_by_presence(const char* const presence);
const char* roster_get_display_name(const char* const barejid);
gchar* roster_get_msg_display_name(const char* const barejid, const char* const resource);
gint roster_compare_name(PContact a, PContact b);
gint roster_compare_presence(PContact a, PContact b);
void roster_process_pending_presence(void);
gboolean roster_exists(void);

#endif
