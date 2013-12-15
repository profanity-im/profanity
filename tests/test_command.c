#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <glib.h>

#include "xmpp/xmpp.h"
#include "ui/ui.h"
#include "command/commands.h"

static void test_with_connection_status(jabber_conn_status_t status)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    
    will_return(jabber_get_connection_status, status);
    expect_string(cons_show, msg, "You are not currently connected.");
    
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
    CommandHelp *help = malloc(sizeof(CommandHelp));
    ProfAccount *account = malloc(sizeof(ProfAccount));
    account->muc_service = "default_conf_server";
    gchar *args[] = { NULL }; 

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(jabber_get_account_name, "account_name");
    will_return(accounts_get_account, account);
    expect_string(iq_room_list_request, conferencejid, "default_conf_server");
    
    gboolean result = cmd_rooms(args, *help);

    assert_true(result);

    free(help);
    free(account);
}

void cmd_arg_used_when_passed(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "conf_server_arg" }; 

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    expect_string(iq_room_list_request, conferencejid, "conf_server_arg");
    
    gboolean result = cmd_rooms(args, *help);

    assert_true(result);

    free(help);
}
