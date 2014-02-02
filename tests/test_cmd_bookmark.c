#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "ui/mock_ui.h"
#include "ui/window.h"
#include "xmpp/xmpp.h"
#include "xmpp/mock_xmpp.h"

#include "muc.h"

#include "command/commands.h"

#include "xmpp/bookmark.h"

static void test_with_connection_status(jabber_conn_status_t status)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));

    mock_connection_status(status);
    expect_cons_show("You are not currently connected.");

    gboolean result = cmd_bookmark(NULL, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_shows_message_when_disconnected(void **state)
{
    test_with_connection_status(JABBER_DISCONNECTED);
}

void cmd_bookmark_shows_message_when_disconnecting(void **state)
{
    test_with_connection_status(JABBER_DISCONNECTING);
}

void cmd_bookmark_shows_message_when_connecting(void **state)
{
    test_with_connection_status(JABBER_CONNECTING);
}

void cmd_bookmark_shows_message_when_started(void **state)
{
    test_with_connection_status(JABBER_STARTED);
}

void cmd_bookmark_shows_message_when_undefined(void **state)
{
    test_with_connection_status(JABBER_UNDEFINED);
}

void cmd_bookmark_shows_usage_when_no_args(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { NULL };

    mock_connection_status(JABBER_CONNECTED);
    expect_cons_show("Usage: some usage");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

static void _free_bookmark(Bookmark *bookmark)
{
    free(bookmark->jid);
    free(bookmark->nick);
    free(bookmark);
}

void cmd_bookmark_list_shows_bookmarks(void **state)
{
    mock_cons_show_bookmarks();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "list", NULL };
    GList *bookmarks = NULL;

    Bookmark *bm1 = malloc(sizeof(Bookmark));
    bm1->jid = strdup("room1@conf.org");
    bm1->nick = strdup("bob");
    bm1->autojoin = FALSE;
    Bookmark *bm2 = malloc(sizeof(Bookmark));
    bm2->jid = strdup("room2@conf.org");
    bm2->nick = strdup("steve");
    bm2->autojoin = TRUE;
    Bookmark *bm3 = malloc(sizeof(Bookmark));
    bm3->jid = strdup("room3@conf.org");
    bm3->nick = strdup("dave");
    bm3->autojoin = TRUE;
    Bookmark *bm4 = malloc(sizeof(Bookmark));
    bm4->jid = strdup("room4@conf.org");
    bm4->nick = strdup("james");
    bm4->autojoin = FALSE;
    Bookmark *bm5 = malloc(sizeof(Bookmark));
    bm5->jid = strdup("room5@conf.org");
    bm5->nick = strdup("mike");
    bm5->autojoin = FALSE;

    bookmarks = g_list_append(bookmarks, bm1);
    bookmarks = g_list_append(bookmarks, bm2);
    bookmarks = g_list_append(bookmarks, bm3);
    bookmarks = g_list_append(bookmarks, bm4);
    bookmarks = g_list_append(bookmarks, bm5);

    mock_connection_status(JABBER_CONNECTED);

    bookmark_get_list_returns(bookmarks);
    expect_cons_show_bookmarks(bookmarks);

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
    g_list_free_full(bookmarks, (GDestroyNotify)_free_bookmark);
}

void cmd_bookmark_add_shows_usage_when_no_args_not_muc(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "add", NULL };

    mock_connection_status(JABBER_CONNECTED);
    mock_current_win_type(WIN_CONSOLE);
    expect_cons_show("Usage: some usage");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_remove_shows_message_when_no_args_not_muc(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "remove", NULL };

    mock_connection_status(JABBER_CONNECTED);
    mock_current_win_type(WIN_CONSOLE);
    expect_cons_show("Usage: some usage");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_add_adds_bookmark_with_jid(void **state)
{
    mock_bookmark_add();
    mock_cons_show();
    char *jid = "room@conf.server";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", jid, NULL };

    mock_connection_status(JABBER_CONNECTED);

    expect_and_return_bookmark_add(jid, NULL, FALSE, TRUE);
    expect_cons_show("Bookmark added for room@conf.server.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_add_adds_bookmark_with_jid_nick(void **state)
{
    mock_bookmark_add();
    mock_cons_show();
    char *jid = "room@conf.server";
    char *nick = "bob";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", jid, nick, NULL };

    mock_connection_status(JABBER_CONNECTED);

    expect_and_return_bookmark_add(jid, nick, FALSE, TRUE);
    expect_cons_show("Bookmark added for room@conf.server, nickname: bob.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_add_adds_bookmark_with_jid_autojoin(void **state)
{
    mock_bookmark_add();
    mock_cons_show();
    char *jid = "room@conf.server";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", jid, "autojoin", NULL };

    mock_connection_status(JABBER_CONNECTED);

    expect_and_return_bookmark_add(jid, NULL, TRUE, TRUE);
    expect_cons_show("Bookmark added for room@conf.server, autojoin enabled.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_add_adds_bookmark_with_jid_nick_autojoin(void **state)
{
    mock_bookmark_add();
    mock_cons_show();
    char *jid = "room@conf.server";
    char *nick = "bob";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", jid, nick, "autojoin", NULL };

    mock_connection_status(JABBER_CONNECTED);

    expect_and_return_bookmark_add(jid, nick, TRUE, TRUE);
    expect_cons_show("Bookmark added for room@conf.server, nickname: bob, autojoin enabled.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_add_adds_bookmark_with_room_details(void **state)
{
    mock_bookmark_add();
    mock_ui_current_recipient();
    mock_cons_show();
    char *jid = "room@conf.server";
    char *nick = "bob";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", NULL };

    muc_init();
    muc_join_room(jid, nick);

    mock_connection_status(JABBER_CONNECTED);
    mock_current_win_type(WIN_MUC);
    ui_current_recipient_returns(jid);

    expect_and_return_bookmark_add(jid, nick, FALSE, TRUE);
    expect_cons_show("Bookmark added for room@conf.server, nickname: bob.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
    muc_close();
}

void cmd_bookmark_add_adds_bookmark_with_room_details_autojoin(void **state)
{
    mock_bookmark_add();
    mock_ui_current_recipient();
    mock_cons_show();
    char *jid = "room@conf.server";
    char *nick = "bob";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", "autojoin", NULL };

    muc_init();
    muc_join_room(jid, nick);

    mock_connection_status(JABBER_CONNECTED);
    mock_current_win_type(WIN_MUC);
    ui_current_recipient_returns(jid);

    expect_and_return_bookmark_add(jid, nick, TRUE, TRUE);
    expect_cons_show("Bookmark added for room@conf.server, nickname: bob, autojoin enabled.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
    muc_close();
}

void cmd_bookmark_remove_removes_bookmark(void **state)
{
    mock_bookmark_remove();
    mock_cons_show();
    char *jid = "room@conf.server";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "remove", jid, NULL };

    mock_connection_status(JABBER_CONNECTED);

    expect_and_return_bookmark_remove(jid, FALSE, TRUE);
    expect_cons_show("Bookmark removed for room@conf.server.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_remove_removes_autojoin(void **state)
{
    mock_bookmark_remove();
    mock_cons_show();
    char *jid = "room@conf.server";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "remove", jid, "autojoin", NULL };

    mock_connection_status(JABBER_CONNECTED);

    expect_and_return_bookmark_remove(jid, TRUE, TRUE);
    expect_cons_show("Autojoin disabled for room@conf.server.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_add_shows_message_when_upated(void **state)
{
    mock_bookmark_add();
    mock_cons_show();
    char *jid = "room@conf.server";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", jid, NULL };

    mock_connection_status(JABBER_CONNECTED);

    expect_and_return_bookmark_add(jid, NULL, FALSE, FALSE);
    expect_cons_show("Bookmark updated for room@conf.server.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_remove_shows_message_when_no_bookmark(void **state)
{
    mock_bookmark_remove();
    mock_cons_show();
    char *jid = "room@conf.server";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "remove", jid, NULL };

    mock_connection_status(JABBER_CONNECTED);

    expect_and_return_bookmark_remove(jid, FALSE, FALSE);
    expect_cons_show("No bookmark exists for room@conf.server.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_remove_autojoin_shows_message_when_no_bookmark(void **state)
{
    mock_bookmark_remove();
    mock_cons_show();
    char *jid = "room@conf.server";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "remove", jid, "autojoin", NULL };

    mock_connection_status(JABBER_CONNECTED);

    expect_and_return_bookmark_remove(jid, TRUE, FALSE);
    expect_cons_show("No bookmark exists for room@conf.server.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}
