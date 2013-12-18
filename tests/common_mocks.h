#ifndef COMMON_MOCKS_H
#define COMMON_MOCKS_H

#include "xmpp/xmpp.h"

void mock_connection_status(jabber_conn_status_t status);
void mock_connection_account_name(char *name);
void expect_room_list_request(char *conf_server);

#endif
