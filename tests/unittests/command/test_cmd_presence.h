#ifndef TESTS_TEST_CMD_PRESENCE_H
#define TESTS_TEST_CMD_PRESENCE_H

void cmd_presence__shows__usage_when_bad_subcmd(void** state);
void cmd_presence__shows__usage_when_bad_console_setting(void** state);
void cmd_presence__shows__usage_when_bad_chat_setting(void** state);
void cmd_presence__shows__usage_when_bad_muc_setting(void** state);
void cmd_presence__updates__console_all(void** state);
void cmd_presence__updates__console_online(void** state);
void cmd_presence__updates__console_none(void** state);
void cmd_presence__updates__chat_all(void** state);
void cmd_presence__updates__chat_online(void** state);
void cmd_presence__updates__chat_none(void** state);
void cmd_presence__updates__room_all(void** state);
void cmd_presence__updates__room_online(void** state);
void cmd_presence__updates__room_none(void** state);

#endif
