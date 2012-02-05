#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <string.h>
#include <strophe/strophe.h>

#include "log.h"
#include "windows.h"
#include "jabber.h"

// refernce to log
extern FILE *logp;

// area for log message in profanity
static const char *prof = "prof";

// window references
extern WINDOW *title_bar;
extern WINDOW *cmd_bar;
extern WINDOW *cmd_win;
extern WINDOW *main_win;

// curses functions
void event_loop(xmpp_ctx_t *ctx, xmpp_conn_t *conn);

int main(void)
{   
    char cmd[50];

    xmpp_ctx_t *ctx;
    xmpp_conn_t *conn;
    xmpp_log_t *log;

    logp = fopen("profanity.log", "a");
    logmsg(prof, "Starting Profanity...");

    // initialise curses, and create windows
    initgui();

    // set cursor in command window and loop on input
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

