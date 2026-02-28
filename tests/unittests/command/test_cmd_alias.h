#ifndef TESTS_TEST_CMD_ALIAS_H
#define TESTS_TEST_CMD_ALIAS_H

void cmd_alias__shows__usage_when_add_no_args(void** state);
void cmd_alias__shows__usage_when_add_no_value(void** state);
void cmd_alias__shows__usage_when_remove_no_args(void** state);
void cmd_alias__shows__usage_when_invalid_subcmd(void** state);
void cmd_alias__updates__adds_alias(void** state);
void cmd_alias__shows__message_when_add_exists(void** state);
void cmd_alias__updates__removes_alias(void** state);
void cmd_alias__shows__message_when_remove_no_alias(void** state);
void cmd_alias__shows__all_aliases(void** state);

#endif
