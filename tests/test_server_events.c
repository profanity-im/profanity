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
#include "ui/mock_ui.h"

void console_doesnt_show_online_presence_when_set_none(void **state)
{
    mock_cons_show_contact_online();
    stub_ui_chat_win_contact_online();
    prefs_set_string(PREF_STATUSES_CONSOLE, "none");
    roster_init();
    roster_add("test1@server", "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_ONLINE, NULL, 10, "caps");

    handle_contact_online("test1@server", resource, NULL);

    roster_clear();
}

void console_shows_online_presence_when_set_online(void **state)
{
    mock_cons_show_contact_online();
    stub_ui_chat_win_contact_online();
    prefs_set_string(PREF_STATUSES_CONSOLE, "online");
    roster_init();
    roster_add("test1@server", "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_ONLINE, NULL, 10, "caps");
    PContact contact = roster_get_contact("test1@server");

    expect_cons_show_contact_online(contact, resource, NULL);

    handle_contact_online("test1@server", resource, NULL);

    roster_clear();
}

void console_shows_online_presence_when_set_all(void **state)
{
    mock_cons_show_contact_online();
    stub_ui_chat_win_contact_online();
    prefs_set_string(PREF_STATUSES_CONSOLE, "all");
    roster_init();
    roster_add("test1@server", "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_ONLINE, NULL, 10, "caps");
    PContact contact = roster_get_contact("test1@server");

    expect_cons_show_contact_online(contact, resource, NULL);

    handle_contact_online("test1@server", resource, NULL);

    roster_clear();
}

void console_doesnt_show_dnd_presence_when_set_none(void **state)
{
    mock_cons_show_contact_online();
    stub_ui_chat_win_contact_online();
    prefs_set_string(PREF_STATUSES_CONSOLE, "none");
    roster_init();
    roster_add("test1@server", "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_DND, NULL, 10, "caps");

    handle_contact_online("test1@server", resource, NULL);

    roster_clear();
}

void console_doesnt_show_dnd_presence_when_set_online(void **state)
{
    mock_cons_show_contact_online();
    stub_ui_chat_win_contact_online();
    prefs_set_string(PREF_STATUSES_CONSOLE, "online");
    roster_init();
    roster_add("test1@server", "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_DND, NULL, 10, "caps");

    handle_contact_online("test1@server", resource, NULL);

    roster_clear();
}

void console_shows_dnd_presence_when_set_all(void **state)
{
    mock_cons_show_contact_online();
    stub_ui_chat_win_contact_online();
    prefs_set_string(PREF_STATUSES_CONSOLE, "all");
    roster_init();
    roster_add("test1@server", "bob", NULL, "both", FALSE);
    Resource *resource = resource_new("resource", RESOURCE_ONLINE, NULL, 10, "caps");
    PContact contact = roster_get_contact("test1@server");

    expect_cons_show_contact_online(contact, resource, NULL);

    handle_contact_online("test1@server", resource, NULL);

    roster_clear();
}

void handle_message_stanza_error_when_no_from(void **state)
{
    char *err_msg = "Some error.";

    expect_ui_handle_error(err_msg);

    handle_message_error(NULL, "cancel", err_msg);
}

void handle_message_stanza_error_from_cancel(void **state)
{
    char *err_msg = "Some error.";
    char *from = "bob@server.com";
    prefs_set_boolean(PREF_STATES, FALSE);
    chat_sessions_init();

    expect_ui_handle_recipient_not_found(from, err_msg);

    handle_message_error(from, "cancel", err_msg);
}

void handle_message_stanza_error_from_cancel_disables_chat_session(void **state)
{
    char *err_msg = "Some error.";
    char *from = "bob@server.com";
    stub_ui_handle_recipient_not_found();
    prefs_set_boolean(PREF_STATES, TRUE);
    chat_sessions_init();
    chat_session_start(from, TRUE);

    handle_message_error(from, "cancel", err_msg);
    gboolean chat_session_supported = chat_session_get_recipient_supports(from);

    assert_false(chat_session_supported);
    chat_sessions_clear();
}

void handle_message_stanza_error_from_no_type(void **state)
{
    char *err_msg = "Some error.";
    char *from = "bob@server.com";

    expect_ui_handle_recipient_error(from, err_msg);

    handle_message_error(from, NULL, err_msg);
}
