#include <stdio.h>
#include <glib.h>
#include <stdlib.h>

void avatar_pep_subscribe(void){};
gboolean
avatar_get_by_nick(const char* nick)
{
    return TRUE;
}

#ifdef HAVE_PIXBUF
gboolean
avatar_set(const char* path)
{
    return TRUE;
}
#endif
