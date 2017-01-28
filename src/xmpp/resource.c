/*
 * resource.c
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "xmpp/resource.h"

Resource*
resource_new(const char *const name, resource_presence_t presence, const char *const status, const int priority)
{
    assert(name != NULL);
    Resource *new_resource = malloc(sizeof(struct resource_t));
    new_resource->name = strdup(name);
    new_resource->presence = presence;
    if (status) {
        new_resource->status = strdup(status);
    } else {
        new_resource->status = NULL;
    }
    new_resource->priority = priority;

    return new_resource;
}

int
resource_compare_availability(Resource *first, Resource *second)
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
resource_destroy(Resource *resource)
{
    if (resource) {
        free(resource->name);
        free(resource->status);
        free(resource);
    }
}

gboolean
valid_resource_presence_string(const char *const str)
{
    assert(str != NULL);
    if ((strcmp(str, "online") == 0) || (strcmp(str, "chat") == 0) ||
            (strcmp(str, "away") == 0) || (strcmp(str, "xa") == 0) ||
            (strcmp(str, "dnd") == 0)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

const char*
string_from_resource_presence(resource_presence_t presence)
{
    switch(presence)
    {
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
resource_presence_from_string(const char *const str)
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
    switch(resource_presence)
    {
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
