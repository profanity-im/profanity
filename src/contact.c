/*
 * contact.c
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

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "contact.h"

#include "common.h"

struct p_contact_t {
    char *jid;
    char *name;
    char *presence;
    char *status;
    char *subscription;
    char *caps_str;
    gboolean pending_out;
    GDateTime *last_activity;
};

PContact
p_contact_new(const char * const jid, const char * const name,
    const char * const presence, const char * const status,
    const char * const subscription, gboolean pending_out,
    const char * const caps_str)
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
        contact->subscription = strdup("none");

    if (caps_str != NULL)
        contact->caps_str = strdup(caps_str);
    else
        contact->caps_str = NULL;

    contact->pending_out = pending_out;
    contact->last_activity = NULL;

    return contact;
}

void
p_contact_free(PContact contact)
{
    FREE_SET_NULL(contact->jid);
    FREE_SET_NULL(contact->name);
    FREE_SET_NULL(contact->presence);
    FREE_SET_NULL(contact->status);
    FREE_SET_NULL(contact->subscription);
    FREE_SET_NULL(contact->caps_str);

    if (contact->last_activity != NULL) {
        g_date_time_unref(contact->last_activity);
    }

    FREE_SET_NULL(contact);
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

const char *
p_contact_caps_str(const PContact contact)
{
    return contact->caps_str;
}

void
p_contact_set_presence(const PContact contact, const char * const presence)
{
    FREE_SET_NULL(contact->presence);
    if (presence != NULL) {
        contact->presence = strdup(presence);
    }
}

void
p_contact_set_status(const PContact contact, const char * const status)
{
    FREE_SET_NULL(contact->status);
    if (status != NULL) {
        contact->status = strdup(status);
    }
}

void
p_contact_set_subscription(const PContact contact, const char * const subscription)
{
    FREE_SET_NULL(contact->subscription);
    if (subscription != NULL) {
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

void
p_contact_set_caps_str(const PContact contact, const char * const caps_str)
{
    FREE_SET_NULL(contact->caps_str);
    if (caps_str != NULL) {
        contact->caps_str = strdup(caps_str);
    }
}
