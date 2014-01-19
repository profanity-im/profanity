#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config/preferences.h"

#include "ui/ui.h"
#include "ui/mock_ui.h"

#include "command/commands.h"

void cmd_statuses_shows_usage_when_bad_subcmd(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "badcmd", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_statuses(args, *help);
    assert_true(result);

    free(help);
}

void cmd_statuses_shows_usage_when_bad_console_setting(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "console", "badsetting", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_statuses(args, *help);
    assert_true(result);

    free(help);
}

void cmd_statuses_shows_usage_when_bad_chat_setting(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "chat", "badsetting", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_statuses(args, *help);
    assert_true(result);

    free(help);
}

void cmd_statuses_shows_usage_when_bad_muc_setting(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "muc", "badsetting", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_statuses(args, *help);
    assert_true(result);

    free(help);
}

void cmd_statuses_console_sets_all(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "console", "all", NULL };

    expect_cons_show("All presence updates will appear in the console.");

    prefs_load();
    gboolean result = cmd_statuses(args, *help);
    prefs_close();
    prefs_load();

    char *setting = prefs_get_string(PREF_STATUSES_CONSOLE);
    assert_non_null(setting);
    assert_string_equal("all", setting);
    assert_true(result);

    prefs_close();
    free(help);
}

void cmd_statuses_console_sets_online(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "console", "online", NULL };

    expect_cons_show("Only online/offline presence updates will appear in the console.");

    prefs_load();
    gboolean result = cmd_statuses(args, *help);
    prefs_close();
    prefs_load();

    char *setting = prefs_get_string(PREF_STATUSES_CONSOLE);
    assert_non_null(setting);
    assert_string_equal("online", setting);
    assert_true(result);

    prefs_close();
    free(help);
}

void cmd_statuses_console_sets_none(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "console", "none", NULL };

    expect_cons_show("Presence updates will not appear in the console.");

    prefs_load();
    gboolean result = cmd_statuses(args, *help);
    prefs_close();
    prefs_load();

    char *setting = prefs_get_string(PREF_STATUSES_CONSOLE);
    assert_non_null(setting);
    assert_string_equal("none", setting);
    assert_true(result);

    prefs_close();
    free(help);
}

void cmd_statuses_chat_sets_all(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "chat", "all", NULL };

    expect_cons_show("All presence updates will appear in chat windows.");

    prefs_load();
    gboolean result = cmd_statuses(args, *help);
    prefs_close();
    prefs_load();

    char *setting = prefs_get_string(PREF_STATUSES_CHAT);
    assert_non_null(setting);
    assert_string_equal("all", setting);
    assert_true(result);

    prefs_close();
    free(help);
}

void cmd_statuses_chat_sets_online(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "chat", "online", NULL };

    expect_cons_show("Only online/offline presence updates will appear in chat windows.");

    prefs_load();
    gboolean result = cmd_statuses(args, *help);
    prefs_close();
    prefs_load();

    char *setting = prefs_get_string(PREF_STATUSES_CHAT);
    assert_non_null(setting);
    assert_string_equal("online", setting);
    assert_true(result);

    prefs_close();
    free(help);
}

void cmd_statuses_chat_sets_none(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "chat", "none", NULL };

    expect_cons_show("Presence updates will not appear in chat windows.");

    prefs_load();
    gboolean result = cmd_statuses(args, *help);
    prefs_close();
    prefs_load();

    char *setting = prefs_get_string(PREF_STATUSES_CHAT);
    assert_non_null(setting);
    assert_string_equal("none", setting);
    assert_true(result);

    prefs_close();
    free(help);
}

void cmd_statuses_muc_sets_on(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "muc", "on", NULL };

    expect_cons_show("Chat room presence updates enabled.");

    prefs_load();
    gboolean result = cmd_statuses(args, *help);
    prefs_close();
    prefs_load();

    gboolean setting = prefs_get_boolean(PREF_STATUSES_MUC);
    assert_non_null(setting);
    assert_true(setting);
    assert_true(result);

    prefs_close();
    free(help);
}

void cmd_statuses_muc_sets_off(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "muc", "off", NULL };

    expect_cons_show("Chat room presence updates disabled.");

    prefs_load();
    gboolean result = cmd_statuses(args, *help);
    prefs_close();
    prefs_load();

    gboolean setting = prefs_get_boolean(PREF_STATUSES_MUC);
    assert_false(setting);
    assert_true(result);

    prefs_close();
    free(help);
}
