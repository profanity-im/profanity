/*
 * avatar.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <glib.h>
#ifdef HAVE_PIXBUF
#include <gdk-pixbuf-2.0/gdk-pixbuf/gdk-pixbuf.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#include "log.h"
#include "common.h"
#include "xmpp/connection.h"
#include "xmpp/jid.h"
#include "xmpp/iq.h"
#include "xmpp/message.h"
#include "xmpp/stanza.h"
#include "ui/ui.h"
#include "config/files.h"
#include "config/preferences.h"

typedef struct avatar_metadata
{
    char* type;
    char* id;
} avatar_metadata;

static GHashTable* looking_for = NULL; // contains nicks/barejids from who we want to get the avatar
static GHashTable* shall_open = NULL;  // contains a list of nicks that shall not just downloaded but also opened
const int MAX_PIXEL = 192;             // max pixel width/height for an avatar

static void _avatar_request_item_by_id(const char* jid, avatar_metadata* data);
static int _avatar_metadata_handler(xmpp_stanza_t* const stanza, void* const userdata);
static int _avatar_request_item_result_handler(xmpp_stanza_t* const stanza, void* const userdata);

static void
_free_avatar_data(avatar_metadata* data)
{
    if (data) {
        g_free(data->type);
        g_free(data->id);
        g_free(data);
    }
}

static void
_avatar_cleanup(void)
{
    if (looking_for) {
        g_hash_table_destroy(looking_for);
    }
    if (shall_open) {
        g_hash_table_destroy(shall_open);
    }
    looking_for = NULL;
    shall_open = NULL;
}

void
avatar_pep_subscribe(void)
{
    prof_add_shutdown_routine(_avatar_cleanup);
    message_pubsub_event_handler_add(STANZA_NS_USER_AVATAR_METADATA, _avatar_metadata_handler, NULL, NULL);
    message_pubsub_event_handler_add(STANZA_NS_USER_AVATAR_DATA, _avatar_metadata_handler, NULL, NULL);

    // caps_add_feature(XMPP_FEATURE_USER_AVATAR_METADATA_NOTIFY);

    _avatar_cleanup();
    looking_for = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    shall_open = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

#ifdef HAVE_PIXBUF
gboolean
avatar_set(const gchar* path)
{
    auto_gchar gchar* expanded_path = get_expanded_path(path);

    auto_gerror GError* err = NULL;
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(expanded_path, &err);

    if (pixbuf == NULL) {
        cons_show_error("An error occurred while opening %s: %s.", expanded_path, PROF_GERROR_MESSAGE(err));
        return FALSE;
    }

    // Scale img
    int w = gdk_pixbuf_get_width(pixbuf);
    int h = gdk_pixbuf_get_height(pixbuf);

    if (w >= h && w > MAX_PIXEL) {
        int dest_height = (int)((float)MAX_PIXEL / w * h);
        GdkPixbuf* new_pixbuf = gdk_pixbuf_scale_simple(pixbuf, MAX_PIXEL, dest_height, GDK_INTERP_BILINEAR);
        g_object_unref(pixbuf);
        pixbuf = new_pixbuf;
    } else if (h > w && w > MAX_PIXEL) {
        int dest_width = (int)((float)MAX_PIXEL / h * w);
        GdkPixbuf* new_pixbuf = gdk_pixbuf_scale_simple(pixbuf, dest_width, MAX_PIXEL, GDK_INTERP_BILINEAR);
        g_object_unref(pixbuf);
        pixbuf = new_pixbuf;
    }

    auto_gchar gchar* img_data = NULL;
    gsize len = -1;

    if (!gdk_pixbuf_save_to_buffer(pixbuf, &img_data, &len, "png", &err, NULL)) {
        cons_show_error("Unable to scale and convert avatar: %s.", PROF_GERROR_MESSAGE(err));
        return FALSE;
    }

    xmpp_ctx_t* const ctx = connection_get_ctx();
    xmpp_stanza_t* iq = stanza_create_avatar_data_publish_iq(ctx, img_data, len);
    iq_send_stanza(iq);
    xmpp_stanza_release(iq);

    iq = stanza_create_avatar_metadata_publish_iq(ctx, img_data, len, gdk_pixbuf_get_height(pixbuf), gdk_pixbuf_get_width(pixbuf));
    g_object_unref(pixbuf);
    iq_send_stanza(iq);
    xmpp_stanza_release(iq);

    return TRUE;
}
#endif

gboolean
avatar_publishing_disable()
{
    xmpp_ctx_t* const ctx = connection_get_ctx();
    xmpp_stanza_t* iq = stanza_disable_avatar_publish_iq(ctx);
    iq_send_stanza(iq);
    xmpp_stanza_release(iq);

    return TRUE;
}

gboolean
avatar_get_by_nick(const gchar* nick, gboolean open)
{
    // in case we set the feature, remove it
    caps_remove_feature(XMPP_FEATURE_USER_AVATAR_METADATA_NOTIFY);

    // save nick in list so we look for this one
    g_hash_table_insert(looking_for, g_strdup(nick), NULL);

    // add the feature. this will trigger the _avatar_metadata_notfication_handler handler
    caps_add_feature(XMPP_FEATURE_USER_AVATAR_METADATA_NOTIFY);

    if (open) {
        g_hash_table_insert(shall_open, g_strdup(nick), NULL);
    }

    return TRUE;
}

static int
_avatar_metadata_handler(xmpp_stanza_t* const stanza, void* const userdata)
{
    const char* from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    if (!from) {
        return 1;
    }

    if (!g_hash_table_contains(looking_for, from)) {
        return 1;
    }

    xmpp_stanza_t* root = NULL;
    xmpp_stanza_t* event = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_PUBSUB_EVENT);
    if (event) {
        root = event;
    }

    xmpp_stanza_t* pubsub = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_PUBSUB);
    if (pubsub) {
        root = pubsub;
    }

    if (!root) {
        return 1;
    }

    xmpp_stanza_t* items = xmpp_stanza_get_child_by_name(root, "items");
    if (!items) {
        return 1;
    }

    xmpp_stanza_t* item = xmpp_stanza_get_child_by_name(items, "item");
    if (item) {
        xmpp_stanza_t* metadata = xmpp_stanza_get_child_by_name(item, "metadata");
        if (metadata) {

            xmpp_stanza_t* info = xmpp_stanza_get_child_by_name(metadata, "info");
            if (info) {

                const char* id = xmpp_stanza_get_id(info);
                const char* type = xmpp_stanza_get_attribute(info, "type");

                if (id && type) {
                    log_debug("Avatar ID for %s is: %s", from, id);

                    avatar_metadata* data = g_new0(avatar_metadata, 1);
                    if (data) {
                        data->type = g_strdup(type);
                        data->id = g_strdup(id);

                        // request the actual (image) data
                        _avatar_request_item_by_id(from, data);
                    }
                }
            } else {
                cons_show("We couldn't get the user's avatar, possibly because they haven't set one or have disabled avatar publishing. "
                          "During this Profanity session, you will receive future changes to this user's avatar.");
            }
        }
    }

    return 1;
}

static void
_avatar_request_item_by_id(const char* jid, avatar_metadata* data)
{
    caps_remove_feature(XMPP_FEATURE_USER_AVATAR_METADATA_NOTIFY);

    xmpp_ctx_t* const ctx = connection_get_ctx();

    auto_gchar gchar* uid = connection_create_stanza_id();

    xmpp_stanza_t* iq = stanza_create_avatar_retrieve_data_request(ctx, uid, data->id, jid);
    iq_id_handler_add(uid, _avatar_request_item_result_handler, (ProfIqFreeCallback)_free_avatar_data, data);

    iq_send_stanza(iq);
    xmpp_stanza_release(iq);
}

static int
_avatar_request_item_result_handler(xmpp_stanza_t* const stanza, void* const userdata)
{
    const char* from_attr = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

    if (!from_attr) {
        return 1;
    }

    if (!g_hash_table_contains(looking_for, from_attr)) {
        return 1;
    }
    g_hash_table_remove(looking_for, from_attr);

    xmpp_stanza_t* pubsub = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_PUBSUB);
    if (!pubsub) {
        return 1;
    }

    xmpp_stanza_t* items = xmpp_stanza_get_child_by_name(pubsub, "items");
    if (!items) {
        return 1;
    }

    xmpp_stanza_t* item = xmpp_stanza_get_child_by_name(items, "item");
    if (!item) {
        return 1;
    }

    xmpp_stanza_t* st_data = xmpp_stanza_get_child_by_name_and_ns(item, "data", STANZA_NS_USER_AVATAR_DATA);
    if (!st_data) {
        return 1;
    }

    auto_char char* buf = xmpp_stanza_get_text(st_data);
    if (!buf) {
        return 1;
    }

    gsize size;
    auto_gchar gchar* de = (gchar*)g_base64_decode(buf, &size);

    auto_gchar gchar* path = files_get_data_path("");
    GString* filename = g_string_new(path);

    g_string_append(filename, "avatars/");

    errno = 0;
    int res = g_mkdir_with_parents(filename->str, S_IRWXU);
    if (res == -1) {
        const char* errmsg = strerror(errno);
        if (errmsg) {
            log_error("Avatar: error creating directory: %s, %s", filename->str, errmsg);
        } else {
            log_error("Avatar: creating directory: %s", filename->str);
        }
    }

    GString* from = g_string_new(from_attr);
    g_string_replace(from, "@", "_at_", 0);
    g_string_append(filename, from->str);
    g_string_free(from, TRUE);

    avatar_metadata* data = (avatar_metadata*)userdata;

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

    auto_gerror GError* err = NULL;
    if (g_file_set_contents(filename->str, de, size, &err) == FALSE) {
        log_error("Unable to save picture: %s", err->message);
        cons_show("Unable to save picture %s", err->message);
    } else {
        cons_show("Avatar saved as %s", filename->str);
    }

    // if we shall open it
    if (g_hash_table_contains(shall_open, from_attr)) {
        auto_gchar gchar* cmdtemplate = prefs_get_string(PREF_AVATAR_CMD);

        if (cmdtemplate == NULL) {
            cons_show_error("No default `avatar open` command found in executables preferences.");
        } else {
            auto_gcharv gchar** argv = format_call_external_argv(cmdtemplate, NULL, filename->str);

            if (!call_external(argv)) {
                cons_show_error("Unable to display avatar: check the logs for more information.");
            }
        }

        g_hash_table_remove(shall_open, from_attr);
    }

    g_string_free(filename, TRUE);

    return 1;
}

gchar*
avatar_get_path(const gchar* const jid)
{
    if (!jid) {
        return NULL;
    }

    auto_jid Jid* jidp = jid_create(jid);
    const gchar* target_jid = (jidp && jidp->barejid) ? jidp->barejid : jid;
    auto_gchar gchar* sanitized = files_sanitize_jid(target_jid);
    const gchar* dirs[] = { "avatars", DIR_PHOTOS, NULL };
    const gchar* extensions[] = { ".png", ".jpeg", ".jpg", ".webp", "", NULL };

    for (int d = 0; dirs[d] != NULL; d++) {
        auto_gchar gchar* base_path = files_get_data_path(dirs[d]);
        for (int i = 0; extensions[i] != NULL; i++) {
            gchar* full_path = g_strdup_printf("%s/%s%s", base_path, sanitized, extensions[i]);
            if (g_file_test(full_path, G_FILE_TEST_EXISTS)) {
                return full_path;
            }
            g_free(full_path);
        }
    }

    return NULL;
}
