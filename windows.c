#include <ncurses.h>

#include "windows.h"

// common window references
static WINDOW *title_bar;
static WINDOW *inp_bar;
static WINDOW *inp_win;

// main windows
static WINDOW *chat_win;
static WINDOW *cons_win;

static void create_title_bar(void);
static void create_input_bar(void);
static void create_input_window(void);

static void create_chat_window(void);
static void create_console_window(void);

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

    create_chat_window();
    create_console_window();

    wmove(inp_win, 0, 0);
}

void gui_close(void)
{
    endwin();
}

void show_incomming_msg(char *from, char *message) 
{
    char line[100];
    sprintf(line, "%s: %s\n", from, message);

    wprintw(chat_win, line);
}

void show_outgoing_msg(char *from, char* message)
{
    char line[100];
    sprintf(line, "%s: %s\n", from, message);

    wprintw(chat_win, line);
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
    else if (*ch != ERR && *ch != '\n' && *ch != KEY_F(1) && *ch != KEY_F(2)) {
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
    waddstr(cons_win, "Help\n");
    waddstr(cons_win, "----\n");
    waddstr(cons_win, "/quit - Quits Profanity.\n");
    waddstr(cons_win, "/connect <username@host> - Login to jabber.\n");
}

void cons_bad_command(char *cmd)
{
    wprintw(cons_win, "Unknown command: %s\n", cmd);
}

void cons_show(void)
{
    touchwin(cons_win);
    wrefresh(cons_win);
}

void chat_show(void)
{
    touchwin(chat_win);
    wrefresh(chat_win);
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

static void create_chat_window(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    chat_win = newwin(rows-3, cols, 1, 0);
    scrollok(chat_win, TRUE);
}

static void create_console_window(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    cons_win = newwin(rows-3, cols, 1, 0);
    scrollok(cons_win, TRUE);
    
    waddstr(cons_win, "Welcome to Profanity.\n");
    touchwin(cons_win);
    wrefresh(cons_win);

}
