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

#define CMD_SUB "/sub"

void cmd_sub_shows_message_when_not_connected(void **state)
{
    gchar *args[] = { NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_cons_show("You are currently not connected.");

    gboolean result = cmd_sub(NULL, CMD_SUB, args);
    assert_true(result);
}

void cmd_sub_shows_usage_when_no_arg(void **state)
{
    gchar *args[] = { NULL };

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_SUB);

    gboolean result = cmd_sub(NULL, CMD_SUB, args);
    assert_true(result);
}
