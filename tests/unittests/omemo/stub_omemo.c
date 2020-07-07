#include <glib.h>

#include "config/account.h"
#include "ui/ui.h"

void
omemo_init(void)
{
}
void
omemo_close(void)
{
}

char*
omemo_fingerprint_autocomplete(const char* const search_str, gboolean previous)
{
    return NULL;
}

void
omemo_fingerprint_autocomplete_reset(void)
{
}

char*
omemo_format_fingerprint(const char* const fingerprint)
{
    return NULL;
}

void
omemo_generate_crypto_materials(ProfAccount* account)
{
}

gboolean
omemo_automatic_start(const char* const jid)
{
    return TRUE;
}

gboolean
omemo_is_trusted_identity(const char* const jid, const char* const fingerprint)
{
    return TRUE;
}

GList*
omemo_known_device_identities(const char* const jid)
{
    return NULL;
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

char*
omemo_on_message_send(ProfWin* win, const char* const message, gboolean request_receipt, gboolean muc)
{
    return NULL;
}

char*
omemo_own_fingerprint(gboolean formatted)
{
    return NULL;
}

void
omemo_start_muc_sessions(const char* const roomjid)
{
}
void
omemo_start_session(const char* const barejid)
{
}
void
omemo_trust(const char* const jid, const char* const fingerprint_formatted)
{
}
void
omemo_untrust(const char* const jid, const char* const fingerprint_formatted)
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
