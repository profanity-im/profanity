#ifndef __H_PROFTEST
#define __H_PROFTEST

#define XDG_CONFIG_HOME "./tests/functionaltests/files/xdg_config_home"
#define XDG_DATA_HOME   "./tests/functionaltests/files/xdg_data_home"

void init_prof_test(void **state);
void close_prof_test(void **state);

void prof_start(void);
void prof_connect(void);
void prof_connect_with_roster(char *roster);
void prof_input(char *input);

int prof_output_exact(char *text);
int prof_output_regex(char *text);
int prof_output_glob(char *text);

void prof_timeout(int timeout);
void prof_timeout_reset(void);

#endif
