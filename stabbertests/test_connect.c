#include <glib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include <stabber.h>

#include "proftest.h"
#include "xmpp/xmpp.h"
#include "ui/stub_ui.h"
#include "command/command.h"

void
connect_with_jid(void **state)
{
    char *connect = "/connect stabber@localhost port 5230";
    char *password = "password";

    if (stbbr_start(5230) != 0) {
        assert_true(FALSE);
        return;
    }

    stbbr_auth_passwd(password);
    will_return(ui_ask_password, strdup(password));

    expect_cons_show("Connecting as stabber@localhost");

    cmd_process_input(strdup(connect));
    prof_process_xmpp();

    jabber_conn_status_t status = jabber_get_connection_status();
    assert_true(status == JABBER_CONNECTED);

    stbbr_stop();
}
