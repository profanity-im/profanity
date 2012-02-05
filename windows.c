#include <ncurses.h>

#include "windows.h"

// window references
extern WINDOW *title_bar;
extern WINDOW *cmd_bar;
extern WINDOW *cmd_win;
extern WINDOW *main_win;

void create_title_bar(void)
{
    char *title = "Profanity";

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    title_bar = newwin(1, cols, 0, 0);
    wbkgd(title_bar, COLOR_PAIR(3));
    mvwprintw(title_bar, 0, 0, title);
}

void create_command_bar(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    cmd_bar = newwin(1, cols, rows-2, 0);
    wbkgd(cmd_bar, COLOR_PAIR(3));
}

void create_command_window(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    cmd_win = newwin(1, cols, rows-1, 0);
}

void create_main_window(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    main_win = newwin(rows-3, cols, 1, 0);
    scrollok(main_win, TRUE);
}
