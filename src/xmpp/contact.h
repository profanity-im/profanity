/*
 * contact.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_CONTACT_H
#define XMPP_CONTACT_H

#include "tools/autocomplete.h"
#include "xmpp/resource.h"

typedef struct p_contact_t* PContact;

PContact p_contact_new(const char* const barejid, const char* const name, GSList* groups,
                       const char* const subscription, const char* const offline_message, gboolean pending_out);
void p_contact_add_resource(PContact contact, Resource* resource);
gboolean p_contact_remove_resource(PContact contact, const char* const resource);
void p_contact_free(PContact contact);
const char* p_contact_barejid(PContact contact);
const char* p_contact_barejid_collate_key(PContact contact);
const char* p_contact_name(PContact contact);
const char* p_contact_name_collate_key(PContact contact);
const char* p_contact_name_or_jid(const PContact contact);
const char* p_contact_presence(PContact contact);
const char* p_contact_status(PContact contact);
const char* p_contact_subscription(const PContact contact);
GList* p_contact_get_available_resources(const PContact contact);
GDateTime* p_contact_last_activity(const PContact contact);
gboolean p_contact_pending_out(const PContact contact);
void p_contact_set_presence(const PContact contact, Resource* resource);
void p_contact_set_status(const PContact contact, const char* const status);
void p_contact_set_name(const PContact contact, const char* const name);
void p_contact_set_subscription(const PContact contact, const char* const subscription);
void p_contact_set_pending_out(const PContact contact, gboolean pending_out);
void p_contact_set_last_activity(const PContact contact, GDateTime* last_activity);
gboolean p_contact_is_available(const PContact contact);
gboolean p_contact_has_available_resource(const PContact contact);
Resource* p_contact_get_resource(const PContact contact, const char* const resource);
void p_contact_set_groups(const PContact contact, GSList* groups);
GSList* p_contact_groups(const PContact contact);
gboolean p_contact_in_group(const PContact contact, const char* const group);
gboolean p_contact_subscribed(const PContact contact);
char* p_contact_create_display_string(const PContact contact, const char* const resource);
Autocomplete p_contact_resource_ac(const PContact contact);
void p_contact_resource_ac_reset(const PContact contact);

#endif
