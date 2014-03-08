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
#include "muc.h"

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

void handle_message_error_when_no_recipient(void **state)
{
    char *err_msg = "Some error.";
    char *from = NULL;
    char *type = "cancel";

    expect_ui_handle_error(err_msg);

    handle_message_error(from, type, err_msg);
}

void handle_message_error_when_recipient_cancel(void **state)
{
    char *err_msg = "Some error.";
    char *from = "bob@server.com";
    char *type = "cancel";

    prefs_set_boolean(PREF_STATES, FALSE);
    chat_sessions_init();

    expect_ui_handle_recipient_not_found(from, err_msg);

    handle_message_error(from, type, err_msg);
}

void handle_message_error_when_recipient_cancel_disables_chat_session(void **state)
{
    char *err_msg = "Some error.";
    char *from = "bob@server.com";
    char *type = "cancel";

    stub_ui_handle_recipient_not_found();
    prefs_set_boolean(PREF_STATES, TRUE);
    chat_sessions_init();
    chat_session_start(from, TRUE);

    handle_message_error(from, type, err_msg);
    gboolean chat_session_supported = chat_session_get_recipient_supports(from);

    assert_false(chat_session_supported);
    chat_sessions_clear();
}

void handle_message_error_when_recipient_and_no_type(void **state)
{
    char *err_msg = "Some error.";
    char *from = "bob@server.com";
    char *type = NULL;

    expect_ui_handle_recipient_error(from, err_msg);

    handle_message_error(from, type, err_msg);
}

void handle_presence_error_when_no_recipient(void **state)
{
    char *err_msg = "Some error.";
    char *from = NULL;
    char *type = NULL;

    expect_ui_handle_error(err_msg);

    handle_presence_error(from, type, err_msg);
}

void handle_presence_error_when_no_recipient_and_conflict(void **state)
{
    char *err_msg = "conflict";
    char *from = NULL;
    char *type = NULL;

    expect_ui_handle_error(err_msg);

    handle_presence_error(from, type, err_msg);
}

void handle_presence_error_when_nick_conflict_shows_recipient_error(void **state)
{
    char *err_msg = "conflict";
    char *from = "room@rooms.org/nick";
    char *barejid = "room@rooms.org";
    char *nick = "nick";
    char *type = NULL;

    muc_init();
    muc_join_room(barejid, nick);

    expect_ui_handle_recipient_error(barejid, err_msg);

    handle_presence_error(from, type, err_msg);

    muc_close();
}

void handle_presence_error_when_nick_conflict_does_not_join_room(void **state)
{
    char *err_msg = "conflict";
    char *from = "room@rooms.org/nick";
    char *barejid = "room@rooms.org";
    char *nick = "nick";
    char *type = NULL;
    Jid *jidp = jid_create(from);
    stub_ui_handle_recipient_error();

    muc_init();
    muc_join_room(barejid, nick);

    handle_presence_error(from, type, err_msg);

    gboolean room_is_active = muc_room_is_active(jidp->barejid);
    assert_false(room_is_active);

    muc_close();
    jid_destroy(jidp);
}

void handle_presence_error_when_from_recipient_not_conflict(void **state)
{
    char *err_msg = "Some error.";
    char *from = "bob@server.com";
    char *type = NULL;

    expect_ui_handle_recipient_error(from, err_msg);

    handle_presence_error(from, type, err_msg);
}
