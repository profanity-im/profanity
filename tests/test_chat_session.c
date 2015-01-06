#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>

#include "chat_session.h"

void returns_false_when_chat_session_does_not_exist(void **state)
{
    gboolean result = chat_session_exists("somejid@server.org");
    assert_false(result);
}

void creates_chat_session_on_message_send(void **state)
{
    char *barejid = "myjid@server.org";

    chat_session_on_message_send(barejid);
    gboolean exists = chat_session_exists(barejid);

    assert_true(exists);
}

void creates_chat_session_on_activity(void **state)
{
    char *barejid = "myjid@server.org";

    chat_session_on_activity(barejid);
    gboolean exists = chat_session_exists(barejid);

    assert_true(exists);
}

void returns_null_resource_for_new_session(void **state)
{
    char *barejid = "myjid@server.org";

    chat_session_on_message_send(barejid);
    char *resource = chat_session_get_resource(barejid);

    assert_null(resource);
}

void returns_true_send_states_for_new_session(void **state)
{
    char *barejid = "myjid@server.org";

    chat_session_on_message_send(barejid);
    gboolean send_states = chat_session_send_states(barejid);

    assert_true(send_states);
}

void sets_resource_on_incoming_message(void **state)
{
    char *barejid = "myjid@server.org";
    char *expected_resource = "laptop";

    chat_session_on_message_send(barejid);
    chat_session_on_incoming_message(barejid, expected_resource, FALSE);
    char *actual_resource = chat_session_get_resource(barejid);

    assert_string_equal(expected_resource, actual_resource);
}

void sets_send_states_on_incoming_message(void **state)
{
    char *barejid = "myjid@server.org";

    chat_session_on_message_send(barejid);
    chat_session_on_incoming_message(barejid, "resource", TRUE);
    gboolean send_states = chat_session_send_states(barejid);

    assert_true(send_states);
}

void replaces_chat_session_when_new_resource(void **state)
{
    char *barejid = "myjid@server.org";
    char *first_resource = "laptop";
    char *second_resource = "mobile";

    chat_session_on_message_send(barejid);
    chat_session_on_incoming_message(barejid, first_resource, TRUE);
    chat_session_on_incoming_message(barejid, second_resource, TRUE);
    char *actual_resource = chat_session_get_resource(barejid);

    assert_string_equal(second_resource, actual_resource);
}

void removes_chat_session_on_window_close(void **state)
{
    char *barejid = "myjid@server.org";

    chat_session_on_message_send(barejid);
    chat_session_on_window_close(barejid);
    gboolean exists = chat_session_exists(barejid);

    assert_false(exists);
}

void removes_chat_session_on_cancel_for_barejid(void **state)
{
    char *barejid = "myjid@server.org";

    chat_session_on_message_send(barejid);
    chat_session_on_cancel(barejid);
    gboolean exists = chat_session_exists(barejid);

    assert_false(exists);
}

void removes_chat_session_on_cancel_for_fulljid(void **state)
{
    char *barejid = "myjid@server.org";
    char *fulljid = "myjid@server.org/desktop";

    chat_session_on_message_send(barejid);
    chat_session_on_cancel(fulljid);
    gboolean exists = chat_session_exists(barejid);

    assert_false(exists);
}