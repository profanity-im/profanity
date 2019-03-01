#include <glib.h>

#include "xmpp/connection.h"
#include "xmpp/message.h"
#include "xmpp/iq.h"
#include "xmpp/stanza.h"

#include "omemo/omemo.h"

static int _omemo_receive_devicelist(xmpp_stanza_t *const stanza, void *const userdata);

void
omemo_devicelist_subscribe(void)
{
    message_pubsub_event_handler_add(STANZA_NS_OMEMO_DEVICELIST, _omemo_receive_devicelist, NULL, NULL);

    caps_add_feature(XMPP_FEATURE_OMEMO_DEVICELIST_NOTIFY);
}

void
omemo_devicelist_publish(GList *device_list)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *iq = stanza_create_omemo_devicelist_publish(ctx, device_list);
    iq_send_stanza(iq);
    xmpp_stanza_release(iq);
}

void
omemo_devicelist_request(const char * const jid)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    char *id = connection_create_stanza_id("devicelist_request");

    xmpp_stanza_t *iq = stanza_create_omemo_devicelist_request(ctx, id, jid);
    iq_id_handler_add(id, _omemo_receive_devicelist, NULL, NULL);

    iq_send_stanza(iq);

    free(id);
    xmpp_stanza_release(iq);
}

void
omemo_bundle_publish(void)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    unsigned char *identity_key = NULL;
    size_t identity_key_length;
    unsigned char *signed_prekey = NULL;
    size_t signed_prekey_length;
    unsigned char *signed_prekey_signature = NULL;
    size_t signed_prekey_signature_length;
    GList *prekeys = NULL, *ids = NULL, *lengths = NULL;

    omemo_identity_key(&identity_key, &identity_key_length);
    omemo_signed_prekey(&signed_prekey, &signed_prekey_length);
    omemo_signed_prekey_signature(&signed_prekey_signature, &signed_prekey_signature_length);
    omemo_prekeys(&prekeys, &ids, &lengths);

    xmpp_stanza_t *iq = stanza_create_omemo_bundle_publish(ctx, omemo_device_id(),
        identity_key, identity_key_length, signed_prekey, signed_prekey_length,
        signed_prekey_signature,  signed_prekey_signature_length,
        prekeys, ids, lengths);
    iq_send_stanza(iq);
    xmpp_stanza_release(iq);

    free(identity_key);
    free(signed_prekey);
    free(signed_prekey_signature);
}

void
omemo_bundle_request(const char * const jid, uint32_t device_id, ProfIqCallback func, ProfIqFreeCallback free_func, void *userdata)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    char *id = connection_create_stanza_id("devicelist_request");

    xmpp_stanza_t *iq = stanza_create_omemo_bundle_request(ctx, id, jid, device_id);
    iq_id_handler_add(id, func, free_func, userdata);

    iq_send_stanza(iq);

    free(id);
    xmpp_stanza_release(iq);
}

int
omemo_start_device_session_handle_bundle(xmpp_stanza_t *const stanza, void *const userdata)
{
    const char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    if (!from) {
        return 1;
    }

    if (g_strcmp0(from, userdata) != 0) {
        return 1;
    }

    xmpp_stanza_t *pubsub = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_PUBSUB);
    if (!pubsub) {
        return 1;
    }

    xmpp_stanza_t *items = xmpp_stanza_get_child_by_name(pubsub, "items");
    if (!items) {
        return 1;
    }
    const char *node = xmpp_stanza_get_attribute(items, "node");
    char *device_id_str = strstr(node, ":");
    if (!device_id_str) {
        return 1;
    }

    uint32_t device_id = strtoul(++device_id_str, NULL, 10);

    xmpp_stanza_t *item = xmpp_stanza_get_child_by_name(items, "item");
    if (!item) {
        return 1;
    }

    xmpp_stanza_t *bundle = xmpp_stanza_get_child_by_ns(item, STANZA_NS_OMEMO);
    if (!bundle) {
        return 1;
    }

    xmpp_stanza_t *prekeys = xmpp_stanza_get_child_by_name(bundle, "prekeys");
    if (!prekeys) {
        return 1;
    }

    /* Should be random */
    xmpp_stanza_t *prekey = xmpp_stanza_get_children(prekeys);
    if (!prekey) {
        return 1;
    }
    const char *prekey_id_text = xmpp_stanza_get_attribute(prekey, "preKeyId");
    if (!prekey_id_text) {
        return 1;
    }
    uint32_t prekey_id = strtoul(prekey_id_text, NULL, 10);
    xmpp_stanza_t *prekey_text = xmpp_stanza_get_children(prekey);
    if (!prekey_text) {
        return 1;
    }
    size_t prekey_len;
    unsigned char *prekey_raw = g_base64_decode(xmpp_stanza_get_text(prekey_text), &prekey_len);

    xmpp_stanza_t *signed_prekey = xmpp_stanza_get_child_by_name(bundle, "signedPreKeyPublic");
    if (!signed_prekey) {
        return 1;
    }
    const char *signed_prekey_id_text = xmpp_stanza_get_attribute(signed_prekey, "signedPreKeyId");
    if (!signed_prekey_id_text) {
        return 1;
    }
    uint32_t signed_prekey_id = strtoul(signed_prekey_id_text, NULL, 10);
    xmpp_stanza_t *signed_prekey_text = xmpp_stanza_get_children(signed_prekey);
    if (!signed_prekey_text) {
        return 1;
    }
    size_t signed_prekey_len;
    unsigned char *signed_prekey_raw = g_base64_decode(xmpp_stanza_get_text(signed_prekey_text), &signed_prekey_len);

    xmpp_stanza_t *signed_prekey_signature = xmpp_stanza_get_child_by_name(bundle, "signedPreKeySignature");
    if (!signed_prekey_signature) {
        return 1;
    }
    xmpp_stanza_t *signed_prekey_signature_text = xmpp_stanza_get_children(signed_prekey_signature);
    if (!signed_prekey_signature_text) {
        return 1;
    }
    size_t signed_prekey_signature_len;
    unsigned char *signed_prekey_signature_raw = g_base64_decode(xmpp_stanza_get_text(signed_prekey_signature_text), &signed_prekey_signature_len);

    xmpp_stanza_t *identity_key = xmpp_stanza_get_child_by_name(bundle, "identityKey");
    if (!identity_key) {
        return 1;
    }
    xmpp_stanza_t *identity_key_text = xmpp_stanza_get_children(identity_key);
    if (!identity_key_text) {
        return 1;
    }
    size_t identity_key_len;
    unsigned char *identity_key_raw = g_base64_decode(xmpp_stanza_get_text(identity_key_text), &identity_key_len);

    omemo_start_device_session(from, device_id, prekey_id, prekey_raw,
        prekey_len, signed_prekey_id, signed_prekey_raw, signed_prekey_len,
        signed_prekey_signature_raw, signed_prekey_signature_len,
        identity_key_raw, identity_key_len);
    return 1;
}

