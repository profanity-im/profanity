/* 
 * command.c
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
 * 
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include "command.h"
#include "jabber.h"
#include "windows.h"
#include "util.h"

static int _handle_command(char *command, char *inp);
static int _cmd_quit(void);
static int _cmd_help(void);
static int _cmd_who(void);
static int _cmd_connect(char *inp);
static int _cmd_msg(char *inp);
static int _cmd_close(char *inp);
static int _cmd_default(char *inp);

int process_input(char *inp)
{
    int result = FALSE;

    if (strlen(inp) == 0) {
        result = TRUE;
    } else if (inp[0] == '/') {
        inp = trim(inp);
        char inp_cpy[strlen(inp) + 1];
        strcpy(inp_cpy, inp);
        char *command = strtok(inp_cpy, " ");
        result = _handle_command(command, inp);
    } else {
        result = _cmd_default(inp);
    }

    inp_clear();

    return result;
}

static int _handle_command(char *command, char *inp)
{
    int result = FALSE;

    if (strcmp(command, "/quit") == 0) {
        result = _cmd_quit();
    } else if (strcmp(command, "/help") == 0) {
        result = _cmd_help();
    } else if (strcmp(command, "/who") == 0) {
        result = _cmd_who();
    } else if (strcmp(command, "/msg") == 0) {
        result = _cmd_msg(inp);
    } else if (strcmp(command, "/close") == 0) {
        result = _cmd_close(inp);
    } else if (strcmp(command, "/connect") == 0) {
        result = _cmd_connect(inp);
    } else {
        result = _cmd_default(inp);
    }

    return result;
}

static int _cmd_connect(char *inp)
{
    int result = FALSE;
    jabber_status_t conn_status = jabber_connection_status();

    if ((conn_status != JABBER_DISCONNECTED) && (conn_status != JABBER_STARTED)) {
        cons_not_disconnected();
        result = TRUE;
    } else if (strlen(inp) < 10) {
        cons_bad_connect();
        result = TRUE;
    } else {
        char *user;
        user = strndup(inp+9, strlen(inp)-9);
        status_bar_get_password();
        status_bar_refresh();
        char passwd[21];
        inp_block();
        inp_get_password(passwd);
        inp_non_block();
        
        conn_status = jabber_connect(user, passwd);
        if (conn_status == JABBER_CONNECTING)
            cons_good_show("Connecting...");
        if (conn_status == JABBER_DISCONNECTED)
            cons_bad_show("Connection to server failed.");

        result = TRUE;
    }
    
    return result;
}

static int _cmd_quit(void)
{
    return FALSE;
}

static int _cmd_help(void)
{
    cons_help();

    return TRUE;
}

static int _cmd_who(void)
{
    jabber_status_t conn_status = jabber_connection_status();

    if (conn_status != JABBER_CONNECTED)
        cons_not_connected();
    else
        jabber_roster_request();

    return TRUE;
}

static int _cmd_msg(char *inp)
{
    char *usr = NULL;
    char *msg = NULL;

    jabber_status_t conn_status = jabber_connection_status();

    if (conn_status != JABBER_CONNECTED) {
        cons_not_connected();
    } else {
        // copy input    
        char inp_cpy[strlen(inp) + 1];
        strcpy(inp_cpy, inp);

        // get user
        strtok(inp_cpy, " ");
        usr = strtok(NULL, " ");
        if ((usr != NULL) && (strlen(inp) > (5 + strlen(usr) + 1))) {
            msg = strndup(inp+5+strlen(usr)+1, strlen(inp)-(5+strlen(usr)+1));
            if (msg != NULL) {
                jabber_send(msg, usr);
                win_show_outgoing_msg("me", usr, msg);
            } else {
                cons_bad_message();
            }
        } else {
            cons_bad_message();
        }
    }

    return TRUE;
}

static int _cmd_close(char *inp)
{
    if (win_in_chat()) {
        win_close_win();
    } else {
        cons_bad_command(inp);
    }
    
    return TRUE;
}

static int _cmd_default(char *inp)
{
    if (win_in_chat()) {
        char *recipient = win_get_recipient();
        jabber_send(inp, recipient);
        win_show_outgoing_msg("me", recipient, inp);
        free(recipient);
    } else {
        cons_bad_command(inp);
    }

    return TRUE;
}
