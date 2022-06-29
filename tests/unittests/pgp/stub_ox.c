#include <glib.h>

#include "pgp/gpg.h"

gboolean
ox_is_private_key_available(const char* const barejid)
{
    return FALSE;
}

gboolean
ox_is_public_key_available(const char* const barejid)
{
    return FALSE;
}

GHashTable*
ox_gpg_public_keys(void)
{
    return NULL;
}
