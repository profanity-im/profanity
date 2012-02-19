#include <string.h>

#include "jabber.h"
#include "log.h"
#include "windows.h"

// log reference
extern FILE *logp;

// private XMPP structs
static xmpp_log_t *_log;
static xmpp_ctx_t *_ctx;
static xmpp_conn_t *_conn;

// connection status
static int _conn_status = CONNECTING;

void xmpp_file_logger(void * const userdata, const xmpp_log_level_t level,
    const char * const area, const char * const msg);

static const xmpp_log_t file_log = { &xmpp_file_logger, XMPP_LEVEL_DEBUG };

xmpp_log_t *xmpp_get_file_logger()
{
    return (xmpp_log_t*) &file_log;
}

void xmpp_file_logger(void * const userdata, const xmpp_log_level_t level,
    const char * const area, const char * const msg)
{
    log_msg(area, msg);
}

// private XMPP handlers
static void _jabber_conn_handler(xmpp_conn_t * const conn, 
    const xmpp_conn_event_t status, const int error, 
    xmpp_stream_error_t * const stream_error, void * const userdata);

static int _jabber_message_handler(xmpp_conn_t * const conn, 
    xmpp_stanza_t * const stanza, void * const userdata);

static int _roster_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata);

int jabber_connection_status(void)
{
    return (_conn_status);
}

int jabber_connect(char *user, char *passwd)
{
    xmpp_initialize();

    _log = xmpp_get_file_logger();
    _ctx = xmpp_ctx_new(NULL, _log);
    _conn = xmpp_conn_new(_ctx);

    xmpp_conn_set_jid(_conn, user);
    xmpp_conn_set_pass(_conn, passwd);

    int connect_status = xmpp_connect_client(_conn, NULL, 0, _jabber_conn_handler, _ctx);

    if (connect_status == 0) {
        cons_good_show("Connecting...");
        _conn_status = CONNECTING;
    }
    else { 
        cons_bad_show("XMPP connection failure");
        _conn_status = DISCONNECTED;
    }

    return _conn_status;
}

void jabber_disconnect(void)
{
    xmpp_conn_release(_conn);
    xmpp_ctx_free(_ctx);
    xmpp_shutdown();
    _conn_status = DISCONNECTED;
}

void jabber_process_events(void)
{
    xmpp_run_once(_ctx, 10);
}

void jabber_send(char *msg, char *recipient)
{
    xmpp_stanza_t *reply, *body, *text;

    reply = xmpp_stanza_new(_ctx);
    xmpp_stanza_set_name(reply, "message");
    xmpp_stanza_set_type(reply, "chat");
    xmpp_stanza_set_attribute(reply, "to", recipient);

    body = xmpp_stanza_new(_ctx);
    xmpp_stanza_set_name(body, "body");

    text = xmpp_stanza_new(_ctx);
    xmpp_stanza_set_text(text, msg);
    xmpp_stanza_add_child(body, text);
    xmpp_stanza_add_child(reply, body);

    xmpp_send(_conn, reply);
    xmpp_stanza_release(reply);
}

void jabber_roster_request(void)
{
    xmpp_stanza_t *iq, *query;

    iq = xmpp_stanza_new(_ctx);
    xmpp_stanza_set_name(iq, "iq");
    xmpp_stanza_set_type(iq, "get");
    xmpp_stanza_set_id(iq, "roster");

    query = xmpp_stanza_new(_ctx);
    xmpp_stanza_set_name(query, "query");
    xmpp_stanza_set_ns(query, XMPP_NS_ROSTER);

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);
    xmpp_send(_conn, iq);
    xmpp_stanza_release(iq);
}

static int _jabber_message_handler(xmpp_conn_t * const conn, 
    xmpp_stanza_t * const stanza, void * const userdata)
{
    char *message;

    if(!xmpp_stanza_get_child_by_name(stanza, "body"))
        return 1;
    if(!strcmp(xmpp_stanza_get_attribute(stanza, "type"), "error"))
        return 1;

    message = xmpp_stanza_get_text(xmpp_stanza_get_child_by_name(stanza, "body"));

    char *from = xmpp_stanza_get_attribute(stanza, "from");
    win_show_incomming_msg(from, message);

    return 1;
}

static void _jabber_conn_handler(xmpp_conn_t * const conn, 
    const xmpp_conn_event_t status, const int error, 
    xmpp_stream_error_t * const stream_error, void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

    if (status == XMPP_CONN_CONNECT) {
        char line[100];
        sprintf(line, "%s logged in successfully.", xmpp_conn_get_jid(conn));
        title_bar_connected();

        cons_good_show(line);
        status_bar_print_message(xmpp_conn_get_jid(conn));
        status_bar_refresh();

        xmpp_stanza_t* pres;
        xmpp_handler_add(conn, _jabber_message_handler, NULL, "message", NULL, ctx);
        xmpp_id_handler_add(conn, _roster_handler, "roster", ctx);

        pres = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(pres, "presence");
        xmpp_send(conn, pres);
        xmpp_stanza_release(pres);
        jabber_roster_request();
        _conn_status = CONNECTED;
    }
    else {
        cons_bad_show("Login failed.");
        log_msg(CONN, "disconnected");
        xmpp_stop(ctx);
        _conn_status = DISCONNECTED;
    }
}

static int _roster_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata)
{
    xmpp_stanza_t *query, *item;
    char *type, *name;

    type = xmpp_stanza_get_type(stanza);
    
    if (strcmp(type, "error") == 0)
        log_msg(CONN, "ERROR: query failed");
    else {
        query = xmpp_stanza_get_child_by_name(stanza, "query");
        cons_highlight_show("Roster:");
        for (item = xmpp_stanza_get_children(query); item; 
                item = xmpp_stanza_get_next(item)) {
            if ((name = xmpp_stanza_get_attribute(item, "name"))) {
                char line[200];
                sprintf(line, "  %s (%s)", name,
                    xmpp_stanza_get_attribute(item, "jid"));
                cons_show(line);
            } else {
                char line[200];
                sprintf(line, "  %s",
                    xmpp_stanza_get_attribute(item, "jid"));
                cons_show(line);
            }
        }
    }
    
    return 1;
}
