#ifndef LOG_H
#define LOG_H

FILE *logp;

xmpp_log_t *xmpp_get_file_logger();
void logmsg(const char * const area, const char * const msg);

#endif
