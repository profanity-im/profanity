#include <stdio.h>

#include "log.h"

extern FILE *logp;

void log_msg(const char * const area, const char * const msg)
{
    fprintf(logp, "%s DEBUG: %s\n", area, msg);
}

void log_init(void)
{
    logp = fopen("profanity.log", "a");
    log_msg(PROF, "Starting Profanity...");
}

void log_close(void)
{
    fclose(logp);
}
