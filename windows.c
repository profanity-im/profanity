#include <string.h>
#include <ncurses.h>
#include "windows.h"

static struct prof_win wins[10];
static int curr_win = 0;

static void create_windows(void);

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
    create_input_bar();
    create_input_window();

    create_windows();
}

void gui_close(void)
{
    endwin();
}

void switch_to(int i)
{
    touchwin(wins[i].win);
    wrefresh(wins[i].win);
    curr_win = i;
}

void close_win(void)
{
    // reset the chat win to unused
    strcpy(wins[curr_win].from, "");
    wclear(wins[curr_win].win);

    // set it as inactive in the status bar
    inp_bar_inactive(curr_win);
    
    // go back to console window
    touchwin(wins[0].win);
    wrefresh(wins[0].win);
}

int in_chat(void)
{
    return (curr_win != 0);
}

void get_recipient(char *recipient)
{
    strcpy(recipient, wins[curr_win].from);
}

void show_incomming_msg(char *from, char *message) 
{
    char line[100];
    sprintf(line, "%s: %s\n", from, message);

    // find the chat window for sender
    int i;
    for (i = 1; i < 10; i++)
        if (strcmp(wins[i].from, from) == 0)
            break;

    // if we didn't find a window
    if (i == 10) {
        // find the first unused one
        for (i = 1; i < 10; i++)
            if (strcmp(wins[i].from, "") == 0)
                break;

        // set it up and print the message to it
        strcpy(wins[i].from, from);
        wclear(wins[i].win);
        wprintw(wins[i].win, line);
        
        // signify active window in status bar
        inp_bar_active(i);
        
        // if its the current window, draw it
        if (curr_win == i) {
            touchwin(wins[i].win);
            wrefresh(wins[i].win);
        }
    }
    // otherwise 
    else {
        // add the line to the senders window
        wprintw(wins[i].win, line);
        
        // signify active window in status bar
        inp_bar_active(i);
        
        // if its the current window, draw it
        if (curr_win == i) {
            touchwin(wins[i].win);
            wrefresh(wins[i].win);
        }
    }
}

void show_outgoing_msg(char *from, char* message)
{
    char line[100];
    sprintf(line, "%s: %s\n", from, message);

    wprintw(wins[curr_win].win, line);
    touchwin(wins[curr_win].win);
    wrefresh(wins[curr_win].win);
}

void cons_help(void)
{
    waddstr(wins[9].win, "Help\n");
    waddstr(wins[0].win, "----\n");
    waddstr(wins[0].win, "/quit - Quits Profanity.\n");
    waddstr(wins[0].win, "/connect <username@host> - Login to jabber.\n");

    // if its the current window, draw it
    if (curr_win == 0) {
        touchwin(wins[0].win);
        wrefresh(wins[0].win);
    }
}

void cons_bad_command(char *cmd)
{
    wprintw(wins[0].win, "Unknown command: %s\n", cmd);
    
    // if its the current window, draw it
    if (curr_win == 0) {
        touchwin(wins[0].win);
        wrefresh(wins[0].win);
    }
}

static void create_windows(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // create the console window in 0
    struct prof_win cons;
    strcpy(cons.from, "_cons");
    cons.win = newwin(rows-3, cols, 1, 0);
    scrollok(cons.win, TRUE);
    
    waddstr(cons.win, "Welcome to Profanity.\n");
    touchwin(cons.win);
    wrefresh(cons.win);
    wins[0] = cons;
    
    // create the chat windows
    int i;
    for (i = 1; i < 10; i++) {
        struct prof_win chat;
        strcpy(chat.from, "");
        chat.win = newwin(rows-3, cols, 1, 0);
        scrollok(chat.win, TRUE);
        wins[i] = chat;
    }    
}
