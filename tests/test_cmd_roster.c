#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "ui/ui.h"
#include "ui/mock_ui.h"

#include "xmpp/xmpp.h"
#include "xmpp/mock_xmpp.h"

#include "roster_list.h"
#include "command/commands.h"

static void test_with_connection_status(jabber_conn_status_t status)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));

    mock_connection_status(status);

    expect_cons_show("You are not currently connected.");

    gboolean result = cmd_roster(NULL, *help);
    assert_true(result);

    free(help);
}

void cmd_roster_shows_message_when_disconnecting(void **state)
{
    test_with_connection_status(JABBER_DISCONNECTING);
}

void cmd_roster_shows_message_when_connecting(void **state)
{
    test_with_connection_status(JABBER_CONNECTING);
}

void cmd_roster_shows_message_when_disconnected(void **state)
{
    test_with_connection_status(JABBER_DISCONNECTED);
}

void cmd_roster_shows_message_when_undefined(void **state)
{
    test_with_connection_status(JABBER_UNDEFINED);
}

void cmd_roster_shows_roster_when_no_args(void **state)
{
    mock_cons_show_roster();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { NULL };

    mock_connection_status(JABBER_CONNECTED);
    roster_init();
    roster_add("bob@server.org", "bob", NULL, "both", FALSE);
    GSList *roster = roster_get_contacts();
    cons_show_roster_expect(roster);

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    free(help);
    roster_free();
}

void cmd_roster_add_shows_message_when_no_jid(void)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "add", NULL };

    mock_connection_status(JABBER_CONNECTED);
    expect_cons_show("Usage: some usage");

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    free(help);
}

void cmd_roster_add_sends_roster_add_request(void)
{
    char *jid = "bob@server.org";
    char *nick = "bob";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", jid, nick, NULL };

    mock_roster_send_add_new();
    mock_connection_status(JABBER_CONNECTED);
    roster_send_add_new_expect(jid, nick);

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    free(help);
}
