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
#include "command/commands.h"

#define CMD_ROOMS "/rooms"

static void test_with_connection_status(jabber_conn_status_t status)
{
    will_return(connection_get_status, status);

    expect_cons_show("You are not currently connected.");

    gboolean result = cmd_rooms(NULL, CMD_ROOMS, NULL);
    assert_true(result);
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

void cmd_rooms_uses_account_default_when_no_arg(void **state)
{
    gchar *args[] = { NULL };

    ProfAccount *account = account_new("testaccount", NULL, NULL, NULL, TRUE, NULL, 0, NULL, NULL, NULL,
        0, 0, 0, 0, 0, strdup("default_conf_server"), NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    will_return(connection_get_status, JABBER_CONNECTED);
    will_return(session_get_account_name, "account_name");
    expect_any(accounts_get_account, name);
    will_return(accounts_get_account, account);

    expect_string(iq_room_list_request, conferencejid, "default_conf_server");

    gboolean result = cmd_rooms(NULL, CMD_ROOMS, args);
    assert_true(result);
}

void cmd_rooms_arg_used_when_passed(void **state)
{
    gchar *args[] = { "conf_server_arg" };

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(iq_room_list_request, conferencejid, "conf_server_arg");

    gboolean result = cmd_rooms(NULL, CMD_ROOMS, args);
    assert_true(result);
}
