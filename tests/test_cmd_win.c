#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <glib.h>

#include "ui/ui.h"
#include "ui/stub_ui.h"

#include "command/commands.h"

void cmd_win_shows_message_when_win_doesnt_exist(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "3", NULL };

    expect_value(ui_switch_win, i, 3);
    will_return(ui_switch_win, FALSE);

    expect_cons_show("Window 3 does not exist.");

    gboolean result = cmd_win(args, *help);
    assert_true(result);

    free(help);
}

void cmd_win_switches_to_given_win_when_exists(void **state)
{
    CommandHelp *help = malloc(sizeof(CommandHelp));
    gchar *args[] = { "12", NULL };

    expect_value(ui_switch_win, i, 12);
    will_return(ui_switch_win, TRUE);

    gboolean result = cmd_win(args, *help);
    assert_true(result);

    free(help);
}
