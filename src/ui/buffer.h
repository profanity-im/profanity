#ifndef UI_BUFFER_H
#define UI_BUFFER_H

#include "config.h"

typedef struct prof_buff_entry_t {
    char show_char;
    char *date_fmt;
    int flags;
    int attrs;
    char *from;
    char *message;
} ProfBuffEntry;

typedef struct prof_buff_t *ProfBuff;

ProfBuff buffer_create();
void buffer_free(ProfBuff buffer);
void buffer_push(ProfBuff buffer, const char show_char, const char * const date_fmt, int flags, int attrs, const char * const from, const char * const message);
int buffer_size(ProfBuff buffer);
ProfBuffEntry* buffer_yield_entry(ProfBuff buffer, int entry);
#endif
