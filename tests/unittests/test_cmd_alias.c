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

#include "config/preferences.h"

#include "command/cmd_defs.h"
#include "command/cmd_funcs.h"
#include "command/cmd_ac.h"

#define CMD_ALIAS "/alias"

void cmd_alias_add_shows_usage_when_no_args(void **state)
{
    gchar *args[] = { "add", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ALIAS);

    gboolean result = cmd_alias(NULL, CMD_ALIAS, args);
    assert_true(result);
}

void cmd_alias_add_shows_usage_when_no_value(void **state)
{
    gchar *args[] = { "add", "alias", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ALIAS);

    gboolean result = cmd_alias(NULL, CMD_ALIAS, args);
    assert_true(result);
}

void cmd_alias_remove_shows_usage_when_no_args(void **state)
{
    gchar *args[] = { "remove", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ALIAS);

    gboolean result = cmd_alias(NULL, CMD_ALIAS, args);
    assert_true(result);
}

void cmd_alias_show_usage_when_invalid_subcmd(void **state)
{
    gchar *args[] = { "blah", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_ALIAS);

    gboolean result = cmd_alias(NULL, CMD_ALIAS, args);
    assert_true(result);
}

void cmd_alias_add_adds_alias(void **state)
{
    gchar *args[] = { "add", "hc", "/help commands", NULL };

    expect_cons_show("Command alias added /hc -> /help commands");

    gboolean result = cmd_alias(NULL, CMD_ALIAS, args);
    assert_true(result);

    char *returned_val = prefs_get_alias("hc");
    assert_string_equal("/help commands", returned_val);
}

void cmd_alias_add_shows_message_when_exists(void **state)
{
    gchar *args[] = { "add", "hc", "/help commands", NULL };

    cmd_init();
    prefs_add_alias("hc", "/help commands");
    cmd_ac_add("/hc");

    expect_cons_show("Command or alias '/hc' already exists.");

    gboolean result = cmd_alias(NULL, CMD_ALIAS, args);
    assert_true(result);
}

void cmd_alias_remove_removes_alias(void **state)
{
    gchar *args[] = { "remove", "hn", NULL };

    prefs_add_alias("hn", "/help navigation");

    expect_cons_show("Command alias removed -> /hn");

    gboolean result = cmd_alias(NULL, CMD_ALIAS, args);
    assert_true(result);

    char *returned_val = prefs_get_alias("hn");
    assert_null(returned_val);
}

void cmd_alias_remove_shows_message_when_no_alias(void **state)
{
    gchar *args[] = { "remove", "hn", NULL };

    expect_cons_show("No such command alias /hn");

    gboolean result = cmd_alias(NULL, CMD_ALIAS, args);
    assert_true(result);
}

void cmd_alias_list_shows_all_aliases(void **state)
{
    gchar *args[] = { "list", NULL };

    prefs_add_alias("vy", "/vercheck on");
    prefs_add_alias("q", "/quit");
    prefs_add_alias("hn", "/help navigation");
    prefs_add_alias("hc", "/help commands");
    prefs_add_alias("vn", "/vercheck off");

    // write a custom checker to check the correct list is passed
    expect_any(cons_show_aliases, aliases);

    gboolean result = cmd_alias(NULL, CMD_ALIAS, args);
    assert_true(result);
}
