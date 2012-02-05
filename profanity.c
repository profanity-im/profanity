#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include <strophe/strophe.h>

#include "log.h"

// refernce to log
extern FILE *logp;

// chat windows
static WINDOW *incoming_border;
static WINDOW *outgoing_border;
static WINDOW *incoming;
static WINDOW *outgoing;
static int incoming_ypos = 0;
static int inc_scroll_max = 0;

// message handlers
int in_message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, 
    void * const userdata);

int out_message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, 
    void * const userdata);

// connection handler
void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, 
          const int error, xmpp_stream_error_t * const stream_error,
          void * const userdata);

// curses functions
void init(void);
void print_title(void);
void close(void);

WINDOW *create_win(int, int, int, int, int);
void create_conversation(void);

int main(void)
{   
    int ypos = 2;
    int xpos = 2;
    char user[50];
    char passwd[50];

    xmpp_ctx_t *ctx;
    xmpp_conn_t *conn;
    xmpp_log_t *log;

    logp = fopen("profanity.log", "a");
    fprintf(logp, "Starting Profanity...\n");

    init();
    print_title();
    ypos += 2;
    mvprintw(ypos, xpos, "Enter jabber ID (username@host) : ");
    echo();
    getstr(user);

    ypos += 2;
    mvprintw(ypos, xpos, "Enter passwd : ");
    noecho();
    getstr(passwd);
    echo();
    
    fprintf(logp, "Log in, user = %s\n", user);
    
    xmpp_initialize();
    
    log = xmpp_get_file_logger(); 
    ctx = xmpp_ctx_new(NULL, log);
    conn = xmpp_conn_new(ctx);

    xmpp_conn_set_jid(conn, user);
    xmpp_conn_set_pass(conn, passwd);

    xmpp_connect_client(conn, NULL, 0, conn_handler, ctx);
    
    clear();
    print_title();
    refresh();
    move(3, 0);

    create_conversation();

    xmpp_run(ctx);

    xmpp_conn_release(conn);
    xmpp_ctx_free(ctx);

    xmpp_shutdown();

    close();

    fclose(logp);

    return 0;
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

    char in_line[100];    
    sprintf(in_line, "%s: %s\n", xmpp_stanza_get_attribute(stanza, "from"), intext);

    if (incoming_ypos == inc_scroll_max-1) {
        scroll(incoming);
        mvwaddstr(incoming, incoming_ypos-1, 0, in_line);
    } else {
        mvwaddstr(incoming, incoming_ypos, 0, in_line);
        incoming_ypos++;
    }

    wrefresh(incoming);

    return 1;
}

int out_message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, 
    void * const userdata)
{
    char replytxt[100];;

    echo();
    wgetstr(outgoing, replytxt);
        
    wrefresh(outgoing);
    refresh();

    xmpp_stanza_t *reply, *body, *text;
    xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata;

    reply = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(reply, "message");
    xmpp_stanza_set_type(reply, 
        xmpp_stanza_get_type(stanza)?xmpp_stanza_get_type(stanza):"chat");
    xmpp_stanza_set_attribute(reply, "to", xmpp_stanza_get_attribute(stanza, "from"));

    body = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(body, "body");

    text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(text, replytxt);
    xmpp_stanza_add_child(body, text);
    xmpp_stanza_add_child(reply, body);

    xmpp_send(conn, reply);
    xmpp_stanza_release(reply);

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
        xmpp_handler_add(conn,out_message_handler, NULL, "message", NULL, ctx);

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

WINDOW *create_win(int height, int width, int starty, int startx, int border)
{   
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    if (border)    
        box(local_win, 0 , 0);     
    wrefresh(local_win);

    return local_win;
}

void create_conversation()
{
    int rows, cols;
    char *title = "PROFANITY";

    getmaxyx(stdscr, rows, cols);
    
    curs_set(0);
    attron(COLOR_PAIR(3));
    mvprintw(1, (cols - strlen(title))/2, title);
    attroff(COLOR_PAIR(3));

    int in_height, in_width, in_y, in_x;
    int out_height, out_width, out_y, out_x;

    out_height = 5;
    out_width = cols;
    out_y = rows - 5;
    out_x = 0;

    in_height = (rows - 3) - (out_height);
    in_width = cols;
    in_y = 3;
    in_x = 0;
    
    incoming_border = create_win(in_height, in_width, in_y, in_x, 1);
    outgoing_border = create_win(out_height, out_width, out_y, out_x, 1);
    inc_scroll_max = in_height-2;
    incoming = create_win(in_height-2, in_width-2, in_y+1, in_x+1, 0);
    scrollok(incoming, TRUE);
    outgoing = create_win(out_height-2, out_width-2, out_y+1, out_x+1, 0);
    scrollok(outgoing, TRUE);
}

void print_title(void)
{
    int rows, cols;
    char *title = "PROFANITY";

    getmaxyx(stdscr, rows, cols);
    
    attron(COLOR_PAIR(3));
    mvprintw(1, (cols - strlen(title))/2, title);
    attroff(COLOR_PAIR(3));
}

void close(void)
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
