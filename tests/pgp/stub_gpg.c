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
