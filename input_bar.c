#include <ncurses.h>
#include "windows.h"

static WINDOW *inp_bar;

void create_input_bar(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    inp_bar = newwin(1, cols, rows-2, 0);
    wbkgd(inp_bar, COLOR_PAIR(3));
    wrefresh(inp_bar);
}

void inp_bar_inactive(int win)
{
    mvwaddch(inp_bar, 0, 30 + win, ' ');
    if (win == 9)
        mvwaddch(inp_bar, 0, 30 + win + 1, ' ');
    wrefresh(inp_bar);
}

void inp_bar_active(int win)
{
    mvwprintw(inp_bar, 0, 30 + win, "%d", win+1);
    touchwin(inp_bar);
    wrefresh(inp_bar);
}

void inp_bar_print_message(char *msg)
{
    mvwprintw(inp_bar, 0, 0, msg);
    wrefresh(inp_bar);
}

