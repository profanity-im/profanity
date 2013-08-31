#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static FILE * f = NULL;

void
prof_init (const char * const version, const char * const status)
{
    fprintf (stderr, "called %s with args=<%s, %s>\n", __func__, version, status);
}

void
prof_on_start (void)
{
    fprintf (stderr, "called %s with no args\n", __func__);
}

void
prof_on_connect (const char * const account_name, const char * const fulljid)
{
    fprintf (stderr, "called %s with args=<%s, %s>\n", __func__, account_name, fulljid);
}

void
prof_on_message_received (const char * const jid, const char * const message)
{
    fprintf (stderr, "called %s with args=<%s, %s>\n", __func__, jid, message);
}


