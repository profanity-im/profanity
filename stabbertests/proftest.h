#ifndef __H_PROFTEST
#define __H_PROFTEST

void init_prof_test(void **state);
void close_prof_test(void **state);
void prof_process_xmpp(int loops);

#endif