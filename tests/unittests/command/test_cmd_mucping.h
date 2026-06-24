#ifndef TESTS_TEST_CMD_MUCPING_H
#define TESTS_TEST_CMD_MUCPING_H

void cmd_mucping__shows__usage_when_invalid_interval_chars(void** state);
void cmd_mucping__shows__usage_when_invalid_interval_range(void** state);
void cmd_mucping__shows__usage_when_invalid_timeout_chars(void** state);
void cmd_mucping__shows__usage_when_invalid_timeout_range(void** state);
void cmd_mucping__shows__error_when_interval_smaller_than_timeout(void** state);
void cmd_mucping__shows__error_when_timeout_greater_than_interval(void** state);

#endif
