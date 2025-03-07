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
p_gpg_verify(const char* const barejid, const char* const sign)
{
}

char*
p_gpg_sign(const char* const str, const char* const fp)
{
    return NULL;
}

gboolean
p_gpg_valid_key(const char* const keyid, char** err_str)
{
    return FALSE;
}

gboolean
p_gpg_available(const char* const barejid)
{
    return FALSE;
}
char*
p_gpg_decrypt(const char* const cipher)
{
    return NULL;
}

void
p_gpg_on_connect(const char* const barejid)
{
}
void
p_gpg_on_disconnect(void)
{
}

gboolean
p_gpg_addkey(const char* const jid, const char* const keyid)
{
    return TRUE;
}

void
p_gpg_free_decrypted(char* decrypted)
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

char*
p_gpg_autocomplete_key(const char* const search_str, gboolean previous, void* context)
{
    return NULL;
}

char*
p_gpg_format_fp_str(char* fp)
{
    return NULL;
}

char*
p_gpg_get_pubkey(const char* const keyid)
{
    return NULL;
}

gboolean
p_gpg_is_public_key_format(const char* buffer)
{
    return TRUE;
}

ProfPGPKey*
p_gpg_import_pubkey(const char* buffer)
{
    return NULL;
}
