/*
 * avatar.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 - 2020 Michael Vetter <jubalh@iodoru.org>
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#include "log.h"
#include "xmpp/connection.h"
#include "xmpp/form.h"
#include "xmpp/iq.h"
#include "xmpp/message.h"
#include "xmpp/stanza.h"
#include "ui/ui.h"
#include "config/files.h"

typedef struct avatar_metadata {
    char *type;
    char *id;
} avatar_metadata;

char *looking_for = NULL; // nick/barejid from who we want to get the avatar

static void _avatar_request_item_by_id(const char *jid, avatar_metadata *data);
static int _avatar_metadata_handler(xmpp_stanza_t *const stanza, void *const userdata);
static int _avatar_request_item_result_handler(xmpp_stanza_t *const stanza, void *const userdata);

static void
_free_avatar_data(avatar_metadata *data)
{
    if (data) {
        free(data->type);
        free(data);
    }
}

void
avatar_pep_subscribe(void)
{
    message_pubsub_event_handler_add(STANZA_NS_USER_AVATAR_METADATA, _avatar_metadata_handler, NULL, NULL);
    message_pubsub_event_handler_add(STANZA_NS_USER_AVATAR_DATA, _avatar_metadata_handler, NULL, NULL);

    //caps_add_feature(XMPP_FEATURE_USER_AVATAR_METADATA_NOTIFY);
}

gboolean
avatar_get_by_nick(const char* nick)
{
    // in case we set the feature, remove it
    caps_remove_feature(XMPP_FEATURE_USER_AVATAR_METADATA_NOTIFY);
    free(looking_for);

    looking_for = strdup(nick);

    // add the feature. this will trigger the _avatar_metadata_notfication_handler handler
    caps_add_feature(XMPP_FEATURE_USER_AVATAR_METADATA_NOTIFY);

    return TRUE;
}

static int
_avatar_metadata_handler(xmpp_stanza_t *const stanza, void *const userdata)
{
    const char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    if (!(looking_for &&
                (g_strcmp0(looking_for, from) == 0))) {
        return 1;
    }

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
        const char *type = xmpp_stanza_get_attribute(info, "type");

        log_debug("Avatar ID for %s is: %s", from, id);

        avatar_metadata *data = malloc(sizeof(avatar_metadata));
        data->type = strdup(type);
        data->id = strdup(id);

        // request the actual (image) data
        _avatar_request_item_by_id(from, data);
    }

    return 1;
}

static void
_avatar_request_item_by_id(const char *jid, avatar_metadata *data)
{
    caps_remove_feature(XMPP_FEATURE_USER_AVATAR_METADATA_NOTIFY);

    xmpp_ctx_t * const ctx = connection_get_ctx();

    xmpp_stanza_t *iq = stanza_create_avatar_retrieve_data_request(ctx, data->id, jid);
    iq_id_handler_add("retrieve1", _avatar_request_item_result_handler, (ProfIqFreeCallback)_free_avatar_data, data);

    iq_send_stanza(iq);

    xmpp_stanza_release(iq);
}

static int
_avatar_request_item_result_handler(xmpp_stanza_t *const stanza, void *const userdata)
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

    xmpp_stanza_t *st_data = stanza_get_child_by_name_and_ns(item, "data", STANZA_NS_USER_AVATAR_DATA);
    if (!st_data) {
        return 1;
    }

    char *buf = xmpp_stanza_get_text(st_data);
    gsize size;
    gchar *de = (gchar*)g_base64_decode(buf, &size);
    free(buf);

    char *path = files_get_data_path("");
    GString *filename = g_string_new(path);
    free(path);

    g_string_append(filename, "avatars/");

    errno = 0;
    int res = g_mkdir_with_parents(filename->str, S_IRWXU);
    if (res == -1) {
        char *errmsg = strerror(errno);
        if (errmsg) {
            log_error("Avatar: error creating directory: %s, %s", filename->str, errmsg);
        } else {
            log_error("Avatar: creating directory: %s", filename->str);
        }
    }

    gchar *from = str_replace(from_attr, "@", "_at_");
    g_string_append(filename, from);

    avatar_metadata *data = (avatar_metadata*)userdata;

    // check a few image types ourselves
    // if none matches we won't add an extension but linux will
    // be able to open it anyways
    // TODO: we could use /etc/mime-types
    if (g_strcmp0(data->type, "image/png") == 0) {
        g_string_append(filename, ".png");
    } else if (g_strcmp0(data->type, "image/jpeg") == 0) {
        g_string_append(filename, ".jpeg");
    } else if (g_strcmp0(data->type, "image/webp") == 0) {
        g_string_append(filename, ".webp");
    }

    free(from);

    GError *err = NULL;
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
