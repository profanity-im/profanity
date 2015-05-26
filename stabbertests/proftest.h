#ifndef __H_PROFTEST
#define __H_PROFTEST

#define XDG_CONFIG_HOME "./stabbertests/files/xdg_config_home"
#define XDG_DATA_HOME   "./stabbertests/files/xdg_data_home"

void init_prof_test(void **state);
void close_prof_test(void **state);
void prof_process_xmpp(int loops);

#endif
