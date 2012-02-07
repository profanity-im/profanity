#include <string.h>
#include <unistd.h>

#include <ncurses.h>

#include "log.h"
#include "windows.h"
#include "jabber.h"

#define CHAT 0
#define CONS 1

static void main_event_loop(void);

void start_profanity(void)
{
    char cmd[50];
    while (TRUE) {
        inp_get_command_str(cmd);

        if (strcmp(cmd, "/quit") == 0) {
            break;
        } else if (strncmp(cmd, "/help", 5) == 0) {
            cons_help();
            cons_show();
            inp_clear();
        } else if (strncmp(cmd, "/connect ", 9) == 0) {
            char *user;
            user = strndup(cmd+9, strlen(cmd)-9);

            bar_print_message("Enter password:");
            char passwd[20];
            inp_get_password(passwd);

            bar_print_message(user);
            jabber_connect(user, passwd);
            chat_show();
            main_event_loop();
            break;
        } else {
            cons_bad_command(cmd);
            cons_show();
            inp_clear();
        }
    }
}

static void main_event_loop(void)
{
    int showing = CHAT;

    inp_non_block();

    while(TRUE) {
        int ch = ERR;
        char command[100];
        int size = 0;

        // while not enter, process all events, and try to get another char
        while(ch != '\n') {
            usleep(1);
        
            // handle incoming messages
            jabber_process_events();
            if (showing == CHAT) {
                chat_show();
            }

            // determine if they changed windows
            if (ch == KEY_F(1)) {
                cons_show();
                showing = CONS;
            } else if (ch == KEY_F(2)) {
                chat_show();
                showing = CHAT;
            }
            // get another character from the command box
            inp_poll_char(&ch, command, &size);
        }

        // null terminate the input    
        command[size++] = '\0';

        // newline was hit, check if /quit command issued
        if (strcmp(command, "/quit") == 0) {
            break;
        } else if (strncmp(command, "/help", 5) == 0) {
            cons_help();
            if (showing == CONS) {
                cons_show();
            }
            inp_clear();
        } else {
            if (showing == CONS) {
                cons_bad_command(command);
                cons_show();
            }
            else {
                jabber_send(command);
                show_outgoing_msg("me", command);
                chat_show();
            }
            inp_clear();
        }
    }

    jabber_disconnect();
}

