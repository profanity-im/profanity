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

    jabber_connect_with_username_password_expect_and_return("user@server.org", "password", JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_with_altdomain_when_provided(void **state)
{
    stub_ui_ask_password();
    stub_cons_show();
    mock_accounts_get_account();
    mock_jabber_connect_with_details();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "user@server.org", "altdomain" };

    mock_connection_status(JABBER_DISCONNECTED);

    accounts_get_account_return(NULL);

    jabber_connect_with_altdomain_expect_and_return("altdomain", JABBER_CONNECTING);

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
    stub_ui_ask_password();
    mock_accounts_get_account();
    mock_accounts_create_full_jid();
    mock_jabber_connect_with_account();
    stub_accounts_free_account();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "jabber_org", NULL };
    ProfAccount *account = malloc(sizeof(ProfAccount));
    account->password = NULL;

    mock_connection_status(JABBER_DISCONNECTED);

    accounts_get_account_return(account);

    accounts_create_full_jid_return("user@jabber.org");

    jabber_connect_with_account_return(JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
    free(account);
}

void cmd_connect_shows_message_when_connecting_with_account(void **state)
{
    mock_cons_show();
    mock_accounts_get_account();
    mock_accounts_create_full_jid();
    mock_jabber_connect_with_account();
    stub_accounts_free_account();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "jabber_org", NULL };
    ProfAccount *account = malloc(sizeof(ProfAccount));
    account->password = "password";
    account->name = "jabber_org";

    mock_connection_status(JABBER_DISCONNECTED);

    accounts_get_account_return(account);

    accounts_create_full_jid_return("user@jabber.org/laptop");

    expect_cons_show("Connecting with account jabber_org as user@jabber.org/laptop");

    jabber_connect_with_account_return(JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
    free(account);
}

void cmd_connect_connects_with_account(void **state)
{
    stub_cons_show();
    mock_accounts_get_account();
    mock_accounts_create_full_jid();
    mock_jabber_connect_with_account();
    stub_accounts_free_account();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "jabber_org", NULL };
    ProfAccount *account = malloc(sizeof(ProfAccount));
    account->password = "password";
    account->name = "jabber_org";

    mock_connection_status(JABBER_DISCONNECTED);

    accounts_get_account_return(account);

    accounts_create_full_jid_return("user@jabber.org/laptop");

    jabber_connect_with_account_expect_and_return(account, JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
    free(account);
}

void cmd_connect_frees_account_after_connecting(void **state)
{
    stub_cons_show();
    mock_accounts_get_account();
    mock_accounts_create_full_jid();
    mock_jabber_connect_with_account();
    mock_accounts_free_account();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "jabber_org", NULL };
    ProfAccount *account = malloc(sizeof(ProfAccount));

    mock_connection_status(JABBER_DISCONNECTED);

    accounts_get_account_return(account);

    accounts_create_full_jid_return("user@jabber.org/laptop");

    jabber_connect_with_account_return(JABBER_CONNECTING);

    accounts_free_account_expect(account);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
    free(account);
}
