#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config.h"

#include "command/commands.h"

#include "ui/stub_ui.h"

#ifdef HAVE_LIBGPGME
void cmd_pgp_shows_usage_when_no_args(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "Some usage";
    gchar *args[] = { NULL };

    expect_cons_show("Usage: Some usage");

    gboolean result = cmd_pgp(NULL, args, *help);
    assert_true(result);

    free(help);
}

void cmd_pgp_start_shows_message_when_connection(jabber_conn_status_t conn_status)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "Some usage";
    gchar *args[] = { "start", NULL };
    ProfWin window;
    window.type = WIN_CHAT;

    will_return(jabber_get_connection_status, conn_status);

    expect_cons_show("You must be connected to start PGP encrpytion.");

    gboolean result = cmd_pgp(&window, args, *help);
    assert_true(result);

    free(help);
}

void cmd_pgp_start_shows_message_when_disconnected(void **state)
{
    cmd_pgp_start_shows_message_when_connection(JABBER_DISCONNECTED);
}

void cmd_pgp_start_shows_message_when_disconnecting(void **state)
{
    cmd_pgp_start_shows_message_when_connection(JABBER_DISCONNECTING);
}

void cmd_pgp_start_shows_message_when_connecting(void **state)
{
    cmd_pgp_start_shows_message_when_connection(JABBER_CONNECTING);
}

void cmd_pgp_start_shows_message_when_undefined(void **state)
{
    cmd_pgp_start_shows_message_when_connection(JABBER_UNDEFINED);
}

void cmd_pgp_start_shows_message_when_started(void **state)
{
    cmd_pgp_start_shows_message_when_connection(JABBER_STARTED);
}

void cmd_pgp_start_shows_message_when_no_arg_in_wintype(win_type_t wintype)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "Some usage";
    gchar *args[] = { "start", NULL };
    ProfWin window;
    window.type = wintype;

    will_return(jabber_get_connection_status, JABBER_CONNECTED);

    expect_cons_show("You must be in a regular chat window to start PGP encrpytion.");

    gboolean result = cmd_pgp(&window, args, *help);
    assert_true(result);

    free(help);
}

void cmd_pgp_start_shows_message_when_no_arg_in_console(void **state)
{
    cmd_pgp_start_shows_message_when_no_arg_in_wintype(WIN_CONSOLE);
}

void cmd_pgp_start_shows_message_when_no_arg_in_muc(void **state)
{
    cmd_pgp_start_shows_message_when_no_arg_in_wintype(WIN_MUC);
}

void cmd_pgp_start_shows_message_when_no_arg_in_mucconf(void **state)
{
    cmd_pgp_start_shows_message_when_no_arg_in_wintype(WIN_MUC_CONFIG);
}

void cmd_pgp_start_shows_message_when_no_arg_in_private(void **state)
{
    cmd_pgp_start_shows_message_when_no_arg_in_wintype(WIN_PRIVATE);
}

void cmd_pgp_start_shows_message_when_no_arg_in_xmlconsole(void **state)
{
    cmd_pgp_start_shows_message_when_no_arg_in_wintype(WIN_XML);
}

#else
void cmd_pgp_shows_message_when_pgp_unsupported(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "gen", NULL };

    expect_cons_show("This version of Profanity has not been built with PGP support enabled");

    gboolean result = cmd_pgp(NULL, args, *help);
    assert_true(result);

    free(help);
}
#endif
