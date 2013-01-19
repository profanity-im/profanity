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

typedef struct p_contact_t *PContact;

PContact p_contact_new(const char * const jid, const char * const name,
    const char * const presence, const char * const status,
    const char * const subscription, gboolean pending_out);
void p_contact_free(PContact contact);
const char* p_contact_jid(PContact contact);
const char* p_contact_name(PContact contact);
const char* p_contact_presence(PContact contact);
const char* p_contact_status(PContact contact);
const char* p_contact_subscription(const PContact contact);
const char* p_contact_caps_str(const PContact contact);
GDateTime* p_contact_last_activity(const PContact contact);
gboolean p_contact_pending_out(const PContact contact);
void p_contact_set_presence(const PContact contact, const char * const presence);
void p_contact_set_status(const PContact contact, const char * const status);
void p_contact_set_subscription(const PContact contact, const char * const subscription);
void p_contact_set_caps_str(const PContact contact, const char * const caps_str);
void p_contact_set_pending_out(const PContact contact, gboolean pending_out);
void p_contact_set_last_activity(const PContact contact, GDateTime *last_activity);

#endif
