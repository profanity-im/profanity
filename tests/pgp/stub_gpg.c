#include <glib.h>

#include "pgp/gpg.h"

void p_gpg_init(void) {}

GSList* p_gpg_list_keys(void)
{
    return NULL;
}

const char* p_gpg_libver(void) {
    return NULL;
}

void p_gpg_free_key(ProfPGPKey *key) {}

