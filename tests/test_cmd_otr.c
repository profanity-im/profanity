#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "config.h"

#include "ui/mock_ui.h"

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
