#include <string.h>
#include <unistd.h>

#include <ncurses.h>

#include "profanity.h"
#include "log.h"
#include "windows.h"
#include "jabber.h"
#include "command.h"

static void _profanity_main(void);
static void _profanity_event_loop(int *ch, char *cmd, int *size);

void profanity_start(void)
{
    int cmd_result = AWAIT_COMMAND;
    char cmd[50];

    while (cmd_result == AWAIT_COMMAND) {
        inp_get_command_str(cmd);
        cmd_result = handle_start_command(cmd);
    }

    if (cmd_result == START_MAIN) {
        _profanity_main();
    }
}

static void _profanity_main(void)
{
    int cmd_result = TRUE;

    inp_non_block();
    while(cmd_result == TRUE) {
        int ch = ERR;
        char cmd[100];
        int size = 0;

        while(ch != '\n')
            _profanity_event_loop(&ch, cmd, &size);

        cmd[size++] = '\0';
        cmd_result = handle_command(cmd);
    }

    jabber_disconnect();
}

static void _profanity_event_loop(int *ch, char *cmd, int *size)
{
    usleep(1);

    inp_bar_update_time();

    // handle incoming messages
    jabber_process_events();

    // determine if they changed windows
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

    // get another character from the command box
    inp_poll_char(ch, cmd, size);
} 
