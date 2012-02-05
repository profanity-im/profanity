#include <string.h>
#include <strophe/strophe.h>
#include <ncurses.h>

#include "windows.h"
#include "log.h"

extern FILE *logp;

extern WINDOW *main_win;

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

void prof_send(char *msg, xmpp_ctx_t *ctx, xmpp_conn_t *conn)
{
    xmpp_stanza_t *reply, *body, *text;

    reply = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(reply, "message");
    xmpp_stanza_set_type(reply, "chat");
    xmpp_stanza_set_attribute(reply, "to", "boothj5@localhost");

    body = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(body, "body");

    text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(text, msg);
    xmpp_stanza_add_child(body, text);
    xmpp_stanza_add_child(reply, body);

    xmpp_send(conn, reply);
    xmpp_stanza_release(reply);
}



