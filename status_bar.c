#include <ncurses.h>
#include "windows.h"
#include "util.h"

static WINDOW *status_bar;
static char _active[29] = "[ ][ ][ ][ ][ ][ ][ ][ ][  ]";

static void _status_bar_update_time(void);

void create_status_bar(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    status_bar = newwin(1, cols, rows-2, 0);
    wbkgd(status_bar, COLOR_PAIR(3));
    wattron(status_bar, COLOR_PAIR(4));
    mvwprintw(status_bar, 0, cols - 29, _active);
    wattroff(status_bar, COLOR_PAIR(4));
    wrefresh(status_bar);
}

void status_bar_refresh(void)
{
    _status_bar_update_time();
    touchwin(status_bar);
    wrefresh(status_bar);
    inp_put_back();
}

void status_bar_inactive(int win)
{
    int active_pos = 1 + ((win -1) * 3);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
 
    mvwaddch(status_bar, 0, cols - 29 + active_pos, ' ');
    if (win == 9)
        mvwaddch(status_bar, 0, cols - 29 + active_pos + 1, ' ');
}

void status_bar_active(int win)
{
    int active_pos = 1 + ((win -1) * 3);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
 
    if (win < 9)
        mvwprintw(status_bar, 0, cols - 29 + active_pos, "%d", win+1);
    else
        mvwprintw(status_bar, 0, cols - 29 + active_pos, "10");
}

void status_bar_get_password(void)
{
    mvwprintw(status_bar, 0, 9, "Enter password:");
}

void status_bar_print_message(const char *msg)
{
    mvwprintw(status_bar, 0, 9, msg);
}

void status_bar_clear(void)
{
    wclear(status_bar);

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    wattron(status_bar, COLOR_PAIR(4));
    mvwprintw(status_bar, 0, cols - 29, _active);
    wattroff(status_bar, COLOR_PAIR(4));
}

static void _status_bar_update_time(void)
{
    char bar_time[6];
    char tstmp[80];
    get_time(tstmp);
    sprintf(bar_time, "%s", tstmp);

    wattron(status_bar, COLOR_PAIR(4));
    mvwaddch(status_bar, 0, 1, '[');
    wattroff(status_bar, COLOR_PAIR(4));
    mvwprintw(status_bar, 0, 2, bar_time);
    wattron(status_bar, COLOR_PAIR(4));
    mvwaddch(status_bar, 0, 7, ']');
    wattroff(status_bar, COLOR_PAIR(4));
    
}

