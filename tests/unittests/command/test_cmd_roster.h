#ifndef TESTS_TEST_CMD_ROSTER_H
#define TESTS_TEST_CMD_ROSTER_H

void cmd_roster__shows__message_when_disconnecting(void** state);
void cmd_roster__shows__message_when_connecting(void** state);
void cmd_roster__shows__message_when_disconnected(void** state);
void cmd_roster__shows__roster_when_no_args(void** state);
void cmd_roster__shows__message_when_add_no_jid(void** state);
void cmd_roster__tests__add_sends_roster_add_request(void** state);
void cmd_roster__shows__message_when_remove_no_jid(void** state);
void cmd_roster__tests__remove_sends_roster_remove_request(void** state);
void cmd_roster__tests__remove_nickname_sends_roster_remove_request(void** state);
void cmd_roster__shows__message_when_nick_no_jid(void** state);
void cmd_roster__shows__message_when_nick_no_nick(void** state);
void cmd_roster__shows__message_when_nick_no_contact_exists(void** state);
void cmd_roster__tests__nick_sends_name_change_request(void** state);
void cmd_roster__shows__message_when_clearnick_no_jid(void** state);
void cmd_roster__shows__message_when_clearnick_no_contact_exists(void** state);
void cmd_roster__tests__clearnick_sends_name_change_request_with_empty_nick(void** state);

#endif
