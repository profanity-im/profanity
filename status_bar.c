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
    _active[1 + ((win - 1) * 3)] = ' ';
    if (win == 9)
        _active[1 + ((win -1) * 3)] = ' ';
        
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    mvwprintw(status_bar, 0, cols - 29, _active);
}

void status_bar_active(int win)
{
    if (win < 9) {
        _active[1 + ((win -1) * 3)] = (char)( ((int)'0') + (win + 1));
    } else {
        _active[25] = '1';
        _active[26] = '0';
    }
    
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    mvwprintw(status_bar, 0, cols - 29, _active);
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
}

static void _status_bar_update_time(void)
{
    char bar_time[8];
    char tstmp[80];
    get_time(tstmp);
    sprintf(bar_time, "[%s]", tstmp);

    mvwprintw(status_bar, 0, 1, bar_time);
}

