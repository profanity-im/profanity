#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "xmpp/xmpp.h"
#include "ui/ui.h"
#include "command/commands.h"

void cmd_account_shows_usage_when_not_connected_and_no_args(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { NULL };

    will_return(jabber_get_connection_status, JABBER_DISCONNECTED);

    expect_string(cons_show, output, "Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_shows_account_when_connected_and_no_args(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    ProfAccount *account = malloc(sizeof(ProfAccount));
    gchar *args[] = { NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);

    will_return(jabber_get_account_name, "account_name");

    expect_string(accounts_get_account, name, "account_name");
    will_return(accounts_get_account, account);

    expect_memory(cons_show_account, account, account, sizeof(ProfAccount));

    expect_any(accounts_free_account, account);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
    free(account);
}

void cmd_account_list_shows_accounts(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "list", NULL };

    gchar **accounts = malloc(sizeof(gchar *) * 4);
    accounts[0] = strdup("account1");
    accounts[1] = strdup("account2");
    accounts[2] = strdup("account3");
    accounts[3] = NULL;

    will_return(accounts_get_list, accounts);

    expect_memory(cons_show_account_list, accounts, accounts, sizeof(accounts));

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_show_shows_usage_when_no_arg(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "show", NULL };

    expect_string(cons_show, output, "Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_show_shows_message_when_account_does_not_exist(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "show", "account_name" };

    expect_string(accounts_get_account, name, "account_name");
    will_return(accounts_get_account, NULL);

    expect_string(cons_show, output, "No such account.");
    expect_string(cons_show, output, "");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_show_shows_message_when_account_exists(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "show", "account_name" };
    ProfAccount *account = malloc(sizeof(ProfAccount));

    expect_string(accounts_get_account, name, "account_name");
    will_return(accounts_get_account, account);

    expect_memory(cons_show_account, account, account, sizeof(ProfAccount));

    expect_any(accounts_free_account, account);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_add_shows_usage_when_no_arg(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "add", NULL };

    expect_string(cons_show, output, "Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}
