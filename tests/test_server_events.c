#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "server_events.h"
#include "roster_list.h"
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
    mock_ui_handle_error();
    expect_ui_handle_error(err_msg);

    handle_message_error(NULL, "cancel", err_msg);
}
