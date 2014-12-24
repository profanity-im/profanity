#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "xmpp/xmpp.h"

#include "ui/ui.h"
#include "ui/stub_ui.h"

#include "muc.h"
#include "common.h"

#include "command/commands.h"

#include "xmpp/bookmark.h"

#include "helpers.h"

static void test_with_connection_status(jabber_conn_status_t status)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));

    will_return(jabber_get_connection_status, status);
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
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, WIN_CONSOLE);

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

static gboolean
_cmp_bookmark(Bookmark *bm1, Bookmark *bm2)
{
    if (strcmp(bm1->jid, bm2->jid) != 0) {
        return FALSE;
    }
    if (strcmp(bm1->nick, bm2->nick) != 0) {
        return FALSE;
    }
    if (bm1->autojoin != bm2->autojoin) {
        return FALSE;
    }

    return TRUE;
}

void cmd_bookmark_list_shows_bookmarks(void **state)
{
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

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, WIN_CONSOLE);
    will_return(bookmark_get_list, bookmarks);

    // TODO - Custom list compare
    glist_set_cmp((GCompareFunc)_cmp_bookmark);
    expect_any(cons_show_bookmarks, list);

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
    g_list_free_full(bookmarks, (GDestroyNotify)_free_bookmark);
}

void cmd_bookmark_add_shows_message_when_invalid_jid(void **state)
{
    char *jid = "room";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", jid, NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, WIN_CONSOLE);

    expect_cons_show("Can't add bookmark with JID 'room'; should be 'room@domain.tld'");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_add_adds_bookmark_with_jid(void **state)
{
    char *jid = "room@conf.server";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", jid, NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, WIN_CONSOLE);

    expect_string(bookmark_add, jid, jid);
    expect_any(bookmark_add, nick);
    expect_any(bookmark_add, password);
    expect_any(bookmark_add, autojoin_str);
    will_return(bookmark_add, TRUE);

    expect_cons_show("Bookmark added for room@conf.server.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_add_adds_bookmark_with_jid_nick(void **state)
{    char *jid = "room@conf.server";
    char *nick = "bob";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", jid, "nick", nick, NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, WIN_CONSOLE);

    expect_string(bookmark_add, jid, jid);
    expect_string(bookmark_add, nick, nick);
    expect_any(bookmark_add, password);
    expect_any(bookmark_add, autojoin_str);
    will_return(bookmark_add, TRUE);

    expect_cons_show("Bookmark added for room@conf.server.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_add_adds_bookmark_with_jid_autojoin(void **state)
{
    char *jid = "room@conf.server";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", jid, "autojoin", "on", NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, WIN_CONSOLE);

    expect_string(bookmark_add, jid, jid);
    expect_any(bookmark_add, nick);
    expect_any(bookmark_add, password);
    expect_string(bookmark_add, autojoin_str, "on");
    will_return(bookmark_add, TRUE);

    expect_cons_show("Bookmark added for room@conf.server.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_add_adds_bookmark_with_jid_nick_autojoin(void **state)
{
    char *jid = "room@conf.server";
    char *nick = "bob";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", jid, "nick", nick, "autojoin", "on", NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, WIN_CONSOLE);

    expect_string(bookmark_add, jid, jid);
    expect_string(bookmark_add, nick, nick);
    expect_any(bookmark_add, password);
    expect_string(bookmark_add, autojoin_str, "on");
    will_return(bookmark_add, TRUE);

    expect_cons_show("Bookmark added for room@conf.server.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_remove_removes_bookmark(void **state)
{
    char *jid = "room@conf.server";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "remove", jid, NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, WIN_CONSOLE);

    expect_string(bookmark_remove, jid, jid);
    will_return(bookmark_remove, TRUE);

    expect_cons_show("Bookmark removed for room@conf.server.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}

void cmd_bookmark_remove_shows_message_when_no_bookmark(void **state)
{
    char *jid = "room@conf.server";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "remove", jid, NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, WIN_CONSOLE);

    expect_any(bookmark_remove, jid);
    will_return(bookmark_remove, FALSE);

    expect_cons_show("No bookmark exists for room@conf.server.");

    gboolean result = cmd_bookmark(args, *help);
    assert_true(result);

    free(help);
}
