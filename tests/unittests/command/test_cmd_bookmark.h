#ifndef TESTS_TEST_CMD_BOOKMARK_H
#define TESTS_TEST_CMD_BOOKMARK_H

void cmd_bookmark__shows__message_when_disconnected(void** state);
void cmd_bookmark__shows__message_when_disconnecting(void** state);
void cmd_bookmark__shows__message_when_connecting(void** state);
void cmd_bookmark__shows__message_when_undefined(void** state);
void cmd_bookmark__shows__usage_when_no_args(void** state);
void cmd_bookmark_list__shows__bookmarks(void** state);
void cmd_bookmark_add__shows__message_when_invalid_jid(void** state);
void cmd_bookmark_add__updates__bookmark_with_jid(void** state);
void cmd_bookmark__tests__uses_roomjid_in_room(void** state);
void cmd_bookmark_add__tests__uses_roomjid_in_room(void** state);
void cmd_bookmark_add__tests__uses_supplied_jid_in_room(void** state);
void cmd_bookmark_remove__tests__uses_roomjid_in_room(void** state);
void cmd_bookmark_remove__tests__uses_supplied_jid_in_room(void** state);
void cmd_bookmark_add__updates__bookmark_with_jid_nick(void** state);
void cmd_bookmark_add__updates__bookmark_with_jid_autojoin(void** state);
void cmd_bookmark_add__updates__bookmark_with_jid_nick_autojoin(void** state);
void cmd_bookmark_remove__updates__removes_bookmark(void** state);
void cmd_bookmark_remove__shows__message_when_no_bookmark(void** state);

#endif
