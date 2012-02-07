#include <stdio.h>
#include <strophe/strophe.h>

#include "log.h"

extern FILE *logp;

void xmpp_file_logger(void * const userdata, const xmpp_log_level_t level,
    const char * const area, const char * const msg);

static const xmpp_log_t file_log = { &xmpp_file_logger, XMPP_LEVEL_DEBUG };

xmpp_log_t *xmpp_get_file_logger()
{
    return (xmpp_log_t*) &file_log;
}

void xmpp_file_logger(void * const userdata, const xmpp_log_level_t level,
    const char * const area, const char * const msg)
{
    log_msg(area, msg);
}

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
