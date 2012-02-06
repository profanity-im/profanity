#ifndef LOG_H
#define LOG_H

#define PROF "prof"
#define CONN "conn"

FILE *logp; 

xmpp_log_t *xmpp_get_file_logger();
void logmsg(const char * const area, const char * const msg);
void start_log(void);

#endif
