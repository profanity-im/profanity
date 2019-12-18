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
#include <stdio.h>

#include "log.h"
#include "xmpp/connection.h"
#include "xmpp/form.h"
#include "xmpp/iq.h"
#include "xmpp/message.h"
#include "xmpp/stanza.h"
#include "ui/ui.h"
#include "config/files.h"

char *looking_for = NULL;

static int _avatar_metadata_nofication(xmpp_stanza_t *const stanza, void *const userdata);
void avatar_request_item_by_id(const char *jid, const char *id);
int avatar_request_item_handler(xmpp_stanza_t *const stanza, void *const userdata);

void
avatar_pep_subscribe(void)
{
    message_pubsub_event_handler_add(STANZA_NS_USER_AVATAR_METADATA, _avatar_metadata_nofication, NULL, NULL);
    message_pubsub_event_handler_add(STANZA_NS_USER_AVATAR_DATA, _avatar_metadata_nofication, NULL, NULL);

    //caps_add_feature(XMPP_FEATURE_USER_AVATAR_METADATA_NOTIFY);
}

bool
avatar_get_by_nick(const char* nick)
{
    looking_for = strdup(nick);
    caps_add_feature(XMPP_FEATURE_USER_AVATAR_METADATA_NOTIFY);
    return TRUE;
}

static int
_avatar_metadata_nofication(xmpp_stanza_t *const stanza, void *const userdata)
{
    const char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    if (!(looking_for &&
            (g_strcmp0(looking_for, from) == 0))) {
        return 1;
    }

//    free(looking_for);
//    looking_for = NULL;

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

        cons_show("Id for %s is: %s", from, id);
        avatar_request_item_by_id(from, id);
    }

    return 1;
}

void
avatar_request_item_by_id(const char *jid, const char *id)
{
    caps_remove_feature(XMPP_FEATURE_USER_AVATAR_METADATA_NOTIFY);

    xmpp_ctx_t * const ctx = connection_get_ctx();
    //char *id = connection_create_stanza_id();

    xmpp_stanza_t *iq = stanza_create_avatar_retrieve_data_request(ctx, id, jid);
    iq_id_handler_add("retrieve1", avatar_request_item_handler, free, NULL);

    iq_send_stanza(iq);

    //free(id);
    xmpp_stanza_release(iq);
}

int
avatar_request_item_handler(xmpp_stanza_t *const stanza, void *const userdata)
{
    const char *from_attr = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    if (!from_attr) {
        return 1;
    }

    if (g_strcmp0(from_attr, looking_for) != 0) {
        return 1;
    }
    free(looking_for);
    looking_for = NULL;

    xmpp_stanza_t *pubsub = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_PUBSUB);
    if (!pubsub) {
        return 1;
    }

    xmpp_stanza_t *items = xmpp_stanza_get_child_by_name(pubsub, "items");
    if (!items) {
        return 1;
    }

    xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(items, "item");
    if (!item) {
        return 1;
    }

    xmpp_stanza_t *data = stanza_get_child_by_name_and_ns(item, "data", STANZA_NS_USER_AVATAR_DATA);
    if (!data) {
        return 1;
    }

    char *buf = xmpp_stanza_get_text(data);
    gsize size;
    gchar *de = (gchar*)g_base64_decode(buf, &size);
    free(buf);
    GError *err = NULL;
    char *path = files_get_data_path("");
    GString *filename = g_string_new(path);
    g_string_append(filename, from_attr);
    g_string_append(filename, ".png");
    free(path);

    if (g_file_set_contents (filename->str, de, size, &err) == FALSE) {
        log_error("Unable to save picture: %s", err->message);
        cons_show("Unable to save picture %s", err->message);
        g_error_free(err);
    } else {
        cons_show("Avatar saved as %s", filename->str);
    }

    g_string_free(filename, TRUE);
    free(de);

    return 1;
}
