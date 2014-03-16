#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "ui/ui.h"
#include "ui/mock_ui.h"

#include "xmpp/xmpp.h"
#include "xmpp/mock_xmpp.h"

#include "roster_list.h"
#include "command/commands.h"

static void test_with_connection_status(jabber_conn_status_t status)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));

    mock_connection_status(status);

    expect_cons_show("You are not currently connected.");

    gboolean result = cmd_roster(NULL, *help);
    assert_true(result);

    free(help);
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

void cmd_roster_shows_message_when_undefined(void **state)
{
    test_with_connection_status(JABBER_UNDEFINED);
}

void cmd_roster_shows_roster_when_no_args(void **state)
{
    mock_cons_show_roster();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { NULL };

    mock_connection_status(JABBER_CONNECTED);
    roster_init();
    roster_add("bob@server.org", "bob", NULL, "both", FALSE);
    GSList *roster = roster_get_contacts();
    cons_show_roster_expect(roster);

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    free(help);
    roster_free();
}

void cmd_roster_add_shows_message_when_no_jid(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "add", NULL };

    mock_connection_status(JABBER_CONNECTED);
    expect_cons_show("Usage: some usage");

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    free(help);
}

void cmd_roster_add_sends_roster_add_request(void **state)
{
    char *jid = "bob@server.org";
    char *nick = "bob";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", jid, nick, NULL };

    mock_roster_send_add_new();
    mock_connection_status(JABBER_CONNECTED);
    roster_send_add_new_expect(jid, nick);

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    free(help);
}

void cmd_roster_remove_shows_message_when_no_jid(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "remove", NULL };

    mock_connection_status(JABBER_CONNECTED);
    expect_cons_show("Usage: some usage");

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    free(help);
}

void cmd_roster_remove_sends_roster_remove_request(void **state)
{
    char *jid = "bob@server.org";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "remove", jid, NULL };

    mock_roster_send_remove();
    mock_connection_status(JABBER_CONNECTED);
    roster_send_remove_expect(jid);

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    free(help);
}

void cmd_roster_nick_shows_message_when_no_jid(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "nick", NULL };

    mock_connection_status(JABBER_CONNECTED);
    expect_cons_show("Usage: some usage");

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    free(help);
}

void cmd_roster_nick_shows_message_when_no_nick(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "nick", "bob@server.org", NULL };

    mock_connection_status(JABBER_CONNECTED);
    expect_cons_show("Usage: some usage");

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    free(help);
}

void cmd_roster_nick_shows_message_when_no_contact_exists(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "nick", "bob@server.org", "bobster", NULL };

    roster_init();
    mock_connection_status(JABBER_CONNECTED);
    expect_cons_show("Contact not found in roster: bob@server.org");

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    free(help);
    roster_free();
}

void cmd_roster_nick_sends_name_change_request(void **state)
{
    char *jid = "bob@server.org";
    char *nick = "bobster";
    mock_cons_show();
    mock_roster_send_name_change();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "nick", jid, nick, NULL };

    roster_init();
    GSList *groups = NULL;
    groups = g_slist_append(groups, "group1");
    roster_add(jid, "bob", groups, "both", FALSE);

    mock_connection_status(JABBER_CONNECTED);
    roster_send_name_change_expect(jid, nick, groups);
    expect_cons_show("Nickname for bob@server.org set to: bobster.");

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    PContact contact = roster_get_contact(jid);
    assert_string_equal(p_contact_name(contact), nick);

    free(help);
    roster_free();
}

void cmd_roster_clearnick_shows_message_when_no_jid(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "clearnick", NULL };

    mock_connection_status(JABBER_CONNECTED);
    expect_cons_show("Usage: some usage");

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    free(help);
}

void cmd_roster_clearnick_shows_message_when_no_contact_exists(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "clearnick", "bob@server.org", NULL };

    roster_init();
    mock_connection_status(JABBER_CONNECTED);
    expect_cons_show("Contact not found in roster: bob@server.org");

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    free(help);
    roster_free();
}

void cmd_roster_clearnick_sends_name_change_request_with_empty_nick(void **state)
{
    char *jid = "bob@server.org";
    mock_cons_show();
    mock_roster_send_name_change();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "clearnick", jid, NULL };

    roster_init();
    GSList *groups = NULL;
    groups = g_slist_append(groups, "group1");
    roster_add(jid, "bob", groups, "both", FALSE);

    mock_connection_status(JABBER_CONNECTED);
    roster_send_name_change_expect(jid, NULL, groups);
    expect_cons_show("Nickname for bob@server.org removed.");

    gboolean result = cmd_roster(args, *help);
    assert_true(result);

    PContact contact = roster_get_contact(jid);
    assert_null(p_contact_name(contact));

    free(help);
    roster_free();
}
