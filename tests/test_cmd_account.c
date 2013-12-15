#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "xmpp/xmpp.h"
#include "command/commands.h"

void cmd_account_shows_usage_when_not_connected_and_no_args(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { NULL };

    will_return(jabber_get_connection_status, JABBER_DISCONNECTED);

    expect_string(cons_show, output, "Usage: some usage");

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}

void cmd_account_shows_account_when_connected_and_no_args(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    ProfAccount *account = malloc(sizeof(ProfAccount));
    gchar *args[] = { NULL };

    will_return(jabber_get_connection_status, JABBER_CONNECTED);
    
    will_return(jabber_get_account_name, "account_name");

    expect_string(accounts_get_account, name, "account_name");
    will_return(accounts_get_account, account);

    expect_memory(cons_show_account, account, account, sizeof(ProfAccount));

    expect_any(accounts_free_account, account);

    gboolean result = cmd_account(args, *help);
    assert_true(result);

    free(help);
}
