#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include "chat_session.h"
#include "command/commands.h"
#include "xmpp/xmpp.h"
#include "roster_list.h"

#include "ui/stub_ui.h"

void clears_chat_sessions(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));

    chat_sessions_init();
    chat_session_on_recipient_activity("bob@server.org", "laptop");
    roster_init();

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(jabber_get_fulljid, "myjid@myserver.com");
    expect_any_cons_show();

    gboolean result = cmd_disconnect(NULL, *help);

    assert_true(result);

    ChatSession *session = chat_session_get("bob@server.org");
    assert_null(session);
    free(help);
}