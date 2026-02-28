#ifndef TESTS_TEST_CMD_JOIN_H
#define TESTS_TEST_CMD_JOIN_H

void cmd_join__shows__message_when_disconnecting(void** state);
void cmd_join__shows__message_when_connecting(void** state);
void cmd_join__shows__message_when_disconnected(void** state);
void cmd_join__shows__message_when_undefined(void** state);
void cmd_join__shows__error_message_when_invalid_room_jid(void** state);
void cmd_join__tests__uses_account_mucservice_when_no_service_specified(void** state);
void cmd_join__tests__uses_supplied_nick(void** state);
void cmd_join__tests__uses_account_nick_when_not_supplied(void** state);
void cmd_join__tests__uses_password_when_supplied(void** state);

#endif
