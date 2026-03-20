/*
 * resource.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "common.h"
#include "xmpp/resource.h"

Resource*
resource_new(const gchar* const name, resource_presence_t presence, const gchar* const status, const int priority)
{
    assert(name != NULL);
    Resource* new_resource = g_new(Resource, 1);
    new_resource->name = g_strdup(name);
    new_resource->presence = presence;
    if (status) {
        new_resource->status = g_strdup(status);
    } else {
        new_resource->status = NULL;
    }
    new_resource->priority = priority;

    return new_resource;
}

int
resource_compare_availability(Resource* first, Resource* second)
{
    if (first->priority > second->priority) {
        return -1;
    } else if (first->priority < second->priority) {
        return 1;
    } else { // priorities equal
        if (first->presence == RESOURCE_CHAT) {
            return -1;
        } else if (second->presence == RESOURCE_CHAT) {
            return 1;
        } else if (first->presence == RESOURCE_ONLINE) {
            return -1;
        } else if (second->presence == RESOURCE_ONLINE) {
            return 1;
        } else if (first->presence == RESOURCE_AWAY) {
            return -1;
        } else if (second->presence == RESOURCE_AWAY) {
            return 1;
        } else if (first->presence == RESOURCE_XA) {
            return -1;
        } else if (second->presence == RESOURCE_XA) {
            return 1;
        } else {
            return -1;
        }
    }
}

void
resource_destroy(Resource* resource)
{
    if (resource) {
        g_free(resource->name);
        g_free(resource->status);
        g_free(resource);
    }
}

Resource*
resource_copy(Resource* resource)
{
    if (resource == NULL) {
        return NULL;
    }

    return resource_new(resource->name, resource->presence, resource->status, resource->priority);
}

gboolean
valid_resource_presence_string(const gchar* const str)
{
    assert(str != NULL);
    if ((strcmp(str, "online") == 0) || (strcmp(str, "chat") == 0) || (strcmp(str, "away") == 0) || (strcmp(str, "xa") == 0) || (strcmp(str, "dnd") == 0)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

const gchar*
string_from_resource_presence(resource_presence_t presence)
{
    switch (presence) {
    case RESOURCE_CHAT:
        return "chat";
    case RESOURCE_AWAY:
        return "away";
    case RESOURCE_XA:
        return "xa";
    case RESOURCE_DND:
        return "dnd";
    default:
        return "online";
    }
}

resource_presence_t
resource_presence_from_string(const gchar* const str)
{
    if (str == NULL) {
        return RESOURCE_ONLINE;
    } else if (strcmp(str, "online") == 0) {
        return RESOURCE_ONLINE;
    } else if (strcmp(str, "chat") == 0) {
        return RESOURCE_CHAT;
    } else if (strcmp(str, "away") == 0) {
        return RESOURCE_AWAY;
    } else if (strcmp(str, "xa") == 0) {
        return RESOURCE_XA;
    } else if (strcmp(str, "dnd") == 0) {
        return RESOURCE_DND;
    } else {
        return RESOURCE_ONLINE;
    }
}

contact_presence_t
contact_presence_from_resource_presence(resource_presence_t resource_presence)
{
    switch (resource_presence) {
    case RESOURCE_CHAT:
        return CONTACT_CHAT;
    case RESOURCE_AWAY:
        return CONTACT_AWAY;
    case RESOURCE_XA:
        return CONTACT_XA;
    case RESOURCE_DND:
        return CONTACT_DND;
    default:
        return CONTACT_ONLINE;
    }
}
