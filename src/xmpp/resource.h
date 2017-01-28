/*
 * resource.h
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

#ifndef XMPP_RESOURCE_H
#define XMPP_RESOURCE_H

#include "common.h"

typedef struct resource_t {
    char *name;
    resource_presence_t presence;
    char *status;
    int priority;
} Resource;

Resource* resource_new(const char *const name, resource_presence_t presence, const char *const status,
    const int priority);
void resource_destroy(Resource *resource);
int resource_compare_availability(Resource *first, Resource *second);

gboolean valid_resource_presence_string(const char *const str);
const char* string_from_resource_presence(resource_presence_t presence);
resource_presence_t resource_presence_from_string(const char *const str);
contact_presence_t contact_presence_from_resource_presence(resource_presence_t resource_presence);

#endif
