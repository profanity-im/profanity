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

#include "command/commands.h"

void cmd_sub_shows_message_when_not_connected(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { NULL };

    will_return(jabber_get_connection_status, JABBER_DISCONNECTED);

    expect_cons_show("You are currently not connected.");

    gboolean result = cmd_sub(args, *help);
    assert_true(result);

    free(help);
}

void cmd_sub_shows_usage_when_no_arg(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "Some usage";
    gchar *args[] = { NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);

    expect_cons_show("Usage: Some usage");

    gboolean result = cmd_sub(args, *help);
    assert_true(result);

    free(help);
}