char *
omemo_receive_message(xmpp_stanza_t *const stanza)
{
    xmpp_stanza_t *encrypted = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_OMEMO);
    if (!encrypted) {
        return NULL;
    }

    xmpp_stanza_t *header = xmpp_stanza_get_child_by_name(encrypted, "header");
    if (!header) {
        return NULL;
    }

    const char *sid_text = xmpp_stanza_get_attribute(header, "sid");
    if (!sid_text) {
        return NULL;
    }
    uint32_t sid = strtoul(sid_text, NULL, 10);

    xmpp_stanza_t *iv = xmpp_stanza_get_child_by_name(header, "iv");
    if (!iv) {
        return NULL;
    }
    const char *iv_text = xmpp_stanza_get_text(iv);
    if (!iv_text) {
        return NULL;
    }
    size_t iv_len;
    const unsigned char *iv_raw = g_base64_decode(iv_text, &iv_len);

    xmpp_stanza_t *payload = xmpp_stanza_get_child_by_name(encrypted, "payload");
    if (!payload) {
        return NULL;
    }
    const char *payload_text = xmpp_stanza_get_text(payload);
    if (!payload_text) {
        return NULL;
    }
    size_t payload_len;
    const unsigned char *payload_raw = g_base64_decode(payload_text, &payload_len);

    GList *keys = NULL;
    xmpp_stanza_t *key_stanza;
    for (key_stanza = xmpp_stanza_get_children(header); key_stanza != NULL; key_stanza = xmpp_stanza_get_next(key_stanza)) {
        if (g_strcmp0(xmpp_stanza_get_name(key_stanza), "key") != 0) {
            continue;
        }

        omemo_key_t *key = malloc(sizeof(omemo_key_t));
        const char *key_text = xmpp_stanza_get_text(key_stanza);
        if (!key_text) {
            goto skip;
        }


        const char *rid_text = xmpp_stanza_get_attribute(key_stanza, "rid");
        key->device_id = strtoul(rid_text, NULL, 10);
        if (!key->device_id) {
            goto skip;
        }
        key->data = g_base64_decode(key_text, &key->length);
        key->prekey = g_strcmp0(xmpp_stanza_get_attribute(key_stanza, "prekey"), "true") == 0;
        keys = g_list_append(keys, key);
        continue;

skip:
        free(key);
    }

    const char *from = xmpp_stanza_get_from(stanza);
    Jid *jid = jid_create(from);

    char *plaintext = omemo_on_message_recv(jid->barejid, sid, iv_raw, iv_len, keys, payload_raw, payload_len);

    jid_destroy(jid);

    return plaintext;
}

static int
_omemo_receive_devicelist(xmpp_stanza_t *const stanza, void *const userdata)
{
    GList *device_list = NULL;
    const char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    if (!from) {
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
    if (!item) {
        return 1;
    }

    xmpp_stanza_t *list = xmpp_stanza_get_child_by_ns(item, STANZA_NS_OMEMO);
    if (!list) {
        return 1;
    }

    xmpp_stanza_t *device;
    for (device = xmpp_stanza_get_children(list); device != NULL; device = xmpp_stanza_get_next(device)) {
        const char *id = xmpp_stanza_get_id(device);
        device_list = g_list_append(device_list, GINT_TO_POINTER(strtoul(id, NULL, 10)));
    }
    omemo_set_device_list(from, device_list);

    return 1;
}
