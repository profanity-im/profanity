#include <ncurses.h>
#include <string.h>

void init()
{
    initscr();
    raw();
    keypad(stdscr, TRUE);
    start_color();

    init_color(COLOR_WHITE, 1000, 1000, 1000);
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);

    init_color(COLOR_BLUE, 0, 0, 250);
    init_pair(3, COLOR_WHITE, COLOR_BLUE);

    attron(A_BOLD);
    attron(COLOR_PAIR(1));
}

void print_title()
{
    int rows, cols;
    char *title = "PROFANITY";

    getmaxyx(stdscr, rows, cols);
    
    attron(COLOR_PAIR(3));
    mvprintw(1, (cols - strlen(title))/2, title);
    attroff(COLOR_PAIR(3));
}

void close()
{
    int rows, cols;
    char *exit_msg = "< HIT ANY KEY TO EXIT >";

    getmaxyx(stdscr, rows, cols);
    
    attron(A_BLINK);
    curs_set(0);
    mvprintw(rows-10, (cols - strlen(exit_msg))/2, exit_msg);
    
    refresh();
    getch();
    endwin();
}

int main()
{   
    int ypos = 2;
    int xpos = 2;
    int ch;
    char *name;

    init();

    print_title();
    ypos += 2;
    mvprintw(ypos, xpos, "Enter your name: ");
    echo();
    getstr(name);
    noecho();
    
    ypos += 2;
    mvprintw(ypos, xpos, "Shit, ");
    attron(COLOR_PAIR(2));
    printw("%s", name);
    attroff(COLOR_PAIR(2));

    printw("\n");
    close();

    return 0;
}
