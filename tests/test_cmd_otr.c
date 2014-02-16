#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config.h"

#ifdef HAVE_LIBOTR
#include <libotr/proto.h>
#include "otr/otr.h"
#include "otr/mock_otr.h"
#endif

#include "config/preferences.h"

#include "ui/mock_ui.h"
#include "xmpp/mock_xmpp.h"
#include "config/mock_accounts.h"

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

void cmd_otr_libver_shows_libotr_version(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "libver", NULL };
    char *version = "9.9.9";
    GString *message = g_string_new("Using libotr version ");
    g_string_append(message, version);
    mock_otr_libotr_version();
    otr_libotr_version_returns(version);

    expect_cons_show(message->str);
    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    g_string_free(message, TRUE);
    free(help);
}

void cmd_otr_gen_shows_message_when_not_connected(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "gen", NULL };

    mock_connection_status(JABBER_DISCONNECTED);
    expect_cons_show("You must be connected with an account to load OTR information.");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

static void test_with_command_and_connection_status(char *command, jabber_conn_status_t status)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { command, NULL };

    mock_connection_status(status);
    expect_cons_show("You must be connected with an account to load OTR information.");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_gen_shows_message_when_disconnected(void **state)
{
    test_with_command_and_connection_status("gen", JABBER_DISCONNECTED);
}

void cmd_otr_gen_shows_message_when_undefined(void **state)
{
    test_with_command_and_connection_status("gen", JABBER_UNDEFINED);
}

void cmd_otr_gen_shows_message_when_started(void **state)
{
    test_with_command_and_connection_status("gen", JABBER_STARTED);
}

void cmd_otr_gen_shows_message_when_connecting(void **state)
{
    test_with_command_and_connection_status("gen", JABBER_CONNECTING);
}

void cmd_otr_gen_shows_message_when_disconnecting(void **state)
{
    test_with_command_and_connection_status("gen", JABBER_DISCONNECTING);
}

void cmd_otr_gen_generates_key_for_connected_account(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "gen", NULL };
    char *account_name = "myaccount";
    ProfAccount *account = account_new(account_name, "me@jabber.org", NULL,
        TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL);

    stub_cons_show();
    mock_connection_status(JABBER_CONNECTED);
    mock_accounts_get_account();
    mock_connection_account_name(account_name);
    accounts_get_account_expect_and_return(account_name, account);

    otr_keygen_expect(account);

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_myfp_shows_message_when_disconnected(void **state)
{
    test_with_command_and_connection_status("myfp", JABBER_DISCONNECTED);
}

void cmd_otr_myfp_shows_message_when_undefined(void **state)
{
    test_with_command_and_connection_status("myfp", JABBER_UNDEFINED);
}

void cmd_otr_myfp_shows_message_when_started(void **state)
{
    test_with_command_and_connection_status("myfp", JABBER_STARTED);
}

void cmd_otr_myfp_shows_message_when_connecting(void **state)
{
    test_with_command_and_connection_status("myfp", JABBER_CONNECTING);
}

void cmd_otr_myfp_shows_message_when_disconnecting(void **state)
{
    test_with_command_and_connection_status("myfp", JABBER_DISCONNECTING);
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
