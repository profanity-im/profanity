#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config.h"
#include "config/preferences.h"

#include "ui/mock_ui.h"
#include "xmpp/mock_xmpp.h"

#include "command/command.h"
#include "command/commands.h"

#ifdef HAVE_LIBOTR
void cmd_otr_shows_usage_when_no_args(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "Some usage";
    gchar *args[] = { NULL };

    expect_cons_show("Usage: Some usage");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_shows_usage_when_invalid_subcommand(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "Some usage";
    gchar *args[] = { "unknown", NULL };

    mock_connection_status(JABBER_CONNECTED);
    expect_cons_show("Usage: Some usage");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_log_shows_usage_when_no_args(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "Some usage";
    gchar *args[] = { "log", NULL };

    expect_cons_show("Usage: Some usage");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_log_shows_usage_when_invalid_subcommand(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "Some usage";
    gchar *args[] = { "log", "wrong", NULL };

    expect_cons_show("Usage: Some usage");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_log_on_enables_logging(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "log", "on", NULL };

    prefs_set_string(PREF_OTR_LOG, "off");
    prefs_set_boolean(PREF_CHLOG, TRUE);
    expect_cons_show("OTR messages will be logged as plaintext.");

    gboolean result = cmd_otr(args, *help);
    char *pref_otr_log = prefs_get_string(PREF_OTR_LOG);

    assert_true(result);
    assert_string_equal("on", pref_otr_log);

    free(help);
}

void cmd_otr_log_on_shows_warning_when_chlog_disabled(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "log", "on", NULL };

    prefs_set_string(PREF_OTR_LOG, "off");
    prefs_set_boolean(PREF_CHLOG, FALSE);
    expect_cons_show("OTR messages will be logged as plaintext.");
    expect_cons_show("Chat logging is currently disabled, use '/chlog on' to enable.");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_log_off_disables_logging(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "log", "off", NULL };

    prefs_set_string(PREF_OTR_LOG, "on");
    prefs_set_boolean(PREF_CHLOG, TRUE);
    expect_cons_show("OTR message logging disabled.");

    gboolean result = cmd_otr(args, *help);
    char *pref_otr_log = prefs_get_string(PREF_OTR_LOG);

    assert_true(result);
    assert_string_equal("off", pref_otr_log);

    free(help);
}

void cmd_otr_redact_redacts_logging(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "log", "redact", NULL };

    prefs_set_string(PREF_OTR_LOG, "on");
    prefs_set_boolean(PREF_CHLOG, TRUE);
    expect_cons_show("OTR messages will be logged as '[redacted]'.");

    gboolean result = cmd_otr(args, *help);
    char *pref_otr_log = prefs_get_string(PREF_OTR_LOG);

    assert_true(result);
    assert_string_equal("redact", pref_otr_log);

    free(help);
}

void cmd_otr_log_redact_shows_warning_when_chlog_disabled(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "log", "redact", NULL };

    prefs_set_string(PREF_OTR_LOG, "off");
    prefs_set_boolean(PREF_CHLOG, FALSE);
    expect_cons_show("OTR messages will be logged as '[redacted]'.");
    expect_cons_show("Chat logging is currently disabled, use '/chlog on' to enable.");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_warn_shows_usage_when_no_args(void **state)
{
    mock_cons_show();
    stub_ui_current_refresh();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "Some usage";
    gchar *args[] = { "warn", NULL };

    expect_cons_show("Usage: Some usage");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_warn_shows_usage_when_invalid_arg(void **state)
{
    mock_cons_show();
    stub_ui_current_refresh();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "Some usage";
    gchar *args[] = { "warn", "badarg", NULL };

    expect_cons_show("Usage: Some usage");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_warn_on_enables_unencrypted_warning(void **state)
{
    mock_cons_show();
    stub_ui_current_refresh();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "warn", "on", NULL };

    prefs_set_boolean(PREF_OTR_WARN, FALSE);
    expect_cons_show("OTR warning message enabled.");

    gboolean result = cmd_otr(args, *help);
    gboolean otr_warn_enabled = prefs_get_boolean(PREF_OTR_WARN);

    assert_true(result);
    assert_true(otr_warn_enabled);

    free(help);
}

void cmd_otr_warn_off_disables_unencrypted_warning(void **state)
{
    mock_cons_show();
    stub_ui_current_refresh();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "warn", "off", NULL };

    prefs_set_boolean(PREF_OTR_WARN, TRUE);
    expect_cons_show("OTR warning message disabled.");

    gboolean result = cmd_otr(args, *help);
    gboolean otr_warn_enabled = prefs_get_boolean(PREF_OTR_WARN);

    assert_true(result);
    assert_false(otr_warn_enabled);

    free(help);
}

#else
void cmd_otr_shows_message_when_otr_unsupported(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "gen", NULL };

    expect_cons_show("This version of Profanity has not been built with OTR support enabled");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}
#endif
