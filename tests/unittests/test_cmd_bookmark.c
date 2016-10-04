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

#include "xmpp/muc.h"
#include "common.h"

#include "command/cmd_funcs.h"

#include "xmpp/bookmark.h"

#include "helpers.h"

#define CMD_BOOKMARK "/bookmark"

static void test_with_connection_status(jabber_conn_status_t status)
{
    will_return(connection_get_status, status);
    expect_cons_show("You are not currently connected.");

    gboolean result = cmd_bookmark(NULL, CMD_BOOKMARK, NULL);
    assert_true(result);
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

void cmd_bookmark_shows_usage_when_no_args(void **state)
{
    gchar *args[] = { NULL };
    ProfWin window;
    window.type = WIN_CONSOLE;

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(cons_bad_cmd_usage, cmd, CMD_BOOKMARK);

    gboolean result = cmd_bookmark(&window, CMD_BOOKMARK, args);
    assert_true(result);
}

static void _free_bookmark(Bookmark *bookmark)
{
    free(bookmark->barejid);
    free(bookmark->nick);
    free(bookmark);
}

static gboolean
_cmp_bookmark(Bookmark *bm1, Bookmark *bm2)
{
    if (strcmp(bm1->barejid, bm2->barejid) != 0) {
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
    gchar *args[] = { "list", NULL };
    GList *bookmarks = NULL;
    ProfWin window;
    window.type = WIN_CONSOLE;

    Bookmark *bm1 = malloc(sizeof(Bookmark));
    bm1->barejid = strdup("room1@conf.org");
    bm1->nick = strdup("bob");
    bm1->autojoin = FALSE;
    Bookmark *bm2 = malloc(sizeof(Bookmark));
    bm2->barejid = strdup("room2@conf.org");
    bm2->nick = strdup("steve");
    bm2->autojoin = TRUE;
    Bookmark *bm3 = malloc(sizeof(Bookmark));
    bm3->barejid = strdup("room3@conf.org");
    bm3->nick = strdup("dave");
    bm3->autojoin = TRUE;
    Bookmark *bm4 = malloc(sizeof(Bookmark));
    bm4->barejid = strdup("room4@conf.org");
    bm4->nick = strdup("james");
    bm4->autojoin = FALSE;
    Bookmark *bm5 = malloc(sizeof(Bookmark));
    bm5->barejid = strdup("room5@conf.org");
    bm5->nick = strdup("mike");
    bm5->autojoin = FALSE;

    bookmarks = g_list_append(bookmarks, bm1);
    bookmarks = g_list_append(bookmarks, bm2);
    bookmarks = g_list_append(bookmarks, bm3);
    bookmarks = g_list_append(bookmarks, bm4);
    bookmarks = g_list_append(bookmarks, bm5);

    will_return(connection_get_status, JABBER_CONNECTED);
    will_return(bookmark_get_list, bookmarks);

    // TODO - Custom list compare
    glist_set_cmp((GCompareFunc)_cmp_bookmark);
    expect_any(cons_show_bookmarks, list);

    gboolean result = cmd_bookmark(&window, CMD_BOOKMARK, args);
    assert_true(result);
}

void cmd_bookmark_add_shows_message_when_invalid_jid(void **state)
{
    char *jid = "room";
    gchar *args[] = { "add", jid, NULL };
    ProfWin window;
    window.type = WIN_CONSOLE;

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_cons_show("Invalid room, must be of the form room@domain.tld");
    expect_cons_show("");

    gboolean result = cmd_bookmark(&window, CMD_BOOKMARK, args);
    assert_true(result);
}

void cmd_bookmark_add_adds_bookmark_with_jid(void **state)
{
    char *jid = "room@conf.server";
    gchar *args[] = { "add", jid, NULL };
    ProfWin window;
    window.type = WIN_CONSOLE;

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(bookmark_add, jid, jid);
    expect_any(bookmark_add, nick);
    expect_any(bookmark_add, password);
    expect_any(bookmark_add, autojoin_str);
    will_return(bookmark_add, TRUE);

    expect_cons_show("Bookmark added for room@conf.server.");

    gboolean result = cmd_bookmark(&window, CMD_BOOKMARK, args);
    assert_true(result);
}

void cmd_bookmark_add_adds_bookmark_with_jid_nick(void **state)
{
    char *jid = "room@conf.server";
    char *nick = "bob";
    gchar *args[] = { "add", jid, "nick", nick, NULL };
    ProfWin window;
    window.type = WIN_CONSOLE;

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(bookmark_add, jid, jid);
    expect_string(bookmark_add, nick, nick);
    expect_any(bookmark_add, password);
    expect_any(bookmark_add, autojoin_str);
    will_return(bookmark_add, TRUE);

    expect_cons_show("Bookmark added for room@conf.server.");

    gboolean result = cmd_bookmark(&window, CMD_BOOKMARK, args);
    assert_true(result);
}

void cmd_bookmark_add_adds_bookmark_with_jid_autojoin(void **state)
{
    char *jid = "room@conf.server";
    gchar *args[] = { "add", jid, "autojoin", "on", NULL };
    ProfWin window;
    window.type = WIN_CONSOLE;

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(bookmark_add, jid, jid);
    expect_any(bookmark_add, nick);
    expect_any(bookmark_add, password);
    expect_string(bookmark_add, autojoin_str, "on");
    will_return(bookmark_add, TRUE);

    expect_cons_show("Bookmark added for room@conf.server.");

    gboolean result = cmd_bookmark(&window, CMD_BOOKMARK, args);
    assert_true(result);
}

void cmd_bookmark_add_adds_bookmark_with_jid_nick_autojoin(void **state)
{
    char *jid = "room@conf.server";
    char *nick = "bob";
    gchar *args[] = { "add", jid, "nick", nick, "autojoin", "on", NULL };
    ProfWin window;
    window.type = WIN_CONSOLE;

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(bookmark_add, jid, jid);
    expect_string(bookmark_add, nick, nick);
    expect_any(bookmark_add, password);
    expect_string(bookmark_add, autojoin_str, "on");
    will_return(bookmark_add, TRUE);

    expect_cons_show("Bookmark added for room@conf.server.");

    gboolean result = cmd_bookmark(&window, CMD_BOOKMARK, args);
    assert_true(result);
}

void cmd_bookmark_remove_removes_bookmark(void **state)
{
    char *jid = "room@conf.server";
    gchar *args[] = { "remove", jid, NULL };
    ProfWin window;
    window.type = WIN_CONSOLE;

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(bookmark_remove, jid, jid);
    will_return(bookmark_remove, TRUE);

    expect_cons_show("Bookmark removed for room@conf.server.");

    gboolean result = cmd_bookmark(&window, CMD_BOOKMARK, args);
    assert_true(result);
}

void cmd_bookmark_remove_shows_message_when_no_bookmark(void **state)
{
    char *jid = "room@conf.server";
    gchar *args[] = { "remove", jid, NULL };
    ProfWin window;
    window.type = WIN_CONSOLE;

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_any(bookmark_remove, jid);
    will_return(bookmark_remove, FALSE);

    expect_cons_show("No bookmark exists for room@conf.server.");

    gboolean result = cmd_bookmark(&window, CMD_BOOKMARK, args);
    assert_true(result);
}
