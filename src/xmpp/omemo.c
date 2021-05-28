/*
 * omemo.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 Paul Fariello <paul@fariello.eu>
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

#include "config.h"

#include <glib.h>

#include "log.h"
#include "xmpp/connection.h"
#include "xmpp/form.h"
#include "xmpp/iq.h"
#include "xmpp/message.h"
#include "xmpp/stanza.h"

#include "omemo/omemo.h"

static int _omemo_receive_devicelist(xmpp_stanza_t* const stanza, void* const userdata);
static int _omemo_bundle_publish_result(xmpp_stanza_t* const stanza, void* const userdata);
static int _omemo_bundle_publish_configure(xmpp_stanza_t* const stanza, void* const userdata);
static int _omemo_bundle_publish_configure_result(xmpp_stanza_t* const stanza, void* const userdata);

void
omemo_devicelist_subscribe(void)
{
    message_pubsub_event_handler_add(STANZA_NS_OMEMO_DEVICELIST, _omemo_receive_devicelist, NULL, NULL);

    caps_add_feature(XMPP_FEATURE_OMEMO_DEVICELIST_NOTIFY);
}

void
omemo_devicelist_publish(GList* device_list)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();
    xmpp_stanza_t* iq = stanza_create_omemo_devicelist_publish(ctx, device_list);

    log_debug("[OMEMO] publish device list");

    if (connection_supports(XMPP_FEATURE_PUBSUB_PUBLISH_OPTIONS)) {
        stanza_attach_publish_options(ctx, iq, "pubsub#access_model", "open");
    }

    iq_send_stanza(iq);
    xmpp_stanza_release(iq);
}

void
omemo_devicelist_request(const char* const jid)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();
    char* id = connection_create_stanza_id();

    log_debug("[OMEMO] request device list for jid: %s", jid);

    xmpp_stanza_t* iq = stanza_create_omemo_devicelist_request(ctx, id, jid);
    iq_id_handler_add(id, _omemo_receive_devicelist, NULL, NULL);

    iq_send_stanza(iq);

    free(id);
    xmpp_stanza_release(iq);
}

void
omemo_bundle_publish(gboolean first)
{
    log_debug("[OMEMO] publish own OMEMO bundle");
    xmpp_ctx_t* const ctx = connection_get_ctx();
    unsigned char* identity_key = NULL;
    size_t identity_key_length;
    unsigned char* signed_prekey = NULL;
    size_t signed_prekey_length;
    unsigned char* signed_prekey_signature = NULL;
    size_t signed_prekey_signature_length;
    GList *prekeys = NULL, *ids = NULL, *lengths = NULL;

    omemo_identity_key(&identity_key, &identity_key_length);
    omemo_signed_prekey(&signed_prekey, &signed_prekey_length);
    omemo_signed_prekey_signature(&signed_prekey_signature, &signed_prekey_signature_length);
    omemo_prekeys(&prekeys, &ids, &lengths);

    char* id = connection_create_stanza_id();
    xmpp_stanza_t* iq = stanza_create_omemo_bundle_publish(ctx, id,
                                                           omemo_device_id(), identity_key, identity_key_length, signed_prekey,
                                                           signed_prekey_length, signed_prekey_signature,
                                                           signed_prekey_signature_length, prekeys, ids, lengths);

    g_list_free_full(prekeys, free);
    g_list_free(lengths);
    g_list_free(ids);

    if (connection_supports(XMPP_FEATURE_PUBSUB_PUBLISH_OPTIONS)) {
        stanza_attach_publish_options_va(ctx, iq,
                4, // 2 * number of key-value pairs
                "pubsub#persist_items", "true",
                "pubsub#access_model", "open");
    }

    iq_id_handler_add(id, _omemo_bundle_publish_result, NULL, GINT_TO_POINTER(first));

    iq_send_stanza(iq);

    xmpp_stanza_release(iq);
    free(identity_key);
    free(signed_prekey);
    free(signed_prekey_signature);
    free(id);
}

void
omemo_bundle_request(const char* const jid, uint32_t device_id, ProfIqCallback func, ProfIqFreeCallback free_func, void* userdata)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();
    char* id = connection_create_stanza_id();

    log_debug("[OMEMO] request omemo bundle (jid: %s, device: %d)", jid, device_id);

    xmpp_stanza_t* iq = stanza_create_omemo_bundle_request(ctx, id, jid, device_id);
    iq_id_handler_add(id, func, free_func, userdata);

    iq_send_stanza(iq);

    free(id);
    xmpp_stanza_release(iq);
}

int
omemo_start_device_session_handle_bundle(xmpp_stanza_t* const stanza, void* const userdata)
{
    GList* prekeys_list = NULL;
    unsigned char* signed_prekey_raw = NULL;
    unsigned char* signed_prekey_signature_raw = NULL;
    char* from = NULL;

    const char* from_attr = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    log_debug("[OMEMO] omemo_start_device_session_handle_bundle: %s", from_attr);

    const char* type = xmpp_stanza_get_type(stanza);
    if ( g_strcmp0( type, "error") == 0 ) {
        log_error("[OMEMO] Error to get key for a device from : %s", from_attr);
    }

    if (!from_attr) {
        Jid* jid = jid_create(connection_get_fulljid());
        from = strdup(jid->barejid);
        jid_destroy(jid);
    } else {
        from = strdup(from_attr);
    }

    if (g_strcmp0(from, userdata) != 0) {
        goto out;
    }

    xmpp_stanza_t* pubsub = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_PUBSUB);
    if (!pubsub) {
        goto out;
    }

    xmpp_stanza_t* items = xmpp_stanza_get_child_by_name(pubsub, "items");
    if (!items) {
        goto out;
    }
    const char* node = xmpp_stanza_get_attribute(items, "node");
    char* device_id_str = strstr(node, ":");
    if (!device_id_str) {
        goto out;
    }

    uint32_t device_id = strtoul(++device_id_str, NULL, 10);
    log_debug("[OMEMO] omemo_start_device_session_handle_bundle: %d", device_id);

    xmpp_stanza_t* item = xmpp_stanza_get_child_by_name(items, "item");
    if (!item) {
        goto out;
    }

    xmpp_stanza_t* bundle = xmpp_stanza_get_child_by_ns(item, STANZA_NS_OMEMO);
    if (!bundle) {
        goto out;
    }

    xmpp_stanza_t* prekeys = xmpp_stanza_get_child_by_name(bundle, "prekeys");
    if (!prekeys) {
        goto out;
    }

    xmpp_stanza_t* prekey;
    for (prekey = xmpp_stanza_get_children(prekeys); prekey != NULL; prekey = xmpp_stanza_get_next(prekey)) {
        if (g_strcmp0(xmpp_stanza_get_name(prekey), "preKeyPublic") != 0) {
            continue;
        }

        omemo_key_t* key = malloc(sizeof(omemo_key_t));
        key->data = NULL;

        const char* prekey_id_text = xmpp_stanza_get_attribute(prekey, "preKeyId");
        if (!prekey_id_text) {
            omemo_key_free(key);
            continue;
        }

        key->id = strtoul(prekey_id_text, NULL, 10);
        xmpp_stanza_t* prekey_text = xmpp_stanza_get_children(prekey);

        if (!prekey_text) {
            omemo_key_free(key);
            continue;
        }

        char* prekey_b64 = xmpp_stanza_get_text(prekey_text);
        key->data = g_base64_decode(prekey_b64, &key->length);
        free(prekey_b64);
        if (!key->data) {
            omemo_key_free(key);
            continue;
        }
        key->prekey = TRUE;
        key->device_id = device_id;

        prekeys_list = g_list_append(prekeys_list, key);
    }

    xmpp_stanza_t* signed_prekey = xmpp_stanza_get_child_by_name(bundle, "signedPreKeyPublic");
    if (!signed_prekey) {
        goto out;
    }
    const char* signed_prekey_id_text = xmpp_stanza_get_attribute(signed_prekey, "signedPreKeyId");
    if (!signed_prekey_id_text) {
        goto out;
    }

    uint32_t signed_prekey_id = strtoul(signed_prekey_id_text, NULL, 10);
    xmpp_stanza_t* signed_prekey_text = xmpp_stanza_get_children(signed_prekey);
    if (!signed_prekey_text) {
        goto out;
    }

    size_t signed_prekey_len;
    char* signed_prekey_b64 = xmpp_stanza_get_text(signed_prekey_text);
    signed_prekey_raw = g_base64_decode(signed_prekey_b64, &signed_prekey_len);
    free(signed_prekey_b64);
    if (!signed_prekey_raw) {
        goto out;
    }

    xmpp_stanza_t* signed_prekey_signature = xmpp_stanza_get_child_by_name(bundle, "signedPreKeySignature");
    if (!signed_prekey_signature) {
        goto out;
    }
    xmpp_stanza_t* signed_prekey_signature_text = xmpp_stanza_get_children(signed_prekey_signature);
    if (!signed_prekey_signature_text) {
        goto out;
    }
    size_t signed_prekey_signature_len;
    char* signed_prekey_signature_b64 = xmpp_stanza_get_text(signed_prekey_signature_text);
    signed_prekey_signature_raw = g_base64_decode(signed_prekey_signature_b64, &signed_prekey_signature_len);
    free(signed_prekey_signature_b64);
    if (!signed_prekey_signature_raw) {
        goto out;
    }

    xmpp_stanza_t* identity_key = xmpp_stanza_get_child_by_name(bundle, "identityKey");
    if (!identity_key) {
        goto out;
    }
    xmpp_stanza_t* identity_key_text = xmpp_stanza_get_children(identity_key);
    if (!identity_key_text) {
        goto out;
    }
    size_t identity_key_len;
    char* identity_key_b64 = xmpp_stanza_get_text(identity_key_text);
    unsigned char* identity_key_raw = g_base64_decode(identity_key_b64, &identity_key_len);
    free(identity_key_b64);
    if (!identity_key_raw) {
        goto out;
    }

    omemo_start_device_session(from, device_id, prekeys_list, signed_prekey_id,
                               signed_prekey_raw, signed_prekey_len, signed_prekey_signature_raw,
                               signed_prekey_signature_len, identity_key_raw, identity_key_len);

    g_free(identity_key_raw);

out:
    free(from);
    if (signed_prekey_raw) {
        g_free(signed_prekey_raw);
    }
    if (signed_prekey_signature_raw) {
        g_free(signed_prekey_signature_raw);
    }
    if (prekeys_list) {
        g_list_free_full(prekeys_list, (GDestroyNotify)omemo_key_free);
    }
    return 1;
}

char*
omemo_receive_message(xmpp_stanza_t* const stanza, gboolean* trusted)
{
    char* plaintext = NULL;
    const char* type = xmpp_stanza_get_type(stanza);
    GList* keys = NULL;
    unsigned char* iv_raw = NULL;
    unsigned char* payload_raw = NULL;
    char* iv_text = NULL;
    char* payload_text = NULL;

    xmpp_stanza_t* encrypted = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_OMEMO);
    if (!encrypted) {
        return NULL;
    }

    xmpp_stanza_t* header = xmpp_stanza_get_child_by_name(encrypted, "header");
    if (!header) {
        return NULL;
    }

    const char* sid_text = xmpp_stanza_get_attribute(header, "sid");
    if (!sid_text) {
        return NULL;
    }
    uint32_t sid = strtoul(sid_text, NULL, 10);

    xmpp_stanza_t* iv = xmpp_stanza_get_child_by_name(header, "iv");
    if (!iv) {
        return NULL;
    }
    iv_text = xmpp_stanza_get_text(iv);
    if (!iv_text) {
        return NULL;
    }
    size_t iv_len;
    iv_raw = g_base64_decode(iv_text, &iv_len);
    if (!iv_raw) {
        goto out;
    }

    xmpp_stanza_t* payload = xmpp_stanza_get_child_by_name(encrypted, "payload");
    if (!payload) {
        goto out;
    }
    payload_text = xmpp_stanza_get_text(payload);
    if (!payload_text) {
        goto out;
    }
    size_t payload_len;
    payload_raw = g_base64_decode(payload_text, &payload_len);
    if (!payload_raw) {
        goto out;
    }

    xmpp_stanza_t* key_stanza;
    for (key_stanza = xmpp_stanza_get_children(header); key_stanza != NULL; key_stanza = xmpp_stanza_get_next(key_stanza)) {
        if (g_strcmp0(xmpp_stanza_get_name(key_stanza), "key") != 0) {
            continue;
        }

        omemo_key_t* key = malloc(sizeof(omemo_key_t));
        char* key_text = xmpp_stanza_get_text(key_stanza);
        if (!key_text) {
            goto skip;
        }

        const char* rid_text = xmpp_stanza_get_attribute(key_stanza, "rid");
        key->device_id = strtoul(rid_text, NULL, 10);
        if (!key->device_id) {
            goto skip;
        }

        key->data = g_base64_decode(key_text, &key->length);
        free(key_text);
        if (!key->data) {
            goto skip;
        }
        key->prekey = g_strcmp0(xmpp_stanza_get_attribute(key_stanza, "prekey"), "true") == 0
                      || g_strcmp0(xmpp_stanza_get_attribute(key_stanza, "prekey"), "1") == 0;
        keys = g_list_append(keys, key);
        continue;

    skip:
        free(key);
    }

    const char* from = xmpp_stanza_get_from(stanza);

    plaintext = omemo_on_message_recv(from, sid, iv_raw, iv_len,
                                      keys, payload_raw, payload_len,
                                      g_strcmp0(type, STANZA_TYPE_GROUPCHAT) == 0, trusted);

out:
    if (keys) {
        g_list_free_full(keys, (GDestroyNotify)omemo_key_free);
    }

    g_free(iv_raw);
    g_free(payload_raw);
    g_free(iv_text);
    g_free(payload_text);

    return plaintext;
}

static int
_omemo_receive_devicelist(xmpp_stanza_t* const stanza, void* const userdata)
{
    GList* device_list = NULL;
    const char* from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);

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

    // Looking for "current" item - if there is no current, take the first item.
    xmpp_stanza_t* first = NULL;
    xmpp_stanza_t* current = NULL;

    xmpp_stanza_t* item = xmpp_stanza_get_children(items);
    while (item) {
        if (g_strcmp0(xmpp_stanza_get_name(item), "item") == 0) {
            first = item;
            if (g_strcmp0(xmpp_stanza_get_id(item), "current") == 0) {
                current = item;
                break;
            }
        }
        item = xmpp_stanza_get_next(item);
    }

    if (current) {
        item = current;
    } else if (first) {
        log_warning("[OMEMO] User %s has a non 'current' device item list: %s.", from, xmpp_stanza_get_id(first));
        item = first;
    } else {
        return 1;
    }

    xmpp_stanza_t* list = xmpp_stanza_get_child_by_ns(item, STANZA_NS_OMEMO);
    if (!list) {
        return 1;
    }

    xmpp_stanza_t* device;
    for (device = xmpp_stanza_get_children(list); device != NULL; device = xmpp_stanza_get_next(device)) {
        if (g_strcmp0(xmpp_stanza_get_name(device), "device") != 0) {
            continue;
        }

        const char* id = xmpp_stanza_get_id(device);
        if (id != NULL) {
            device_list = g_list_append(device_list, GINT_TO_POINTER(strtoul(id, NULL, 10)));
        } else {
            log_error("[OMEMO] received device without ID");
        }
    }

    omemo_set_device_list(from, device_list);

    return 1;
}


static int
_omemo_bundle_publish_result(xmpp_stanza_t* const stanza, void* const userdata)
{
    log_debug("[OMEMO] _omemo_bundle_publish_result()");

    const char* type = xmpp_stanza_get_type(stanza);

    if (g_strcmp0(type, STANZA_TYPE_RESULT) == 0) {
        log_debug("[OMEMO] bundle published successfully");
        return 0;
    }

    if (!GPOINTER_TO_INT(userdata)) {
        log_error("[OMEMO] definitely cannot publish bundle with an open access model");
        return 0;
    }

    log_debug("[OMEMO] cannot publish bundle with open access model, trying to configure node");
    xmpp_ctx_t* const ctx = connection_get_ctx();
    Jid* jid = jid_create(connection_get_fulljid());
    char* id = connection_create_stanza_id();
    char* node = g_strdup_printf("%s:%d", STANZA_NS_OMEMO_BUNDLES, omemo_device_id());
    log_debug("[OMEMO] node: %s", node);

    xmpp_stanza_t* iq = stanza_create_pubsub_configure_request(ctx, id, jid->barejid, node);
    g_free(node);

    iq_id_handler_add(id, _omemo_bundle_publish_configure, NULL, userdata);

    iq_send_stanza(iq);

    xmpp_stanza_release(iq);
    free(id);
    jid_destroy(jid);
    return 0;
}

static int
_omemo_bundle_publish_configure(xmpp_stanza_t* const stanza, void* const userdata)
{
    log_debug("[OMEMO] _omemo_bundle_publish_configure()");

    xmpp_stanza_t* pubsub = xmpp_stanza_get_child_by_name(stanza, "pubsub");
    if (!pubsub) {
        log_error("[OMEMO] The stanza doesn't contain a 'pubsub' child");
        return 0;
    }
    xmpp_stanza_t* configure = xmpp_stanza_get_child_by_name(pubsub, STANZA_NAME_CONFIGURE);
    if (!configure) {
        log_error("[OMEMO] The stanza doesn't contain a 'configure' child");
        return 0;
    }
    xmpp_stanza_t* x = xmpp_stanza_get_child_by_name(configure, "x");
    if (!x) {
        log_error("[OMEMO] The stanza doesn't contain an 'x' child");
        return 0;
    }

    DataForm* form = form_create(x);
    char* tag = g_hash_table_lookup(form->var_to_tag, "pubsub#access_model");
    if (!tag) {
        log_error("[OMEMO] cannot configure bundle to an open access model");
        return 0;
    }

    xmpp_ctx_t* const ctx = connection_get_ctx();
    Jid* jid = jid_create(connection_get_fulljid());
    char* id = connection_create_stanza_id();
    char* node = g_strdup_printf("%s:%d", STANZA_NS_OMEMO_BUNDLES, omemo_device_id());
    xmpp_stanza_t* iq = stanza_create_pubsub_configure_submit(ctx, id, jid->barejid, node, form);
    g_free(node);

    iq_id_handler_add(id, _omemo_bundle_publish_configure_result, NULL, userdata);

    iq_send_stanza(iq);

    xmpp_stanza_release(iq);
    free(id);
    jid_destroy(jid);
    return 0;
}

static int
_omemo_bundle_publish_configure_result(xmpp_stanza_t* const stanza, void* const userdata)
{
    const char* type = xmpp_stanza_get_type(stanza);

    if (g_strcmp0(type, STANZA_TYPE_ERROR) == 0) {
        log_error("[OMEMO] cannot configure bundle to an open access model: Result error");
        return 0;
    }

    log_debug("[OMEMO] node configured");

    // Try to publish again
    omemo_bundle_publish(TRUE);

    return 0;
}
