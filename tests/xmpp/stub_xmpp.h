#include "xmpp/xmpp.h"

void mock_connection_status(jabber_conn_status_t given_connection_status);
void mock_connection_account_name(char *given_account_name);

void reset_xmpp_mocks(void);