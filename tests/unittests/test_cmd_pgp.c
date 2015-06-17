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
