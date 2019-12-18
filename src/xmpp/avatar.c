/*
 * omemo.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 Michael Vetter <jubalh@iodoru.org>
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

#include <glib.h>

#include "log.h"
#include "xmpp/connection.h"
#include "xmpp/form.h"
#include "xmpp/iq.h"
#include "xmpp/message.h"
#include "xmpp/stanza.h"

static int _avatar_metadata_nofication(xmpp_stanza_t *const stanza, void *const userdata);

void
avatar_pep_subscribe(void)
{
    message_pubsub_event_handler_add(STANZA_NS_USER_AVATAR_METADATA, _avatar_metadata_nofication, NULL, NULL);
    message_pubsub_event_handler_add(STANZA_NS_USER_AVATAR_DATA, _avatar_metadata_nofication, NULL, NULL);

    caps_add_feature(XMPP_FEATURE_USER_AVATAR_METADATA_NOTIFY);
}

static int
_avatar_metadata_nofication(xmpp_stanza_t *const stanza, void *const userdata)
{
    const char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    from = from;

    xmpp_stanza_t *root = NULL;
    xmpp_stanza_t *event = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_PUBSUB_EVENT);
    if (event) {
        root = event;
    }

    xmpp_stanza_t *pubsub = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_PUBSUB);
    if (pubsub) {
        root = pubsub;
    }

    if (!root) {
        return 1;
    }

    xmpp_stanza_t *items = xmpp_stanza_get_child_by_name(root, "items");
    if (!items) {
        return 1;
    }

    xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(items, "item");
    if (item) {
        xmpp_stanza_t *metadata = xmpp_stanza_get_child_by_name(item, "metadata");
        if (!metadata)
            return 1;

        xmpp_stanza_t *info = xmpp_stanza_get_child_by_name(metadata, "info");
        const char *id = xmpp_stanza_get_id(info);
        id = id;
    }

    return 1;
}
