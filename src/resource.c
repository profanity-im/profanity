/*
 * resource.c
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

#include <common.h>
#include <resource.h>

Resource * resource_new(const char * const name, resource_presence_t presence,
    const char * const status, const int priority, const char * const caps_str)
{
    assert(name != NULL);
    Resource *new_resource = malloc(sizeof(struct resource_t));
    new_resource->name = strdup(name);
    new_resource->presence = presence;
    if (status != NULL) {
        new_resource->status = strdup(status);
    } else {
        new_resource->status = NULL;
    }
    new_resource->priority = priority;
    if (caps_str != NULL) {
        new_resource->caps_str = strdup(caps_str);
    } else {
        new_resource->caps_str = NULL;
    }

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

void resource_destroy(Resource *resource)
{
    if (resource != NULL) {
        free(resource->name);
        free(resource->status);
        free(resource->caps_str);
        free(resource);
    }
}
