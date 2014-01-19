#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "ui/ui.h"
#include "ui/mock_ui.h"

#include "command/commands.h"

void cmd_statuses_shows_usage_when_bad_subcmd(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "badcmd", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_statuses(args, *help);
    assert_true(result);

    free(help);
}

void cmd_statuses_shows_usage_when_bad_console_setting(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "console", "badsetting", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_statuses(args, *help);
    assert_true(result);

    free(help);
}

void cmd_statuses_shows_usage_when_bad_chat_setting(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "chat", "badsetting", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_statuses(args, *help);
    assert_true(result);

    free(help);
}

void cmd_statuses_shows_usage_when_bad_muc_setting(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    help->usage = "some usage";
    gchar *args[] = { "muc", "badsetting", NULL };

    expect_cons_show("Usage: some usage");

    gboolean result = cmd_statuses(args, *help);
    assert_true(result);

    free(help);
}
