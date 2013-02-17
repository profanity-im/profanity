/*
 * contact.h
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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

#ifndef CONTACT_H
#define CONTACT_H

#include "resource.h"

typedef struct p_contact_t *PContact;

PContact p_contact_new(const char * const barejid, const char * const name,
    const char * const subscription, const char * const offline_message,
    gboolean pending_out);
PContact p_contact_new_subscription(const char * const barejid,
    const char * const subscription, gboolean pending_out);
void p_contact_add_resource(PContact contact, Resource *resource);
gboolean p_contact_remove_resource(PContact contact, const char * const resource);
void p_contact_free(PContact contact);
const char* p_contact_barejid(PContact contact);
const char* p_contact_name(PContact contact);
const char* p_contact_presence(PContact contact);
const char* p_contact_status(PContact contact);
const char* p_contact_subscription(const PContact contact);
GList * p_contact_get_available_resources(const PContact contact);
GDateTime* p_contact_last_activity(const PContact contact);
gboolean p_contact_pending_out(const PContact contact);
void p_contact_set_presence(const PContact contact, Resource *resource);
void p_contact_set_status(const PContact contact, const char * const status);
void p_contact_set_subscription(const PContact contact, const char * const subscription);
void p_contact_set_pending_out(const PContact contact, gboolean pending_out);
void p_contact_set_last_activity(const PContact contact, GDateTime *last_activity);
gboolean p_contact_is_available(const PContact contact);
gboolean p_contact_has_available_resource(const PContact contact);
Resource * p_contact_get_resource(const PContact contact, const char * const resource);

#endif
