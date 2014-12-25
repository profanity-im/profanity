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

    expect_string(ui_handle_recipient_not_found, recipient, from);
    expect_string(ui_handle_recipient_not_found, err_msg, err_msg);

    handle_message_error(from, type, err_msg);
}

void handle_message_error_when_recipient_cancel_disables_chat_session(void **state)
{
    char *err_msg = "Some error.";
    char *from = "bob@server.com";
    char *type = "cancel";

    prefs_set_boolean(PREF_STATES, TRUE);
    chat_sessions_init();
    chat_session_start(from, TRUE);

    expect_any(ui_handle_recipient_not_found, recipient);
    expect_any(ui_handle_recipient_not_found, err_msg);

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
