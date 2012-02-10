#include <ncurses.h>
#include "windows.h"

static WINDOW *title_bar;

void create_title_bar(void)
{
    char *title = "Profanity";

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    title_bar = newwin(1, cols, 0, 0);
    wbkgd(title_bar, COLOR_PAIR(3));
    title_bar_show(title);
}

void title_bar_show(char *title)
{
    wclear(title_bar);
    mvwprintw(title_bar, 0, 0, " %s", title);
    wrefresh(title_bar);
}

