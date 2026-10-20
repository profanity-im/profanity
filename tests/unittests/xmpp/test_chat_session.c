#include <string.h>
#include "prof_cmocka.h"
#include <stdlib.h>

#include "xmpp/chat_session.h"

void
chat_session_get__returns__null_when_no_session(void** state)
{
    ChatSession* session = chat_session_get("somejid@server.org");
    assert_null(session);
}

void
chat_session_recipient_active__updates__new_session(void** state)
{
    const gchar* barejid = "myjid@server.org";
    const gchar* resource = "tablet";

    chat_session_recipient_active(barejid, resource, FALSE);
    ChatSession* session = chat_session_get(barejid);

    assert_non_null(session);
    assert_string_equal(session->resource, resource);
}

void
chat_session_recipient_active__updates__replace_resource(void** state)
{
    const gchar* barejid = "myjid@server.org";
    const gchar* resource1 = "tablet";
    const gchar* resource2 = "mobile";

    chat_session_recipient_active(barejid, resource1, FALSE);
    chat_session_recipient_active(barejid, resource2, FALSE);
    ChatSession* session = chat_session_get(barejid);

    assert_string_equal(session->resource, resource2);
}

void
chat_session_remove__updates__session_removed(void** state)
{
    const gchar* barejid = "myjid@server.org";
    const gchar* resource1 = "laptop";

    chat_session_recipient_active(barejid, resource1, FALSE);
    chat_session_remove(barejid);
    ChatSession* session = chat_session_get(barejid);

    assert_null(session);
}
