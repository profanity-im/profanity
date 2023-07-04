#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config/preferences.h"

#include "ui/ui.h"
#include "ui/stub_ui.h"

#include "command/cmd_funcs.h"

#define CMD_PRESENCE "/presence"

void
cmd_presence_shows_usage_when_bad_subcmd(void** state)
{
    gchar* args[] = { "badcmd", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_PRESENCE);

    gboolean result = cmd_presence(NULL, CMD_PRESENCE, args);
    assert_true(result);
}

void
cmd_presence_shows_usage_when_bad_console_setting(void** state)
{
    gchar* args[] = { "console", "badsetting", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_PRESENCE);

    gboolean result = cmd_presence(NULL, CMD_PRESENCE, args);
    assert_true(result);
}

void
cmd_presence_shows_usage_when_bad_chat_setting(void** state)
{
    gchar* args[] = { "chat", "badsetting", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_PRESENCE);

    gboolean result = cmd_presence(NULL, CMD_PRESENCE, args);
    assert_true(result);
}

void
cmd_presence_shows_usage_when_bad_muc_setting(void** state)
{
    gchar* args[] = { "muc", "badsetting", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_PRESENCE);

    gboolean result = cmd_presence(NULL, CMD_PRESENCE, args);
    assert_true(result);
}

void
cmd_presence_console_sets_all(void** state)
{
    gchar* args[] = { "console", "all", NULL };

    expect_cons_show("All presence updates will appear in the console.");

    gboolean result = cmd_presence(NULL, CMD_PRESENCE, args);

    auto_gchar gchar* setting = prefs_get_string(PREF_STATUSES_CONSOLE);
    assert_non_null(setting);
    assert_string_equal("all", setting);
    assert_true(result);
}

void
cmd_presence_console_sets_online(void** state)
{
    gchar* args[] = { "console", "online", NULL };

    expect_cons_show("Only online/offline presence updates will appear in the console.");

    gboolean result = cmd_presence(NULL, CMD_PRESENCE, args);

    auto_gchar gchar* setting = prefs_get_string(PREF_STATUSES_CONSOLE);
    assert_non_null(setting);
    assert_string_equal("online", setting);
    assert_true(result);
}

void
cmd_presence_console_sets_none(void** state)
{
    gchar* args[] = { "console", "none", NULL };

    expect_cons_show("Presence updates will not appear in the console.");

    gboolean result = cmd_presence(NULL, CMD_PRESENCE, args);

    auto_gchar gchar* setting = prefs_get_string(PREF_STATUSES_CONSOLE);
    assert_non_null(setting);
    assert_string_equal("none", setting);
    assert_true(result);
}

void
cmd_presence_chat_sets_all(void** state)
{
    gchar* args[] = { "chat", "all", NULL };

    expect_cons_show("All presence updates will appear in chat windows.");

    gboolean result = cmd_presence(NULL, CMD_PRESENCE, args);

    auto_gchar gchar* setting = prefs_get_string(PREF_STATUSES_CHAT);
    assert_non_null(setting);
    assert_string_equal("all", setting);
    assert_true(result);
}

void
cmd_presence_chat_sets_online(void** state)
{
    gchar* args[] = { "chat", "online", NULL };

    expect_cons_show("Only online/offline presence updates will appear in chat windows.");

    gboolean result = cmd_presence(NULL, CMD_PRESENCE, args);

    auto_gchar gchar* setting = prefs_get_string(PREF_STATUSES_CHAT);
    assert_non_null(setting);
    assert_string_equal("online", setting);
    assert_true(result);
}

void
cmd_presence_chat_sets_none(void** state)
{
    gchar* args[] = { "chat", "none", NULL };

    expect_cons_show("Presence updates will not appear in chat windows.");

    gboolean result = cmd_presence(NULL, CMD_PRESENCE, args);

    auto_gchar gchar* setting = prefs_get_string(PREF_STATUSES_CHAT);
    assert_non_null(setting);
    assert_string_equal("none", setting);
    assert_true(result);
}

void
cmd_presence_room_sets_all(void** state)
{
    gchar* args[] = { "room", "all", NULL };

    expect_cons_show("All presence updates will appear in chat room windows.");

    gboolean result = cmd_presence(NULL, CMD_PRESENCE, args);

    auto_gchar gchar* setting = prefs_get_string(PREF_STATUSES_MUC);
    assert_non_null(setting);
    assert_string_equal("all", setting);
    assert_true(result);
}

void
cmd_presence_room_sets_online(void** state)
{
    gchar* args[] = { "room", "online", NULL };

    expect_cons_show("Only join/leave presence updates will appear in chat room windows.");

    gboolean result = cmd_presence(NULL, CMD_PRESENCE, args);

    auto_gchar gchar* setting = prefs_get_string(PREF_STATUSES_MUC);
    assert_non_null(setting);
    assert_string_equal("online", setting);
    assert_true(result);
}

void
cmd_presence_room_sets_none(void** state)
{
    gchar* args[] = { "room", "none", NULL };

    expect_cons_show("Presence updates will not appear in chat room windows.");

    gboolean result = cmd_presence(NULL, CMD_PRESENCE, args);

    auto_gchar gchar* setting = prefs_get_string(PREF_STATUSES_MUC);
    assert_non_null(setting);
    assert_string_equal("none", setting);
    assert_true(result);
}
