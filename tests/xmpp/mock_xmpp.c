#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "xmpp/xmpp.h"

static jabber_conn_status_t
_mock_jabber_get_connection_status(void)
{
    return (jabber_conn_status_t)mock();
}

static char *
_mock_jabber_get_account_name(void)
{
    return (char *)mock();
}

static void
_mock_iq_room_list_request(gchar *conf_server)
{
    check_expected(conf_server);
}

void
mock_connection_status(jabber_conn_status_t status)
{
    jabber_get_connection_status = _mock_jabber_get_connection_status;
    will_return(_mock_jabber_get_connection_status, status);
}

void
mock_connection_account_name(char *name)
{
    jabber_get_account_name = _mock_jabber_get_account_name;
    will_return(_mock_jabber_get_account_name, name);
}

void
expect_room_list_request(char *conf_server)
{
    iq_room_list_request = _mock_iq_room_list_request;
    expect_string(_mock_iq_room_list_request, conf_server, conf_server);
}
