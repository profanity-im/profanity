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

#include "config/accounts.h"

#include "command/cmd_funcs.h"

#define CMD_ACCOUNT "/account"

void cmd_account_shows_usage_when_not_connected_and_no_args(void **state)
{
    gchar *args[] = { NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_ACCOUNT);

    gboolean result = cmd_account(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}


void cmd_account_shows_account_when_connected_and_no_args(void **state)
{
    ProfAccount *account = account_new("jabber_org", "me@jabber.org", NULL, NULL,
        TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    gchar *args[] = { NULL };

    will_return(connection_get_status, JABBER_CONNECTED);
    will_return(session_get_account_name, "account_name");
    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, account);

    expect_memory(cons_show_account, account, account, sizeof(ProfAccount));

    gboolean result = cmd_account(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_list_shows_accounts(void **state)
{
    gchar *args[] = { "list", NULL };

    gchar **accounts = malloc(sizeof(gchar *) * 4);
    accounts[0] = strdup("account1");
    accounts[1] = strdup("account2");
    accounts[2] = strdup("account3");
    accounts[3] = NULL;

    will_return(accounts_get_list, accounts);

    expect_memory(cons_show_account_list, accounts, accounts, sizeof(accounts));

    gboolean result = cmd_account_list(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_show_shows_usage_when_no_arg(void **state)
{
    gchar *args[] = { "show", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ACCOUNT);

    gboolean result = cmd_account_show(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_show_shows_message_when_account_does_not_exist(void **state)
{
    gchar *args[] = { "show", "account_name", NULL };

    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, NULL);

    expect_cons_show("No such account.");
    expect_cons_show("");

    gboolean result = cmd_account_show(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_show_shows_account_when_exists(void **state)
{
    gchar *args[] = { "show", "account_name", NULL };
    ProfAccount *account = account_new("jabber_org", "me@jabber.org", NULL, NULL,
        TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, account);

    expect_memory(cons_show_account, account, account, sizeof(account));

    gboolean result = cmd_account_show(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_add_shows_usage_when_no_arg(void **state)
{
    gchar *args[] = { "add", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ACCOUNT);

    gboolean result = cmd_account_add(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_add_adds_account(void **state)
{
    gchar *args[] = { "add", "new_account", NULL };

    expect_string(accounts_add, jid, "new_account");
    expect_value(accounts_add, altdomain, NULL);
    expect_value(accounts_add, port, 0);
    expect_cons_show("Account created.");
    expect_cons_show("");

    gboolean result = cmd_account_add(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_enable_shows_usage_when_no_arg(void **state)
{
    gchar *args[] = { "enable", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ACCOUNT);

    gboolean result = cmd_account_enable(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_enable_enables_account(void **state)
{
    gchar *args[] = { "enable", "account_name", NULL };

    expect_string(accounts_enable, name, "account_name");
    will_return(accounts_enable, TRUE);

    expect_cons_show("Account enabled.");
    expect_cons_show("");

    gboolean result = cmd_account_enable(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_enable_shows_message_when_account_doesnt_exist(void **state)
{
    gchar *args[] = { "enable", "account_name", NULL };

    expect_any(accounts_enable, name);
    will_return(accounts_enable, FALSE);

    expect_cons_show("No such account: account_name");
    expect_cons_show("");

    gboolean result = cmd_account_enable(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_disable_shows_usage_when_no_arg(void **state)
{
    gchar *args[] = { "disable", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ACCOUNT);

    gboolean result = cmd_account_disable(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_disable_disables_account(void **state)
{
    gchar *args[] = { "disable", "account_name", NULL };

    expect_string(accounts_disable, name, "account_name");
    will_return(accounts_disable, TRUE);

    expect_cons_show("Account disabled.");
    expect_cons_show("");

    gboolean result = cmd_account_disable(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_disable_shows_message_when_account_doesnt_exist(void **state)
{
    gchar *args[] = { "disable", "account_name", NULL };

    expect_any(accounts_disable, name);
    will_return(accounts_disable, FALSE);

    expect_cons_show("No such account: account_name");
    expect_cons_show("");

    gboolean result = cmd_account_disable(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_rename_shows_usage_when_no_args(void **state)
{
    gchar *args[] = { "rename", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ACCOUNT);

    gboolean result = cmd_account_rename(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_rename_shows_usage_when_one_arg(void **state)
{
    gchar *args[] = { "rename", "original_name", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ACCOUNT);

    gboolean result = cmd_account_rename(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_rename_renames_account(void **state)
{
    gchar *args[] = { "rename", "original_name", "new_name", NULL };

    expect_string(accounts_rename, account_name, "original_name");
    expect_string(accounts_rename, new_name, "new_name");
    will_return(accounts_rename, TRUE);

    expect_cons_show("Account renamed.");
    expect_cons_show("");

    gboolean result = cmd_account_rename(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_rename_shows_message_when_not_renamed(void **state)
{
    gchar *args[] = { "rename", "original_name", "new_name", NULL };

    expect_any(accounts_rename, account_name);
    expect_any(accounts_rename, new_name);
    will_return(accounts_rename, FALSE);

    expect_cons_show("Either account original_name doesn't exist, or account new_name already exists.");
    expect_cons_show("");

    gboolean result = cmd_account_rename(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_shows_usage_when_no_args(void **state)
{
    gchar *args[] = { "set", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ACCOUNT);

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_shows_usage_when_one_arg(void **state)
{
    gchar *args[] = { "set", "a_account", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ACCOUNT);

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_shows_usage_when_two_args(void **state)
{
    gchar *args[] = { "set", "a_account", "a_property", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ACCOUNT);

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_shows_message_when_account_doesnt_exist(void **state)
{
    gchar *args[] = { "set", "a_account", "a_property", "a_value", NULL };

    expect_string(accounts_account_exists, account_name, "a_account");
    will_return(accounts_account_exists, FALSE);

    expect_cons_show("Account a_account doesn't exist");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_jid_shows_message_for_malformed_jid(void **state)
{
    gchar *args[] = { "set", "a_account", "jid", "@malformed", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_cons_show("Malformed jid: @malformed");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_jid_sets_barejid(void **state)
{
    gchar *args[] = { "set", "a_account", "jid", "a_local@a_domain", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_jid, account_name, "a_account");
    expect_string(accounts_set_jid, value, "a_local@a_domain");

    expect_cons_show("Updated jid for account a_account: a_local@a_domain");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_jid_sets_resource(void **state)
{
    gchar *args[] = { "set", "a_account", "jid", "a_local@a_domain/a_resource", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_jid, account_name, "a_account");
    expect_string(accounts_set_jid, value, "a_local@a_domain");

    expect_cons_show("Updated jid for account a_account: a_local@a_domain");

    expect_string(accounts_set_resource, account_name, "a_account");
    expect_string(accounts_set_resource, value, "a_resource");

    expect_cons_show("Updated resource for account a_account: a_resource");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_server_sets_server(void **state)
{
    gchar *args[] = { "set", "a_account", "server", "a_server", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_server, account_name, "a_account");
    expect_string(accounts_set_server, value, "a_server");

    expect_cons_show("Updated server for account a_account: a_server");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_resource_sets_resource(void **state)
{
    gchar *args[] = { "set", "a_account", "resource", "a_resource", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_resource, account_name, "a_account");
    expect_string(accounts_set_resource, value, "a_resource");

    expect_cons_show("Updated resource for account a_account: a_resource");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_resource_sets_resource_with_online_message(void **state)
{
    gchar *args[] = { "set", "a_account", "resource", "a_resource", NULL };

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_resource, account_name, "a_account");
    expect_string(accounts_set_resource, value, "a_resource");

    expect_cons_show("Updated resource for account a_account: a_resource, reconnect to pick up the change.");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_password_sets_password(void **state)
{
    gchar *args[] = { "set", "a_account", "password", "a_password", NULL };
    ProfAccount *account = account_new("a_account", NULL, NULL, NULL,
    TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);


    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_get_account, name, "a_account");
    will_return(accounts_get_account, account);

    expect_string(accounts_set_password, account_name, "a_account");
    expect_string(accounts_set_password, value, "a_password");

    expect_cons_show("Updated password for account a_account");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_eval_password_sets_eval_password(void **state)
{
    gchar *args[] = { "set", "a_account", "eval_password", "a_password", NULL };
    ProfAccount *account = account_new("a_account", NULL, NULL, NULL,
    TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_get_account, name, "a_account");
    will_return(accounts_get_account, account);

    expect_string(accounts_set_eval_password, account_name, "a_account");
    expect_string(accounts_set_eval_password, value, "a_password");

    expect_cons_show("Updated eval_password for account a_account");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_password_when_eval_password_set(void **state) {
    gchar *args[] = { "set", "a_account", "password", "a_password", NULL };
    ProfAccount *account = account_new("a_account", NULL, NULL, "a_password",
    TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_get_account, name, "a_account");
    will_return(accounts_get_account, account);

    expect_cons_show("Cannot set password when eval_password is set.");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_eval_password_when_password_set(void **state) {
    gchar *args[] = { "set", "a_account", "eval_password", "a_password", NULL };
    ProfAccount *account = account_new("a_account", NULL, "a_password", NULL,
    TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_get_account, name, "a_account");
    will_return(accounts_get_account, account);

    expect_cons_show("Cannot set eval_password when password is set.");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_muc_sets_muc(void **state)
{
    gchar *args[] = { "set", "a_account", "muc", "a_muc", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_muc_service, account_name, "a_account");
    expect_string(accounts_set_muc_service, value, "a_muc");

    expect_cons_show("Updated muc service for account a_account: a_muc");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_nick_sets_nick(void **state)
{
    gchar *args[] = { "set", "a_account", "nick", "a_nick", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_muc_nick, account_name, "a_account");
    expect_string(accounts_set_muc_nick, value, "a_nick");

    expect_cons_show("Updated muc nick for account a_account: a_nick");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_show_message_for_missing_otr_policy(void **state)
{
    gchar *args[] = { "set", "a_account", "otr", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ACCOUNT);

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_show_message_for_invalid_otr_policy(void **state)
{
    gchar *args[] = { "set", "a_account", "otr", "bad_otr_policy", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_cons_show("OTR policy must be one of: manual, opportunistic or always.");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_otr_sets_otr(void **state)
{
    gchar *args[] = { "set", "a_account", "otr", "opportunistic", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_otr_policy, account_name, "a_account");
    expect_string(accounts_set_otr_policy, value, "opportunistic");

    expect_cons_show("Updated OTR policy for account a_account: opportunistic");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_status_shows_message_when_invalid_status(void **state)
{
    gchar *args[] = { "set", "a_account", "status", "bad_status", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_cons_show("Invalid status: bad_status");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_status_sets_status_when_valid(void **state)
{
    gchar *args[] = { "set", "a_account", "status", "away", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_login_presence, account_name, "a_account");
    expect_string(accounts_set_login_presence, value, "away");

    expect_cons_show("Updated login status for account a_account: away");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_status_sets_status_when_last(void **state)
{
    gchar *args[] = { "set", "a_account", "status", "last", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_login_presence, account_name, "a_account");
    expect_string(accounts_set_login_presence, value, "last");

    expect_cons_show("Updated login status for account a_account: last");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_invalid_presence_string_priority_shows_message(void **state)
{
    gchar *args[] = { "set", "a_account", "blah", "10", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_cons_show("Invalid property: blah");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_last_priority_shows_message(void **state)
{
    gchar *args[] = { "set", "a_account", "last", "10", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_cons_show("Invalid property: last");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_online_priority_sets_preference(void **state)
{
    gchar *args[] = { "set", "a_account", "online", "10", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_priority_online, account_name, "a_account");
    expect_value(accounts_set_priority_online, value, 10);

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_cons_show("Updated online priority for account a_account: 10");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_chat_priority_sets_preference(void **state)
{
    gchar *args[] = { "set", "a_account", "chat", "10", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_priority_chat, account_name, "a_account");
    expect_value(accounts_set_priority_chat, value, 10);

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_cons_show("Updated chat priority for account a_account: 10");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_away_priority_sets_preference(void **state)
{
    gchar *args[] = { "set", "a_account", "away", "10", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_priority_away, account_name, "a_account");
    expect_value(accounts_set_priority_away, value, 10);

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_cons_show("Updated away priority for account a_account: 10");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_xa_priority_sets_preference(void **state)
{
    gchar *args[] = { "set", "a_account", "xa", "10", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_priority_xa, account_name, "a_account");
    expect_value(accounts_set_priority_xa, value, 10);

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_cons_show("Updated xa priority for account a_account: 10");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_dnd_priority_sets_preference(void **state)
{
    gchar *args[] = { "set", "a_account", "dnd", "10", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_string(accounts_set_priority_dnd, account_name, "a_account");
    expect_value(accounts_set_priority_dnd, value, 10);

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_cons_show("Updated dnd priority for account a_account: 10");
    expect_cons_show("");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_priority_too_low_shows_message(void **state)
{
    gchar *args[] = { "set", "a_account", "online", "-150", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_cons_show("Value -150 out of range. Must be in -128..127.");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_priority_too_high_shows_message(void **state)
{
    gchar *args[] = { "set", "a_account", "online", "150", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_cons_show("Value 150 out of range. Must be in -128..127.");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_priority_when_not_number_shows_message(void **state)
{
    gchar *args[] = { "set", "a_account", "online", "abc", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_cons_show("Could not convert \"abc\" to a number.");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_priority_when_empty_shows_message(void **state)
{
    gchar *args[] = { "set", "a_account", "online", "", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_cons_show("Could not convert \"\" to a number.");

    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_set_priority_updates_presence_when_account_connected_with_presence(void **state)
{
    gchar *args[] = { "set", "a_account", "online", "10", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_any(accounts_set_priority_online, account_name);
    expect_any(accounts_set_priority_online, value);

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_any(accounts_get_last_presence, account_name);
    will_return(accounts_get_last_presence, RESOURCE_ONLINE);

    will_return(session_get_account_name, "a_account");

#ifdef HAVE_LIBGPGME
    ProfAccount *account = account_new("a_account", "a_jid", NULL, NULL, TRUE, NULL, 5222, "a_resource",
        NULL, NULL, 10, 10, 10, 10, 10, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    will_return(session_get_account_name, "a_account");
    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, account);
#endif
    expect_value(presence_send, status, RESOURCE_ONLINE);
    expect_value(presence_send, idle, 0);
    expect_value(presence_send, signed_status, NULL);

    expect_cons_show("Updated online priority for account a_account: 10");
    expect_cons_show("");


    gboolean result = cmd_account_set(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_clear_shows_usage_when_no_args(void **state)
{
    gchar *args[] = { "clear", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ACCOUNT);

    gboolean result = cmd_account_clear(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_clear_shows_usage_when_one_arg(void **state)
{
    gchar *args[] = { "clear", "a_account", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ACCOUNT);

    gboolean result = cmd_account_clear(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_clear_shows_message_when_account_doesnt_exist(void **state)
{
    gchar *args[] = { "clear", "a_account", "a_property", NULL };

    expect_string(accounts_account_exists, account_name, "a_account");
    will_return(accounts_account_exists, FALSE);

    expect_cons_show("Account a_account doesn't exist");
    expect_cons_show("");

    gboolean result = cmd_account_clear(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}

void cmd_account_clear_shows_message_when_invalid_property(void **state)
{
    gchar *args[] = { "clear", "a_account", "badproperty", NULL };

    expect_any(accounts_account_exists, account_name);
    will_return(accounts_account_exists, TRUE);

    expect_cons_show("Invalid property: badproperty");
    expect_cons_show("");

    gboolean result = cmd_account_clear(NULL, CMD_ACCOUNT, args);
    assert_true(result);
}
