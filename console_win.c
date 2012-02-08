#include <ncurses.h>
#include "windows.h"

static WINDOW *cons_win;

void create_console_window(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    cons_win = newwin(rows-3, cols, 1, 0);
    scrollok(cons_win, TRUE);

    waddstr(cons_win, "Welcome to Profanity.\n");
    touchwin(cons_win);
    wrefresh(cons_win);
}

