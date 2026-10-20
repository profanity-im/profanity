#include <glib.h>

#include "config/account.h"
#include "ui/ui.h"

void
omemo_init(void)
{
}

gchar*
omemo_fingerprint_autocomplete(const gchar* const search_str, gboolean previous, void* context)
{
    return NULL;
}

void
omemo_fingerprint_autocomplete_reset(void)
{
}

gchar*
omemo_format_fingerprint(const gchar* const fingerprint)
{
    return NULL;
}

void
omemo_generate_crypto_materials(ProfAccount* account)
{
}

gboolean
omemo_automatic_start(const gchar* const jid)
{
    return TRUE;
}

gboolean
omemo_is_trusted_identity(const gchar* const jid, const gchar* const fingerprint)
{
    return TRUE;
}

gboolean
omemo_is_jid_trusted(const gchar* const jid)
{
    return TRUE;
}

GList*
omemo_get_jid_untrusted_fingerprints(const gchar* const jid)
{
    return NULL;
}

GList*
omemo_known_device_identities(const gchar* const jid)
{
    return NULL;
}

gboolean
omemo_is_device_active(const gchar* const jid, const gchar* const fingerprint)
{
    return TRUE;
}

gboolean
omemo_loaded(void)
{
    return TRUE;
}

void
omemo_on_connect(ProfAccount* account)
{
}
void
omemo_on_disconnect(void)
{
}

gchar*
omemo_on_message_send(ProfWin* win, const gchar* const message, gboolean request_receipt, gboolean muc, const gchar* const replace_id)
{
    return NULL;
}

gchar*
omemo_own_fingerprint(gboolean formatted)
{
    return NULL;
}

void
omemo_start_muc_sessions(const gchar* const roomjid)
{
}
void
omemo_start_session(const gchar* const barejid)
{
}
void
omemo_trust(const gchar* const jid, const gchar* const fingerprint_formatted)
{
}
void
omemo_untrust(const gchar* const jid, const gchar* const fingerprint_formatted)
{
}
void
omemo_devicelist_publish(GList* device_list)
{
}
void
omemo_publish_crypto_materials(void)
{
}
void
omemo_start_sessions(void)
{
}

char*
omemo_encrypt_file(FILE* in, FILE* out, off_t file_size, int* gcry_res)
{
    return NULL;
};
void omemo_free(void* a) {};

char*
omemo_on_message_recv(const gchar* const from, uint32_t sid,
                      const unsigned char* const iv, size_t iv_len,
                      GList* keys, const unsigned char* const payload,
                      size_t payload_len, gboolean muc, gboolean* trusted, omemo_error_t* error)
{
    *error = OMEMO_ERR_NONE;
    return NULL;
}

char*
omemo_receive_message(xmpp_stanza_t* const stanza, gboolean* trusted, omemo_error_t* error)
{
    *error = OMEMO_ERR_NONE;
    return NULL;
}

uint32_t
omemo_device_id()
{
    return 123;
}

gchar*
omemo_qrcode_str()
{
    return NULL;
}
