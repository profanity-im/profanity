#include <string.h>
#include <unistd.h>

#include <ncurses.h>

#include "profanity.h"
#include "log.h"
#include "windows.h"
#include "jabber.h"

#define AWAIT_COMMAND 1
#define START_MAIN 2
#define QUIT_PROF 3
#define CONTINUE_EVENT_LOOP 4

static int handle_start_command(char *cmd);
static int handle_main_command(char *cmd);

static void profanity_main(void);
static void profanity_event_loop(int *ch, char *cmd, int *size);

void profanity_start(void)
{
    int cmd_result = AWAIT_COMMAND;
    char cmd[50];

    while (cmd_result == AWAIT_COMMAND) {
        inp_get_command_str(cmd);
        cmd_result = handle_start_command(cmd);
    }

    if (cmd_result == START_MAIN) {
        profanity_main();
    }
}

static void profanity_main(void)
{
    int cmd_result = CONTINUE_EVENT_LOOP;

    inp_non_block();
    while(cmd_result == CONTINUE_EVENT_LOOP) {
        int ch = ERR;
        char cmd[100];
        int size = 0;

        while(ch != '\n')
            profanity_event_loop(&ch, cmd, &size);

        cmd[size++] = '\0';
        cmd_result = handle_main_command(cmd);
    }

    jabber_disconnect();
}

static void profanity_event_loop(int *ch, char *cmd, int *size)
{
    usleep(1);

    // handle incoming messages
    jabber_process_events();

    // determine if they changed windows
    if (*ch == KEY_F(1)) {
        switch_to(0);
    } else if (*ch == KEY_F(2)) {
        switch_to(1);
    } else if (*ch == KEY_F(3)) {
        switch_to(2);
    } else if (*ch == KEY_F(4)) {
        switch_to(3);
    } else if (*ch == KEY_F(5)) {
        switch_to(4);
    } else if (*ch == KEY_F(6)) {
        switch_to(5);
    } else if (*ch == KEY_F(7)) {
        switch_to(6);
    } else if (*ch == KEY_F(8)) {
        switch_to(7);
    } else if (*ch == KEY_F(9)) {
        switch_to(8);
    } else if (*ch == KEY_F(10)) {
        switch_to(9);
    }

    // get another character from the command box
    inp_poll_char(ch, cmd, size);
} 

static int handle_start_command(char *cmd)
{
    int result;

    if (strcmp(cmd, "/quit") == 0) {
        result = QUIT_PROF;
    } else if (strncmp(cmd, "/help", 5) == 0) {
        cons_help();
        inp_clear();
        result = AWAIT_COMMAND;
    } else if (strncmp(cmd, "/connect ", 9) == 0) {
        char *user;
        user = strndup(cmd+9, strlen(cmd)-9);

        inp_bar_print_message("Enter password:");
        char passwd[20];
        inp_get_password(passwd);

        inp_bar_print_message(user);
        jabber_connect(user, passwd);
        result = START_MAIN;
    } else {
        cons_bad_command(cmd);
        inp_clear();
        result = AWAIT_COMMAND;
    }

    return result;
}

static int handle_main_command(char *cmd)
{
    int result;

    // /quit command -> exit
    if (strcmp(cmd, "/quit") == 0) {
        result = QUIT_PROF;

    // /help -> print help to console
    } else if (strncmp(cmd, "/help", 5) == 0) {
        cons_help();
        inp_clear();
        result = CONTINUE_EVENT_LOOP;

    // /who -> request roster
    } else if (strncmp(cmd, "/who", 4) == 0) {
        jabber_roster_request();
        inp_clear();
        result = CONTINUE_EVENT_LOOP;

    // /msg -> send message to a user
    } else if (strncmp(cmd, "/msg ", 5) == 0) {
        char *usr_msg = NULL;
        char *usr = NULL;
        char *msg = NULL;
        
        usr_msg = strndup(cmd+5, strlen(cmd)-5);
        usr = strtok(usr_msg, " ");
        if (usr != NULL) {
            msg = strndup(cmd+5+strlen(usr)+1, strlen(cmd)-(5+strlen(usr)+1));
            if (msg != NULL) {
                jabber_send(msg, usr);
                show_outgoing_msg("me", usr, msg);
            }
        }
        inp_clear();
        result = CONTINUE_EVENT_LOOP;

    // /close -> close the current chat window, if in chat
    } else if (strncmp(cmd, "/close", 6) == 0) {
        if (in_chat()) {
            close_win();
        } else {
            cons_bad_command(cmd);
        }
        inp_clear();
        result = CONTINUE_EVENT_LOOP;

    // send message to recipient, if in chat
    } else {
        if (in_chat()) {
            char recipient[100] = "";
            get_recipient(recipient);
            jabber_send(cmd, recipient);
            show_outgoing_msg("me", recipient, cmd);
        } else {
            cons_bad_command(cmd);
        }
        inp_clear();
        result = CONTINUE_EVENT_LOOP;
    }

    return result;
}
