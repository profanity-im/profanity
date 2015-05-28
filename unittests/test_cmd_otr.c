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
#endif

#include "config/preferences.h"

#include "command/command.h"
#include "command/commands.h"

#include "ui/ui.h"
#include "ui/stub_ui.h"

#ifdef HAVE_LIBOTR
void cmd_otr_shows_usage_when_no_args(void **state)
{
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
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "Some usage";
    gchar *args[] = { "unknown", NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);

    expect_cons_show("Usage: Some usage");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_log_shows_usage_when_no_args(void **state)
{
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
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "libver", NULL };
    char *version = "9.9.9";
    GString *message = g_string_new("Using libotr version ");
    g_string_append(message, version);

    will_return(otr_libotr_version, version);

    expect_cons_show(message->str);

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    g_string_free(message, TRUE);
    free(help);
}

void cmd_otr_gen_shows_message_when_not_connected(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "gen", NULL };

    will_return(jabber_get_connection_status, JABBER_DISCONNECTED);

    expect_cons_show("You must be connected with an account to load OTR information.");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

static void test_with_command_and_connection_status(char *command, jabber_conn_status_t status)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { command, NULL };

    will_return(jabber_get_connection_status, status);

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
    ProfAccount *account = account_new(account_name, "me@jabber.org", NULL, NULL,
        TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(jabber_get_account_name, account_name);

    expect_string(accounts_get_account, name, account_name);

    will_return(accounts_get_account, account);

    expect_memory(otr_keygen, account, account, sizeof(ProfAccount));

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

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(otr_key_loaded, FALSE);

    expect_ui_current_print_formatted_line('!', 0, "You have not generated or loaded a private key, use '/otr gen'");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_myfp_shows_my_fingerprint(void **state)
{
    char *fingerprint = "AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD EEEEEEEE";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "myfp", NULL };
    GString *message = g_string_new("Your OTR fingerprint: ");
    g_string_append(message, fingerprint);

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(otr_key_loaded, TRUE);
    will_return(otr_get_my_fingerprint, strdup(fingerprint));

    expect_ui_current_print_formatted_line('!', 0, message->str);

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

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, wintype);

    expect_ui_current_print_line("You must be in a regular chat window to view a recipient's fingerprint.");

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

void cmd_otr_theirfp_shows_message_when_non_otr_chat_window(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "theirfp", NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, WIN_CHAT);
    will_return(ui_current_win_is_otr, FALSE);

    expect_ui_current_print_formatted_line('!', 0, "You are not currently in an OTR session.");

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
    ProfChatWin *chatwin = malloc(sizeof(ProfChatWin));
    chatwin->barejid = strdup(recipient);
    GString *message = g_string_new(chatwin->barejid);
    g_string_append(message, "'s OTR fingerprint: ");
    g_string_append(message, fingerprint);

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, WIN_CHAT);
    will_return(ui_get_current_chat, chatwin);
    will_return(ui_current_win_is_otr, TRUE);

    expect_string(otr_get_their_fingerprint, recipient, chatwin->barejid);
    will_return(otr_get_their_fingerprint, strdup(fingerprint));

    expect_ui_current_print_formatted_line('!', 0, message->str);

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    g_string_free(message, TRUE);
    free(help);
    free(chatwin->barejid);
    free(chatwin);
}

static void
test_cmd_otr_start_from_wintype(win_type_t wintype)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "start", NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, wintype);

    expect_ui_current_print_line("You must be in a regular chat window to start an OTR session.");

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

void cmd_otr_start_shows_message_when_already_started(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "start", NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, WIN_CHAT);
    will_return(ui_current_win_is_otr, TRUE);

    expect_ui_current_print_formatted_line('!', 0, "You are already in an OTR session.");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void cmd_otr_start_shows_message_when_no_key(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "start", NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, WIN_CHAT);
    will_return(ui_current_win_is_otr, FALSE);
    will_return(otr_key_loaded, FALSE);

    expect_ui_current_print_formatted_line('!', 0, "You have not generated or loaded a private key, use '/otr gen'");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}

void
cmd_otr_start_sends_otr_query_message_to_current_recipeint(void **state)
{
    char *recipient = "buddy@chat.com";
    ProfChatWin *chatwin = malloc(sizeof(ProfChatWin));
    chatwin->barejid = strdup(recipient);
    char *query_message = "?OTR?";
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "start", NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    will_return(ui_current_win_type, WIN_CHAT);
    will_return(ui_get_current_chat, chatwin);
    will_return(ui_current_win_is_otr, FALSE);
    will_return(otr_key_loaded, TRUE);
    will_return(otr_start_query, query_message);

    expect_string(message_send_chat_encrypted, barejid, chatwin->barejid);
    expect_string(message_send_chat_encrypted, msg, query_message);

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
    free(chatwin->barejid);
    free(chatwin);
}

#else
void cmd_otr_shows_message_when_otr_unsupported(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "gen", NULL };

    expect_cons_show("This version of Profanity has not been built with OTR support enabled");

    gboolean result = cmd_otr(args, *help);
    assert_true(result);

    free(help);
}
#endif
