#ifndef TESTS_TEST_CMD_CONNECT_H
#define TESTS_TEST_CMD_CONNECT_H

void cmd_connect__shows__message_when_disconnecting(void** state);
void cmd_connect__shows__message_when_connecting(void** state);
void cmd_connect__shows__message_when_connected(void** state);
void cmd_connect__shows__message_when_undefined(void** state);
void cmd_connect__tests__no_account(void** state);
void cmd_connect__tests__with_altdomain_when_provided(void** state);
void cmd_connect__shows__fail_message(void** state);
void cmd_connect__tests__lowercases_argument_with_no_account(void** state);
void cmd_connect__tests__lowercases_argument_with_account(void** state);
void cmd_connect__tests__asks_password_when_not_in_account(void** state);
void cmd_connect__shows__message_when_connecting_with_account(void** state);
void cmd_connect__tests__connects_with_account(void** state);
void cmd_connect__shows__usage_when_no_server_value(void** state);
void cmd_connect__shows__usage_when_server_no_port_value(void** state);
void cmd_connect__shows__usage_when_no_port_value(void** state);
void cmd_connect__shows__usage_when_port_no_server_value(void** state);
void cmd_connect__shows__message_when_port_0(void** state);
void cmd_connect__shows__message_when_port_minus1(void** state);
void cmd_connect__shows__message_when_port_65536(void** state);
void cmd_connect__shows__message_when_port_contains_chars(void** state);
void cmd_connect__tests__with_server_when_provided(void** state);
void cmd_connect__tests__with_port_when_provided(void** state);
void cmd_connect__tests__with_server_and_port_when_provided(void** state);
void cmd_connect__shows__usage_when_server_provided_twice(void** state);
void cmd_connect__shows__usage_when_port_provided_twice(void** state);
void cmd_connect__shows__usage_when_invalid_first_property(void** state);
void cmd_connect__shows__usage_when_invalid_second_property(void** state);

#endif
