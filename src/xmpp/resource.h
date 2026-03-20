/*
 * resource.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_RESOURCE_H
#define XMPP_RESOURCE_H

#include "common.h"

typedef struct resource_t
{
    gchar* name;
    resource_presence_t presence;
    gchar* status;
    int priority;
} Resource;

Resource* resource_new(const gchar* const name, resource_presence_t presence, const gchar* const status,
                       const int priority);
Resource* resource_copy(Resource* resource);
void resource_destroy(Resource* resource);
int resource_compare_availability(Resource* first, Resource* second);

gboolean valid_resource_presence_string(const gchar* const str);
const gchar* string_from_resource_presence(resource_presence_t presence);
resource_presence_t resource_presence_from_string(const gchar* const str);
contact_presence_t contact_presence_from_resource_presence(resource_presence_t resource_presence);

#endif
