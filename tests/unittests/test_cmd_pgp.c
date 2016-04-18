#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config.h"

#include "command/commands.h"
#include "xmpp/xmpp.h"

#include "ui/stub_ui.h"

#define CMD_PGP "/pgp"

#ifdef HAVE_LIBGPGME
void cmd_pgp_shows_usage_when_no_args(void **state)
{
    gchar *args[] = { NULL };

    expect_string(cons_bad_cmd_usage, cmd, CMD_PGP);

    gboolean result = cmd_pgp(NULL, CMD_PGP, args);
    assert_true(result);
}

void cmd_pgp_start_shows_message_when_connection(jabber_conn_status_t conn_status)
{
    gchar *args[] = { "start", NULL };
    ProfWin window;
    window.type = WIN_CHAT;

    will_return(jabber_get_connection_status, conn_status);

    expect_cons_show("You must be connected to start PGP encrpytion.");

    gboolean result = cmd_pgp(&window, CMD_PGP, args);
    assert_true(result);
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
    gchar *args[] = { "start", NULL };
    ProfWin window;
    window.type = wintype;

    will_return(jabber_get_connection_status, JABBER_CONNECTED);

    expect_cons_show("You must be in a regular chat window to start PGP encrpytion.");

    gboolean result = cmd_pgp(&window, CMD_PGP, args);
    assert_true(result);
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
    gchar *args[] = { "gen", NULL };

    expect_cons_show("This version of Profanity has not been built with PGP support enabled");

    gboolean result = cmd_pgp(NULL, CMD_PGP, args);
    assert_true(result);
}
#endif
