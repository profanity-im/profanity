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

#include "command/cmd_defs.h"
#include "command/cmd_funcs.h"
#include "ui/window_list.h"
#include "xmpp/xmpp.h"

#include "ui/ui.h"
#include "ui/stub_ui.h"

#define CMD_OTR "/otr"

#ifdef HAVE_LIBOTR
void cmd_otr_log_shows_usage_when_no_args(void **state)
{
    gchar *args[] = { "log", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_OTR);

    gboolean result = cmd_otr_log(NULL, CMD_OTR, args);
    assert_true(result);
}

void cmd_otr_log_shows_usage_when_invalid_subcommand(void **state)
{
    gchar *args[] = { "log", "wrong", NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_OTR);

    gboolean result = cmd_otr_log(NULL, CMD_OTR, args);
    assert_true(result);
}

void cmd_otr_log_on_enables_logging(void **state)
{
    gchar *args[] = { "log", "on", NULL };
    prefs_set_string(PREF_OTR_LOG, "off");
    prefs_set_boolean(PREF_CHLOG, TRUE);

    expect_cons_show("OTR messages will be logged as plaintext.");

    gboolean result = cmd_otr_log(NULL, CMD_OTR, args);
    char *pref_otr_log = prefs_get_string(PREF_OTR_LOG);

    assert_true(result);
    assert_string_equal("on", pref_otr_log);
}

void cmd_otr_log_on_shows_warning_when_chlog_disabled(void **state)
{
    gchar *args[] = { "log", "on", NULL };
    prefs_set_string(PREF_OTR_LOG, "off");
    prefs_set_boolean(PREF_CHLOG, FALSE);

    expect_cons_show("OTR messages will be logged as plaintext.");
    expect_cons_show("Chat logging is currently disabled, use '/chlog on' to enable.");

    gboolean result = cmd_otr_log(NULL, CMD_OTR, args);
    assert_true(result);
}

void cmd_otr_log_off_disables_logging(void **state)
{
    gchar *args[] = { "log", "off", NULL };
    prefs_set_string(PREF_OTR_LOG, "on");
    prefs_set_boolean(PREF_CHLOG, TRUE);

    expect_cons_show("OTR message logging disabled.");

    gboolean result = cmd_otr_log(NULL, CMD_OTR, args);
    char *pref_otr_log = prefs_get_string(PREF_OTR_LOG);

    assert_true(result);
    assert_string_equal("off", pref_otr_log);
}

void cmd_otr_redact_redacts_logging(void **state)
{
    gchar *args[] = { "log", "redact", NULL };
    prefs_set_string(PREF_OTR_LOG, "on");
    prefs_set_boolean(PREF_CHLOG, TRUE);

    expect_cons_show("OTR messages will be logged as '[redacted]'.");

    gboolean result = cmd_otr_log(NULL, CMD_OTR, args);
    char *pref_otr_log = prefs_get_string(PREF_OTR_LOG);

    assert_true(result);
    assert_string_equal("redact", pref_otr_log);
}

void cmd_otr_log_redact_shows_warning_when_chlog_disabled(void **state)
{
    gchar *args[] = { "log", "redact", NULL };
    prefs_set_string(PREF_OTR_LOG, "off");
    prefs_set_boolean(PREF_CHLOG, FALSE);

    expect_cons_show("OTR messages will be logged as '[redacted]'.");
    expect_cons_show("Chat logging is currently disabled, use '/chlog on' to enable.");

    gboolean result = cmd_otr_log(NULL, CMD_OTR, args);
    assert_true(result);
}

void cmd_otr_libver_shows_libotr_version(void **state)
{
    gchar *args[] = { "libver", NULL };
    char *version = "9.9.9";
    GString *message = g_string_new("Using libotr version ");
    g_string_append(message, version);

    will_return(otr_libotr_version, version);

    expect_cons_show(message->str);

    gboolean result = cmd_otr_libver(NULL, CMD_OTR, args);
    assert_true(result);

    g_string_free(message, TRUE);
}

void cmd_otr_gen_shows_message_when_not_connected(void **state)
{
    gchar *args[] = { "gen", NULL };

    will_return(connection_get_status, JABBER_DISCONNECTED);

    expect_cons_show("You must be connected with an account to load OTR information.");

    gboolean result = cmd_otr_gen(NULL, CMD_OTR, args);
    assert_true(result);
}

