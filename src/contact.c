/*
 * contact.c
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "contact.h"

struct p_contact_t {
    char *jid;
    char *name;
    char *presence;
    char *status;
    char *subscription;
};

PContact
p_contact_new(const char * const jid, const char * const name,
    const char * const presence, const char * const status,
    const char * const subscription)
{
    PContact contact = malloc(sizeof(struct p_contact_t));
    contact->jid = strdup(jid);

    if (name != NULL) {
        contact->name = strdup(name);
    } else {
        contact->name = NULL;
    }

    if (presence == NULL || (strcmp(presence, "") == 0))
        contact->presence = strdup("online");
    else
        contact->presence = strdup(presence);

    if (status != NULL)
        contact->status = strdup(status);
    else
        contact->status = NULL;

    return contact;
}

PContact
p_contact_copy(PContact contact)
{
    PContact copy = malloc(sizeof(struct p_contact_t));
    copy->jid = strdup(contact->jid);

    if (contact->name != NULL) {
        copy->name = strdup(contact->name);
    } else {
        copy->name = NULL;
    }

    copy->presence = strdup(contact->presence);

    if (contact->status != NULL)
        copy->status = strdup(contact->status);
    else
        copy->status = NULL;

    return copy;
}

void
p_contact_free(PContact contact)
{
    if (contact->jid != NULL) {
        free(contact->jid);
        contact->jid = NULL;
    }

    if (contact->name != NULL) {
        free(contact->name);
        contact->name = NULL;
    }

    if (contact->presence != NULL) {
        free(contact->presence);
        contact->presence = NULL;
    }

    if (contact->status != NULL) {
        free(contact->status);
        contact->status = NULL;
    }

    free(contact);
    contact = NULL;
}

const char *
p_contact_jid(const PContact contact)
{
    return contact->jid;
}

const char *
p_contact_name(const PContact contact)
{
    return contact->name;
}

const char *
p_contact_presence(const PContact contact)
{
    return contact->presence;
}

const char *
p_contact_status(const PContact contact)
{
    return contact->status;
}

int
p_contacts_equal_deep(const PContact c1, const PContact c2)
{
    int jid_eq = (g_strcmp0(c1->jid, c2->jid) == 0);
    int name_eq = (g_strcmp0(c1->name, c2->name) == 0);
    int presence_eq = (g_strcmp0(c1->presence, c2->presence) == 0);
    int status_eq = (g_strcmp0(c1->status, c2->status) == 0);

    return (jid_eq && name_eq && presence_eq && status_eq);
}
