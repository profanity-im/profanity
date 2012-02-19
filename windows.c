#include <string.h>
#include <ncurses.h>
#include "windows.h"
#include "util.h"

static struct prof_win _wins[10];
static int _curr_win = 0;

static void _create_windows(void);
static int _find_win(char *contact);
static void _current_window_refresh();

void gui_init(void)
{
    initscr();
    cbreak();
    keypad(stdscr, TRUE);

    if (has_colors()) {    
        start_color();
        
        init_pair(1, COLOR_WHITE, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_WHITE, COLOR_BLUE);
        init_pair(4, COLOR_CYAN, COLOR_BLUE);
        init_pair(5, COLOR_CYAN, COLOR_BLACK);
    }

    refresh();

    create_title_bar();
    create_status_bar();
    create_input_window();
    _create_windows();
}

void gui_refresh(void)
{
    title_bar_refresh();
    status_bar_refresh();
    _current_window_refresh();
    inp_put_back();
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
    _curr_win = i;

    if (i == 0) {
        title_bar_title();
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
    title_bar_title();
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
    
    char *short_from = strtok(from_cpy, "/");
    char tstmp[80];
    get_time(tstmp);

    int win = _find_win(short_from);

    // print time   
    wattron(_wins[win].win, COLOR_PAIR(5));
    wprintw(_wins[win].win, " [");
    wattroff(_wins[win].win, COLOR_PAIR(5));

    wprintw(_wins[win].win, "%s", tstmp);

    wattron(_wins[win].win, COLOR_PAIR(5));
    wprintw(_wins[win].win, "] ");
    wattroff(_wins[win].win, COLOR_PAIR(5));
    
    // print user
    wattron(_wins[win].win, A_DIM);
    wprintw(_wins[win].win, "<");
    wattroff(_wins[win].win, A_DIM);

    wattron(_wins[win].win, COLOR_PAIR(2));
    wattron(_wins[win].win, A_BOLD);
    wprintw(_wins[win].win, "%s", short_from);
    wattroff(_wins[win].win, COLOR_PAIR(2));
    wattroff(_wins[win].win, A_BOLD);

    wattron(_wins[win].win, A_DIM);
    wprintw(_wins[win].win, "> ");
    wattroff(_wins[win].win, A_DIM);

    // print message
    wprintw(_wins[win].win, "%s\n", message);

    // update window
    status_bar_active(win);
}

void win_show_outgoing_msg(char *from, char *to, char *message)
{
    char line[100];
    char tstmp[80];
    get_time(tstmp);

    sprintf(line, " [%s] <%s> %s\n", tstmp, from, message);

    int win = _find_win(to);

    // print time   
    wattron(_wins[win].win, COLOR_PAIR(5));
    wprintw(_wins[win].win, " [");
    wattroff(_wins[win].win, COLOR_PAIR(5));

    wprintw(_wins[win].win, "%s", tstmp);

    wattron(_wins[win].win, COLOR_PAIR(5));
    wprintw(_wins[win].win, "] ");
    wattroff(_wins[win].win, COLOR_PAIR(5));

    // print user
    wattron(_wins[win].win, A_DIM);
    wprintw(_wins[win].win, "<");
    wattroff(_wins[win].win, A_DIM);

    wattron(_wins[win].win, A_BOLD);
    wprintw(_wins[win].win, "%s", from);
    wattroff(_wins[win].win, A_BOLD);

    wattron(_wins[win].win, A_DIM);
    wprintw(_wins[win].win, "> ");
    wattroff(_wins[win].win, A_DIM);

    // print message
    wprintw(_wins[win].win, "%s\n", message);

    // update window
    status_bar_active(win);
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
        " [%s]     /msg user@host mesg  : Send mesg to user.\n", tstmp);
    wprintw(_wins[0].win, 
        " [%s]     /quit                : Quit Profanity.\n", tstmp);
    wprintw(_wins[0].win, 
        " [%s]   Shortcuts:\n", tstmp);
    wprintw(_wins[0].win, 
        " [%s]     F1                   : This console window.\n", tstmp);
    wprintw(_wins[0].win, 
        " [%s]     F2-10                : Chat windows.\n", tstmp);
}

void cons_show(char *msg)
{
    char tstmp[80];
    get_time(tstmp);
   
    wprintw(_wins[0].win, " [%s] %s\n", tstmp, msg); 
}

void cons_bad_command(char *cmd)
{
    char tstmp[80];
    get_time(tstmp);

    wprintw(_wins[0].win, " [%s] Unknown command: %s\n", tstmp, cmd);
}

void cons_bad_connect(void)
{
    char tstmp[80];
    get_time(tstmp);

    wprintw(_wins[0].win, 
        " [%s] Usage: /connect user@host\n", tstmp);
}

void cons_bad_message(void)
{
    char tstmp[80];
    get_time(tstmp);

    wprintw(_wins[0].win, 
        " [%s] Usage: /msg user@host message\n", tstmp);
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

static int _find_win(char *contact)
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

        // set it up
        strcpy(_wins[i].from, contact);
        wclear(_wins[i].win);
    }

    return i;
}

static void _current_window_refresh()
{
    touchwin(_wins[_curr_win].win);
    wrefresh(_wins[_curr_win].win);
}
