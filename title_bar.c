#include <ncurses.h>
#include "windows.h"

static WINDOW *title_bar;

void create_title_bar(void)
{
    char *title = "Profanity. Type /help for help information.";

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    title_bar = newwin(1, cols, 0, 0);
    wbkgd(title_bar, COLOR_PAIR(3));
    title_bar_show(title);
}

void title_bar_connected(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
   
    mvwprintw(title_bar, 0, cols - 12, "   connected");
}

void title_bar_disconnected(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
   
    mvwprintw(title_bar, 0, cols - 12, "disconnected");
}

void title_bar_refresh(void)
{
    touchwin(title_bar);
    wrefresh(title_bar);
    inp_put_back();
}

void title_bar_show(char *title)
{
    wclear(title_bar);
    mvwprintw(title_bar, 0, 0, " %s", title);
}

