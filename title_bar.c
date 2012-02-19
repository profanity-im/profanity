#include <ncurses.h>
#include "windows.h"

static WINDOW *title_bar;

void create_title_bar(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    title_bar = newwin(1, cols, 0, 0);
    wbkgd(title_bar, COLOR_PAIR(3));
    title_bar_title();
}

void title_bar_title()
{
    char *title = "Profanity. Type /help for help information.";
    title_bar_show(title);
}

void title_bar_connected(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    wattron(title_bar, COLOR_PAIR(4));
    mvwaddch(title_bar, 0, cols - 14, '[');
    wattroff(title_bar, COLOR_PAIR(4));

    mvwprintw(title_bar, 0, cols - 13, " ...online ");
    
    wattron(title_bar, COLOR_PAIR(4));
    mvwaddch(title_bar, 0, cols - 2, ']');
    wattroff(title_bar, COLOR_PAIR(4));
}

void title_bar_disconnected(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
   
    wattron(title_bar, COLOR_PAIR(4));
    mvwaddch(title_bar, 0, cols - 14, '[');
    wattroff(title_bar, COLOR_PAIR(4));
    
    mvwprintw(title_bar, 0, cols - 13, " ..offline ");
    
    wattron(title_bar, COLOR_PAIR(4));
    mvwaddch(title_bar, 0, cols - 2, ']');
    wattroff(title_bar, COLOR_PAIR(4));
}

void title_bar_refresh(void)
{
    touchwin(title_bar);
    wrefresh(title_bar);
    inp_put_back();
}

void title_bar_show(char *title)
{
    wmove(title_bar, 0, 0);
    int i;
    for (i = 0; i < 45; i++)
        waddch(title_bar, ' ');
    mvwprintw(title_bar, 0, 0, " %s", title);
}

