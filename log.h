#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#include <strophe/strophe.h>

// log areas
#define PROF "prof"
#define CONN "conn"

// file log
FILE *logp; 

void log_init(void);
void log_msg(const char * const area, const char * const msg);
void log_close(void);

#endif
