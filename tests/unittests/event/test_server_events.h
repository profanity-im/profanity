#ifndef TESTS_TEST_SERVER_EVENTS_H
#define TESTS_TEST_SERVER_EVENTS_H

void sv_ev_contact_online__shows__presence_in_console_when_set_online(void** state);
void sv_ev_contact_online__shows__presence_in_console_when_set_all(void** state);
void sv_ev_contact_online__shows__dnd_presence_in_console_when_set_all(void** state);
void sv_ev_contact_offline__updates__removes_chat_session(void** state);
void sv_ev_lost_connection__updates__clears_chat_sessions(void** state);

#endif
