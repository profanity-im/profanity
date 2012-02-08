#include <ncurses.h>
#include "windows.h"

static WINDOW *inp_win;

void create_input_window(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    inp_win = newwin(1, cols, rows-1, 0);
    keypad(inp_win, TRUE);
    wrefresh(inp_win);
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

