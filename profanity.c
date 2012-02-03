#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <string.h>
#include <strophe/strophe.h>

static FILE *logp;

xmpp_log_t *xmpp_get_file_logger();

void xmpp_file_logger(void * const userdata, const xmpp_log_level_t level,
    const char * const area, const char * const msg);

static const xmpp_log_t file_log = { &xmpp_file_logger, XMPP_LEVEL_DEBUG };

int message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, 
    void * const userdata);

void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, 
          const int error, xmpp_stream_error_t * const stream_error,
          void * const userdata);

void init(void);

void print_title(void);

void close(void);

int main(void)
{   
    int ypos = 2;
    int xpos = 2;
    int ch;
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
    refresh();

    xmpp_run(ctx);

    xmpp_conn_release(conn);
    xmpp_ctx_free(ctx);

    xmpp_shutdown();

    close();

    fclose(logp);

    return 0;
}

int message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, 
    void * const userdata)
{
    char *append_reply = ", recieved";

    xmpp_stanza_t *reply, *body, *text;
    char *intext, *replytext;
    xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata;

    if(!xmpp_stanza_get_child_by_name(stanza, "body")) 
        return 1;
    if(!strcmp(xmpp_stanza_get_attribute(stanza, "type"), "error")) 
        return 1;

    intext = xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(stanza, "body"));

    printw("Incoming message from %s: %s\n", xmpp_stanza_get_attribute(stanza, "from"), 
        intext);

    refresh();

    reply = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(reply, "message");
    xmpp_stanza_set_type(reply, 
        xmpp_stanza_get_type(stanza)?xmpp_stanza_get_type(stanza):"chat");
    xmpp_stanza_set_attribute(reply, "to", xmpp_stanza_get_attribute(stanza, "from"));

    body = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(body, "body");

    replytext = malloc(strlen(append_reply) + strlen(intext) + 1);
    strcpy(replytext, intext);
    strcat(replytext, append_reply);

    text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(text, replytext);
    xmpp_stanza_add_child(body, text);
    xmpp_stanza_add_child(reply, body);

    xmpp_send(conn, reply);
    xmpp_stanza_release(reply);
    free(replytext);

    return 1;
}


xmpp_log_t *xmpp_get_file_logger()
{
    return (xmpp_log_t*) &file_log;
}

void xmpp_file_logger(void * const userdata, const xmpp_log_level_t level,
    const char * const area, const char * const msg)
{
    fprintf(logp, "%s %s %s\n", area, XMPP_LEVEL_DEBUG, msg);
}

void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, 
          const int error, xmpp_stream_error_t * const stream_error,
          void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

    if (status == XMPP_CONN_CONNECT) {
        xmpp_stanza_t* pres;
        fprintf(logp, "DEBUG: connected\n");
        xmpp_handler_add(conn,message_handler, NULL, "message", NULL, ctx);

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
