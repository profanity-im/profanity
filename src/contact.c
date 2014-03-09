/*
 * contact.c
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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
    GSList *groups;
    char *subscription;
    char *offline_message;
    gboolean pending_out;
    GDateTime *last_activity;
    GHashTable *available_resources;
};

PContact
p_contact_new(const char * const barejid, const char * const name,
    GSList *groups, const char * const subscription,
    const char * const offline_message, gboolean pending_out)
{
    PContact contact = malloc(sizeof(struct p_contact_t));
    contact->barejid = strdup(barejid);

    if (name != NULL) {
        contact->name = strdup(name);
    } else {
        contact->name = NULL;
    }

    contact->groups = groups;

    if (subscription != NULL)
        contact->subscription = strdup(subscription);
    else
        contact->subscription = strdup("none");

    if (offline_message != NULL)
        contact->offline_message = strdup(offline_message);
    else
        contact->offline_message = NULL;

    contact->pending_out = pending_out;
    contact->last_activity = NULL;

    contact->available_resources = g_hash_table_new_full(g_str_hash, g_str_equal, free,
        (GDestroyNotify)resource_destroy);

    return contact;
}

void
p_contact_set_name(const PContact contact, const char * const name)
{
    FREE_SET_NULL(contact->name);
    if (name != NULL) {
        contact->name = strdup(name);
    }
}

void
p_contact_set_groups(const PContact contact, GSList *groups)
{
    if (contact->groups != NULL) {
        g_slist_free_full(contact->groups, g_free);
        contact->groups = NULL;
    }

    contact->groups = groups;
}

gboolean
p_contact_in_group(const PContact contact, const char * const group)
{
    GSList *groups = contact->groups;
    while (groups != NULL) {
        if (strcmp(groups->data, group) == 0) {
            return TRUE;
        }
        groups = g_slist_next(groups);
    }

    return FALSE;
}

GSList *
p_contact_groups(const PContact contact)
{
    return contact->groups;
}

gboolean
p_contact_remove_resource(PContact contact, const char * const resource)
{
    return g_hash_table_remove(contact->available_resources, resource);
}

void
p_contact_free(PContact contact)
{
    if (contact != NULL) {
        free(contact->barejid);
        free(contact->name);
        free(contact->subscription);
        free(contact->offline_message);

        if (contact->groups != NULL) {
            g_slist_free_full(contact->groups, g_free);
        }

        if (contact->last_activity != NULL) {
            g_date_time_unref(contact->last_activity);
        }

        g_hash_table_destroy(contact->available_resources);
        free(contact);
    }
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
p_contact_name_or_jid(const PContact contact)
{
    if (contact->name != NULL) {
        return contact->name;
    } else {
        return contact->barejid;
    }
}

char *
p_contact_create_display_string(const PContact contact, const char * const resource)
{
    GString *result_str = g_string_new("");

    // use nickname if exists
    const char *display_name = p_contact_name_or_jid(contact);
    g_string_append(result_str, display_name);

    // add resource if not default provided by profanity
    if (strcmp(resource, "__prof_default") != 0) {
        g_string_append(result_str, " (");
        g_string_append(result_str, resource);
        g_string_append(result_str, ")");
    }

    char *result = result_str->str;
    g_string_free(result_str, FALSE);

    return result;
}

static Resource *
_highest_presence(Resource *first, Resource *second)
{
    if (first->presence == RESOURCE_CHAT) {
        return first;
    } else if (second->presence == RESOURCE_CHAT) {
        return second;
    } else if (first->presence == RESOURCE_ONLINE) {
        return first;
    } else if (second->presence == RESOURCE_ONLINE) {
        return second;
    } else if (first->presence == RESOURCE_AWAY) {
        return first;
    } else if (second->presence == RESOURCE_AWAY) {
        return second;
    } else if (first->presence == RESOURCE_XA) {
        return first;
    } else if (second->presence == RESOURCE_XA) {
        return second;
    } else {
        return first;
    }
}

Resource *
_get_most_available_resource(PContact contact)
{
    // find resource with highest priority, if more than one,
    // use highest availability, in the following order:
    //      chat
    //      online
    //      away
    //      xa
    //      dnd
    GList *resources = g_hash_table_get_values(contact->available_resources);
    Resource *current = resources->data;
    Resource *highest = current;
    resources = g_list_next(resources);
    while (resources != NULL) {
        current = resources->data;

        // priority is same as current highest, choose presence
        if (current->priority == highest->priority) {
            highest = _highest_presence(highest, current);

        // priority higher than current highest, set new presence
        } else if (current->priority > highest->priority) {
            highest = current;
        }

        resources = g_list_next(resources);
    }

    return highest;
}

const char *
p_contact_presence(const PContact contact)
{
    assert(contact != NULL);

    // no available resources, offline
    if (g_hash_table_size(contact->available_resources) == 0) {
        return "offline";
    }

    Resource *resource = _get_most_available_resource(contact);

    return string_from_resource_presence(resource->presence);
}

const char *
p_contact_status(const PContact contact)
{
    assert(contact != NULL);

    // no available resources, use offline message
    if (g_hash_table_size(contact->available_resources) == 0) {
        return contact->offline_message;
    }

    Resource *resource = _get_most_available_resource(contact);

    return resource->status;
}

const char *
p_contact_subscription(const PContact contact)
{
    return contact->subscription;
}

gboolean
p_contact_subscribed(const PContact contact)
{
    if (contact->subscription == NULL) {
        return FALSE;
    } else if (strcmp(contact->subscription, "to") == 0) {
        return TRUE;
    } else if (strcmp(contact->subscription, "both") == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
}

Resource *
p_contact_get_resource(const PContact contact, const char * const resource)
{
    return g_hash_table_lookup(contact->available_resources, resource);
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

GList *
p_contact_get_available_resources(const PContact contact)
{
    assert(contact != NULL);

    return g_hash_table_get_values(contact->available_resources);
}

gboolean
p_contact_is_available(const PContact contact)
{
    // no available resources, unavailable
    if (g_hash_table_size(contact->available_resources) == 0) {
        return FALSE;
    }

    // if most available resource is CHAT or ONLINE, available
    Resource *most_available = _get_most_available_resource(contact);
    if ((most_available->presence == RESOURCE_ONLINE) ||
        (most_available->presence == RESOURCE_CHAT)) {
        return TRUE;
    } else {
        return FALSE;
    }
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
