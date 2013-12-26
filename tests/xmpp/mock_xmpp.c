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

static jabber_conn_status_t
_mock_jabber_connect_with_details(const char * const jid,
    const char * const passwd, const char * const altdomain)
{
    check_expected(jid);
    check_expected(passwd);
    check_expected(altdomain);
    return (jabber_conn_status_t)mock();
}

static jabber_conn_status_t
_mock_jabber_connect_with_account(const ProfAccount * const account)
{
    check_expected(account);
    return (jabber_conn_status_t)mock();
}

void
mock_jabber_connect_with_details(void)
{
    jabber_connect_with_details = _mock_jabber_connect_with_details;
}

void
mock_jabber_connect_with_account(void)
{
    jabber_connect_with_account = _mock_jabber_connect_with_account;
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

void
jabber_connect_with_username_password_expect_and_return(char *jid,
    char *password, jabber_conn_status_t result)
{
    expect_string(_mock_jabber_connect_with_details, jid, jid);
    expect_string(_mock_jabber_connect_with_details, passwd, password);
    expect_any(_mock_jabber_connect_with_details, altdomain);
    will_return(_mock_jabber_connect_with_details, result);
}

void
jabber_connect_with_altdomain_expect_and_return(char *altdomain,
    jabber_conn_status_t result)
{
    expect_any(_mock_jabber_connect_with_details, jid);
    expect_any(_mock_jabber_connect_with_details, passwd);
    expect_string(_mock_jabber_connect_with_details, altdomain, altdomain);
    will_return(_mock_jabber_connect_with_details, result);
}

void
jabber_connect_with_details_return(jabber_conn_status_t result)
{
    expect_any(_mock_jabber_connect_with_details, jid);
    expect_any(_mock_jabber_connect_with_details, passwd);
    expect_any(_mock_jabber_connect_with_details, altdomain);
    will_return(_mock_jabber_connect_with_details, result);
}

void
jabber_connect_with_account_expect_and_return(ProfAccount *account,
    jabber_conn_status_t result)
{
    expect_memory(_mock_jabber_connect_with_account, account, account, sizeof(ProfAccount));
    will_return(_mock_jabber_connect_with_account, result);
}

void
jabber_connect_with_account_return(ProfAccount *account,
    jabber_conn_status_t result)
{
    expect_any(_mock_jabber_connect_with_account, account);
    will_return(_mock_jabber_connect_with_account, result);
}
