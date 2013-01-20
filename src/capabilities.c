/*
 * capabilities.c
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

#include "common.h"
#include "capabilities.h"

Capabilities *
caps_create(const char * const client, const char * const version)
{
    Capabilities *new_caps = malloc(sizeof(struct capabilities_t));

    if (client != NULL) {
        new_caps->client = strdup(client);
    } else {
        new_caps->client = NULL;
    }

    if (version != NULL) {
        new_caps->version = strdup(version);
    } else {
        new_caps->version = NULL;
    }

    return new_caps;
}

void
caps_destroy(Capabilities *caps)
{
    if (caps != NULL) {
        FREE_SET_NULL(caps->client);
        FREE_SET_NULL(caps->version);
        FREE_SET_NULL(caps);
    }
}
