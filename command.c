#include <string.h>
#include <ncurses.h>
#include "command.h"
#include "jabber.h"
#include "windows.h"

static int cmd_quit(void);
static int cmd_help(void);
static int cmd_who(void);
static int cmd_msg(char *cmd);
static int cmd_close(char *cmd);
static int cmd_default(char *cmd);

int handle_start_command(char *cmd)
{
    int result;

    if (strcmp(cmd, "/quit") == 0) {
        result = QUIT_PROF;
    } else if (strncmp(cmd, "/help", 5) == 0) {
        cons_help();
        result = AWAIT_COMMAND;
    } else if (strncmp(cmd, "/connect ", 9) == 0) {
        char *user;
        user = strndup(cmd+9, strlen(cmd)-9);

        inp_bar_get_password();
        char passwd[20];
        inp_get_password(passwd);

        inp_bar_print_message(user);
        jabber_connect(user, passwd);
        result = START_MAIN;
    } else {
        cons_bad_command(cmd);
        result = AWAIT_COMMAND;
    }

    inp_clear();

    return result;
}

int handle_command(char *cmd)
{
    int result = FALSE;
    if (strcmp(cmd, "/quit") == 0) {
        result = cmd_quit();
    } else if (strncmp(cmd, "/help", 5) == 0) {
        result = cmd_help();
    } else if (strncmp(cmd, "/who", 4) == 0) {
        result = cmd_who();
    } else if (strncmp(cmd, "/msg ", 5) == 0) {
        result = cmd_msg(cmd);
    } else if (strncmp(cmd, "/close", 6) == 0) {
        result = cmd_close(cmd);
    } else {
        result = cmd_default(cmd);
    }

    inp_clear();

    return result;

}

static int cmd_quit(void)
{
    return FALSE;
}

static int cmd_help(void)
{
    cons_help();

    return TRUE;
}

static int cmd_who(void)
{
    jabber_roster_request();

    return TRUE;
}

static int cmd_msg(char *cmd)
{
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

    return TRUE;
}

static int cmd_close(char *cmd)
{
    if (in_chat()) {
        close_win();
    } else {
        cons_bad_command(cmd);
    }
    
    return TRUE;
}

static int cmd_default(char *cmd)
{
    if (in_chat()) {
        char recipient[100] = "";
        get_recipient(recipient);
        jabber_send(cmd, recipient);
        show_outgoing_msg("me", recipient, cmd);
    } else {
        cons_bad_command(cmd);
    }

    return TRUE;
}
