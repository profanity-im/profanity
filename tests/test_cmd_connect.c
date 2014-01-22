#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "xmpp/xmpp.h"
#include "xmpp/mock_xmpp.h"

#include "ui/ui.h"
#include "ui/mock_ui.h"

#include "command/commands.h"

#include "config/accounts.h"
#include "config/mock_accounts.h"

static void test_with_connection_status(jabber_conn_status_t status)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));

    mock_connection_status(status);

    expect_cons_show("You are either connected already, or a login is in process.");

    gboolean result = cmd_connect(NULL, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_shows_message_when_disconnecting(void **state)
{
    test_with_connection_status(JABBER_DISCONNECTING);
}

void cmd_connect_shows_message_when_connecting(void **state)
{
    test_with_connection_status(JABBER_CONNECTING);
}

void cmd_connect_shows_message_when_connected(void **state)
{
    test_with_connection_status(JABBER_CONNECTED);
}

void cmd_connect_shows_message_when_undefined(void **state)
{
    test_with_connection_status(JABBER_UNDEFINED);
}

void cmd_connect_shows_usage_when_no_server_value(void **state)
{
    stub_ui_ask_password();
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "user@server.org", "server", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_cons_show("Usage: some usage");
    expect_cons_show("");

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_shows_usage_when_server_no_port_value(void **state)
{
    stub_ui_ask_password();
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "user@server.org", "server", "aserver", "port", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_cons_show("Usage: some usage");
    expect_cons_show("");

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_shows_usage_when_no_port_value(void **state)
{
    stub_ui_ask_password();
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "user@server.org", "port", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_cons_show("Usage: some usage");
    expect_cons_show("");

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_shows_usage_when_port_no_server_value(void **state)
{
    stub_ui_ask_password();
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "user@server.org", "port", "5678", "server", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_cons_show("Usage: some usage");
    expect_cons_show("");

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_shows_message_when_port_0(void **state)
{
    stub_ui_ask_password();
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "user@server.org", "port", "0", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_cons_show("Value 0 out of range. Must be in 1..65535.");
    expect_cons_show("");

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_shows_message_when_port_minus1(void **state)
{
    stub_ui_ask_password();
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "user@server.org", "port", "-1", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_cons_show("Value -1 out of range. Must be in 1..65535.");
    expect_cons_show("");

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_shows_message_when_port_65536(void **state)
{
    stub_ui_ask_password();
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "user@server.org", "port", "65536", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_cons_show("Value 65536 out of range. Must be in 1..65535.");
    expect_cons_show("");

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_shows_message_when_port_contains_chars(void **state)
{
    stub_ui_ask_password();
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "user@server.org", "port", "52f66", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_cons_show("Could not convert \"52f66\" to a number.");
    expect_cons_show("");

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_shows_usage_when_server_provided_twice(void **state)
{
    stub_ui_ask_password();
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "user@server.org", "server", "server1", "server", "server2", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_cons_show("Usage: some usage");
    expect_cons_show("");

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_shows_usage_when_port_provided_twice(void **state)
{
    stub_ui_ask_password();
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "user@server.org", "port", "1111", "port", "1111", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_cons_show("Usage: some usage");
    expect_cons_show("");

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_shows_usage_when_invalid_first_property(void **state)
{
    stub_ui_ask_password();
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "user@server.org", "wrong", "server", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_cons_show("Usage: some usage");
    expect_cons_show("");

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_shows_usage_when_invalid_second_property(void **state)
{
    stub_ui_ask_password();
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "user@server.org", "server", "aserver", "wrong", "1234", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_cons_show("Usage: some usage");
    expect_cons_show("");

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_when_no_account(void **state)
{
    mock_cons_show();
    mock_accounts_get_account();
    mock_ui_ask_password();
    mock_jabber_connect_with_details();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "user@server.org", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    accounts_get_account_expect_and_return("user@server.org", NULL);

    mock_ui_ask_password_returns("password");

    expect_cons_show("Connecting as user@server.org");

    jabber_connect_with_details_expect_and_return("user@server.org", "password", NULL, 0, JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_with_server_when_provided(void **state)
{
    mock_ui_ask_password();
    stub_cons_show();
    mock_accounts_get_account();
    mock_jabber_connect_with_details();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "user@server.org", "server", "aserver", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    accounts_get_account_return(NULL);

    mock_ui_ask_password_returns("password");

    jabber_connect_with_details_expect_and_return("user@server.org", "password", "aserver", 0, JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_with_port_when_provided(void **state)
{
    mock_ui_ask_password();
    stub_cons_show();
    mock_accounts_get_account();
    mock_jabber_connect_with_details();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "user@server.org", "port", "5432", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    accounts_get_account_return(NULL);

    mock_ui_ask_password_returns("password");

    jabber_connect_with_details_expect_and_return("user@server.org", "password", NULL, 5432, JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_with_server_and_port_when_provided(void **state)
{
    mock_ui_ask_password();
    stub_cons_show();
    mock_accounts_get_account();
    mock_jabber_connect_with_details();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "user@server.org", "port", "5432", "server", "aserver", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    accounts_get_account_return(NULL);

    mock_ui_ask_password_returns("password");

    jabber_connect_with_details_expect_and_return("user@server.org", "password", "aserver", 5432, JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_fail_message(void **state)
{
    stub_cons_show();
    mock_cons_show_error();
    stub_ui_ask_password();
    mock_accounts_get_account();
    mock_jabber_connect_with_details();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "user@server.org", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    accounts_get_account_return(NULL);

    jabber_connect_with_details_return(JABBER_DISCONNECTED);

    expect_cons_show_error("Connection attempt for user@server.org failed.");

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_lowercases_argument(void **state)
{
    stub_cons_show();
    stub_ui_ask_password();
    mock_accounts_get_account();
    mock_jabber_connect_with_details();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "USER@server.ORG", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    accounts_get_account_expect_and_return("user@server.org", NULL);

    jabber_connect_with_details_return(JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_asks_password_when_not_in_account(void **state)
{
    stub_cons_show();
    mock_ui_ask_password();
    mock_accounts_get_account();
    mock_jabber_connect_with_account();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "jabber_org", NULL };
    ProfAccount *account = account_new("jabber_org", "me@jabber.org", NULL,
        TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL);

    mock_connection_status(JABBER_DISCONNECTED);

    accounts_get_account_return(account);

    mock_ui_ask_password_returns("password");

    jabber_connect_with_account_return(JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_shows_message_when_connecting_with_account(void **state)
{
    mock_cons_show();
    mock_accounts_get_account();
    mock_jabber_connect_with_account();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "jabber_org", NULL };
    ProfAccount *account = account_new("jabber_org", "user@jabber.org", "password",
        TRUE, NULL, 0, "laptop", NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL);

    mock_connection_status(JABBER_DISCONNECTED);

    accounts_get_account_return(account);

    expect_cons_show("Connecting with account jabber_org as user@jabber.org/laptop");

    jabber_connect_with_account_return(JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_connects_with_account(void **state)
{
    stub_cons_show();
    mock_accounts_get_account();
    mock_jabber_connect_with_account();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "jabber_org", NULL };
    ProfAccount *account = account_new("jabber_org", "me@jabber.org", "password",
        TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL);

    mock_connection_status(JABBER_DISCONNECTED);

    accounts_get_account_return(account);

    jabber_connect_with_account_expect_and_return(account, JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}
