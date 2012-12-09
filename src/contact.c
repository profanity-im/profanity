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
    gboolean pending_out;
    GDateTime *last_activity;
};

PContact
p_contact_new(const char * const jid, const char * const name,
    const char * const presence, const char * const status,
    const char * const subscription, gboolean pending_out)
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

    if (subscription != NULL)
        contact->subscription = strdup(subscription);
    else
        contact->subscription = strdup("none");;

    contact->pending_out = pending_out;

    contact->last_activity = NULL;

    return contact;
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

    if (contact->subscription != NULL) {
        free(contact->subscription);
        contact->subscription = NULL;
    }

    if (contact->last_activity != NULL) {
        g_date_time_unref(contact->last_activity);
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

const char *
p_contact_subscription(const PContact contact)
{
    return contact->subscription;
}

gboolean
p_contact_pending_out(const PContact contact)
{
    return contact->pending_out;
}

GDateTime *
p_contact_last_activity(const PContact contact)
{
    return contact->last_activity;
}

void
p_contact_set_presence(const PContact contact, const char * const presence)
{
    if (contact->presence != NULL) {
        free(contact->presence);
        contact->presence = NULL;
    }

    if (presence == NULL) {
        contact->presence = NULL;
    } else {
        contact->presence = strdup(presence);
    }
}

void
p_contact_set_status(const PContact contact, const char * const status)
{
    if (contact->status != NULL) {
        free(contact->status);
        contact->status = NULL;
    }

    if (status == NULL) {
        contact->status = NULL;
    } else {
        contact->status = strdup(status);
    }
}

void
p_contact_set_subscription(const PContact contact, const char * const subscription)
{
    if (contact->subscription != NULL) {
        free(contact->subscription);
        contact->subscription = NULL;
    }

    if (subscription == NULL) {
        contact->subscription = strdup("none");
    } else {
        contact->subscription = strdup(subscription);
    }
}

void
p_contact_set_pending_out(const PContact contact, gboolean pending_out)
{
    contact->pending_out = pending_out;
}

void
p_contact_set_last_activity(const PContact contact, GDateTime *last_activity)
{
    if (contact->last_activity != NULL) {
        g_date_time_unref(contact->last_activity);
        contact->last_activity = NULL;
    }

    if (last_activity != NULL) {
        contact->last_activity = g_date_time_ref(last_activity);
    }
}
