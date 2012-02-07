#include <ncurses.h>

#include "windows.h"

// window references
static WINDOW *title_bar;
static WINDOW *cmd_bar;
static WINDOW *cmd_win;
static WINDOW *main_win;

static void create_title_bar(void);
static void create_command_bar(void);
static void create_command_window(void);
static void create_main_window(void);

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
    create_command_bar();
    wrefresh(cmd_bar);
    create_command_window();
    wrefresh(cmd_win);
    create_main_window();
    wrefresh(main_win);

    wmove(cmd_win, 0, 0);
}

void gui_close(void)
{
    endwin();
}

void show_incomming_msg(char *from, char *message) 
{
    char line[100];
    sprintf(line, "%s: %s\n", from, message);

    wprintw(main_win, line);
    wrefresh(main_win);
}

void show_outgoing_msg(char *from, char* message)
{
    char line[100];
    sprintf(line, "%s: %s\n", from, message);

    wprintw(main_win, line);
    wrefresh(main_win);
}

void cmd_get_command_str(char *cmd)
{
    wmove(cmd_win, 0, 0);
    wgetstr(cmd_win, cmd);
}

void cmd_clear(void)
{
    wclear(cmd_win);
    wmove(cmd_win, 0, 0);
    wrefresh(cmd_win);
}

void cmd_non_block(void)
{
    wtimeout(cmd_win, 0);
}

void cmd_poll_char(int *ch, char command[], int *size)
{
    int cmd_y = 0;
    int cmd_x = 0;

    // move cursor back to cmd_win
    getyx(cmd_win, cmd_y, cmd_x);
    wmove(cmd_win, cmd_y, cmd_x);

    // echo off, and get some more input
    noecho();
    *ch = wgetch(cmd_win);

    // if delete pressed, go back and delete it
    if (*ch == 127) {
        if (*size > 0) {
            getyx(cmd_win, cmd_y, cmd_x);
            wmove(cmd_win, cmd_y, cmd_x-1);
            wdelch(cmd_win);
            (*size)--;
        }
    }
    // else if not error or newline, show it and store it
    else if (*ch != ERR && *ch != '\n') {
        waddch(cmd_win, *ch);
        command[(*size)++] = *ch;
    }

    echo();


}

void cmd_get_password(char *passwd)
{
    wclear(cmd_win);
    noecho();
    mvwgetstr(cmd_win, 0, 0, passwd);
    echo();
}

void bar_print_message(char *msg)
{
    mvwprintw(cmd_bar, 0, 0, msg);
    wrefresh(cmd_bar);
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

static void create_command_bar(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    cmd_bar = newwin(1, cols, rows-2, 0);
    wbkgd(cmd_bar, COLOR_PAIR(3));
}

static void create_command_window(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    cmd_win = newwin(1, cols, rows-1, 0);
    keypad(cmd_win, TRUE);
}

static void create_main_window(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    main_win = newwin(rows-3, cols, 1, 0);
    scrollok(main_win, TRUE);
}

