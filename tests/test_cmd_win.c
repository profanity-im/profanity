#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <glib.h>

#include "ui/ui.h"
#include "ui/mock_ui.h"

#include "command/commands.h"

void cmd_win_shows_message_when_win_doesnt_exist(void **state)
{
    mock_cons_show();
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "3", NULL };

    ui_switch_win_expect_and_return(3, FALSE);
    expect_cons_show("Window 3 does not exist.");

    gboolean result = cmd_win(args, *help);
    assert_true(result);

    free(help);
}

void cmd_win_switches_to_given_win_when_exists(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "12", NULL };

    ui_switch_win_expect_and_return(12, TRUE);

    gboolean result = cmd_win(args, *help);
    assert_true(result);

    free(help);
}
