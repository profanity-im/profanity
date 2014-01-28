#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "xmpp/xmpp.h"
#include "xmpp/mock_xmpp.h"

#include "ui/ui.h"
#include "ui/mock_ui.h"

#include "config/preferences.h"

#include "command/command.h"
#include "command/commands.h"

void cmd_alias_add_shows_usage_when_no_args(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "add", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_alias(args, *help);
    assert_true(result);

    free(help);
}

void cmd_alias_add_shows_usage_when_no_value(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "add", "alias", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_alias(args, *help);
    assert_true(result);

    free(help);
}

void cmd_alias_remove_shows_usage_when_no_args(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "remove", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_alias(args, *help);
    assert_true(result);

    free(help);
}

void cmd_alias_show_usage_when_invalid_subcmd(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "blah", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_alias(args, *help);
    assert_true(result);

    free(help);
}

void cmd_alias_add_adds_alias(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", "hc", "/help commands", NULL };

    expect_cons_show("Command alias added /hc -> /help commands");

    gboolean result = cmd_alias(args, *help);

    char *returned_val = prefs_get_alias("hc");

    assert_true(result);
    assert_string_equal("/help commands", returned_val);

    free(help);
}

void cmd_alias_add_shows_message_when_exists(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "add", "hc", "/help commands", NULL };

    cmd_init();
    prefs_add_alias("hc", "/help commands");
    cmd_autocomplete_add("/hc");

    expect_cons_show("Command or alias '/hc' already exists.");

    gboolean result = cmd_alias(args, *help);
    assert_true(result);

    free(help);
}

void cmd_alias_remove_removes_alias(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "remove", "hn", NULL };

    prefs_add_alias("hn", "/help navigation");

    expect_cons_show("Command alias removed -> /hn");

    gboolean result = cmd_alias(args, *help);

    char *returned_val = prefs_get_alias("hn");

    assert_true(result);
    assert_null(returned_val);

    free(help);
}

void cmd_alias_remove_shows_message_when_no_alias(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "remove", "hn", NULL };

    expect_cons_show("No such command alias /hn");

    gboolean result = cmd_alias(args, *help);
    assert_true(result);

    free(help);
}

void cmd_alias_list_shows_all_aliases(void **state)
{
    mock_cons_show_aliases();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "list", NULL };

    prefs_add_alias("vy", "/vercheck on");
    prefs_add_alias("q", "/quit");
    prefs_add_alias("hn", "/help navigation");
    prefs_add_alias("hc", "/help commands");
    prefs_add_alias("vn", "/vercheck off");

    // write a custom checker to check the correct list is passed
    expect_cons_show_aliases();

    gboolean result = cmd_alias(args, *help);
    assert_true(result);

    free(help);
}
