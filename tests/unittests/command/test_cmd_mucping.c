#include "prof_cmocka.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config/preferences.h"
#include "ui/ui.h"
#include "ui/stub_ui.h"
#include "command/cmd_funcs.h"

#define CMD_MUCPING "/mucping"

void
cmd_mucping__shows__usage_when_invalid_interval_chars(void** state)
{
    gchar* args[] = { "set", "abc", NULL };

    expect_any_cons_show();
    expect_string(cons_bad_cmd_usage, cmd, CMD_MUCPING);

    gboolean result = cmd_mucping(NULL, CMD_MUCPING, args);
    assert_true(result);
}

void
cmd_mucping__shows__usage_when_invalid_interval_range(void** state)
{
    gchar* args[] = { "set", "-5", NULL };

    expect_any_cons_show();
    expect_string(cons_bad_cmd_usage, cmd, CMD_MUCPING);

    gboolean result = cmd_mucping(NULL, CMD_MUCPING, args);
    assert_true(result);
}

void
cmd_mucping__shows__usage_when_invalid_timeout_chars(void** state)
{
    gchar* args[] = { "timeout", "xyz", NULL };

    expect_any_cons_show();
    expect_string(cons_bad_cmd_usage, cmd, CMD_MUCPING);

    gboolean result = cmd_mucping(NULL, CMD_MUCPING, args);
    assert_true(result);
}

void
cmd_mucping__shows__usage_when_invalid_timeout_range(void** state)
{
    gchar* args[] = { "timeout", "-1", NULL };

    expect_any_cons_show();
    expect_string(cons_bad_cmd_usage, cmd, CMD_MUCPING);

    gboolean result = cmd_mucping(NULL, CMD_MUCPING, args);
    assert_true(result);
}

void
cmd_mucping__shows__error_when_interval_smaller_than_timeout(void** state)
{
    // Default interval = 600, timeout = 60
    assert_int_equal(600, prefs_get_muc_ping_interval());
    assert_int_equal(60, prefs_get_muc_ping_timeout());

    // Set timeout to 80 (valid)
    gchar* args_timeout[] = { "timeout", "80", NULL };
    expect_cons_show("MUC self-ping timeout set to 80 seconds.");
    gboolean result1 = cmd_mucping(NULL, CMD_MUCPING, args_timeout);
    assert_true(result1);
    assert_int_equal(80, prefs_get_muc_ping_timeout());

    // Try setting interval to 50 (invalid: <= timeout of 80)
    gchar* args_interval[] = { "set", "50", NULL };
    expect_cons_show_error("MUC self-ping interval must be greater than timeout (80 seconds).");
    gboolean result2 = cmd_mucping(NULL, CMD_MUCPING, args_interval);
    assert_true(result2);
    // Interval remains 600
    assert_int_equal(600, prefs_get_muc_ping_interval());
}

void
cmd_mucping__shows__error_when_timeout_greater_than_interval(void** state)
{
    // Reset interval and timeout
    prefs_set_muc_ping_interval(100);
    prefs_set_muc_ping_timeout(60);

    // Try setting timeout to 120 (invalid: >= interval of 100)
    gchar* args_timeout[] = { "timeout", "120", NULL };
    expect_cons_show_error("MUC self-ping timeout must be less than interval (100 seconds).");
    gboolean result1 = cmd_mucping(NULL, CMD_MUCPING, args_timeout);
    assert_true(result1);
    // Timeout remains 60
    assert_int_equal(60, prefs_get_muc_ping_timeout());
}
