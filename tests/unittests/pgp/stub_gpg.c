#include <glib.h>

#include "pgp/gpg.h"

void
p_gpg_init(void)
{
}

GHashTable*
p_gpg_list_keys(void)
{
    return NULL;
}

GHashTable*
p_gpg_pubkeys(void)
{
    return NULL;
}

const char*
p_gpg_libver(void)
{
    return NULL;
}

void
p_gpg_verify(const gchar* const barejid, const gchar* const sign)
{
}

gchar*
p_gpg_sign(const gchar* const str, const gchar* const fp)
{
    return NULL;
}

gboolean
p_gpg_valid_key(const gchar* const keyid, gchar** err_str)
{
    return FALSE;
}

gboolean
p_gpg_available(const gchar* const barejid)
{
    return FALSE;
}
gchar*
p_gpg_decrypt(const gchar* const cipher)
{
    return NULL;
}

void
p_gpg_on_connect(const gchar* const barejid)
{
}
void
p_gpg_on_disconnect(void)
{
}

gboolean
p_gpg_addkey(const gchar* const jid, const gchar* const keyid)
{
    return TRUE;
}

void
p_gpg_free_decrypted(gchar* decrypted)
{
}

void
p_gpg_free_keys(GHashTable* keys)
{
}

void
p_gpg_autocomplete_key_reset(void)
{
}

gchar*
p_gpg_autocomplete_key(const gchar* const search_str, gboolean previous, void* context)
{
    return NULL;
}

gchar*
p_gpg_format_fp_str(gchar* fp)
{
    return NULL;
}

gchar*
p_gpg_get_pubkey(const gchar* const keyid)
{
    return NULL;
}

gboolean
p_gpg_is_public_key_format(const gchar* buffer)
{
    return TRUE;
}

ProfPGPKey*
p_gpg_import_pubkey(const gchar* buffer)
{
    return NULL;
}
