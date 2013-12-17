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

    expect_any(accounts_get_account, name);
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
    gchar *args[] = { "show", "account_name", NULL };

    expect_any(accounts_get_account, name);
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
    gchar *args[] = { "show", "account_name", NULL };
    ProfAccount *account = malloc(sizeof(ProfAccount));

    expect_any(accounts_get_account, name);
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

void cmd_account_add_adds_account(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", "new_account", NULL };

    expect_string(accounts_add, jid, "new_account");

    expect_any_count(cons_show, output, 2);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_add_shows_message(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", "new_account", NULL };

    expect_any(accounts_add, jid);

    expect_string(cons_show, output, "Account created.");;
    expect_string(cons_show, output, "");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_enable_shows_usage_when_no_arg(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "enable", NULL };

    expect_string(cons_show, output, "Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_enable_enables_account(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "enable", "account_name", NULL };

    expect_string(accounts_enable, name, "account_name");
    will_return(accounts_enable, TRUE);

    expect_any_count(cons_show, output, 2);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_enable_shows_message_when_enabled(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "enable", "account_name", NULL };

    expect_any(accounts_enable, name);
    will_return(accounts_enable, TRUE);

    expect_string(cons_show, output, "Account enabled.");
    expect_string(cons_show, output, "");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_enable_shows_message_when_account_doesnt_exist(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "enable", "account_name", NULL };

    expect_any(accounts_enable, name);
    will_return(accounts_enable, FALSE);

    expect_string(cons_show, output, "No such account: account_name");
    expect_string(cons_show, output, "");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_disable_shows_usage_when_no_arg(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "disable", NULL };

    expect_string(cons_show, output, "Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_disable_disables_account(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "disable", "account_name", NULL };

    expect_string(accounts_disable, name, "account_name");
    will_return(accounts_disable, TRUE);

    expect_any_count(cons_show, output, 2);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_disable_shows_message_when_disabled(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "disable", "account_name", NULL };

    expect_any(accounts_disable, name);
    will_return(accounts_disable, TRUE);

    expect_string(cons_show, output, "Account disabled.");
    expect_string(cons_show, output, "");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_disable_shows_message_when_account_doesnt_exist(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "disable", "account_name", NULL };

    expect_any(accounts_disable, name);
    will_return(accounts_disable, FALSE);

    expect_string(cons_show, output, "No such account: account_name");
    expect_string(cons_show, output, "");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_rename_shows_usage_when_no_args(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "rename", NULL };

    expect_string(cons_show, output, "Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_rename_shows_usage_when_one_arg(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "rename", "original_name", NULL };

    expect_string(cons_show, output, "Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_rename_renames_account(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "rename", "original_name", "new_name", NULL };

    expect_string(accounts_rename, account_name, "original_name");
    expect_string(accounts_rename, new_name, "new_name");
    will_return(accounts_rename, TRUE);

    expect_any_count(cons_show, output, 2);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_rename_shows_message_when_renamed(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "rename", "original_name", "new_name", NULL };

    expect_any(accounts_rename, account_name);
    expect_any(accounts_rename, new_name);
    will_return(accounts_rename, TRUE);

    expect_string(cons_show, output, "Account renamed.");
    expect_string(cons_show, output, "");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_rename_shows_message_when_not_renamed(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "rename", "original_name", "new_name", NULL };

    expect_any(accounts_rename, account_name);
    expect_any(accounts_rename, new_name);
    will_return(accounts_rename, FALSE);

    expect_string(cons_show, output, "Either account original_name doesn't exist, or account new_name already exists.");
    expect_string(cons_show, output, "");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_shows_usage_when_no_args(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", NULL };

    expect_string(cons_show, output, "Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_shows_usage_when_one_arg(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", NULL };

    expect_string(cons_show, output, "Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_shows_usage_when_two_args(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "a_property", NULL };

    expect_string(cons_show, output, "Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_checks_account_exists(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "a_property", "a_value", NULL };

    expect_string(accounts_account_exists, account_name, "a_account");
    will_return(accounts_account_exists, FALSE);

    expect_any_count(cons_show, output, 2);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_shows_message_when_account_doesnt_exist(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "a_property", "a_value", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, FALSE);

    expect_string(cons_show, output, "Account a_account doesn't exist");
    expect_string(cons_show, output, "");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_jid_shows_message_for_malformed_jid(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "jid", "@malformed", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(cons_show, output, "Malformed jid: @malformed");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_jid_sets_barejid(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "jid", "a_local@a_domain/a_resource", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_jid, account_name, "a_account");
    expect_string(accounts_set_jid, value, "a_local@a_domain");

    expect_string(cons_show, output, "Updated jid for account a_account: a_local@a_domain");

    expect_any(accounts_set_resource, account_name);
    expect_any(accounts_set_resource, value);

    expect_any(cons_show, output);
    expect_string(cons_show, output, "");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_jid_sets_resource(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "jid", "a_local@a_domain/a_resource", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_any(accounts_set_jid, account_name);
    expect_any(accounts_set_jid, value);

    expect_any(cons_show, output);

    expect_string(accounts_set_resource, account_name, "a_account");
    expect_string(accounts_set_resource, value, "a_resource");

    expect_string(cons_show, output, "Updated resource for account a_account: a_resource");
    expect_string(cons_show, output, "");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_server_sets_server(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "server", "a_server", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_server, account_name, "a_account");
    expect_string(accounts_set_server, value, "a_server");

    expect_any_count(cons_show, output, 2);
    
    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_server_shows_message(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "server", "a_server", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_any(accounts_set_server, account_name);
    expect_any(accounts_set_server, value);

    expect_string(cons_show, output, "Updated server for account a_account: a_server");
    expect_string(cons_show, output, "");
    
    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_resource_sets_resource(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "resource", "a_resource", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_resource, account_name, "a_account");
    expect_string(accounts_set_resource, value, "a_resource");

    expect_any_count(cons_show, output, 2);
    
    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_resource_shows_message(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "resource", "a_resource", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_any(accounts_set_resource, account_name);
    expect_any(accounts_set_resource, value);

    expect_string(cons_show, output, "Updated resource for account a_account: a_resource");
    expect_string(cons_show, output, "");
    
    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_password_sets_password(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "password", "a_password", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_password, account_name, "a_account");
    expect_string(accounts_set_password, value, "a_password");

    expect_any_count(cons_show, output, 2);
    
    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_password_shows_message(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "password", "a_password", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_any(accounts_set_password, account_name);
    expect_any(accounts_set_password, value);

    expect_string(cons_show, output, "Updated password for account a_account");
    expect_string(cons_show, output, "");
    
    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_muc_sets_muc(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "muc", "a_muc", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_muc_service, account_name, "a_account");
    expect_string(accounts_set_muc_service, value, "a_muc");

    expect_any_count(cons_show, output, 2);
    
    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_muc_shows_message(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "muc", "a_muc", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_any(accounts_set_muc_service, account_name);
    expect_any(accounts_set_muc_service, value);

    expect_string(cons_show, output, "Updated muc service for account a_account: a_muc");
    expect_string(cons_show, output, "");
    
    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_nick_sets_nick(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "nick", "a_nick", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_muc_nick, account_name, "a_account");
    expect_string(accounts_set_muc_nick, value, "a_nick");

    expect_any_count(cons_show, output, 2);
    
    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_set_nick_shows_message(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "set", "a_account", "nick", "a_nick", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_any(accounts_set_muc_nick, account_name);
    expect_any(accounts_set_muc_nick, value);

    expect_string(cons_show, output, "Updated muc nick for account a_account: a_nick");
    expect_string(cons_show, output, "");
    
    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}
