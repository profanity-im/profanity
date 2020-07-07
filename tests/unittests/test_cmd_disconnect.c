#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "command/cmd_funcs.h"
#include "xmpp/chat_session.h"
#include "xmpp/roster_list.h"
#include "xmpp/xmpp.h"

#include "ui/stub_ui.h"

#define CMD_DISCONNECT "/disconnect"

void
clears_chat_sessions(void** state)
{
    chat_sessions_init();
    roster_create();
    chat_session_recipient_active("bob@server.org", "laptop", FALSE);
    chat_session_recipient_active("mike@server.org", "work", FALSE);

    will_return(connection_get_status, JABBER_CONNECTED);
    will_return(connection_get_barejid, strdup("myjid@myserver.com"));
    expect_any_cons_show();

    gboolean result = cmd_disconnect(NULL, CMD_DISCONNECT, NULL);

    assert_true(result);

    ChatSession* session1 = chat_session_get("bob@server.org");
    ChatSession* session2 = chat_session_get("mike@server.org");
    assert_null(session1);
    assert_null(session2);
}
