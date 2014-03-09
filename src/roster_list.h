/*
 * roster_list.h
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

#ifndef ROSTER_LIST_H
#define ROSTER_LIST_H

#include <glib.h>

#include "resource.h"
#include "contact.h"

void roster_clear(void);
gboolean roster_update_presence(const char * const barejid, Resource *resource,
    GDateTime *last_activity);
PContact roster_get_contact(const char * const barejid);
gboolean roster_contact_offline(const char * const barejid,
    const char * const resource, const char * const status);
void roster_reset_search_attempts(void);
void roster_init(void);
void roster_free(void);
void roster_change_name(PContact contact, const char * const new_name);
void roster_remove(const char * const name, const char * const barejid);
void roster_update(const char * const barejid, const char * const name,
    GSList *groups, const char * const subscription, gboolean pending_out);
gboolean roster_add(const char * const barejid, const char * const name, GSList *groups,
    const char * const subscription, gboolean pending_out);
char * roster_barejid_from_name(const char * const name);
GSList * roster_get_contacts(void);
gboolean roster_has_pending_subscriptions(void);
char * roster_find_contact(char *search_str);
char * roster_find_resource(char *search_str);
GSList * roster_get_group(const char * const group);
GSList * roster_get_groups(void);
char * roster_find_group(char *search_str);
char * roster_find_jid(char *search_str);

#endif
