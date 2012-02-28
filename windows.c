/* 
 * windows.c
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
#include "windows.h"
#include "util.h"

static struct prof_win _wins[10];
static int _curr_win = 0;

static void _create_windows(void);
static int _find_win(char *contact);
static void _current_window_refresh();
static void _win_switch_if_active(int i);
static void _win_show_time(int win);
static void _win_show_user(int win, char *user, int colour);

void gui_init(void)
{
    initscr();
    cbreak();
    keypad(stdscr, TRUE);

    if (has_colors()) {    
        start_color();
        
        init_pair(1, COLOR_WHITE, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_WHITE, COLOR_BLUE);
        init_pair(4, COLOR_CYAN, COLOR_BLUE);
        init_pair(5, COLOR_CYAN, COLOR_BLACK);
        init_pair(6, COLOR_RED, COLOR_BLACK);
        init_pair(7, COLOR_MAGENTA, COLOR_BLACK);
    }

    refresh();

    create_title_bar();
    create_status_bar();
    create_input_window();
    _create_windows();
}

void gui_refresh(void)
{
    title_bar_refresh();
    status_bar_refresh();
    _current_window_refresh();
    inp_put_back();
}

void gui_close(void)
{
    endwin();
}

void win_close_win(void)
{
    // reset the chat win to unused
    strcpy(_wins[_curr_win].from, "");
    wclear(_wins[_curr_win].win);

    // set it as inactive in the status bar
    status_bar_inactive(_curr_win);
    
    // go back to console window
    _curr_win = 0;
    title_bar_title();
}

int win_in_chat(void)
{
    return ((_curr_win != 0) && (strcmp(_wins[_curr_win].from, "") != 0));
}

char *win_get_recipient(void)
{
    char *recipient = (char *) malloc(sizeof(char) * (strlen(_wins[_curr_win].from) + 1));
    strcpy(recipient, _wins[_curr_win].from);
    return recipient;
}

void win_show_incomming_msg(char *from, char *message) 
{
    char from_cpy[strlen(from) + 1];
    strcpy(from_cpy, from);
    char *short_from = strtok(from_cpy, "/");

    int win = _find_win(short_from);
    _win_show_time(win);
    _win_show_user(win, short_from, 1);
    wprintw(_wins[win].win, "%s\n", message);

    status_bar_active(win);
}

void win_show_outgoing_msg(char *from, char *to, char *message)
{
    int win = _find_win(to);
    _win_show_time(win);
    _win_show_user(win, from, 0);
    wprintw(_wins[win].win, "%s\n", message);
    
    status_bar_active(win);
}

void win_show_contact_online(char *from, char *show, char *status)
{
    // find the chat window for recipient
    int i;
    for (i = 1; i < 10; i++)
        if (strcmp(_wins[i].from, from) == 0)
            break;

    // if we found a window
    if (i != 10) {
        _win_show_time(i);    
        wattron(_wins[i].win, COLOR_PAIR(2));
        wattron(_wins[i].win, A_BOLD);
       
        wprintw(_wins[i].win, "++ %s", from);

        if (show != NULL) 
            wprintw(_wins[i].win, " is %s", show);
        else
            wprintw(_wins[i].win, " is online");
            
        if (status != NULL)
            wprintw(_wins[i].win, ", \"%s\"", status);

        wprintw(_wins[i].win, "\n");
        
        wattroff(_wins[i].win, COLOR_PAIR(2));
        wattroff(_wins[i].win, A_BOLD);
    }
}

void win_show_contact_offline(char *from, char *show, char *status)
{
    // find the chat window for recipient
    int i;
    for (i = 1; i < 10; i++)
        if (strcmp(_wins[i].from, from) == 0)
            break;

    // if we found a window
    if (i != 10) {
        _win_show_time(i);    
        wattron(_wins[i].win, COLOR_PAIR(5));

        wprintw(_wins[i].win, "-- %s", from);

        if (show != NULL) 
            wprintw(_wins[i].win, " is %s", show);
        else
            wprintw(_wins[i].win, " is offline");
        
        if (status != NULL)
            wprintw(_wins[i].win, ", \"%s\"", status);
        
        wprintw(_wins[i].win, "\n");
        
        wattroff(_wins[i].win, COLOR_PAIR(5));
    }
}

void cons_help(void)
{
    _win_show_time(0);
    wattron(_wins[0].win, A_BOLD);
    wprintw(_wins[0].win, "Help:\n");
    wattroff(_wins[0].win, A_BOLD);

    cons_show("  Commands:");
    cons_show("    /help                : This help.");
    cons_show("    /connect user@host   : Login to jabber.");
    cons_show("    /who                 : Get roster.");
    cons_show("    /close               : Close a chat window.");
    cons_show("    /msg user@host mesg  : Send mesg to user.");
    cons_show("    /quit                : Quit Profanity.");
    cons_show("  Shortcuts:");
    cons_show("    F1                   : This console window.");
    cons_show("    F2-10                : Chat windows.");
}

void cons_good_show(char *msg)
{
    _win_show_time(0);    
    wattron(_wins[0].win, A_BOLD);
    wprintw(_wins[0].win, "%s\n", msg);
    wattroff(_wins[0].win, A_BOLD);
}

void cons_bad_show(char *msg)
{
    _win_show_time(0);
    wattron(_wins[0].win, COLOR_PAIR(6));
    wattron(_wins[0].win, A_BOLD);
    wprintw(_wins[0].win, "%s\n", msg);
    wattroff(_wins[0].win, COLOR_PAIR(6));
    wattroff(_wins[0].win, A_BOLD);
}

void cons_show(char *msg)
{
    _win_show_time(0);
    wattron(_wins[0].win, A_BOLD);
    wprintw(_wins[0].win, "%s\n", msg); 
    wattroff(_wins[0].win, A_BOLD);
}

void cons_bad_command(char *cmd)
{
    _win_show_time(0);
    wattron(_wins[0].win, A_BOLD);
    wprintw(_wins[0].win, "Unknown command: %s\n", cmd);
    wattroff(_wins[0].win, A_BOLD);
}

void cons_bad_connect(void)
{
    cons_show("Usage: /connect user@host");
}

void cons_not_disconnected(void)
{
    cons_show("You are either connected already, or a login is in process.");
}

void cons_not_connected(void)
{
    cons_show("You are not currently connected.");
}

void cons_bad_message(void)
{
    cons_show("Usage: /msg user@host message");
}

void cons_show_contact_online(char *from, char *show, char *status)
{
    _win_show_time(0);    
    wattron(_wins[0].win, COLOR_PAIR(2));
    wattron(_wins[0].win, A_BOLD);
   
    wprintw(_wins[0].win, "++ %s", from);

    if (show != NULL) 
        wprintw(_wins[0].win, " is %s", show);
    else
        wprintw(_wins[0].win, " is online");
        
    if (status != NULL)
        wprintw(_wins[0].win, ", \"%s\"", status);

    wprintw(_wins[0].win, "\n");
    
    wattroff(_wins[0].win, COLOR_PAIR(2));
    wattroff(_wins[0].win, A_BOLD);
}

void cons_show_contact_offline(char *from, char *show, char *status)
{
    _win_show_time(0);    
    wattron(_wins[0].win, COLOR_PAIR(5));

    wprintw(_wins[0].win, "-- %s", from);

    if (show != NULL) 
        wprintw(_wins[0].win, " is %s", show);
    else
        wprintw(_wins[0].win, " is offline");
        
    if (status != NULL)
        wprintw(_wins[0].win, ", \"%s\"", status);
    
    wprintw(_wins[0].win, "\n");
    
    wattroff(_wins[0].win, COLOR_PAIR(5));
}

void win_handle_switch(int *ch)
{
    if (*ch == KEY_F(1)) {
        _win_switch_if_active(0);
    } else if (*ch == KEY_F(2)) {
        _win_switch_if_active(1);
    } else if (*ch == KEY_F(3)) {
        _win_switch_if_active(2);
    } else if (*ch == KEY_F(4)) {
        _win_switch_if_active(3);
    } else if (*ch == KEY_F(5)) {
        _win_switch_if_active(4);
    } else if (*ch == KEY_F(6)) {
        _win_switch_if_active(5);
    } else if (*ch == KEY_F(7)) {
        _win_switch_if_active(6);
    } else if (*ch == KEY_F(8)) {
        _win_switch_if_active(7);
    } else if (*ch == KEY_F(9)) {
        _win_switch_if_active(8);
    } else if (*ch == KEY_F(10)) {
        _win_switch_if_active(9);
    }
}

static void _create_windows(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // create the console window in 0
    struct prof_win cons;
    strcpy(cons.from, "_cons");
    cons.win = newwin(rows-3, cols, 1, 0);
    scrollok(cons.win, TRUE);

    _wins[0] = cons;
    _win_show_time(0);
    wattron(_wins[0].win, A_BOLD);
    wprintw(_wins[0].win, "Welcome to Profanity.\n");
    wattroff(_wins[0].win, A_BOLD);
    touchwin(_wins[0].win);
    wrefresh(_wins[0].win);
    
    // create the chat windows
    int i;
    for (i = 1; i < 10; i++) {
        struct prof_win chat;
        strcpy(chat.from, "");
        chat.win = newwin(rows-3, cols, 1, 0);
        scrollok(chat.win, TRUE);
        _wins[i] = chat;
    }    
}

static int _find_win(char *contact)
{
    // find the chat window for recipient
    int i;
    for (i = 1; i < 10; i++)
        if (strcmp(_wins[i].from, contact) == 0)
            break;

    // if we didn't find a window
    if (i == 10) {
        // find the first unused one
        for (i = 1; i < 10; i++)
            if (strcmp(_wins[i].from, "") == 0)
                break;

        // set it up
        strcpy(_wins[i].from, contact);
        wclear(_wins[i].win);
    }

    return i;
}

static void _win_switch_if_active(int i)
{
    if (strcmp(_wins[i].from, "") != 0) {
        _curr_win = i;

        if (i == 0)
            title_bar_title();
        else
            title_bar_show(_wins[i].from);
    }
}

static void _win_show_time(int win)
{
    char tstmp[80];
    get_time(tstmp);
    wattron(_wins[win].win, A_BOLD);
    wprintw(_wins[win].win, "%s - ", tstmp);
    wattroff(_wins[win].win, A_BOLD);
}

static void _win_show_user(int win, char *user, int colour)
{
    if (colour)
        wattron(_wins[win].win, COLOR_PAIR(2));
    wattron(_wins[win].win, A_BOLD);
    wprintw(_wins[win].win, "%s: ", user);
    if (colour)
        wattroff(_wins[win].win, COLOR_PAIR(2));
    wattroff(_wins[win].win, A_BOLD);
}

static void _current_window_refresh()
{
    touchwin(_wins[_curr_win].win);
    wrefresh(_wins[_curr_win].win);
}
