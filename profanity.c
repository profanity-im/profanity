#include <string.h>
#include <unistd.h>

#include <ncurses.h>

#include "profanity.h"
#include "log.h"
#include "windows.h"
#include "jabber.h"
#include "command.h"

static int _profanity_main(void);
static void _profanity_event_loop(int *ch, char *cmd, int *size);
static void _process_special_keys(int *ch);

int profanity_start(void)
{
    int exit_status = QUIT;
    int cmd_result = AWAIT_COMMAND;
    char cmd[50];

    title_bar_disconnected();
    gui_refresh();

    while (cmd_result == AWAIT_COMMAND) {
        title_bar_refresh();
        status_bar_refresh();
        inp_get_command_str(cmd);
        cmd_result = handle_start_command(cmd);
    }

    if (cmd_result == START_MAIN) {
        exit_status = _profanity_main();
    }

    return exit_status;
}

static int _profanity_main(void)
{
    int cmd_result = TRUE;

    inp_non_block();
    while(cmd_result == TRUE) {
        int ch = ERR;
        char cmd[100];
        int size = 0;

        while(ch != '\n') {
            _profanity_event_loop(&ch, cmd, &size);

            int conn_status = jabber_connection_status();
            if (conn_status == DISCONNECTED) {
                inp_block();
                return LOGIN_FAIL;
            }
        }

        cmd[size++] = '\0';
        cmd_result = handle_command(cmd);
    }

    jabber_disconnect();
    return QUIT;
}

static void _profanity_event_loop(int *ch, char *cmd, int *size)
{
    usleep(1);
    gui_refresh();
    jabber_process_events();
    _process_special_keys(ch);
    inp_poll_char(ch, cmd, size);
} 

static void _process_special_keys(int *ch)
{
    if (*ch == KEY_F(1)) {
        if (win_is_active(0))
            win_switch_to(0);
    } else if (*ch == KEY_F(2)) {
        if (win_is_active(1))
            win_switch_to(1);
    } else if (*ch == KEY_F(3)) {
        if (win_is_active(2))
            win_switch_to(2);
    } else if (*ch == KEY_F(4)) {
        if (win_is_active(3))
            win_switch_to(3);
    } else if (*ch == KEY_F(5)) {
        if (win_is_active(4))
            win_switch_to(4);
    } else if (*ch == KEY_F(6)) {
        if (win_is_active(5))
            win_switch_to(5);
    } else if (*ch == KEY_F(7)) {
        if (win_is_active(6))
            win_switch_to(6);
    } else if (*ch == KEY_F(8)) {
        if (win_is_active(7))
            win_switch_to(7);
    } else if (*ch == KEY_F(9)) {
        if (win_is_active(8))
            win_switch_to(8);
    } else if (*ch == KEY_F(10)) {
        if (win_is_active(9))
            win_switch_to(9);
    }
}
