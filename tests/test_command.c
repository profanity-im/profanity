#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include "xmpp/xmpp.h"
#include "ui/ui.h"
#include "command/command.h"

void cmd_rooms_shows_message_when_not_connected(void **state)
{
    will_return(jabber_get_connection_status, JABBER_DISCONNECTED);
    expect_string(cons_show, msg, "You are not currently connected.");
    CommandHelp *help = malloc(sizeof(CommandHelp));

    assert_true(_cmd_rooms(NULL, *help));

    free(help);
}

