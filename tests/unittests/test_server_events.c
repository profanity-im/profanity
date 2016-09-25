#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "event/server_events.h"
#include "xmpp/roster_list.h"
#include "xmpp/chat_session.h"
#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/stub_ui.h"
#include "xmpp/muc.h"

void console_shows_online_presence_when_set_online(void **state)
{
    prefs_set_string(PREF_STATUSES_CONSOLE, "online");
    plugins_init();
    roster_create();
    char *barejid = "test1@server";
    roster_add(barejid, "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_ONLINE, NULL, 10);

    expect_memory(ui_contact_online, barejid, barejid, sizeof(barejid));
    expect_memory(ui_contact_online, resource, resource, sizeof(resource));
    expect_value(ui_contact_online, last_activity, NULL);

    sv_ev_contact_online(barejid, resource, NULL, NULL);

    roster_destroy();
    plugins_shutdown();
}

void console_shows_online_presence_when_set_all(void **state)
{
    prefs_set_string(PREF_STATUSES_CONSOLE, "all");
    plugins_init();
    roster_create();
    char *barejid = "test1@server";
    roster_add(barejid, "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_ONLINE, NULL, 10);

    expect_memory(ui_contact_online, barejid, barejid, sizeof(barejid));
    expect_memory(ui_contact_online, resource, resource, sizeof(resource));
    expect_value(ui_contact_online, last_activity, NULL);

    sv_ev_contact_online(barejid, resource, NULL, NULL);

    roster_destroy();
    plugins_shutdown();
}

void console_shows_dnd_presence_when_set_all(void **state)
{
    prefs_set_string(PREF_STATUSES_CONSOLE, "all");
    plugins_init();
    roster_create();
    char *barejid = "test1@server";
    roster_add(barejid, "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_ONLINE, NULL, 10);

    expect_memory(ui_contact_online, barejid, barejid, sizeof(barejid));
    expect_memory(ui_contact_online, resource, resource, sizeof(resource));
    expect_value(ui_contact_online, last_activity, NULL);

    sv_ev_contact_online(barejid, resource, NULL, NULL);

    roster_destroy();
    plugins_shutdown();
}

void handle_offline_removes_chat_session(void **state)
{
    plugins_init();
    roster_create();
    chat_sessions_init();
    char *barejid = "friend@server.chat.com";
    char *resource = "home";
    roster_add(barejid, "bob", NULL, "both", FALSE);
    Resource *resourcep = resource_new(resource, RESOURCE_ONLINE, NULL, 10);
    roster_update_presence(barejid, resourcep, NULL);
    chat_session_recipient_active(barejid, resource, FALSE);
    ProfConsoleWin *console = malloc(sizeof(ProfConsoleWin));
    will_return(win_create_console, &console->window);
    wins_init();
    sv_ev_contact_offline(barejid, resource, NULL);
    ChatSession *session = chat_session_get(barejid);

    assert_null(session);

    roster_destroy();
    chat_sessions_clear();
    plugins_shutdown();
}

void lost_connection_clears_chat_sessions(void **state)
{
    roster_create();
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
