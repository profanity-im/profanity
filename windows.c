#include <string.h>
#include <ncurses.h>
#include "windows.h"
#include "util.h"

static struct prof_win _wins[10];
static int _curr_win = 0;

static void _create_windows(void);
static void _send_message_to_win(char *contact, char *line);

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
    create_status_bar();
    create_input_window();

    _create_windows();
}

void gui_close(void)
{
    endwin();
}

int win_is_active(int i)
{
    if (strcmp(_wins[i].from, "") == 0)
        return FALSE;
    else
        return TRUE;
}

void win_switch_to(int i)
{    
    touchwin(_wins[i].win);
    wrefresh(_wins[i].win);
    _curr_win = i;

    if (i == 0) {
        title_bar_show("Console, type /help for help information");
    } else {
        title_bar_show(_wins[i].from);
    }
}

void win_close_win(void)
{
    // reset the chat win to unused
    strcpy(_wins[_curr_win].from, "");
    wclear(_wins[_curr_win].win);

    // set it as inactive in the status bar
    status_bar_inactive(_curr_win);
    
    // go back to console window
    _curr_win = 0;
    touchwin(_wins[0].win);
    wrefresh(_wins[0].win);

    title_bar_show("Console, type /help for help information");
}

int win_in_chat(void)
{
    return ((_curr_win != 0) && (strcmp(_wins[_curr_win].from, "") != 0));
}

void win_get_recipient(char *recipient)
{
    strcpy(recipient, _wins[_curr_win].from);
}

void win_show_incomming_msg(char *from, char *message) 
{
    char from_cpy[100];
    strcpy(from_cpy, from);
    
    char line[100];
    char *short_from = strtok(from_cpy, "/");
    char tstmp[80];
    get_time(tstmp);

    sprintf(line, " [%s] <%s> %s\n", tstmp, short_from, message);
    _send_message_to_win(short_from, line);
}

void win_show_outgoing_msg(char *from, char *to, char *message)
{
    char line[100];
    char tstmp[80];
    get_time(tstmp);

    sprintf(line, " [%s] <%s> %s\n", tstmp, from, message);
    _send_message_to_win(to, line);
}

void cons_help(void)
{
    char tstmp[80];
    get_time(tstmp);

    wprintw(_wins[0].win, 
        " [%s] Help:\n", tstmp);
    wprintw(_wins[0].win, 
        " [%s]   Commands:\n", tstmp);
    wprintw(_wins[0].win, 
        " [%s]     /help                : This help.\n", tstmp);
    wprintw(_wins[0].win, 
        " [%s]     /connect user@host   : Login to jabber.\n", tstmp);
    wprintw(_wins[0].win, 
        " [%s]     /who                 : Get roster.\n", tstmp);
    wprintw(_wins[0].win, 
        " [%s]     /close               : Close a chat window.\n", tstmp);
    wprintw(_wins[0].win, 
        " [%s]     /msg user@host       : Send a message to user.\n", tstmp);
    wprintw(_wins[0].win, 
        " [%s]     /quit                : Quit Profanity.\n", tstmp);
    wprintw(_wins[0].win, 
        " [%s]   Shortcuts:\n", tstmp);
    wprintw(_wins[0].win, 
        " [%s]     F1    : Console window.\n", tstmp);
    wprintw(_wins[0].win, 
        " [%s]     F2-10 : Chat windows.\n", tstmp);

    // if its the current window, draw it
    if (_curr_win == 0) {
        touchwin(_wins[0].win);
        wrefresh(_wins[0].win);
    }
}

void cons_show(char *msg)
{
    char tstmp[80];
    get_time(tstmp);
   
    wprintw(_wins[0].win, " [%s] %s\n", tstmp, msg); 
    
    // if its the current window, draw it
    if (_curr_win == 0) {
        touchwin(_wins[0].win);
        wrefresh(_wins[0].win);
    }
}

void cons_bad_command(char *cmd)
{
    char tstmp[80];
    get_time(tstmp);

    wprintw(_wins[0].win, " [%s] Unknown command: %s\n", tstmp, cmd);
    
    // if its the current window, draw it
    if (_curr_win == 0) {
        touchwin(_wins[0].win);
        wrefresh(_wins[0].win);
    }
}

static void _create_windows(void)
{
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    // create the console window in 0
    struct prof_win cons;
    strcpy(cons.from, "_cons");
    cons.win = newwin(rows-3, cols, 1, 0);
    scrollok(cons.win, TRUE);

    char tstmp[80];
    get_time(tstmp);
    
    wprintw(cons.win, " [%s] Welcome to Profanity.\n", tstmp);
    touchwin(cons.win);
    wrefresh(cons.win);
    _wins[0] = cons;
    
    // create the chat windows
    int i;
    for (i = 1; i < 10; i++) {
        struct prof_win chat;
        strcpy(chat.from, "");
        chat.win = newwin(rows-3, cols, 1, 0);
        scrollok(chat.win, TRUE);
        _wins[i] = chat;
    }    
}

static void _send_message_to_win(char *contact, char *line)
{
    // find the chat window for recipient
    int i;
    for (i = 1; i < 10; i++)
        if (strcmp(_wins[i].from, contact) == 0)
            break;

    // if we didn't find a window
    if (i == 10) {
        // find the first unused one
        for (i = 1; i < 10; i++)
            if (strcmp(_wins[i].from, "") == 0)
                break;

        // set it up and print the message to it
        strcpy(_wins[i].from, contact);
        wclear(_wins[i].win);
        wprintw(_wins[i].win, line);

        // signify active window in status bar
        status_bar_active(i);

        // if its the current window, draw it
        if (_curr_win == i) {
            touchwin(_wins[i].win);
            wrefresh(_wins[i].win);
        }
    }
    // otherwise 
    else {
        // add the line to the senders window
        wprintw(_wins[i].win, line);

        // signify active window in status bar
        status_bar_active(i);

        // if its the current window, draw it
        if (_curr_win == i) {
            touchwin(_wins[i].win);
            wrefresh(_wins[i].win);
        }
    }
}

