#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "server_events.h"
#include "roster_list.h"
#include "chat_session.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/stub_ui.h"
#include "muc.h"

void console_doesnt_show_online_presence_when_set_none(void **state)
{
    prefs_set_string(PREF_STATUSES_CONSOLE, "none");
    roster_init();
    roster_add("test1@server", "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_ONLINE, NULL, 10);

    handle_contact_online("test1@server", resource, NULL);

    roster_clear();
}

void console_shows_online_presence_when_set_online(void **state)
{
    prefs_set_string(PREF_STATUSES_CONSOLE, "online");
    roster_init();
    roster_add("test1@server", "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_ONLINE, NULL, 10);
    PContact contact = roster_get_contact("test1@server");

    expect_memory(cons_show_contact_online, contact, contact, sizeof(contact));
    expect_memory(cons_show_contact_online, resource, resource, sizeof(resource));
    expect_value(cons_show_contact_online, last_activity, NULL);

    handle_contact_online("test1@server", resource, NULL);

    roster_clear();
}

void console_shows_online_presence_when_set_all(void **state)
{
    prefs_set_string(PREF_STATUSES_CONSOLE, "all");
    roster_init();
    roster_add("test1@server", "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_ONLINE, NULL, 10);
    PContact contact = roster_get_contact("test1@server");

    expect_memory(cons_show_contact_online, contact, contact, sizeof(contact));
    expect_memory(cons_show_contact_online, resource, resource, sizeof(resource));
    expect_value(cons_show_contact_online, last_activity, NULL);

    handle_contact_online("test1@server", resource, NULL);

    roster_clear();
}

void console_doesnt_show_dnd_presence_when_set_none(void **state)
{
    prefs_set_string(PREF_STATUSES_CONSOLE, "none");
    roster_init();
    roster_add("test1@server", "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_DND, NULL, 10);

    handle_contact_online("test1@server", resource, NULL);

    roster_clear();
}

void console_doesnt_show_dnd_presence_when_set_online(void **state)
{
    prefs_set_string(PREF_STATUSES_CONSOLE, "online");
    roster_init();
    roster_add("test1@server", "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_DND, NULL, 10);

    handle_contact_online("test1@server", resource, NULL);

    roster_clear();
}

void console_shows_dnd_presence_when_set_all(void **state)
{
    prefs_set_string(PREF_STATUSES_CONSOLE, "all");
    roster_init();
    roster_add("test1@server", "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_ONLINE, NULL, 10);
    PContact contact = roster_get_contact("test1@server");

    expect_memory(cons_show_contact_online, contact, contact, sizeof(contact));
    expect_memory(cons_show_contact_online, resource, resource, sizeof(resource));
    expect_value(cons_show_contact_online, last_activity, NULL);

    handle_contact_online("test1@server", resource, NULL);

    roster_clear();
}

void handle_message_error_when_no_recipient(void **state)
{
    char *err_msg = "Some error.";
    char *from = NULL;
    char *type = "cancel";

    expect_string(ui_handle_error, err_msg, err_msg);

    handle_message_error(from, type, err_msg);
}

void handle_message_error_when_recipient_cancel(void **state)
{
    char *err_msg = "Some error.";
    char *from = "bob@server.com";
    char *type = "cancel";

    prefs_set_boolean(PREF_STATES, FALSE);
    chat_sessions_init();

    handle_message_error(from, type, err_msg);
}

void handle_message_error_when_recipient_cancel_disables_chat_session(void **state)
{
    char *err_msg = "Some error.";
    char *barejid = "bob@server.com";
    char *resource = "resource";
    char *type = "cancel";

    prefs_set_boolean(PREF_STATES, TRUE);
    chat_sessions_init();
    chat_session_recipient_active(barejid, resource, FALSE);

    handle_message_error(barejid, type, err_msg);
    ChatSession *session = chat_session_get(barejid);

    assert_null(session);
    chat_sessions_clear();
}

void handle_message_error_when_recipient_and_no_type(void **state)
{
    char *err_msg = "Some error.";
    char *from = "bob@server.com";
    char *type = NULL;

    expect_string(ui_handle_recipient_error, recipient, from);
    expect_string(ui_handle_recipient_error, err_msg, err_msg);

    handle_message_error(from, type, err_msg);
}

void handle_presence_error_when_no_recipient(void **state)
{
    char *err_msg = "Some error.";
    char *from = NULL;
    char *type = NULL;

    expect_string(ui_handle_error, err_msg, err_msg);

    handle_presence_error(from, type, err_msg);
}

void handle_presence_error_when_from_recipient(void **state)
{
    char *err_msg = "Some error.";
    char *from = "bob@server.com";
    char *type = NULL;

    expect_string(ui_handle_recipient_error, recipient, from);
    expect_string(ui_handle_recipient_error, err_msg, err_msg);

    handle_presence_error(from, type, err_msg);
}

void handle_offline_removes_chat_session(void **state)
{
    chat_sessions_init();
    char *barejid = "friend@server.chat.com";
    char *resource = "home";
    roster_init();
    roster_add(barejid, "bob", NULL, "both", FALSE);
    Resource *resourcep = resource_new(resource, RESOURCE_ONLINE, NULL, 10);
    roster_update_presence(barejid, resourcep, NULL);
    chat_session_recipient_active(barejid, resource, FALSE);
    handle_contact_offline(barejid, resource, NULL);
    ChatSession *session = chat_session_get(barejid);

    assert_null(session);

    roster_clear();
    chat_sessions_clear();
}

void lost_connection_clears_chat_sessions(void **state)
{
    chat_sessions_init();
    chat_session_recipient_active("bob@server.org", "laptop", FALSE);
    chat_session_recipient_active("steve@server.org", "mobile", FALSE);
    expect_any_cons_show_error();

    handle_lost_connection();

    ChatSession *session1 = chat_session_get("bob@server.org");
    ChatSession *session2 = chat_session_get("steve@server.org");
    assert_null(session1);
    assert_null(session2);
}
