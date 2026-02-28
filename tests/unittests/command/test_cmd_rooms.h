#ifndef TESTS_TEST_CMD_ROOMS_H
#define TESTS_TEST_CMD_ROOMS_H

void cmd_rooms__shows__message_when_disconnected(void** state);
void cmd_rooms__shows__message_when_disconnecting(void** state);
void cmd_rooms__shows__message_when_connecting(void** state);
void cmd_rooms__shows__message_when_undefined(void** state);
void cmd_rooms__tests__account_default_when_no_arg(void** state);
void cmd_rooms__tests__service_arg_used_when_passed(void** state);
void cmd_rooms__tests__filter_arg_used_when_passed(void** state);

#endif
