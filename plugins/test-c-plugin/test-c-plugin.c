#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

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

char *
prof_on_message_received (const char * const jid, const char *message)
{
    fprintf (stderr, "called %s with args=<%s, %s>\n", __func__, jid, message);
    char *result = malloc(strlen(message) + 4);
    sprintf(result, "%s%s", message, "[C]");

    return result;
}

char *
prof_on_message_send (const char * const jid, const char *message)
{
    fprintf (stderr, "called %s with args=<%s, %s>\n", __func__, jid, message);
    char *result = malloc(strlen(message) + 4);
    sprintf(result, "%s%s", message, "[C]");

    return result;
}
