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
#include "resource.h"

struct p_contact_t {
    char *barejid;
    char *name;
    char *subscription;
    gboolean pending_out;
    GDateTime *last_activity;
    GHashTable *resources;
};

PContact
p_contact_new(const char * const barejid, const char * const name,
    const char * const presence, const char * const status,
    const char * const subscription, gboolean pending_out,
    const char * const caps_str)
{
    PContact contact = malloc(sizeof(struct p_contact_t));
    contact->barejid = strdup(barejid);

    if (name != NULL) {
        contact->name = strdup(name);
    } else {
        contact->name = NULL;
    }

    if (subscription != NULL)
        contact->subscription = strdup(subscription);
    else
        contact->subscription = strdup("none");

    contact->pending_out = pending_out;
    contact->last_activity = NULL;

    contact->resources = g_hash_table_new_full(g_str_hash, g_str_equal, free,
        (GDestroyNotify)resource_destroy);
    // TODO, priority, last activity
    Resource *resource = resource_new("default", presence, status, 0, caps_str);
    g_hash_table_insert(contact->resources, strdup(resource->name), resource);

    return contact;
}

PContact
p_contact_new_subscription(const char * const barejid,
    const char * const subscription, gboolean pending_out)
{
    PContact contact = malloc(sizeof(struct p_contact_t));
    contact->barejid = strdup(barejid);

    contact->name = NULL;

    if (subscription != NULL)
        contact->subscription = strdup(subscription);
    else
        contact->subscription = strdup("none");

    contact->pending_out = pending_out;
    contact->last_activity = NULL;

    contact->resources = g_hash_table_new_full(g_str_hash, g_str_equal, free,
        (GDestroyNotify)resource_destroy);
    // TODO, priority, last activity
    Resource *resource = resource_new("default", "offline", NULL, 0, NULL);
    g_hash_table_insert(contact->resources, resource->name, resource);

    return contact;
}

void
p_contact_free(PContact contact)
{
    FREE_SET_NULL(contact->barejid);
    FREE_SET_NULL(contact->name);
    FREE_SET_NULL(contact->subscription);

    if (contact->last_activity != NULL) {
        g_date_time_unref(contact->last_activity);
    }

    g_hash_table_destroy(contact->resources);

    FREE_SET_NULL(contact);
}

const char *
p_contact_barejid(const PContact contact)
{
    return contact->barejid;
}

const char *
p_contact_name(const PContact contact)
{
    return contact->name;
}

const char *
p_contact_presence(const PContact contact)
{
    Resource *resource = g_hash_table_lookup(contact->resources, "default");
    return resource->show;
}

const char *
p_contact_status(const PContact contact)
{
    Resource *resource = g_hash_table_lookup(contact->resources, "default");
    return resource->status;
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
    Resource *resource = g_hash_table_lookup(contact->resources, "default");
    return resource->caps_str;
}

void
p_contact_set_presence(const PContact contact, const char * const presence)
{
    Resource *resource = g_hash_table_lookup(contact->resources, "default");
    FREE_SET_NULL(resource->show);
    if (presence != NULL) {
        resource->show = strdup(presence);
    }
}

void
p_contact_set_status(const PContact contact, const char * const status)
{
    Resource *resource = g_hash_table_lookup(contact->resources, "default");
    FREE_SET_NULL(resource->status);
    if (status != NULL) {
        resource->status = strdup(status);
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
    Resource *resource = g_hash_table_lookup(contact->resources, "default");
    FREE_SET_NULL(resource->caps_str);
    if (caps_str != NULL) {
        resource->caps_str = strdup(caps_str);
    }
}
