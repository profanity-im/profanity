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
#include "muc.h"

static void test_with_connection_status(jabber_conn_status_t status)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));

    mock_connection_status(status);

    expect_cons_show("You are not currently connected.");

    gboolean result = cmd_join(NULL, *help);
    assert_true(result);

    free(help);
}

void cmd_join_shows_message_when_disconnecting(void **state)
{
    test_with_connection_status(JABBER_DISCONNECTING);
}

void cmd_join_shows_message_when_connecting(void **state)
{
    test_with_connection_status(JABBER_CONNECTING);
}

void cmd_join_shows_message_when_disconnected(void **state)
{
    test_with_connection_status(JABBER_DISCONNECTED);
}

void cmd_join_shows_message_when_undefined(void **state)
{
    test_with_connection_status(JABBER_UNDEFINED);
}

void cmd_join_shows_usage_when_no_args(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { NULL };

    mock_connection_status(JABBER_CONNECTED);

    expect_cons_show("Usage: some usage");
    expect_cons_show("");

    gboolean result = cmd_join(args, *help);
    assert_true(result);

    free(help);
}

void cmd_join_shows_error_message_when_invalid_room_jid(void **state)
{
    mock_cons_show_error();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "//@@/", NULL };

    mock_connection_status(JABBER_CONNECTED);

    expect_cons_show_error("Specified room has incorrect format.");
    expect_cons_show("");

    gboolean result = cmd_join(args, *help);
    assert_true(result);

    free(help);
}

void cmd_join_uses_account_mucservice_when_no_service_specified(void **state)
{
    char *account_name = "an_account";
    char *room = "room";
    char *nick = "bob";
    char *account_service = "conference.server.org";
    char *expected_room = "room@conference.server.org";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { room, "nick", nick, NULL };
    ProfAccount *account = account_new(account_name, "user@server.org", NULL,
        TRUE, NULL, 0, "laptop", NULL, NULL, 0, 0, 0, 0, 0, account_service, NULL);

    muc_init();

    mock_connection_status(JABBER_CONNECTED);
    mock_connection_account_name(account_name);
    mock_accounts_get_account();
    accounts_get_account_expect_and_return(account_name, account);

    mock_presence_join_room();
    presence_join_room_expect(expected_room, nick, NULL);
    ui_room_join_expect(expected_room);

    gboolean result = cmd_join(args, *help);
    assert_true(result);

    free(help);
}

void cmd_join_uses_supplied_nick(void **state)
{
    char *account_name = "an_account";
    char *room = "room@conf.server.org";
    char *nick = "bob";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { room, "nick", nick, NULL };
    ProfAccount *account = account_new(account_name, "user@server.org", NULL,
        TRUE, NULL, 0, "laptop", NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL);

    muc_init();

    mock_connection_status(JABBER_CONNECTED);
    mock_connection_account_name(account_name);
    mock_accounts_get_account();
    accounts_get_account_expect_and_return(account_name, account);

    mock_presence_join_room();
    presence_join_room_expect(room, nick, NULL);
    ui_room_join_expect(room);

    gboolean result = cmd_join(args, *help);
    assert_true(result);

    free(help);
}

void cmd_join_uses_account_nick_when_not_supplied(void **state)
{
    char *account_name = "an_account";
    char *room = "room@conf.server.org";
    char *account_nick = "a_nick";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { room, NULL };
    ProfAccount *account = account_new(account_name, "user@server.org", NULL,
        TRUE, NULL, 0, "laptop", NULL, NULL, 0, 0, 0, 0, 0, NULL, account_nick);

    muc_init();

    mock_connection_status(JABBER_CONNECTED);
    mock_connection_account_name(account_name);
    mock_accounts_get_account();
    accounts_get_account_expect_and_return(account_name, account);

    mock_presence_join_room();
    presence_join_room_expect(room, account_nick, NULL);
    ui_room_join_expect(room);

    gboolean result = cmd_join(args, *help);
    assert_true(result);

    free(help);
}

void cmd_join_uses_password_when_supplied(void **state)
{
    char *account_name = "an_account";
    char *room = "room";
    char *password = "a_password";
    char *account_nick = "a_nick";
    char *account_service = "a_service";
    char *expected_room = "room@a_service";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { room, "password", password, NULL };
    ProfAccount *account = account_new(account_name, "user@server.org", NULL,
        TRUE, NULL, 0, "laptop", NULL, NULL, 0, 0, 0, 0, 0, account_service, account_nick);

    muc_init();

    mock_connection_status(JABBER_CONNECTED);
    mock_connection_account_name(account_name);
    mock_accounts_get_account();
    accounts_get_account_expect_and_return(account_name, account);

    mock_presence_join_room();
    presence_join_room_expect(expected_room, account_nick, password);
    ui_room_join_expect(expected_room);

    gboolean result = cmd_join(args, *help);
    assert_true(result);

    free(help);
}
