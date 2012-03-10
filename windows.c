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
#define PAD_SIZE 200
#define NUM_WINS 10

// holds console at index 0 and chat wins 1 through to 9
static struct prof_win _wins[NUM_WINS];

// the window currently being displayed
static int _curr_prof_win = 0;

// shortcut pointer to console window
static WINDOW * _cons_win = NULL;

// current window state
static int dirty;

static void _create_windows(void);
static int _find_prof_win_index(const char * const contact);
static int _new_prof_win(const char * const contact);
static void _current_window_refresh(void);
static void _win_switch_if_active(const int i);
static void _win_show_time(WINDOW *win);
static void _win_show_user(WINDOW *win, const char * const user, const int colour);
static void _win_show_message(WINDOW *win, const char * const message);
static void _show_status_string(WINDOW *win, const char * const from, 
    const char * const show, const char * const status, const char * const pre, 
    const char * const default_show);
static void _cons_show_incoming_message(const char * const short_from, 
    const int win_index);

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
        init_pair(8, COLOR_YELLOW, COLOR_BLACK);
    }

    refresh();

    create_title_bar();
    create_status_bar();
    create_input_window();
    _create_windows();

    dirty = TRUE;
}

void gui_refresh(void)
{
    title_bar_refresh();
    status_bar_refresh();

    if (dirty) {
        _current_window_refresh();
        dirty = FALSE;
    }

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

        dirty = TRUE;
    
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

void win_show_incomming_msg(const char * const from, const char * const message) 
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
    _cons_show_incoming_message(short_from, win_index);

    if (win_index == _curr_prof_win)
        dirty = TRUE;
}

void win_show_outgoing_msg(const char * const from, const char * const to, 
    const char * const message)
{
    int win_index = _find_prof_win_index(to);
    if (win_index == NUM_WINS) 
        win_index = _new_prof_win(to);

    WINDOW *win = _wins[win_index].win;
    _win_show_time(win);
    _win_show_user(win, from, 0);
    _win_show_message(win, message);
    
    status_bar_active(win_index);
    
    if (win_index == _curr_prof_win)
        dirty = TRUE;
}

void win_contact_online(const char * const from, const char * const show, 
    const char * const status)
{
    _show_status_string(_cons_win, from, show, status, "++", "online");

    int win_index = _find_prof_win_index(from);
    if (win_index != NUM_WINS) {
        WINDOW *win = _wins[win_index].win;
        _show_status_string(win, from, show, status, "++", "online");
    }
    
    if (win_index == _curr_prof_win)
        dirty = TRUE;
}

void win_contact_offline(const char * const from, const char * const show, 
    const char * const status)
{
    _show_status_string(_cons_win, from, show, status, "--", "offline");

    int win_index = _find_prof_win_index(from);
    if (win_index != NUM_WINS) {
        WINDOW *win = _wins[win_index].win;
        _show_status_string(win, from, show, status, "--", "offline");
    }
    
    if (win_index == _curr_prof_win)
        dirty = TRUE;
}

void cons_help(void)
{
    _win_show_time(_cons_win);
    wprintw(_cons_win, "Help:\n");

    cons_show("/help                : This help.");
    cons_show("/connect user@host   : Login to jabber.");
    cons_show("/msg user@host mesg  : Send mesg to user.");
    cons_show("/who                 : Find out who is online.");
    cons_show("/ros                 : List all contacts.");
    cons_show("/close               : Close a chat window.");
    cons_show("/quit                : Quit Profanity.");
    cons_show("F1                   : This console window.");
    cons_show("F2-10                : Chat windows.");
    cons_show("UP, DOWN             : Navigate input history.");
    cons_show("LEFT, RIGHT          : Edit current input.");
    cons_show("PAGE UP, PAGE DOWN   : Page the chat window.");

    if (_curr_prof_win == 0)
        dirty = TRUE;
}

void cons_show_online_contacts(const contact_list_t * const list)
{
    _win_show_time(_cons_win);
    wprintw(_cons_win, "Online contacts:\n");

    int i;
    for (i = 0; i < list->size; i++) {
        contact_t *contact = list->contacts[i];
        _win_show_time(_cons_win);
        wattron(_cons_win, COLOR_PAIR(2));
        wprintw(_cons_win, "%s", contact->name);
        if (contact->show)
            wprintw(_cons_win, " is %s", contact->show);
        if (contact->status)
            wprintw(_cons_win, ", \"%s\"", contact->status);
        wprintw(_cons_win, "\n");
        wattroff(_cons_win, COLOR_PAIR(2));
    }
}

void cons_bad_show(const char * const msg)
{
    _win_show_time(_cons_win);
    wattron(_cons_win, COLOR_PAIR(6));
    wprintw(_cons_win, "%s\n", msg);
    wattroff(_cons_win, COLOR_PAIR(6));
    
    if (_curr_prof_win == 0)
        dirty = TRUE;
}

void cons_show(const char * const msg)
{
    _win_show_time(_cons_win);
    wprintw(_cons_win, "%s\n", msg); 
    
    if (_curr_prof_win == 0)
        dirty = TRUE;
}

