/*
 * resource.c
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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

#include <common.h>
#include <resource.h>

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
