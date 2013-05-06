/*
 * contact_list.h
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

#ifndef CONTACT_LIST_H
#define CONTACT_LIST_H

#include <glib.h>

#include "contact.h"

void roster_init(void);
void roster_clear(void);
void roster_free(void);
void roster_reset_search_attempts(void);
void roster_remove(const char * const barejid);
gboolean roster_add(const char * const barejid, const char * const name,
    const char * const subscription, const char * const offline_message,
    gboolean pending_out);
gboolean roster_update_presence(const char * const barejid,
    Resource *resource, GDateTime *last_activity);
void roster_update_subscription(const char * const barejid,
    const char * const subscription, gboolean pending_out);
gboolean roster_has_pending_subscriptions(void);
GSList * roster_get_contacts(void);
char * roster_find_contact(char *search_str);
char * roster_find_resource(char *search_str);
PContact roster_get_contact(const char const *barejid);
gboolean roster_contact_offline(const char * const barejid,
    const char * const resource, const char * const status);

#endif
