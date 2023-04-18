#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "xmpp/xmpp.h"

#include "ui/ui.h"
#include "ui/stub_ui.h"

#include "command/cmd_funcs.h"
#include "config/accounts.h"

#define CMD_CONNECT "/connect"

static void
test_with_connection_status(jabber_conn_status_t status)
{
    will_return(connection_get_status, status);

    expect_cons_show("You are either connected already, or a login is in process.");

    gboolean result = cmd_connect(NULL, CMD_CONNECT, NULL);
    assert_true(result);
}

void
cmd_connect_shows_message_when_disconnecting(void** state)
{
    test_with_connection_status(JABBER_DISCONNECTING);
}

void
cmd_connect_shows_message_when_connecting(void** state)
{
    test_with_connection_status(JABBER_CONNECTING);
}

void
cmd_connect_shows_message_when_connected(void** state)
{
    test_with_connection_status(JABBER_CONNECTED);
}

void
cmd_connect_when_no_account(void** state)
{
    gchar* args[] = { "user@server.org", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_string(accounts_get_account, name, "user@server.org");
    will_return(accounts_get_account, NULL);

    will_return(ui_ask_password, strdup("password"));

    expect_cons_show("Connecting as user@server.org");

    expect_string(session_connect_with_details, jid, "user@server.org");
    expect_string(session_connect_with_details, passwd, "password");
    expect_value(session_connect_with_details, altdomain, NULL);
    expect_value(session_connect_with_details, port, 0);
    will_return(session_connect_with_details, JABBER_CONNECTING);

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_fail_message(void** state)
{
    gchar* args[] = { "user@server.org", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, NULL);

    will_return(ui_ask_password, strdup("password"));

    expect_cons_show("Connecting as user@server.org");

    expect_any(session_connect_with_details, jid);
    expect_any(session_connect_with_details, passwd);
    expect_any(session_connect_with_details, altdomain);
    expect_any(session_connect_with_details, port);
    will_return(session_connect_with_details, JABBER_DISCONNECTED);

    expect_cons_show_error("Connection attempt for user@server.org failed.");

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_lowercases_argument_with_no_account(void** state)
{
    gchar* args[] = { "USER@server.ORG", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_string(accounts_get_account, name, "USER@server.ORG");
    will_return(accounts_get_account, NULL);

    will_return(ui_ask_password, strdup("password"));

    expect_cons_show("Connecting as user@server.org");

    expect_string(session_connect_with_details, jid, "user@server.org");
    expect_string(session_connect_with_details, passwd, "password");
    expect_value(session_connect_with_details, altdomain, NULL);
    expect_value(session_connect_with_details, port, 0);
    will_return(session_connect_with_details, JABBER_CONNECTING);

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_lowercases_argument_with_account(void** state)
{
    gchar* args[] = { "Jabber_org", NULL };
    ProfAccount* account = account_new(g_strdup("Jabber_org"), g_strdup("me@jabber.org"), g_strdup("password"), NULL,
                                       TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0);

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, account);

    expect_cons_show("Connecting with account Jabber_org as me@jabber.org");

    expect_memory(session_connect_with_account, account, account, sizeof(account));
    will_return(session_connect_with_account, JABBER_CONNECTING);

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_asks_password_when_not_in_account(void** state)
{
    gchar* args[] = { "jabber_org", NULL };
    ProfAccount* account = account_new(g_strdup("jabber_org"), g_strdup("me@jabber.org"), NULL, NULL,
                                       TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0);

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, account);

    will_return(ui_ask_password, strdup("password"));

    expect_cons_show("Connecting with account jabber_org as me@jabber.org");

    expect_any(session_connect_with_account, account);
    will_return(session_connect_with_account, JABBER_CONNECTING);

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_shows_usage_when_no_server_value(void** state)
{
    gchar* args[] = { "user@server.org", "server", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_CONNECT);
    expect_cons_show("");

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_shows_usage_when_server_no_port_value(void** state)
{
    gchar* args[] = { "user@server.org", "server", "aserver", "port", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_CONNECT);
    expect_cons_show("");

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_shows_usage_when_no_port_value(void** state)
{
    gchar* args[] = { "user@server.org", "port", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_CONNECT);
    expect_cons_show("");

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_shows_usage_when_port_no_server_value(void** state)
{
    gchar* args[] = { "user@server.org", "port", "5678", "server", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_CONNECT);
    expect_cons_show("");

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_shows_message_when_port_0(void** state)
{
    gchar* args[] = { "user@server.org", "port", "0", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_cons_show("Value 0 out of range. Must be in 1..65535.");
    expect_cons_show("");

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_shows_message_when_port_minus1(void** state)
{
    gchar* args[] = { "user@server.org", "port", "-1", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_cons_show("Value -1 out of range. Must be in 1..65535.");
    expect_cons_show("");

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_shows_message_when_port_65536(void** state)
{
    gchar* args[] = { "user@server.org", "port", "65536", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_cons_show("Value 65536 out of range. Must be in 1..65535.");
    expect_cons_show("");

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_shows_message_when_port_contains_chars(void** state)
{
    gchar* args[] = { "user@server.org", "port", "52f66", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_cons_show("Could not convert \"52f66\" to a number.");
    expect_cons_show("");

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_shows_usage_when_server_provided_twice(void** state)
{
    gchar* args[] = { "user@server.org", "server", "server1", "server", "server2", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_CONNECT);
    expect_cons_show("");

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_shows_usage_when_port_provided_twice(void** state)
{
    gchar* args[] = { "user@server.org", "port", "1111", "port", "1111", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_CONNECT);
    expect_cons_show("");

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_shows_usage_when_invalid_first_property(void** state)
{
    gchar* args[] = { "user@server.org", "wrong", "server", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_CONNECT);
    expect_cons_show("");

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_shows_usage_when_invalid_second_property(void** state)
{
    gchar* args[] = { "user@server.org", "server", "aserver", "wrong", "1234", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_CONNECT);
    expect_cons_show("");

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_with_server_when_provided(void** state)
{
    gchar* args[] = { "user@server.org", "server", "aserver", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_string(accounts_get_account, name, "user@server.org");
    will_return(accounts_get_account, NULL);

    will_return(ui_ask_password, strdup("password"));

    expect_cons_show("Connecting as user@server.org");

    expect_string(session_connect_with_details, jid, "user@server.org");
    expect_string(session_connect_with_details, passwd, "password");
    expect_string(session_connect_with_details, altdomain, "aserver");
    expect_value(session_connect_with_details, port, 0);
    will_return(session_connect_with_details, JABBER_CONNECTING);

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_with_port_when_provided(void** state)
{
    gchar* args[] = { "user@server.org", "port", "5432", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_string(accounts_get_account, name, "user@server.org");
    will_return(accounts_get_account, NULL);

    will_return(ui_ask_password, strdup("password"));

    expect_cons_show("Connecting as user@server.org");

    expect_string(session_connect_with_details, jid, "user@server.org");
    expect_string(session_connect_with_details, passwd, "password");
    expect_value(session_connect_with_details, altdomain, NULL);
    expect_value(session_connect_with_details, port, 5432);
    will_return(session_connect_with_details, JABBER_CONNECTING);

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_with_server_and_port_when_provided(void** state)
{
    gchar* args[] = { "user@server.org", "port", "5432", "server", "aserver", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_string(accounts_get_account, name, "user@server.org");
    will_return(accounts_get_account, NULL);

    will_return(ui_ask_password, strdup("password"));

    expect_cons_show("Connecting as user@server.org");

    expect_string(session_connect_with_details, jid, "user@server.org");
    expect_string(session_connect_with_details, passwd, "password");
    expect_string(session_connect_with_details, altdomain, "aserver");
    expect_value(session_connect_with_details, port, 5432);
    will_return(session_connect_with_details, JABBER_CONNECTING);

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_shows_message_when_connecting_with_account(void** state)
{
    gchar* args[] = { "jabber_org", NULL };
    ProfAccount* account = account_new(g_strdup("jabber_org"), g_strdup("user@jabber.org"), g_strdup("password"), NULL,
                                       TRUE, NULL, 0, g_strdup("laptop"), NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0);

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, account);

    expect_cons_show("Connecting with account jabber_org as user@jabber.org/laptop");

    expect_any(session_connect_with_account, account);
    will_return(session_connect_with_account, JABBER_CONNECTING);

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}

void
cmd_connect_connects_with_account(void** state)
{
    gchar* args[] = { "jabber_org", NULL };
    ProfAccount* account = account_new(g_strdup("jabber_org"), g_strdup("me@jabber.org"), g_strdup("password"), NULL,
                                       TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0);

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, account);

    expect_cons_show("Connecting with account jabber_org as me@jabber.org");

    expect_memory(session_connect_with_account, account, account, sizeof(account));
    will_return(session_connect_with_account, JABBER_CONNECTING);

    gboolean result = cmd_connect(NULL, CMD_CONNECT, args);
    assert_true(result);
}
