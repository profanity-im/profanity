#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <glib.h>

#include "xmpp/xmpp.h"
#include "xmpp/mock_xmpp.h"

#include "ui/ui.h"
#include "ui/mock_ui.h"

#include "config/accounts.h"
#include "config/mock_accounts.h"

#include "command/commands.h"

static void test_with_connection_status(jabber_conn_status_t status)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));

    mock_connection_status(status);
    expect_cons_show("You are not currently connected.");

    gboolean result = cmd_rooms(NULL, *help);
    assert_true(result);

    free(help);
}

void cmd_rooms_shows_message_when_disconnected(void **state)
{
    test_with_connection_status(JABBER_DISCONNECTED);
}

void cmd_rooms_shows_message_when_disconnecting(void **state)
{
    test_with_connection_status(JABBER_DISCONNECTING);
}

void cmd_rooms_shows_message_when_connecting(void **state)
{
    test_with_connection_status(JABBER_CONNECTING);
}

void cmd_rooms_shows_message_when_started(void **state)
{
    test_with_connection_status(JABBER_STARTED);
}

void cmd_rooms_shows_message_when_undefined(void **state)
{
    test_with_connection_status(JABBER_UNDEFINED);
}

void cmd_rooms_uses_account_default_when_no_arg(void **state)
{
    mock_accounts_get_account();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    ProfAccount *account = malloc(sizeof(ProfAccount));
    account->muc_service = "default_conf_server";
    gchar *args[] = { NULL };

    mock_connection_status(JABBER_CONNECTED);
    mock_connection_account_name("account_name");

    accounts_get_account_return(account);

    expect_room_list_request("default_conf_server");

    gboolean result = cmd_rooms(args, *help);

    assert_true(result);

    free(help);
    free(account);
}

void cmd_rooms_arg_used_when_passed(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "conf_server_arg" };

    mock_connection_status(JABBER_CONNECTED);

    expect_room_list_request("conf_server_arg");

    gboolean result = cmd_rooms(args, *help);

    assert_true(result);

    free(help);
}
