#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <string.h>
#include <strophe/strophe.h>

#include "log.h"

// refernce to log
extern FILE *logp;

// area for log message in profanity
static const char *prof = "prof";

// static windows
static WINDOW *title_bar;
static WINDOW *cmd_bar;
static WINDOW *cmd_win;
static WINDOW *main_win;

// message handlers
int in_message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, 
    void * const userdata);

// connection handler
void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, 
          const int error, xmpp_stream_error_t * const stream_error,
          void * const userdata);

// curses functions
void init(void);
void create_title_bar(void);
void create_command_bar(void);
void create_command_window(void);
void create_main_window(void);

void event_loop(xmpp_ctx_t *ctx, xmpp_conn_t *conn);

struct receiver {
    xmpp_ctx_t *ctx;
    xmpp_conn_t *conn;
};

int main(void)
{   
    char cmd[50];

    xmpp_ctx_t *ctx;
    xmpp_conn_t *conn;
    xmpp_log_t *log;

    logp = fopen("profanity.log", "a");
    logmsg(prof, "Starting Profanity...");

    // initialise curses
    init();
    refresh();
    
    // create windows
    create_title_bar();
    wrefresh(title_bar);
    create_command_bar();
    wrefresh(cmd_bar);
    create_command_window();
    wrefresh(cmd_win);
    create_main_window();
    wrefresh(main_win);

    // set cursor in command window and loop in input
    wmove(cmd_win, 0, 0);
    while (TRUE) {
        wgetstr(cmd_win, cmd);
        
        // handle quit
        if (strcmp(cmd, "/quit") == 0) {
            break;
        // handle connect
        } else if (strncmp(cmd, "/connect ", 9) == 0) {
            char *user;
            char passwd[20];

            user = strndup(cmd+9, strlen(cmd)-9);
            
            mvwprintw(cmd_bar, 0, 0, "Enter password:");
            wrefresh(cmd_bar);
            
            wclear(cmd_win);
            noecho();
            mvwgetstr(cmd_win, 0, 0, passwd);
            echo();

            mvwprintw(cmd_bar, 0, 0, "%s", user);
            wrefresh(cmd_bar);
            
            wclear(cmd_win);
            wmove(cmd_win, 0, 0);
            wrefresh(cmd_win);

            char loginmsg[100];
            sprintf(loginmsg, "User <%s> logged in", user);
            logmsg(prof, loginmsg);
           
            xmpp_initialize();
            
            log = xmpp_get_file_logger(); 
            ctx = xmpp_ctx_new(NULL, log);
            conn = xmpp_conn_new(ctx);

            xmpp_conn_set_jid(conn, user);
            xmpp_conn_set_pass(conn, passwd);
            xmpp_connect_client(conn, NULL, 0, conn_handler, ctx);

            event_loop(ctx, conn);

            break;
        } else {
            // echo ignore
            wclear(cmd_win);
            wmove(cmd_win, 0, 0);
        }
    }
    
    endwin();
    fclose(logp);

    return 0;
}

void event_loop(xmpp_ctx_t *ctx, xmpp_conn_t *conn)
{
    int cmd_y = 0;
    int cmd_x = 0;

    wtimeout(cmd_win, 0);

    while(TRUE) {
        char ch = 0;
        char command[100];
        int size = 0;
        
        // while not enter
        while(ch != '\n') {
            usleep(1);
            // handle incoming messages
            xmpp_run_once(ctx, 10);

            // move cursor back to cmd_win
            getyx(cmd_win, cmd_y, cmd_x);
            wmove(cmd_win, cmd_y, cmd_x);
            
            // get some more input
            ch = wgetch(cmd_win);
            if (ch > 0 && ch != '\n') {
                command[size++] = ch;
            }
        }

        command[size++] = '\0';

        // newline was hit, check if /quit command issued
        if (strcmp(command, "/quit") == 0) {
            break;
        } else {
            // send the message
            xmpp_stanza_t *reply, *body, *text;

            reply = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(reply, "message");
            xmpp_stanza_set_type(reply, "chat");
            xmpp_stanza_set_attribute(reply, "to", "boothj5@localhost");

            body = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(body, "body");

            text = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(text, command);
            xmpp_stanza_add_child(body, text);
            xmpp_stanza_add_child(reply, body);

            xmpp_send(conn, reply);
            xmpp_stanza_release(reply);

            // show it in the main window
            wprintw(main_win, "me: %s\n", command);
            wrefresh(main_win);

            // move back to the command window and clear it
            wclear(cmd_win);
            wmove(cmd_win, 0, 0);
            wrefresh(cmd_win);
        }
    }

    xmpp_conn_release(conn);
    xmpp_ctx_free(ctx);

    xmpp_shutdown();

}

void wait_command(void)
{
    wclear(cmd_win);
    wmove(cmd_win, 0, 0);
}

int in_message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, 
    void * const userdata)
{
    char *intext;

    if(!xmpp_stanza_get_child_by_name(stanza, "body")) 
        return 1;
    if(!strcmp(xmpp_stanza_get_attribute(stanza, "type"), "error")) 
        return 1;

    intext = xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(stanza, "body"));

    char in_line[200];
    char *from = xmpp_stanza_get_attribute(stanza, "from");

    char *short_from = strtok(from, "@");

    sprintf(in_line, "%s: %s\n", short_from, intext);

    wprintw(main_win, in_line);
    wrefresh(main_win);

    return 1;
}

void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, 
          const int error, xmpp_stream_error_t * const stream_error,
          void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

    if (status == XMPP_CONN_CONNECT) {
        xmpp_stanza_t* pres;
        fprintf(logp, "DEBUG: connected\n");
        xmpp_handler_add(conn,in_message_handler, NULL, "message", NULL, ctx);

        pres = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(pres, "presence");
        xmpp_send(conn, pres);
        xmpp_stanza_release(pres);
    }
    else {
        fprintf(logp, "DEBUG: disconnected\n");
        xmpp_stop(ctx);
    }
}

void init(void)
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
}

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
