#include "prof_cmocka.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "xmpp/xmpp.h"

#include "ui/ui.h"
#include "ui/stub_ui.h"

#include "config/accounts.h"

#include "command/cmd_funcs.h"
#include "xmpp/muc.h"

#define CMD_JOIN "/join"

static void
test_with_connection_status(jabber_conn_status_t status)
{
    will_return(connection_get_status, status);

    expect_cons_show("You are not currently connected.");

    gboolean result = cmd_join(NULL, CMD_JOIN, NULL);
    assert_true(result);
}

void
cmd_join_shows_message_when_disconnecting(void** state)
{
    test_with_connection_status(JABBER_DISCONNECTING);
}

void
cmd_join_shows_message_when_connecting(void** state)
{
    test_with_connection_status(JABBER_CONNECTING);
}

void
cmd_join_shows_message_when_disconnected(void** state)
{
    test_with_connection_status(JABBER_DISCONNECTED);
}

void
cmd_join_shows_error_message_when_invalid_room_jid(void** state)
{
    gchar* args[] = { "//@@/", NULL };

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_cons_show_error("Specified room has incorrect format.");
    expect_cons_show("");

    gboolean result = cmd_join(NULL, CMD_JOIN, args);
    assert_true(result);
}

void
cmd_join_uses_account_mucservice_when_no_service_specified(void** state)
{
    gchar* account_name = g_strdup("an_account");
    char* room = "room";
    char* nick = "bob";
    gchar* account_service = g_strdup("conference.server.org");
    char* expected_room = "room@conference.server.org";
    gchar* args[] = { room, "nick", nick, NULL };
    ProfAccount* account = account_new(account_name, g_strdup("user@server.org"), NULL, NULL,
                                       TRUE, NULL, 0, g_strdup("laptop"), NULL, NULL, 0, 0, 0, 0, 0, account_service, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0);

    will_return(connection_get_status, JABBER_CONNECTED);
    will_return(session_get_account_name, account_name);

    expect_string(accounts_get_account, name, account_name);
    will_return(accounts_get_account, account);

    expect_string(presence_join_room, room, expected_room);
    expect_string(presence_join_room, nick, nick);
    expect_value(presence_join_room, passwd, NULL);

    gboolean result = cmd_join(NULL, CMD_JOIN, args);
    assert_true(result);
}

void
cmd_join_uses_supplied_nick(void** state)
{
    gchar* account_name = g_strdup("an_account");
    char* room = "room@conf.server.org";
    char* nick = "bob";
    gchar* args[] = { room, "nick", nick, NULL };
    ProfAccount* account = account_new(account_name, g_strdup("user@server.org"), NULL, NULL,
                                       TRUE, NULL, 0, g_strdup("laptop"), NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0);

    will_return(connection_get_status, JABBER_CONNECTED);
    will_return(session_get_account_name, account_name);

    expect_string(accounts_get_account, name, account_name);
    will_return(accounts_get_account, account);

    expect_string(presence_join_room, room, room);
    expect_string(presence_join_room, nick, nick);
    expect_value(presence_join_room, passwd, NULL);

    gboolean result = cmd_join(NULL, CMD_JOIN, args);
    assert_true(result);
}

void
cmd_join_uses_account_nick_when_not_supplied(void** state)
{
    gchar* account_name = g_strdup("an_account");
    char* room = "room2@conf.server.org";
    gchar* account_nick = g_strdup("a_nick");
    gchar* args[] = { room, NULL };
    ProfAccount* account = account_new(account_name, g_strdup("user@server.org"), NULL, NULL,
                                       TRUE, NULL, 0, g_strdup("laptop"), NULL, NULL, 0, 0, 0, 0, 0, NULL, account_nick, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0);

    will_return(connection_get_status, JABBER_CONNECTED);
    will_return(session_get_account_name, account_name);

    expect_string(accounts_get_account, name, account_name);
    will_return(accounts_get_account, account);

    expect_string(presence_join_room, room, room);
    expect_string(presence_join_room, nick, account_nick);
    expect_value(presence_join_room, passwd, NULL);

    gboolean result = cmd_join(NULL, CMD_JOIN, args);
    assert_true(result);
}

void
cmd_join_uses_password_when_supplied(void** state)
{
    gchar* account_name = g_strdup("an_account");
    char* room = "room";
    char* password = "a_password";
    gchar* account_nick = g_strdup("a_nick");
    gchar* account_service = g_strdup("a_service");
    char* expected_room = "room@a_service";
    gchar* args[] = { room, "password", password, NULL };
    ProfAccount* account = account_new(account_name, g_strdup("user@server.org"), NULL, NULL,
                                       TRUE, NULL, 0, g_strdup("laptop"), NULL, NULL, 0, 0, 0, 0, 0, account_service, account_nick, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0);

    will_return(connection_get_status, JABBER_CONNECTED);
    will_return(session_get_account_name, account_name);

    expect_string(accounts_get_account, name, account_name);
    will_return(accounts_get_account, account);

    expect_string(presence_join_room, room, expected_room);
    expect_string(presence_join_room, nick, account_nick);
    expect_value(presence_join_room, passwd, password);

    gboolean result = cmd_join(NULL, CMD_JOIN, args);
    assert_true(result);
}
