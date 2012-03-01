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

#define CONS_WIN_TITLE "_cons"
#define NUM_WINS 10

// holds console at index 0 and chat wins 1 through to 9
static struct prof_win _wins[NUM_WINS];

// the window currently being displayed
static int _curr_prof_win = 0;

// shortcut pointer to console window
static WINDOW * _cons_win = NULL;

static void _create_windows(void);
static int _find_prof_win_index(char *contact);
static int _new_prof_win(char *contact);
static void _current_window_refresh();
static void _win_switch_if_active(int i);
static void _win_show_time(WINDOW *win);
static void _win_show_user(WINDOW *win, char *user, int colour);
static void _win_show_message(WINDOW *win, char *message);
static void _win_show_contact_online(char *from, char *show, char *status);
static void _win_show_contact_offline(char *from, char *show, char *status);
static void _cons_show_contact_online(char *from, char *show, char *status);
static void _cons_show_contact_offline(char *from, char *show, char *status);

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

int win_close_win(void)
{
    if (win_in_chat()) {
        // reset the chat win to unused
        strcpy(_wins[_curr_prof_win].from, "");
        wclear(_wins[_curr_prof_win].win);

        // set it as inactive in the status bar
        status_bar_inactive(_curr_prof_win);
        
        // go back to console window
        _curr_prof_win = 0;
        title_bar_title();
    
        // success
        return 1;
    } else {
        // didn't close anything
        return 0;
    }
}

int win_in_chat(void)
{
    return ((_curr_prof_win != 0) && 
        (strcmp(_wins[_curr_prof_win].from, "") != 0));
}

char *win_get_recipient(void)
{
    struct prof_win current = _wins[_curr_prof_win];
    char *recipient = (char *) malloc(sizeof(char) * (strlen(current.from) + 1));
    strcpy(recipient, current.from);
    return recipient;
}

void win_show_incomming_msg(char *from, char *message) 
{
    char from_cpy[strlen(from) + 1];
    strcpy(from_cpy, from);
    char *short_from = strtok(from_cpy, "/");

    int win_index = _find_prof_win_index(short_from);
    if (win_index == NUM_WINS)
        win_index = _new_prof_win(short_from);

    WINDOW *win = _wins[win_index].win;
    _win_show_time(win);
    _win_show_user(win, short_from, 1);
    _win_show_message(win, message);

    status_bar_active(win_index);
}

void win_show_outgoing_msg(char *from, char *to, char *message)
{
    int win_index = _find_prof_win_index(to);
    if (win_index == NUM_WINS) 
        win_index = _new_prof_win(to);

    WINDOW *win = _wins[win_index].win;
    _win_show_time(win);
    _win_show_user(win, from, 0);
    _win_show_message(win, message);
    
    status_bar_active(win_index);
}

void win_contact_online(char *from, char *show, char *status)
{
    _cons_show_contact_online(from, show, status);
    _win_show_contact_online(from, show, status);
}

void win_contact_offline(char *from, char *show, char *status)
{
    _cons_show_contact_offline(from, show, status);
    _win_show_contact_offline(from, show, status);
}

