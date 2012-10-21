/*
 * contact.h
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
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

PContact p_contact_new(const char * const name, const char * const show,
    const char * const status);
PContact p_contact_copy(PContact contact);
void p_contact_free(PContact contact);
const char * p_contact_name(PContact contact);
const char * p_contact_show(PContact contact);
const char * p_contact_status(PContact contact);
int p_contacts_equal_deep(const PContact c1, const PContact c2);

#endif
