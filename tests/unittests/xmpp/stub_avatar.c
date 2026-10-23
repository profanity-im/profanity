#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

void avatar_pep_subscribe(void) {};
void avatar_auto_enable(void) {};
void avatar_auto_disable(void) {};

gboolean
avatar_get_by_nick(const char* nick, gboolean open)
{
    return TRUE;
}

gchar*
avatar_get_path(const gchar* const jid)
{
    return NULL;
}

gboolean
avatar_set(const char* path)
{
    return TRUE;
}

gboolean
avatar_unset(void)
{
    return TRUE;
}
