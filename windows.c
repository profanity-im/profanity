#include <string.h>
#include <ncurses.h>
#include "windows.h"

// common window references
static WINDOW *title_bar;
static WINDOW *inp_bar;
static WINDOW *inp_win;

static struct prof_win wins[10];
static int curr_win = 0;

// main windows
static void create_title_bar(void);
static void create_input_bar(void);
static void create_input_window(void);

static void create_windows(void);

void gui_init(void)
{
    initscr();
    cbreak();
    keypad(stdscr, TRUE);
    start_color();

    init_color(COLOR_WHITE, 1000, 1000, 1000);
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);

    init_color(COLOR_BLUE, 0, 0, 250);
    init_pair(3, COLOR_WHITE, COLOR_BLUE);

    attron(A_BOLD);
    attron(COLOR_PAIR(1));

    refresh();

    create_title_bar();
    wrefresh(title_bar);
    create_input_bar();
    wrefresh(inp_bar);
    create_input_window();
    wrefresh(inp_win);

    create_windows();

    wmove(inp_win, 0, 0);
}

void gui_close(void)
{
    endwin();
}

void switch_to(int i)
{
    touchwin(wins[i].win);
    wrefresh(wins[i].win);
    curr_win = i;
}

int in_chat(void)
{
    return (curr_win != 0);
}

void get_recipient(char *recipient)
{
    strcpy(recipient, wins[curr_win].from);
}

void show_incomming_msg(char *from, char *message) 
{
    char line[100];
    sprintf(line, "%s: %s\n", from, message);

    // find the chat window for sender
    int i;
    for (i = 1; i < 10; i++)
        if (strcmp(wins[i].from, from) == 0)
            break;

    // if we didn't find a window
    if (i == 10) {
        // find the first unused one
        for (i = 1; i < 10; i++)
            if (strcmp(wins[i].from, "") == 0)
                break;

        // set it up and print the message to it
        strcpy(wins[i].from, from);
        wclear(wins[i].win);
        wprintw(wins[i].win, line);
        
        // signify active window in status bar
        mvwprintw(inp_bar, 0, 30 + i, "%d", i+1);
        touchwin(inp_bar);
        wrefresh(inp_bar);
        
        // if its the current window, draw it
        if (curr_win == i) {
            touchwin(wins[i].win);
            wrefresh(wins[i].win);
        }
    }
    // otherwise 
    else {
        // add the line to the senders window
        wprintw(wins[i].win, line);
        
        // signify active window in status bar
        mvwprintw(inp_bar, 0, 30 + i, "%d", i+1);
        touchwin(inp_bar);
        wrefresh(inp_bar);
        
        // if its the current window, draw it
        if (curr_win == i) {
            touchwin(wins[i].win);
            wrefresh(wins[i].win);
        }
    }
}

void show_outgoing_msg(char *from, char* message)
{
    char line[100];
    sprintf(line, "%s: %s\n", from, message);

    wprintw(wins[curr_win].win, line);
    touchwin(wins[curr_win].win);
    wrefresh(wins[curr_win].win);
}

void inp_get_command_str(char *cmd)
{
    wmove(inp_win, 0, 0);
    wgetstr(inp_win, cmd);
}

void inp_clear(void)
{
    wclear(inp_win);
    wmove(inp_win, 0, 0);
    wrefresh(inp_win);
}

void inp_non_block(void)
{
    wtimeout(inp_win, 0);
}

void inp_poll_char(int *ch, char command[], int *size)
{
    int inp_y = 0;
    int inp_x = 0;

    // move cursor back to inp_win
    getyx(inp_win, inp_y, inp_x);
    wmove(inp_win, inp_y, inp_x);

    // echo off, and get some more input
    noecho();
    *ch = wgetch(inp_win);

    // if delete pressed, go back and delete it
    if (*ch == 127) {
        if (*size > 0) {
            getyx(inp_win, inp_y, inp_x);
            wmove(inp_win, inp_y, inp_x-1);
            wdelch(inp_win);
            (*size)--;
        }
    }

    // else if not error or newline, show it and store it
    else if (*ch != ERR && 
             *ch != '\n' && 
             *ch != KEY_F(1) && 
             *ch != KEY_F(2) && 
             *ch != KEY_F(3) && 
             *ch != KEY_F(4) && 
             *ch != KEY_F(5) && 
             *ch != KEY_F(6) && 
             *ch != KEY_F(7) && 
             *ch != KEY_F(8) && 
             *ch != KEY_F(9) && 
             *ch != KEY_F(10)) {
        waddch(inp_win, *ch);
        command[(*size)++] = *ch;
    }

    echo();
}

void inp_get_password(char *passwd)
{
    wclear(inp_win);
    noecho();
    mvwgetstr(inp_win, 0, 0, passwd);
    echo();
}

void bar_print_message(char *msg)
{
    mvwprintw(inp_bar, 0, 0, msg);
    wrefresh(inp_bar);
}

void cons_help(void)
{
    waddstr(wins[9].win, "Help\n");
    waddstr(wins[0].win, "----\n");
    waddstr(wins[0].win, "/quit - Quits Profanity.\n");
    waddstr(wins[0].win, "/connect <username@host> - Login to jabber.\n");
}

void cons_bad_command(char *cmd)
{
    wprintw(wins[0].win, "Unknown command: %s\n", cmd);
}

void cons_show(void)
{
    touchwin(wins[0].win);
    wrefresh(wins[0].win);
}

static void create_title_bar(void)
{
    char *title = "Profanity";

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    title_bar = newwin(1, cols, 0, 0);
    wbkgd(title_bar, COLOR_PAIR(3));
    mvwprintw(title_bar, 0, 0, title);
}

static void create_input_bar(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    inp_bar = newwin(1, cols, rows-2, 0);
    wbkgd(inp_bar, COLOR_PAIR(3));
}

static void create_input_window(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    inp_win = newwin(1, cols, rows-1, 0);
    keypad(inp_win, TRUE);
}

static void create_windows(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // create the console window in 0
    struct prof_win cons;
    strcpy(cons.from, "_cons");
    cons.win = newwin(rows-3, cols, 1, 0);
    scrollok(cons.win, TRUE);
    
    waddstr(cons.win, "Welcome to Profanity.\n");
    touchwin(cons.win);
    wrefresh(cons.win);
    wins[0] = cons;
    
    // create the chat windows
    int i;
    for (i = 1; i < 10; i++) {
        struct prof_win chat;
        strcpy(chat.from, "");
        chat.win = newwin(rows-3, cols, 1, 0);
        scrollok(chat.win, TRUE);
        wins[i] = chat;
    }    
}
