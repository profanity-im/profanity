#ifndef TESTS_TEST_CHAT_SESSION_H
#define TESTS_TEST_CHAT_SESSION_H

void chat_session_get__returns__null_when_no_session(void** state);
void chat_session_recipient_active__updates__new_session(void** state);
void chat_session_recipient_active__updates__replace_resource(void** state);
void chat_session_remove__updates__session_removed(void** state);

#endif
