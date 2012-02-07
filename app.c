#include <string.h>
#include <unistd.h>

#include <strophe/strophe.h>
#include <ncurses.h>

#include "log.h"
#include "windows.h"
#include "jabber.h"

// window references
extern WINDOW *title_bar;
extern WINDOW *cmd_bar;
extern WINDOW *cmd_win;
extern WINDOW *main_win;

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
            char passwd[20];

            user = strndup(cmd+9, strlen(cmd)-9);

            bar_print_message("Enter password:");
            cmd_get_password(passwd);
            bar_print_message(user);

            jabber_connect(user, passwd);

            main_event_loop();

            break;
        } else {
            // echo ignore
            wclear(cmd_win);
            wmove(cmd_win, 0, 0);
        }
    }
}

static void main_event_loop(void)
{
    int cmd_y = 0;
    int cmd_x = 0;

    wtimeout(cmd_win, 0);

    while(TRUE) {
        int ch = ERR;
        char command[100];
        int size = 0;

        // while not enter
        while(ch != '\n') {
            usleep(1);
            // handle incoming messages
            jabber_process_events();

            // move cursor back to cmd_win
            getyx(cmd_win, cmd_y, cmd_x);
            wmove(cmd_win, cmd_y, cmd_x);

            // echo off, and get some more input
            noecho();
            ch = wgetch(cmd_win);

            // if delete pressed, go back and delete it
            if (ch == 127) {
                if (size > 0) {
                    getyx(cmd_win, cmd_y, cmd_x);
                    wmove(cmd_win, cmd_y, cmd_x-1);
                    wdelch(cmd_win);
                    size--;
                }
            }
            // else if not error or newline, show it and store it
            else if (ch != ERR && ch != '\n') {
                waddch(cmd_win, ch);
                command[size++] = ch;
            }

            echo();
        }

        command[size++] = '\0';

        // newline was hit, check if /quit command issued
        if (strcmp(command, "/quit") == 0) {
            break;
        } else {
            // send the message
            jabber_send(command);

            // show it in the main window
            wprintw(main_win, "me: %s\n", command);
            wrefresh(main_win);

            // move back to the command window and clear it
            wclear(cmd_win);
            wmove(cmd_win, 0, 0);
            wrefresh(cmd_win);
        }
    }

    jabber_disconnect();
}

