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

void contact_list_init(void);
void contact_list_clear(void);
void contact_list_free(void);
void contact_list_reset_search_attempts(void);
void contact_list_remove(const char * const barejid);
gboolean contact_list_add(const char * const barejid, const char * const name,
    const char * const subscription, const char * const offline_message,
    gboolean pending_out);
gboolean contact_list_update_presence(const char * const barejid,
    Resource *resource, GDateTime *last_activity);
void contact_list_update_subscription(const char * const barejid,
    const char * const subscription, gboolean pending_out);
gboolean contact_list_has_pending_subscriptions(void);
GSList * get_contact_list(void);
char * contact_list_find_contact(char *search_str);
PContact contact_list_get_contact(const char const *barejid);
gboolean contact_list_contact_offline(const char * const barejid,
    const char * const resource, const char * const status);

#endif
