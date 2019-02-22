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
omemo_devicelist_fetch(void)
{
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
omemo_bundles_fetch(const char * const jid)
{
}

static int
_omemo_receive_devicelist(xmpp_stanza_t *const stanza, void *const userdata)
{
    GList *device_list = NULL;
    const char *from = xmpp_stanza_get_attribute(stanza, STANZA_ATTR_FROM);
    if (!from) {
        return 1;
    }

    xmpp_stanza_t *event = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_PUBSUB_EVENT);
    if (!event) {
        return 1;
    }

    xmpp_stanza_t *items = xmpp_stanza_get_child_by_name(event, "items");
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
