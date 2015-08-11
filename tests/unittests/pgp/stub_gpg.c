#include <glib.h>

#include "pgp/gpg.h"

void p_gpg_init(void) {}
void p_gpg_close(void) {}

GSList* p_gpg_list_keys(void)
{
    return NULL;
}

GHashTable*
p_gpg_fingerprints(void)
{
    return NULL;
}

const char* p_gpg_libver(void)
{
    return NULL;
}

void p_gpg_free_key(ProfPGPKey *key) {}

void p_gpg_verify(const char * const barejid, const char *const sign) {}

char* p_gpg_sign(const char * const str, const char * const fp)
{
    return NULL;
}

gboolean p_gpg_available(const char * const barejid)
{
    return FALSE;
}
char * p_gpg_decrypt(const char * const barejid, const char * const cipher)
{
    return NULL;
}

void p_gpg_on_connect(const char * const barejid) {}
void p_gpg_on_disconnect(void) {}

gboolean p_gpg_addkey(const char * const jid, const char * const keyid)
{
    return TRUE;
}

void p_gpg_free_decrypted(char *decrypted) {}

