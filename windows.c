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

void gui_init(void)
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

    wmove(cmd_win, 0, 0);
}

void gui_close(void)
{
    endwin();
}

void show_incomming_msg(char *from, char *message) 
{
    char line[100];
    sprintf(line, "%s: %s\n", from, message);

    wprintw(main_win, line);
    wrefresh(main_win);
}

void cmd_get_command_str(char *cmd)
{
    wmove(cmd_win, 0, 0);
    wgetstr(cmd_win, cmd);
}

void cmd_get_password(char *passwd)
{
    wclear(cmd_win);
    noecho();
    mvwgetstr(cmd_win, 0, 0, passwd);
    echo();
}

void bar_print_message(char *msg)
{
    mvwprintw(cmd_bar, 0, 0, msg);
    wrefresh(cmd_bar);
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

