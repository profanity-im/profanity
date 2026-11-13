#ifndef TESTS_TEST_CMD_JOIN_H
#define TESTS_TEST_CMD_JOIN_H

void cmd_join__shows__message_when_disconnecting(void** state);
void cmd_join__shows__message_when_connecting(void** state);
void cmd_join__shows__message_when_disconnected(void** state);
void cmd_join__shows__message_when_undefined(void** state);
void cmd_join__shows__error_message_when_invalid_room_jid(void** state);
void cmd_join__shows__error_message_when_account_mucservice_invalid(void** state);
void cmd_join__tests__uses_account_mucservice_when_no_service_specified(void** state);
void cmd_join__tests__uses_supplied_nick(void** state);
void cmd_join__tests__uses_account_nick_when_not_supplied(void** state);
void cmd_join__tests__uses_password_when_supplied(void** state);
void cmd_join__tests__xmpp_uri_join(void** state);
void cmd_join__tests__xmpp_uri_join_with_password(void** state);
void cmd_join__tests__xmpp_uri_invalid_query_type(void** state);
void cmd_join__tests__uses_resource_as_nick_when_supplied_in_jid(void** state);
void cmd_join__tests__prefers_explicit_nick_over_resource_in_jid(void** state);

#endif
