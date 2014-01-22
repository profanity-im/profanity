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

#include "config/accounts.h"
#include "config/mock_accounts.h"

#include "command/commands.h"

void cmd_account_shows_usage_when_not_connected_and_no_args(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { NULL };

    mock_connection_status(JABBER_DISCONNECTED);

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_shows_account_when_connected_and_no_args(void **state)
{
    mock_cons_show_account();
    mock_accounts_get_account();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    ProfAccount *account = account_new("jabber_org", "me@jabber.org", NULL,
        TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL);
    gchar *args[] = { NULL };

    mock_connection_status(JABBER_CONNECTED);
    mock_connection_account_name("account_name");

    accounts_get_account_return(account);

    expect_cons_show_account(account);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_list_shows_accounts(void **state)
{
    mock_cons_show_account_list();
    mock_accounts_get_list();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "list", NULL };

    gchar **accounts = malloc(sizeof(gchar *) * 4);
    accounts[0] = strdup("account1");
    accounts[1] = strdup("account2");
    accounts[2] = strdup("account3");
    accounts[3] = NULL;

    accounts_get_list_return(accounts);

    expect_cons_show_account_list(accounts);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_show_shows_usage_when_no_arg(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "show", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_show_shows_message_when_account_does_not_exist(void **state)
{
    mock_cons_show();
    mock_accounts_get_account();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "show", "account_name", NULL };

    accounts_get_account_return(NULL);

    expect_cons_show("No such account.");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_show_shows_account_when_exists(void **state)
{
    mock_cons_show_account();
    mock_accounts_get_account();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "show", "account_name", NULL };
    ProfAccount *account = account_new("jabber_org", "me@jabber.org", NULL,
        TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL);

    accounts_get_account_return(account);

    expect_cons_show_account(account);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_add_shows_usage_when_no_arg(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "add", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_add_adds_account(void **state)
{
    stub_cons_show();
    mock_accounts_add();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", "new_account", NULL };

    accounts_add_expect_account_name("new_account");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_add_shows_message(void **state)
{
    mock_cons_show();
    stub_accounts_add();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", "new_account", NULL };

    expect_cons_show("Account created.");;
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_enable_shows_usage_when_no_arg(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "enable", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_enable_enables_account(void **state)
{
    stub_cons_show();
    mock_accounts_enable();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "enable", "account_name", NULL };

    accounts_enable_expect("account_name");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_enable_shows_message_when_enabled(void **state)
{
    mock_cons_show();
    mock_accounts_enable();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "enable", "account_name", NULL };

    accounts_enable_return(TRUE);

    expect_cons_show("Account enabled.");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_enable_shows_message_when_account_doesnt_exist(void **state)
{
    mock_cons_show();
    mock_accounts_enable();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "enable", "account_name", NULL };

    accounts_enable_return(FALSE);

    expect_cons_show("No such account: account_name");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_disable_shows_usage_when_no_arg(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "disable", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_disable_disables_account(void **state)
{
    stub_cons_show();
    mock_accounts_disable();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "disable", "account_name", NULL };

    accounts_disable_expect("account_name");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_disable_shows_message_when_disabled(void **state)
{
    mock_cons_show();
    mock_accounts_disable();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "disable", "account_name", NULL };

    accounts_disable_return(TRUE);

    expect_cons_show("Account disabled.");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_disable_shows_message_when_account_doesnt_exist(void **state)
{
    mock_cons_show();
    mock_accounts_disable();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "disable", "account_name", NULL };

    accounts_disable_return(FALSE);

    expect_cons_show("No such account: account_name");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_rename_shows_usage_when_no_args(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "rename", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_rename_shows_usage_when_one_arg(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "rename", "original_name", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_rename_renames_account(void **state)
{
    stub_cons_show();
    mock_accounts_rename();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "rename", "original_name", "new_name", NULL };

    accounts_rename_expect("original_name", "new_name");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_rename_shows_message_when_renamed(void **state)
{
    mock_cons_show();
    mock_accounts_rename();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "rename", "original_name", "new_name", NULL };

    accounts_rename_return(TRUE);

    expect_cons_show("Account renamed.");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_rename_shows_message_when_not_renamed(void **state)
{
    mock_cons_show();
    mock_accounts_rename();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "rename", "original_name", "new_name", NULL };

    accounts_rename_return(FALSE);

    expect_cons_show("Either account original_name doesn't exist, or account new_name already exists.");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_shows_usage_when_no_args(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_shows_usage_when_one_arg(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_shows_usage_when_two_args(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "a_property", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_checks_account_exists(void **state)
{
    stub_cons_show();
    mock_accounts_account_exists();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "a_property", "a_value", NULL };

    accounts_account_exists_expect("a_account");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_shows_message_when_account_doesnt_exist(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "a_property", "a_value", NULL };

    accounts_account_exists_return(FALSE);

    expect_cons_show("Account a_account doesn't exist");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_jid_shows_message_for_malformed_jid(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "jid", "@malformed", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Malformed jid: @malformed");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_jid_sets_barejid(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    mock_accounts_set_jid();
    stub_accounts_set_resource();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "jid", "a_local@a_domain/a_resource", NULL };

    accounts_account_exists_return(TRUE);

    accounts_set_jid_expect("a_account", "a_local@a_domain");

    expect_cons_show("Updated jid for account a_account: a_local@a_domain");

    expect_cons_show_calls(2);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_jid_sets_resource(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    stub_accounts_set_jid();
    mock_accounts_set_resource();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "jid", "a_local@a_domain/a_resource", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show_calls(1);

    accounts_set_resource_expect("a_account", "a_resource");

    expect_cons_show("Updated resource for account a_account: a_resource");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_server_sets_server(void **state)
{
    stub_cons_show();
    mock_accounts_account_exists();
    mock_accounts_set_server();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "server", "a_server", NULL };

    accounts_account_exists_return(TRUE);

    accounts_set_server_expect("a_account", "a_server");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_server_shows_message(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    stub_accounts_set_server();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "server", "a_server", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Updated server for account a_account: a_server");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_resource_sets_resource(void **state)
{
    stub_cons_show();
    mock_accounts_account_exists();
    mock_accounts_set_resource();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "resource", "a_resource", NULL };

    accounts_account_exists_return(TRUE);

    accounts_set_resource_expect("a_account", "a_resource");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_resource_shows_message(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    stub_accounts_set_resource();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "resource", "a_resource", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Updated resource for account a_account: a_resource");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_password_sets_password(void **state)
{
    stub_cons_show();
    mock_accounts_account_exists();
    mock_accounts_set_password();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "password", "a_password", NULL };

    accounts_account_exists_return(TRUE);

    accounts_set_password_expect("a_account", "a_password");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_password_shows_message(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    stub_accounts_set_password();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "password", "a_password", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Updated password for account a_account");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_muc_sets_muc(void **state)
{
    stub_cons_show();
    mock_accounts_account_exists();
    mock_accounts_set_muc_service();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "muc", "a_muc", NULL };

    accounts_account_exists_return(TRUE);

    accounts_set_muc_service_expect("a_account", "a_muc");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_muc_shows_message(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    stub_accounts_set_muc_service();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "muc", "a_muc", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Updated muc service for account a_account: a_muc");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_nick_sets_nick(void **state)
{
    stub_cons_show();
    mock_accounts_account_exists();
    mock_accounts_set_muc_nick();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "nick", "a_nick", NULL };

    accounts_account_exists_return(TRUE);

    accounts_set_muc_nick_expect("a_account", "a_nick");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_nick_shows_message(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    stub_accounts_set_muc_nick();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "nick", "a_nick", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Updated muc nick for account a_account: a_nick");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_status_shows_message_when_invalid_status(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    stub_accounts_set_login_presence();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "status", "bad_status", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Invalid status: bad_status");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_status_sets_status_when_valid(void **state)
{
    stub_cons_show();
    mock_accounts_account_exists();
    mock_accounts_set_login_presence();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "status", "away", NULL };

    accounts_account_exists_return(TRUE);

    accounts_set_login_presence_expect("a_account", "away");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_status_sets_status_when_last(void **state)
{
    stub_cons_show();
    mock_accounts_account_exists();
    mock_accounts_set_login_presence();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "status", "last", NULL };

    accounts_account_exists_return(TRUE);

    accounts_set_login_presence_expect("a_account", "last");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_status_shows_message_when_set_valid(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    stub_accounts_set_login_presence();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "status", "away", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Updated login status for account a_account: away");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_status_shows_message_when_set_last(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    stub_accounts_set_login_presence();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "status", "last", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Updated login status for account a_account: last");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_invalid_presence_string_priority_shows_message(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "blah", "10", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Invalid property: blah");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_last_priority_shows_message(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "last", "10", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Invalid property: last");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_online_priority_sets_preference(void **state)
{
    stub_cons_show();
    mock_accounts_account_exists();
    mock_accounts_set_priorities();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "online", "10", NULL };

    accounts_account_exists_return(TRUE);

    accounts_set_priority_online_expect("a_account", 10);

    mock_connection_status(JABBER_DISCONNECTED);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_chat_priority_sets_preference(void **state)
{
    stub_cons_show();
    mock_accounts_account_exists();
    mock_accounts_set_priorities();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "chat", "10", NULL };

    accounts_account_exists_return(TRUE);

    accounts_set_priority_chat_expect("a_account", 10);

    mock_connection_status(JABBER_DISCONNECTED);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_away_priority_sets_preference(void **state)
{
    stub_cons_show();
    mock_accounts_account_exists();
    mock_accounts_set_priorities();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "away", "10", NULL };

    accounts_account_exists_return(TRUE);

    accounts_set_priority_away_expect("a_account", 10);

    mock_connection_status(JABBER_DISCONNECTED);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_xa_priority_sets_preference(void **state)
{
    stub_cons_show();
    mock_accounts_account_exists();
    mock_accounts_set_priorities();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "xa", "10", NULL };

    accounts_account_exists_return(TRUE);

    accounts_set_priority_xa_expect("a_account", 10);

    mock_connection_status(JABBER_DISCONNECTED);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_dnd_priority_sets_preference(void **state)
{
    stub_cons_show();
    mock_accounts_account_exists();
    mock_accounts_set_priorities();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "dnd", "10", NULL };

    accounts_account_exists_return(TRUE);

    accounts_set_priority_dnd_expect("a_account", 10);

    mock_connection_status(JABBER_DISCONNECTED);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_online_priority_shows_message(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    stub_accounts_set_priorities();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "online", "10", NULL };

    accounts_account_exists_return(TRUE);

    mock_connection_status(JABBER_DISCONNECTED);

    expect_cons_show("Updated online priority for account a_account: 10");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_priority_too_low_shows_message(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "online", "-150", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Value -150 out of range. Must be in -128..127.");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_priority_too_high_shows_message(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "online", "150", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Value 150 out of range. Must be in -128..127.");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_priority_when_not_number_shows_message(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "online", "abc", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Could not convert \"abc\" to a number.");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_priority_when_empty_shows_message(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "online", "", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Could not convert \"\" to a number.");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_priority_updates_presence_when_account_connected_with_presence(void **state)
{
    stub_cons_show();
    stub_accounts_set_priorities();
    mock_accounts_get_last_presence();
    mock_presence_update();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "set", "a_account", "online", "10", NULL };

    accounts_account_exists_return(TRUE);

    mock_connection_status(JABBER_CONNECTED);
    mock_connection_account_name("a_account");

    accounts_get_last_presence_return(RESOURCE_ONLINE);

    mock_connection_presence_message("Free to chat");

    presence_update_expect(RESOURCE_ONLINE, "Free to chat", 0);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_clear_shows_usage_when_no_args(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "clear", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_clear_shows_usage_when_one_arg(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "clear", "a_account", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_clear_checks_account_exists(void **state)
{
    stub_cons_show();
    mock_accounts_account_exists();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "clear", "a_account", "a_property", NULL };

    accounts_account_exists_expect("a_account");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_clear_shows_message_when_account_doesnt_exist(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "clear", "a_account", "a_property", NULL };

    accounts_account_exists_return(FALSE);

    expect_cons_show("Account a_account doesn't exist");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_clear_shows_message_when_invalid_property(void **state)
{
    mock_cons_show();
    mock_accounts_account_exists();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "clear", "a_account", "badproperty", NULL };

    accounts_account_exists_return(TRUE);

    expect_cons_show("Invalid property: badproperty");
    expect_cons_show("");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);

}