static void test_with_command_and_connection_status(char *command, void *cmd_func, jabber_conn_status_t status)
{
    gchar *args[] = { command, NULL };

    will_return(connection_get_status, status);

    expect_cons_show("You must be connected with an account to load OTR information.");

    gboolean (*func)(ProfWin *window, const char *const command, gchar **args) = cmd_func;
    gboolean result = func(NULL, CMD_OTR, args);
    assert_true(result);
}

void cmd_otr_gen_shows_message_when_disconnected(void **state)
{
    test_with_command_and_connection_status("gen", cmd_otr_gen, JABBER_DISCONNECTED);
}

void cmd_otr_gen_shows_message_when_connecting(void **state)
{
    test_with_command_and_connection_status("gen", cmd_otr_gen, JABBER_CONNECTING);
}

void cmd_otr_gen_shows_message_when_disconnecting(void **state)
{
    test_with_command_and_connection_status("gen", cmd_otr_gen, JABBER_DISCONNECTING);
}

void cmd_otr_gen_generates_key_for_connected_account(void **state)
{
    gchar *args[] = { "gen", NULL };
    char *account_name = "myaccount";
    ProfAccount *account = account_new(account_name, "me@jabber.org", NULL, NULL,
        TRUE, NULL, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    will_return(connection_get_status, JABBER_CONNECTED);
    will_return(session_get_account_name, account_name);

    expect_string(accounts_get_account, name, account_name);

    will_return(accounts_get_account, account);

    expect_memory(otr_keygen, account, account, sizeof(ProfAccount));

    gboolean result = cmd_otr_gen(NULL, CMD_OTR, args);
    assert_true(result);
}

void cmd_otr_myfp_shows_message_when_disconnected(void **state)
{
    test_with_command_and_connection_status("myfp", cmd_otr_myfp, JABBER_DISCONNECTED);
}

void cmd_otr_myfp_shows_message_when_connecting(void **state)
{
    test_with_command_and_connection_status("myfp", cmd_otr_myfp, JABBER_CONNECTING);
}

void cmd_otr_myfp_shows_message_when_disconnecting(void **state)
{
    test_with_command_and_connection_status("myfp", cmd_otr_myfp, JABBER_DISCONNECTING);
}

void cmd_otr_myfp_shows_message_when_no_key(void **state)
{
    gchar *args[] = { "myfp", NULL };

    will_return(connection_get_status, JABBER_CONNECTED);
    will_return(otr_key_loaded, FALSE);

    expect_win_println("You have not generated or loaded a private key, use '/otr gen'");

    gboolean result = cmd_otr_myfp(NULL, CMD_OTR, args);
    assert_true(result);
}

void cmd_otr_myfp_shows_my_fingerprint(void **state)
{
    char *fingerprint = "AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD EEEEEEEE";
    gchar *args[] = { "myfp", NULL };
    GString *message = g_string_new("Your OTR fingerprint: ");
    g_string_append(message, fingerprint);

    will_return(connection_get_status, JABBER_CONNECTED);
    will_return(otr_key_loaded, TRUE);
    will_return(otr_get_my_fingerprint, strdup(fingerprint));

    expect_win_println(message->str);

    gboolean result = cmd_otr_myfp(NULL, CMD_OTR, args);
    assert_true(result);

    g_string_free(message, TRUE);
}

static void
test_cmd_otr_theirfp_from_wintype(win_type_t wintype)
{
    gchar *args[] = { "theirfp", NULL };
    ProfWin window;
    window.type = wintype;

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_win_println("You must be in a regular chat window to view a recipient's fingerprint.");

    gboolean result = cmd_otr_theirfp(&window, CMD_OTR, args);

    assert_true(result);
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
    gchar *args[] = { "theirfp", NULL };

    ProfWin window;
    window.type = WIN_CHAT;
    ProfChatWin chatwin;
    chatwin.window = window;
    chatwin.memcheck = PROFCHATWIN_MEMCHECK;
    chatwin.pgp_send = FALSE;
    chatwin.is_otr = FALSE;

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_win_println("You are not currently in an OTR session.");

    gboolean result = cmd_otr_theirfp((ProfWin*)&chatwin, CMD_OTR, args);

    assert_true(result);
}

void cmd_otr_theirfp_shows_fingerprint(void **state)
{
    char *recipient = "someone@chat.com";
    char *fingerprint = "AAAAAAAA BBBBBBBB CCCCCCCC DDDDDDDD EEEEEEEE";
    gchar *args[] = { "theirfp", NULL };
    GString *message = g_string_new(recipient);
    g_string_append(message, "'s OTR fingerprint: ");
    g_string_append(message, fingerprint);

    ProfWin window;
    window.type = WIN_CHAT;
    ProfChatWin chatwin;
    chatwin.window = window;
    chatwin.barejid = recipient;
    chatwin.memcheck = PROFCHATWIN_MEMCHECK;
    chatwin.pgp_send = FALSE;
    chatwin.is_otr = TRUE;

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_string(otr_get_their_fingerprint, recipient, recipient);
    will_return(otr_get_their_fingerprint, strdup(fingerprint));

    expect_win_println(message->str);

    gboolean result = cmd_otr_theirfp((ProfWin*)&chatwin, CMD_OTR, args);
    assert_true(result);

    g_string_free(message, TRUE);
}

static void
test_cmd_otr_start_from_wintype(win_type_t wintype)
{
    gchar *args[] = { "start", NULL };
    ProfWin window;
    window.type = wintype;

    will_return(connection_get_status, JABBER_CONNECTED);

    expect_win_println("You must be in a regular chat window to start an OTR session.");

    gboolean result = cmd_otr_start(&window, CMD_OTR, args);
    assert_true(result);
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
    char *recipient = "someone@server.org";
    gchar *args[] = { "start", NULL };

    will_return(connection_get_status, JABBER_CONNECTED);

    ProfWin window;
    window.type = WIN_CHAT;
    ProfChatWin chatwin;
    chatwin.window = window;
    chatwin.barejid = recipient;
    chatwin.memcheck = PROFCHATWIN_MEMCHECK;
    chatwin.pgp_send = FALSE;
    chatwin.is_otr = TRUE;

    expect_win_println("You are already in an OTR session.");

    gboolean result = cmd_otr_start((ProfWin*)&chatwin, CMD_OTR, args);
    assert_true(result);
}

void cmd_otr_start_shows_message_when_no_key(void **state)
{
    char *recipient = "someone@server.org";
    gchar *args[] = { "start", NULL };

    will_return(connection_get_status, JABBER_CONNECTED);
    will_return(otr_key_loaded, FALSE);

    ProfWin window;
    window.type = WIN_CHAT;
    ProfChatWin chatwin;
    chatwin.window = window;
    chatwin.barejid = recipient;
    chatwin.memcheck = PROFCHATWIN_MEMCHECK;
    chatwin.pgp_send = FALSE;
    chatwin.is_otr = FALSE;

    expect_win_println("You have not generated or loaded a private key, use '/otr gen'");

    gboolean result = cmd_otr_start((ProfWin*)&chatwin, CMD_OTR, args);
    assert_true(result);
}

void
cmd_otr_start_sends_otr_query_message_to_current_recipeint(void **state)
{
    char *recipient = "buddy@chat.com";
    char *query_message = "?OTR?";
    gchar *args[] = { "start", NULL };

    ProfWin window;
    window.type = WIN_CHAT;
    ProfChatWin chatwin;
    chatwin.window = window;
    chatwin.barejid = recipient;
    chatwin.memcheck = PROFCHATWIN_MEMCHECK;
    chatwin.pgp_send = FALSE;
    chatwin.is_otr = FALSE;

    will_return(connection_get_status, JABBER_CONNECTED);
    will_return(otr_key_loaded, TRUE);
    will_return(otr_start_query, query_message);

    expect_string(message_send_chat_otr, barejid, recipient);
    expect_string(message_send_chat_otr, msg, query_message);

    gboolean result = cmd_otr_start((ProfWin*)&chatwin, CMD_OTR, args);
    assert_true(result);
}

#else
void cmd_otr_shows_message_when_otr_unsupported(void **state)
{
    gchar *args[] = { "gen", NULL };

    expect_cons_show("This version of Profanity has not been built with OTR support enabled");

    gboolean result = cmd_otr_gen(NULL, CMD_OTR, args);
    assert_true(result);
}
#endif