void cons_help(void)
{
    _win_show_time(_cons_win);
    wprintw(_cons_win, "Help:\n");

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

void cons_bad_show(char *msg)
{
    _win_show_time(_cons_win);
    wattron(_cons_win, COLOR_PAIR(6));
    wprintw(_cons_win, "%s\n", msg);
    wattroff(_cons_win, COLOR_PAIR(6));
}

void cons_show(char *msg)
{
    _win_show_time(_cons_win);
    wprintw(_cons_win, "%s\n", msg); 
}

void cons_bad_command(char *cmd)
{
    _win_show_time(_cons_win);
    wprintw(_cons_win, "Unknown command: %s\n", cmd);
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
    strcpy(cons.from, CONS_WIN_TITLE);
    cons.win = newwin(rows-3, cols, 1, 0);
    scrollok(cons.win, TRUE);

    _wins[0] = cons;
    _cons_win = _wins[0].win;
    
    wattrset(_cons_win, A_BOLD);
    _win_show_time(_cons_win);
    wprintw(_cons_win, "Welcome to Profanity.\n");
    touchwin(_cons_win);
    wrefresh(_cons_win);
    
    // create the chat windows
    int i;
    for (i = 1; i < NUM_WINS; i++) {
        struct prof_win chat;
        strcpy(chat.from, "");
        chat.win = newwin(rows-3, cols, 1, 0);
        wattrset(chat.win, A_BOLD);
        scrollok(chat.win, TRUE);
        _wins[i] = chat;
    }    
}

static int _find_prof_win_index(char *contact)
{
    // find the chat window for recipient
    int i;
    for (i = 1; i < NUM_WINS; i++)
        if (strcmp(_wins[i].from, contact) == 0)
            break;

    return i;
}

static int _new_prof_win(char *contact)
{
    int i;
    // find the first unused one
    for (i = 1; i < NUM_WINS; i++)
        if (strcmp(_wins[i].from, "") == 0)
            break;

    // set it up
    strcpy(_wins[i].from, contact);
    wclear(_wins[i].win);

    return i;
}
static void _win_switch_if_active(int i)
{
    if (strcmp(_wins[i].from, "") != 0) {
        _curr_prof_win = i;

        if (i == 0)
            title_bar_title();
        else
            title_bar_show(_wins[i].from);
    }
}

static void _win_show_time(WINDOW *win)
{
    char tstmp[80];
    get_time(tstmp);
    wprintw(win, "%s - ", tstmp);
}

static void _win_show_user(WINDOW *win, char *user, int colour)
{
    if (colour)
        wattron(win, COLOR_PAIR(2));
    wprintw(win, "%s: ", user);
    if (colour)
        wattroff(win, COLOR_PAIR(2));
}

static void _win_show_message(WINDOW *win, char *message)
{
    wattroff(win, A_BOLD);
    wprintw(win, "%s\n", message);
    wattron(win, A_BOLD);
}

static void _current_window_refresh()
{
    WINDOW *current = _wins[_curr_prof_win].win;
    touchwin(current);
    wrefresh(current);
}

static void _win_show_contact_online(char *from, char *show, char *status)
{
    int win_index = _find_prof_win_index(from);
    if (win_index != NUM_WINS) {
        WINDOW *win = _wins[win_index].win;
        _win_show_time(win);    
        wattron(win, COLOR_PAIR(2));
        wprintw(win, "++ %s", from);

        if (show != NULL) 
            wprintw(win, " is %s", show);
        else
            wprintw(win, " is online");
            
        if (status != NULL)
            wprintw(win, ", \"%s\"", status);

        wprintw(win, "\n");
        
        wattroff(win, COLOR_PAIR(2));
    }
}

static void _win_show_contact_offline(char *from, char *show, char *status)
{
    int win_index = _find_prof_win_index(from);
    if (win_index != NUM_WINS) {
        WINDOW *win = _wins[win_index].win;
        _win_show_time(win);    
        wattron(win, COLOR_PAIR(5));
        wattroff(win, A_BOLD);

        wprintw(win, "-- %s", from);

        if (show != NULL) 
            wprintw(win, " is %s", show);
        else
            wprintw(win, " is offline");
        
        if (status != NULL)
            wprintw(win, ", \"%s\"", status);
        
        wprintw(win, "\n");
        
        wattroff(win, COLOR_PAIR(5));
        wattron(win, A_BOLD);
    }
}

static void _cons_show_contact_online(char *from, char *show, char *status)
{
    _win_show_time(_cons_win);    
    wattron(_cons_win, COLOR_PAIR(2));
   
    wprintw(_cons_win, "++ %s", from);

    if (show != NULL) 
        wprintw(_cons_win, " is %s", show);
    else
        wprintw(_cons_win, " is online");
        
    if (status != NULL)
        wprintw(_cons_win, ", \"%s\"", status);

    wprintw(_cons_win, "\n");
    
    wattroff(_cons_win, COLOR_PAIR(2));
}

static void _cons_show_contact_offline(char *from, char *show, char *status)
{
    _win_show_time(_cons_win);    
    wattron(_cons_win, COLOR_PAIR(5));
    wattroff(_cons_win, A_BOLD);

    wprintw(_cons_win, "-- %s", from);

    if (show != NULL) 
        wprintw(_cons_win, " is %s", show);
    else
        wprintw(_cons_win, " is offline");
        
    if (status != NULL)
        wprintw(_cons_win, ", \"%s\"", status);
    
    wprintw(_cons_win, "\n");
    
    wattroff(_cons_win, COLOR_PAIR(5));
    wattron(_cons_win, A_BOLD);
}

