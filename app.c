#include <string.h>
#include <unistd.h>

#include <ncurses.h>

#include "log.h"
#include "windows.h"
#include "jabber.h"

static void main_event_loop(void);

void start_profanity(void)
{
    char cmd[50];
    while (TRUE) {
        cmd_get_command_str(cmd);

        if (strcmp(cmd, "/quit") == 0) {
            break;
        } else if (strncmp(cmd, "/connect ", 9) == 0) {
            char *user;
            user = strndup(cmd+9, strlen(cmd)-9);

            bar_print_message("Enter password:");
            char passwd[20];
            cmd_get_password(passwd);

            bar_print_message(user);
            jabber_connect(user, passwd);
            main_event_loop();
            break;
        } else {
            cmd_clear();
        }
    }
}

static void main_event_loop(void)
{
    cmd_non_block();

    while(TRUE) {
        int ch = ERR;
        char command[100];
        int size = 0;

        // while not enter, process all events, and try to get another char
        while(ch != '\n') {
            usleep(1);
        
            // handle incoming messages
            jabber_process_events();

            // get another character from the command box
            cmd_poll_char(&ch, command, &size);
        }

        // null terminate the input    
        command[size++] = '\0';

        // newline was hit, check if /quit command issued
        if (strcmp(command, "/quit") == 0) {
            break;
        } else {
            jabber_send(command);
            show_outgoing_msg("me", command);
            cmd_clear();
        }
    }

    jabber_disconnect();
}

