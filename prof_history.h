#ifndef PROF_HISTORY_H
#define PROF_HISTORY_H

typedef struct p_history_t  *PHistory;

PHistory p_history_new(unsigned int size);
char * p_history_previous(PHistory history, char *item);
char * p_history_next(PHistory history, char *item);
void p_history_append(PHistory history, char *item);

#endif