void cons_bad_command(const char * const cmd)
{
    _win_show_time(_cons_win);
    wprintw(_cons_win, "Unknown command: %s\n", cmd);
    
    if (_curr_prof_win == 0)
        dirty = TRUE;
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

void win_handle_switch(const int * const ch)
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

void win_page_off(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    _wins[_curr_prof_win].paged = 0;
    
    int y, x;
    getyx(_wins[_curr_prof_win].win, y, x);

    int size = rows - 3;

    _wins[_curr_prof_win].y_pos = y - (size - 1);
    if (_wins[_curr_prof_win].y_pos < 0)
        _wins[_curr_prof_win].y_pos = 0;

    dirty = TRUE;
}

void win_handle_page(const int * const ch)
{
    int rows, cols, y, x;
    getmaxyx(stdscr, rows, cols);
    getyx(_wins[_curr_prof_win].win, y, x);

    int page_space = rows - 4;
    int *page_start = &_wins[_curr_prof_win].y_pos;
    
    // page up
    if (*ch == KEY_PPAGE) {
        *page_start -= page_space;
    
        // went past beginning, show first page
        if (*page_start < 0)
            *page_start = 0;
       
        _wins[_curr_prof_win].paged = 1;
        dirty = TRUE;

    // page down
    } else if (*ch == KEY_NPAGE) {
        *page_start += page_space;

        // only got half a screen, show full screen
        if ((y - (*page_start)) < page_space)
            *page_start = y - page_space;

        // went past end, show full screen
        else if (*page_start >= y)
            *page_start = y - page_space;
           
        _wins[_curr_prof_win].paged = 1;
        dirty = TRUE;
    }
}

static void _create_windows(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // create the console window in 0
    struct prof_win cons;
    strcpy(cons.from, CONS_WIN_TITLE);
    cons.win = newpad(PAD_SIZE, cols);
    cons.y_pos = 0;
    cons.paged = 0;
    scrollok(cons.win, TRUE);

    _wins[0] = cons;
    _cons_win = _wins[0].win;
    
    wattrset(_cons_win, A_BOLD);
    _win_show_time(_cons_win);
    wprintw(_cons_win, "Welcome to Profanity.\n");
    prefresh(_cons_win, 0, 0, 1, 0, rows-3, cols-1);

    dirty = TRUE;
    
    // create the chat windows
    int i;
    for (i = 1; i < NUM_WINS; i++) {
        struct prof_win chat;
        strcpy(chat.from, "");
        chat.win = newpad(PAD_SIZE, cols);
        chat.y_pos = 0;
        chat.paged = 0;
        wattrset(chat.win, A_BOLD);
        scrollok(chat.win, TRUE);
        _wins[i] = chat;
    }    
}

static int _find_prof_win_index(const char * const contact)
{
    // find the chat window for recipient
    int i;
    for (i = 1; i < NUM_WINS; i++)
        if (strcmp(_wins[i].from, contact) == 0)
            break;

    return i;
}

static int _new_prof_win(const char * const contact)
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
static void _win_switch_if_active(const int i)
{
    win_page_off();
    if (strcmp(_wins[i].from, "") != 0) {
        _curr_prof_win = i;
        win_page_off();

        if (i == 0)
            title_bar_title();
        else
            title_bar_show(_wins[i].from);
    }

    dirty = TRUE;
}

static void _win_show_time(WINDOW *win)
{
    char tstmp[80];
    get_time(tstmp);
    wprintw(win, "%s - ", tstmp);
}

static void _win_show_user(WINDOW *win, const char * const user, const int colour)
{
    if (colour)
        wattron(win, COLOR_PAIR(2));
    wprintw(win, "%s: ", user);
    if (colour)
        wattroff(win, COLOR_PAIR(2));
}

static void _win_show_message(WINDOW *win, const char * const message)
{
    wattroff(win, A_BOLD);
    wprintw(win, "%s\n", message);
    wattron(win, A_BOLD);
}

static void _current_window_refresh()
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    WINDOW *current = _wins[_curr_prof_win].win;
    prefresh(current, _wins[_curr_prof_win].y_pos, 0, 1, 0, rows-3, cols-1);
}

static void _show_status_string(WINDOW *win, const char * const from, 
    const char * const show, const char * const status, const char * const pre, 
    const char * const default_show)
{
    _win_show_time(win);    
    if (strcmp(default_show, "online") == 0) {
        wattron(win, COLOR_PAIR(2));
    } else {
        wattron(win, COLOR_PAIR(5));
        wattroff(win, A_BOLD);
    }

    wprintw(win, "%s %s", pre, from);

    if (show != NULL) 
        wprintw(win, " is %s", show);
    else
        wprintw(win, " is %s", default_show);
        
    if (status != NULL)
        wprintw(win, ", \"%s\"", status);
    
    wprintw(win, "\n");
    
    if (strcmp(default_show, "online") == 0) {
        wattroff(win, COLOR_PAIR(2));
    } else {
        wattroff(win, COLOR_PAIR(5));
        wattron(win, A_BOLD);
    }
}


static void _cons_show_incoming_message(const char * const short_from, const int win_index)
{
    _win_show_time(_cons_win);
    wattron(_cons_win, COLOR_PAIR(8));
    wprintw(_cons_win, "<< incoming from %s (%d)\n", short_from, win_index + 1);
    wattroff(_cons_win, COLOR_PAIR(8));
}

