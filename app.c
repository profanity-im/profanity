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

            // determine if they changed windows
            if (ch == KEY_F(1)) {
                switch_to(0);
            } else if (ch == KEY_F(2)) {
                switch_to(1);
            } else if (ch == KEY_F(3)) {
                switch_to(2);
            } else if (ch == KEY_F(4)) {
                switch_to(3);
            } else if (ch == KEY_F(5)) {
                switch_to(4);
            } else if (ch == KEY_F(6)) {
                switch_to(5);
            } else if (ch == KEY_F(7)) {
                switch_to(6);
            } else if (ch == KEY_F(8)) {
                switch_to(7);
            } else if (ch == KEY_F(9)) {
                switch_to(8);
            } else if (ch == KEY_F(10)) {
                switch_to(9);
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
            inp_clear();
        } else {
            jabber_send(command);
            show_outgoing_msg("me", command);
            inp_clear();
        }
    }

    jabber_disconnect();
}

