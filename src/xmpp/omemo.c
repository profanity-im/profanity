#include "xmpp/connection.h"
#include "xmpp/iq.h"
#include "xmpp/stanza.h"

#include "omemo/omemo.h"

void
omemo_devicelist_subscribe(void)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    char *barejid = xmpp_jid_bare(ctx, session_get_account_name());
    xmpp_stanza_t *iq = stanza_create_omemo_devicelist_subscribe(ctx, barejid);
    iq_send_stanza(iq);
    xmpp_stanza_release(iq);

    free(barejid);
}

void
omemo_devicelist_publish(void)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *iq = stanza_create_omemo_devicelist_publish(ctx, omemo_device_list());
    iq_send_stanza(iq);
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
