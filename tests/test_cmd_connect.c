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

#include "command/commands.h"


static jabber_conn_status_t _mock_jabber_connect_with_details_no_altdomain(const char * const jid,
    const char * const passwd, const char * const altdomain)
{
    check_expected(jid);
    check_expected(passwd);
    return (jabber_conn_status_t)mock();
}

static jabber_conn_status_t _mock_jabber_connect_with_details_altdomain(const char * const jid,
    const char * const passwd, const char * const altdomain)
{
    check_expected(altdomain);
    return (jabber_conn_status_t)mock();
}

static jabber_conn_status_t _mock_jabber_connect_with_details_result(const char * const jid,
    const char * const passwd, const char * const altdomain)
{
    return (jabber_conn_status_t)mock();
}

static jabber_conn_status_t _mock_jabber_connect_with_account_result(const ProfAccount * const account)
{
    return (jabber_conn_status_t)mock();
}

static jabber_conn_status_t _mock_jabber_connect_with_account_result_check(const ProfAccount * const account)
{
    check_expected(account);
    return (jabber_conn_status_t)mock();
}

static void test_with_connection_status(jabber_conn_status_t status)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));

    mock_connection_status(status);

    expect_string(cons_show, output, "You are either connected already, or a login is in process.");

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
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "user@server.org", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_string(accounts_get_account, name, "user@server.org");
    will_return(accounts_get_account, NULL);

    will_return(ui_ask_password, strdup("password"));

    expect_string(cons_show, output, "Connecting as user@server.org");

    jabber_connect_with_details = _mock_jabber_connect_with_details_no_altdomain;
    expect_string(_mock_jabber_connect_with_details_no_altdomain, jid, "user@server.org");
    expect_string(_mock_jabber_connect_with_details_no_altdomain, passwd, "password");
    will_return(_mock_jabber_connect_with_details_no_altdomain, JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_with_altdomain_when_provided(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "user@server.org", "altdomain" };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, NULL);

    will_return(ui_ask_password, strdup("password"));

    expect_any(cons_show, output);

    jabber_connect_with_details = _mock_jabber_connect_with_details_altdomain;
    expect_string(_mock_jabber_connect_with_details_altdomain, altdomain, "altdomain");
    will_return(_mock_jabber_connect_with_details_altdomain, JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_fail_message(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "user@server.org", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, NULL);

    will_return(ui_ask_password, strdup("password"));

    expect_any(cons_show, output);

    jabber_connect_with_details = _mock_jabber_connect_with_details_result;
    will_return(_mock_jabber_connect_with_details_result, JABBER_DISCONNECTED);

    expect_string(cons_show_error, output, "Connection attempt for user@server.org failed.");

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_lowercases_argument(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "USER@server.ORG", NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_string(accounts_get_account, name, "user@server.org");
    will_return(accounts_get_account, NULL);

    will_return(ui_ask_password, strdup("password"));

    expect_any(cons_show, output);

    jabber_connect_with_details = _mock_jabber_connect_with_details_result;
    will_return(_mock_jabber_connect_with_details_result, JABBER_CONNECTING);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
}

void cmd_connect_asks_password_when_not_in_account(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "jabber_org", NULL };
    ProfAccount *account = malloc(sizeof(ProfAccount));
    account->password = NULL;

    mock_connection_status(JABBER_DISCONNECTED);

    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, account);

    will_return(accounts_create_full_jid, strdup("user@jabber.org"));

    will_return(ui_ask_password, strdup("password"));

    expect_any(cons_show, output);

    jabber_connect_with_account = _mock_jabber_connect_with_account_result;
    will_return(_mock_jabber_connect_with_account_result, JABBER_CONNECTING);

    expect_any(accounts_free_account, account);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
    free(account);
}

void cmd_connect_shows_message_when_connecting_with_account(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "jabber_org", NULL };
    ProfAccount *account = malloc(sizeof(ProfAccount));
    account->password = "password";
    account->name = "jabber_org";

    mock_connection_status(JABBER_DISCONNECTED);

    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, account);

    will_return(accounts_create_full_jid, strdup("user@jabber.org/laptop"));

    expect_string(cons_show, output, "Connecting with account jabber_org as user@jabber.org/laptop");

    jabber_connect_with_account = _mock_jabber_connect_with_account_result;
    will_return(_mock_jabber_connect_with_account_result, JABBER_CONNECTING);

    expect_any(accounts_free_account, account);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
    free(account);
}

void cmd_connect_connects_with_account(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "jabber_org", NULL };
    ProfAccount *account = malloc(sizeof(ProfAccount));
    account->password = "password";
    account->name = "jabber_org";

    mock_connection_status(JABBER_DISCONNECTED);

    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, account);

    will_return(accounts_create_full_jid, strdup("user@jabber.org/laptop"));

    expect_any(cons_show, output);

    jabber_connect_with_account = _mock_jabber_connect_with_account_result_check;
    expect_memory(_mock_jabber_connect_with_account_result_check, account, account, sizeof(ProfAccount));
    will_return(_mock_jabber_connect_with_account_result_check, JABBER_CONNECTING);

    expect_any(accounts_free_account, account);

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
    free(account);
}

void cmd_connect_frees_account_after_connecting(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "jabber_org", NULL };
    ProfAccount *account = malloc(sizeof(ProfAccount));

    mock_connection_status(JABBER_DISCONNECTED);

    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, account);

    will_return(accounts_create_full_jid, strdup("user@jabber.org/laptop"));

    expect_any(cons_show, output);

    jabber_connect_with_account = _mock_jabber_connect_with_account_result;
    will_return(_mock_jabber_connect_with_account_result, JABBER_CONNECTING);

    expect_memory(accounts_free_account, account, account, sizeof(ProfAccount));

    gboolean result = cmd_connect(args, *help);
    assert_true(result);

    free(help);
    free(account);
}
