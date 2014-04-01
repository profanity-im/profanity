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
    stub_ui_current_update_virtual();
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
    stub_ui_current_update_virtual();
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
    stub_ui_current_update_virtual();
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
    stub_ui_current_update_virtual();
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

void cmd_otr_myfp_shows_message_when_no_key(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "myfp", NULL };
    mock_connection_status(JABBER_CONNECTED);
    otr_key_loaded_returns(FALSE);
    mock_ui_current_print_formatted_line();

    ui_current_print_formatted_line_expect('!', 0, "You have not generated or loaded a private key, use '/otr gen'");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_myfp_shows_my_fingerprint(void **state)
{
    char *fingerprint = "AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD EEEEEEEE";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "myfp", NULL };
    mock_connection_status(JABBER_CONNECTED);
    otr_key_loaded_returns(TRUE);
    otr_get_my_fingerprint_returns(strdup(fingerprint));
    mock_ui_current_print_formatted_line();

    GString *message = g_string_new("Your OTR fingerprint: ");
    g_string_append(message, fingerprint);

    ui_current_print_formatted_line_expect('!', 0, message->str);

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    g_string_free(message, TRUE);
    free(help);
}

static void
test_cmd_otr_theirfp_from_wintype(win_type_t wintype)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "theirfp", NULL };
    mock_connection_status(JABBER_CONNECTED);
    mock_current_win_type(wintype);
    mock_ui_current_print_line();

    ui_current_print_line_expect("You must be in a regular chat window to view a recipient's fingerprint.");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_theirfp_shows_message_when_in_console(void **state)
{
    test_cmd_otr_theirfp_from_wintype(WIN_CONSOLE);
}

void cmd_otr_theirfp_shows_message_when_in_muc(void **state)
{
    test_cmd_otr_theirfp_from_wintype(WIN_MUC);
}

void cmd_otr_theirfp_shows_message_when_in_private(void **state)
{
    test_cmd_otr_theirfp_from_wintype(WIN_PRIVATE);
}

void cmd_otr_theirfp_shows_message_when_in_duck(void **state)
{
    test_cmd_otr_theirfp_from_wintype(WIN_DUCK);
}

void cmd_otr_theirfp_shows_message_when_non_otr_chat_window(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "theirfp", NULL };
    mock_connection_status(JABBER_CONNECTED);
    mock_current_win_type(WIN_CHAT);
    ui_current_win_is_otr_returns(FALSE);
    mock_ui_current_print_formatted_line();

    ui_current_print_formatted_line_expect('!', 0, "You are not currently in an OTR session.");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_theirfp_shows_fingerprint(void **state)
{
    char *recipient = "someone@chat.com";
    char *fingerprint = "AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD EEEEEEEE";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "theirfp", NULL };
    mock_connection_status(JABBER_CONNECTED);
    mock_current_win_type(WIN_CHAT);
    ui_current_win_is_otr_returns(TRUE);
    mock_ui_current_recipient();
    ui_current_recipient_returns(recipient);
    mock_ui_current_print_formatted_line();

    GString *message = g_string_new(recipient);
    g_string_append(message, "'s OTR fingerprint: ");
    g_string_append(message, fingerprint);

    otr_get_their_fingerprint_expect_and_return(recipient, strdup(fingerprint));
    ui_current_print_formatted_line_expect('!', 0, message->str);

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    g_string_free(message, TRUE);
    free(help);
}

static void
test_cmd_otr_start_from_wintype(win_type_t wintype)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "start", NULL };
    mock_connection_status(JABBER_CONNECTED);
    mock_current_win_type(wintype);
    mock_ui_current_print_line();

    ui_current_print_line_expect("You must be in a regular chat window to start an OTR session.");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_start_shows_message_when_in_console(void **state)
{
    test_cmd_otr_start_from_wintype(WIN_CONSOLE);
}

void cmd_otr_start_shows_message_when_in_muc(void **state)
{
    test_cmd_otr_start_from_wintype(WIN_MUC);
}

void cmd_otr_start_shows_message_when_in_private(void **state)
{
    test_cmd_otr_start_from_wintype(WIN_PRIVATE);
}

void cmd_otr_start_shows_message_when_in_duck(void **state)
{
    test_cmd_otr_start_from_wintype(WIN_DUCK);
}

void cmd_otr_start_shows_message_when_already_started(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "start", NULL };
    mock_connection_status(JABBER_CONNECTED);
    mock_current_win_type(WIN_CHAT);
    ui_current_win_is_otr_returns(TRUE);
    mock_ui_current_print_formatted_line();

    ui_current_print_formatted_line_expect('!', 0, "You are already in an OTR session.");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_start_shows_message_when_no_key(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "start", NULL };
    mock_connection_status(JABBER_CONNECTED);
    mock_current_win_type(WIN_CHAT);
    ui_current_win_is_otr_returns(FALSE);
    otr_key_loaded_returns(FALSE);
    mock_ui_current_print_formatted_line();

    ui_current_print_formatted_line_expect('!', 0, "You have not generated or loaded a private key, use '/otr gen'");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void
cmd_otr_start_sends_otr_query_message_to_current_recipeint(void **state)
{
    char *recipient = "buddy@chat.com";
    char *query_message = "?OTR?";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "start", NULL };
    mock_connection_status(JABBER_CONNECTED);
    mock_current_win_type(WIN_CHAT);
    ui_current_win_is_otr_returns(FALSE);
    otr_key_loaded_returns(TRUE);
    ui_current_recipient_returns(recipient);
    otr_start_query_returns(query_message);

    message_send_expect(query_message, recipient);

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

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
