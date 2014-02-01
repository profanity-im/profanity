#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "xmpp/xmpp.h"
#include "xmpp/mock_xmpp.h"

#include "command/commands.h"

#include "ui/mock_ui.h"

static void test_with_connection_status(jabber_conn_status_t status)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));

    mock_connection_status(status);
    expect_cons_show("You are not currently connected.");

    gboolean result = cmd_bookmark(NULL, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_shows_message_when_disconnected(void **state)
{
    test_with_connection_status(JABBER_DISCONNECTED);
}

void cmd_bookmark_shows_message_when_disconnecting(void **state)
{
    test_with_connection_status(JABBER_DISCONNECTING);
}

void cmd_bookmark_shows_message_when_connecting(void **state)
{
    test_with_connection_status(JABBER_CONNECTING);
}

void cmd_bookmark_shows_message_when_started(void **state)
{
    test_with_connection_status(JABBER_STARTED);
}

void cmd_bookmark_shows_message_when_undefined(void **state)
{
    test_with_connection_status(JABBER_UNDEFINED);
}

void cmd_bookmark_shows_usage_when_no_args(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { NULL };

    mock_connection_status(JABBER_CONNECTED);
    expect_cons_show("Usage: some usage");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}
