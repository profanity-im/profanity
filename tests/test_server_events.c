#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "event/server_events.h"
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

    sv_ev_contact_online("test1@server", resource, NULL);

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

    sv_ev_contact_online("test1@server", resource, NULL);

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

    sv_ev_contact_online("test1@server", resource, NULL);

    roster_clear();
}

void console_doesnt_show_dnd_presence_when_set_none(void **state)
{
    prefs_set_string(PREF_STATUSES_CONSOLE, "none");
    roster_init();
    roster_add("test1@server", "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_DND, NULL, 10);

    sv_ev_contact_online("test1@server", resource, NULL);

    roster_clear();
}

void console_doesnt_show_dnd_presence_when_set_online(void **state)
{
    prefs_set_string(PREF_STATUSES_CONSOLE, "online");
    roster_init();
    roster_add("test1@server", "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_DND, NULL, 10);

    sv_ev_contact_online("test1@server", resource, NULL);

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

    sv_ev_contact_online("test1@server", resource, NULL);

    roster_clear();
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
    sv_ev_contact_offline(barejid, resource, NULL);
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

    sv_ev_lost_connection();

    ChatSession *session1 = chat_session_get("bob@server.org");
    ChatSession *session2 = chat_session_get("steve@server.org");
    assert_null(session1);
    assert_null(session2);
}
