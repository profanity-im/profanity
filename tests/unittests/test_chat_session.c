#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include "xmpp/chat_session.h"

void
returns_false_when_chat_session_does_not_exist(void** state)
{
    ChatSession* session = chat_session_get("somejid@server.org");
    assert_null(session);
}

void
creates_chat_session_on_recipient_activity(void** state)
{
    char* barejid = "myjid@server.org";
    char* resource = "tablet";

    chat_session_recipient_active(barejid, resource, FALSE);
    ChatSession* session = chat_session_get(barejid);

    assert_non_null(session);
    assert_string_equal(session->resource, resource);
}

void
replaces_chat_session_on_recipient_activity_with_different_resource(void** state)
{
    char* barejid = "myjid@server.org";
    char* resource1 = "tablet";
    char* resource2 = "mobile";

    chat_session_recipient_active(barejid, resource1, FALSE);
    chat_session_recipient_active(barejid, resource2, FALSE);
    ChatSession* session = chat_session_get(barejid);

    assert_string_equal(session->resource, resource2);
}

void
removes_chat_session(void** state)
{
    char* barejid = "myjid@server.org";
    char* resource1 = "laptop";

    chat_session_recipient_active(barejid, resource1, FALSE);
    chat_session_remove(barejid);
    ChatSession* session = chat_session_get(barejid);

    assert_null(session);
}
