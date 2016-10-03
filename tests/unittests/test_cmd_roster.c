#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "ui/ui.h"
#include "ui/stub_ui.h"

#include "xmpp/xmpp.h"
#include "xmpp/roster_list.h"
#include "command/cmd_funcs.h"

#define CMD_ROSTER "/roster"

static void test_with_connection_status(jabber_conn_status_t status)
{
    gchar *args[] = { NULL };

    will_return(connection_get_status, status);

    expect_cons_show("You are not currently connected.");

    gboolean result = cmd_roster(NULL, CMD_ROSTER, args);
    assert_true(result);
}

void cmd_roster_shows_message_when_disconnecting(void **state)
{
    test_with_connection_status(JABBER_DISCONNECTING);
}

void cmd_roster_shows_message_when_connecting(void **state)
{
    test_with_connection_status(JABBER_CONNECTING);
}

void cmd_roster_shows_message_when_disconnected(void **state)
{
    test_with_connection_status(JABBER_DISCONNECTED);
}

void cmd_roster_shows_roster_when_no_args(void **state)
{
    gchar *args[] = { NULL };

    will_return(connection_get_status, JABBER_CONNECTED);

    roster_create();
    roster_add("bob@server.org", "bob", NULL, "both", FALSE);
    GSList *roster = roster_get_contacts(ROSTER_ORD_NAME);

    expect_memory(cons_show_roster, list, roster, sizeof(roster));

    gboolean result = cmd_roster(NULL, CMD_ROSTER, args);
    assert_true(result);

    roster_destroy();
}

void cmd_roster_add_shows_message_when_no_jid(void **state)
{
    gchar *args[] = { "add", NULL };

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_ROSTER);

    gboolean result = cmd_roster(NULL, CMD_ROSTER, args);
    assert_true(result);
}

void cmd_roster_add_sends_roster_add_request(void **state)
{
    char *jid = "bob@server.org";
    char *nick = "bob";
    gchar *args[] = { "add", jid, nick, NULL };

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(roster_send_add_new, barejid, jid);
    expect_string(roster_send_add_new, name, nick);

    gboolean result = cmd_roster(NULL, CMD_ROSTER, args);
    assert_true(result);
}

void cmd_roster_remove_shows_message_when_no_jid(void **state)
{
    gchar *args[] = { "remove", NULL };

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_ROSTER);

    gboolean result = cmd_roster(NULL, CMD_ROSTER, args);
    assert_true(result);
}

void cmd_roster_remove_sends_roster_remove_request(void **state)
{
    char *jid = "bob@server.org";
    gchar *args[] = { "remove", jid, NULL };

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(roster_send_remove, barejid, jid);

    gboolean result = cmd_roster(NULL, CMD_ROSTER, args);
    assert_true(result);
}

void cmd_roster_nick_shows_message_when_no_jid(void **state)
{
    gchar *args[] = { "nick", NULL };

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_ROSTER);

    gboolean result = cmd_roster(NULL, CMD_ROSTER, args);
    assert_true(result);
}

void cmd_roster_nick_shows_message_when_no_nick(void **state)
{
    gchar *args[] = { "nick", "bob@server.org", NULL };

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_ROSTER);

    gboolean result = cmd_roster(NULL, CMD_ROSTER, args);
    assert_true(result);
}

void cmd_roster_nick_shows_message_when_no_contact_exists(void **state)
{
    gchar *args[] = { "nick", "bob@server.org", "bobster", NULL };

    roster_create();

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_cons_show("Contact not found in roster: bob@server.org");

    gboolean result = cmd_roster(NULL, CMD_ROSTER, args);
    assert_true(result);

    roster_destroy();
}

void cmd_roster_nick_sends_name_change_request(void **state)
{
    char *jid = "bob@server.org";
    char *nick = "bobster";
    gchar *args[] = { "nick", jid, nick, NULL };

    roster_create();
    GSList *groups = NULL;
    groups = g_slist_append(groups, strdup("group1"));
    roster_add(jid, "bob", groups, "both", FALSE);

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(roster_send_name_change, barejid, jid);
    expect_string(roster_send_name_change, new_name, nick);
    expect_memory(roster_send_name_change, groups, groups, sizeof(groups));

    expect_cons_show("Nickname for bob@server.org set to: bobster.");

    gboolean result = cmd_roster(NULL, CMD_ROSTER, args);
    assert_true(result);

    PContact contact = roster_get_contact(jid);
    assert_string_equal(p_contact_name(contact), nick);
    roster_destroy();
}

void cmd_roster_clearnick_shows_message_when_no_jid(void **state)
{
    gchar *args[] = { "clearnick", NULL };

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_ROSTER);

    gboolean result = cmd_roster(NULL, CMD_ROSTER, args);
    assert_true(result);
}

void cmd_roster_clearnick_shows_message_when_no_contact_exists(void **state)
{
    gchar *args[] = { "clearnick", "bob@server.org", NULL };

    roster_create();

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_cons_show("Contact not found in roster: bob@server.org");

    gboolean result = cmd_roster(NULL, CMD_ROSTER, args);
    assert_true(result);

    roster_destroy();
}

void cmd_roster_clearnick_sends_name_change_request_with_empty_nick(void **state)
{
    char *jid = "bob@server.org";
    gchar *args[] = { "clearnick", jid, NULL };

    roster_create();
    GSList *groups = NULL;
    groups = g_slist_append(groups, strdup("group1"));
    roster_add(jid, "bob", groups, "both", FALSE);

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(roster_send_name_change, barejid, jid);
    expect_value(roster_send_name_change, new_name, NULL);
    expect_memory(roster_send_name_change, groups, groups, sizeof(groups));

    expect_cons_show("Nickname for bob@server.org removed.");

    gboolean result = cmd_roster(NULL, CMD_ROSTER, args);
    assert_true(result);

    PContact contact = roster_get_contact(jid);
    assert_null(p_contact_name(contact));

    roster_destroy();
}
