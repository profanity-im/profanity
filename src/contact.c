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

#include <assert.h>
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
    GHashTable *available_resources;
};

PContact
p_contact_new(const char * const barejid, const char * const name,
    const char * const subscription, gboolean pending_out)
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

    contact->available_resources = g_hash_table_new_full(g_str_hash, g_str_equal, free,
        (GDestroyNotify)resource_destroy);

    return contact;
}

gboolean
p_contact_remove_resource(PContact contact, const char * const resource)
{
    return g_hash_table_remove(contact->available_resources, resource);
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

    contact->available_resources = g_hash_table_new_full(g_str_hash, g_str_equal, free,
        (GDestroyNotify)resource_destroy);

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

    g_hash_table_destroy(contact->available_resources);

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

static resource_presence_t
_highest_presence(resource_presence_t first, resource_presence_t second)
{
    if (first == RESOURCE_CHAT) {
        return first;
    } else if (second == RESOURCE_CHAT) {
        return second;
    } else if (first == RESOURCE_ONLINE) {
        return first;
    } else if (second == RESOURCE_ONLINE) {
        return second;
    } else if (first == RESOURCE_AWAY) {
        return first;
    } else if (second == RESOURCE_AWAY) {
        return second;
    } else if (first == RESOURCE_XA) {
        return first;
    } else if (second == RESOURCE_XA) {
        return second;
    } else {
        return first;
    }
}

const char *
p_contact_presence(const PContact contact)
{
    assert(contact != NULL);

    // no available resources, offline
    if (g_hash_table_size(contact->available_resources) == 0) {
        return "offline";
    }

    // find resource with highest priority, if more than one,
    // use highest availability, in the following order:
    //      chat
    //      online
    //      away
    //      xa
    //      dnd
    GList *resources = g_hash_table_get_values(contact->available_resources);
    Resource *resource = resources->data;
    int highest_priority = resource->priority;
    resource_presence_t presence = resource->presence;
    resources = g_list_next(resources);
    while (resources != NULL) {
        resource = resources->data;

        // priority is same as current highest, choose presence
        if (resource->priority == highest_priority) {
            presence = _highest_presence(presence, resource->presence);

        // priority higher than current highest, set new presence
        } else if (resource->priority > highest_priority) {
            highest_priority = resource->priority;
            presence = resource->presence;
        }

        resources = g_list_next(resources);
    }

    return string_from_resource_presence(presence);
}

const char *
p_contact_status(const PContact contact, const char * const resource)
{
    assert(contact != NULL);
    assert(resource != NULL);
    Resource *resourcep = g_hash_table_lookup(contact->available_resources, "default");
    return resourcep->status;
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
    if (g_hash_table_size(contact->available_resources) == 0) {
        return NULL;
    } else {
        Resource *resource = g_hash_table_lookup(contact->available_resources, "default");
        return resource->caps_str;
    }
}

gboolean
p_contact_is_available(const PContact contact)
{
    // no available resources, unavailable
    if (g_hash_table_size(contact->available_resources) == 0) {
        return FALSE;
    }

    // if any resource is CHAT or ONLINE, available
    GList *resources = g_hash_table_get_values(contact->available_resources);
    while (resources != NULL) {
        Resource *resource = resources->data;
        resource_presence_t presence = resource->presence;
        if ((presence == RESOURCE_ONLINE) || (presence == RESOURCE_CHAT)) {
            return TRUE;
        }
        resources = g_list_next(resources);
    }

    return FALSE;
}

gboolean
p_contact_has_available_resource(const PContact contact)
{
    return (g_hash_table_size(contact->available_resources) > 0);
}

void
p_contact_set_presence(const PContact contact, Resource *resource)
{
    g_hash_table_replace(contact->available_resources, strdup(resource->name), resource);
}

void
p_contact_set_status(const PContact contact, const char * const status)
{
    Resource *resource = g_hash_table_lookup(contact->available_resources, "default");
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
    Resource *resource = g_hash_table_lookup(contact->available_resources, "default");
    FREE_SET_NULL(resource->caps_str);
    if (caps_str != NULL) {
        resource->caps_str = strdup(caps_str);
    }
}
