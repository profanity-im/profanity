/*
 * roster_list.h
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
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
gboolean roster_update_presence(const char *const barejid, Resource *resource, GDateTime *last_activity);
PContact roster_get_contact(const char *const barejid);
gboolean roster_contact_offline(const char *const barejid, const char *const resource, const char *const status);
void roster_reset_search_attempts(void);
void roster_create(void);
void roster_destroy(void);
void roster_change_name(PContact contact, const char *const new_name);
void roster_remove(const char *const name, const char *const barejid);
void roster_update(const char *const barejid, const char *const name, GSList *groups, const char *const subscription,
    gboolean pending_out);
gboolean roster_add(const char *const barejid, const char *const name, GSList *groups, const char *const subscription,
    gboolean pending_out);
char* roster_barejid_from_name(const char *const name);
GSList* roster_get_contacts(roster_ord_t order);
GSList* roster_get_contacts_online(void);
gboolean roster_has_pending_subscriptions(void);
char* roster_contact_autocomplete(const char *const search_str, gboolean previous);
char* roster_fulljid_autocomplete(const char *const search_str, gboolean previous);
GSList* roster_get_group(const char *const group, roster_ord_t order);
GList* roster_get_groups(void);
char* roster_group_autocomplete(const char *const search_str, gboolean previous);
char* roster_barejid_autocomplete(const char *const search_str, gboolean previous);
GSList* roster_get_contacts_by_presence(const char *const presence);
char* roster_get_msg_display_name(const char *const barejid, const char *const resource);

#endif
