#ifndef TESTS_TEST_CMD_AC_H
#define TESTS_TEST_CMD_AC_H

void cmd_ac_complete_filepath__segfaults_when_empty(void** state);
void cmd_ac_complete_filepath__finds_files_in_current_dir(void** state);
void cmd_ac_complete_filepath__finds_files_with_dot_slash(void** state);
void cmd_ac_complete_filepath__cycles_through_files(void** state);

#endif
