#include <ncurses.h>

#include "windows.h"

// window references
extern WINDOW *title_bar;
extern WINDOW *cmd_bar;
extern WINDOW *cmd_win;
extern WINDOW *main_win;

static void create_title_bar(void);
static void create_command_bar(void);
static void create_command_window(void);
static void create_main_window(void);

void initgui(void)
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
