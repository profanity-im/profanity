/* 
 * jabber.c
 *
 * Copyright (C) 2012 James Booth <boothj5@gmail.com>
 * 
 * This file is part of Profanity.
 *
 * Profanity is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Profanity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Profanity.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <string.h>
#include <strophe.h>

#include "jabber.h"
#include "common.h"
#include "log.h"
#include "contact_list.h"
#include "ui.h"
#include "util.h"
#include "preferences.h"

#define PING_INTERVAL 120000 // 2 minutes

// log reference
extern FILE *logp;

static struct _jabber_conn_t {
    xmpp_log_t *log;
    xmpp_ctx_t *ctx;
    xmpp_conn_t *conn;
    jabber_conn_status_t conn_status;
    jabber_presence_t presence;
    int tls_disabled;
} jabber_conn;

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

static int _roster_handler(xmpp_conn_t * const conn, 
    xmpp_stanza_t * const stanza, void * const userdata);

static int _jabber_presence_handler(xmpp_conn_t * const conn, 
    xmpp_stanza_t * const stanza, void * const userdata);

static int _ping_timed_handler(xmpp_conn_t * const conn, void * const userdata);

void jabber_init(const int disable_tls)
{
    jabber_conn.conn_status = JABBER_STARTED;
    jabber_conn.presence = PRESENCE_OFFLINE;
    jabber_conn.tls_disabled = disable_tls;
}

jabber_conn_status_t jabber_connection_status(void)
{
    return (jabber_conn.conn_status);
}

jabber_conn_status_t jabber_connect(const char * const user, 
    const char * const passwd)
{
    xmpp_initialize();

    jabber_conn.log = xmpp_get_file_logger();
    jabber_conn.ctx = xmpp_ctx_new(NULL, jabber_conn.log);
    jabber_conn.conn = xmpp_conn_new(jabber_conn.ctx);

    xmpp_conn_set_jid(jabber_conn.conn, user);
    xmpp_conn_set_pass(jabber_conn.conn, passwd);

    if (jabber_conn.tls_disabled)
        xmpp_conn_disable_tls(jabber_conn.conn);

    int connect_status = xmpp_connect_client(jabber_conn.conn, NULL, 0, 
        _jabber_conn_handler, jabber_conn.ctx);

    if (connect_status == 0)
        jabber_conn.conn_status = JABBER_CONNECTING;
    else  
        jabber_conn.conn_status = JABBER_DISCONNECTED;

    return jabber_conn.conn_status;
}

const char * jabber_get_jid(void)
{
    return xmpp_conn_get_jid(jabber_conn.conn);
}

void jabber_disconnect(void)
{
    if (jabber_conn.conn_status == JABBER_CONNECTED) {
        xmpp_conn_release(jabber_conn.conn);
        xmpp_ctx_free(jabber_conn.ctx);
        xmpp_shutdown();
        jabber_conn.conn_status = JABBER_DISCONNECTED;
        jabber_conn.presence = PRESENCE_OFFLINE;
    }
}

void jabber_process_events(void)
{
    if (jabber_conn.conn_status == JABBER_CONNECTED 
            || jabber_conn.conn_status == JABBER_CONNECTING)
        xmpp_run_once(jabber_conn.ctx, 10);
}

void jabber_send(const char * const msg, const char * const recipient)
{
    char *coded_msg = str_replace(msg, "&", "&amp;");

    xmpp_stanza_t *reply, *body, *text;

    reply = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(reply, "message");
    xmpp_stanza_set_type(reply, "chat");
    xmpp_stanza_set_attribute(reply, "to", recipient);

    body = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(body, "body");

    text = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_text(text, coded_msg);
    xmpp_stanza_add_child(body, text);
    xmpp_stanza_add_child(reply, body);

    xmpp_send(jabber_conn.conn, reply);
    xmpp_stanza_release(reply);
}

void jabber_roster_request(void)
{
    xmpp_stanza_t *iq, *query;

    iq = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(iq, "iq");
    xmpp_stanza_set_type(iq, "get");
    xmpp_stanza_set_id(iq, "roster");

    query = xmpp_stanza_new(jabber_conn.ctx);
    xmpp_stanza_set_name(query, "query");
    xmpp_stanza_set_ns(query, XMPP_NS_ROSTER);

    xmpp_stanza_add_child(iq, query);
    xmpp_stanza_release(query);
    xmpp_send(jabber_conn.conn, iq);
    xmpp_stanza_release(iq);
}

void jabber_update_presence(jabber_presence_t status)
{
    jabber_conn.presence = status;
}

static int _jabber_message_handler(xmpp_conn_t * const conn, 
    xmpp_stanza_t * const stanza, void * const userdata)
{
    xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, "body");
    if(body == NULL)
        return 1;

    char *type = xmpp_stanza_get_attribute(stanza, "type");
    if(strcmp(type, "error") == 0)
        return 1;

    char *message = xmpp_stanza_get_text(body);
    char *from = xmpp_stanza_get_attribute(stanza, "from");
    win_show_incomming_msg(from, message);
    win_page_off();

    return 1;
}

static void _jabber_conn_handler(xmpp_conn_t * const conn, 
    const xmpp_conn_event_t status, const int error, 
    xmpp_stream_error_t * const stream_error, void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;

    if (status == XMPP_CONN_CONNECT) {
        const char *jid = xmpp_conn_get_jid(conn);
        const char *msg = " logged in successfully.";
        char line[strlen(jid) + 1 + strlen(msg) + 1];
        sprintf(line, "%s %s", jid, msg);
        title_bar_set_status(PRESENCE_ONLINE);

        cons_show(line);
        win_page_off();
        status_bar_print_message(jid);
        status_bar_refresh();

        xmpp_stanza_t* pres;
        xmpp_handler_add(conn, _jabber_message_handler, NULL, "message", NULL, ctx);
        xmpp_handler_add(conn, _jabber_presence_handler, NULL, "presence", NULL, ctx);
        xmpp_id_handler_add(conn, _roster_handler, "roster", ctx);
        xmpp_timed_handler_add(conn, _ping_timed_handler, PING_INTERVAL, ctx);

        pres = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(pres, "presence");
        xmpp_send(conn, pres);
        xmpp_stanza_release(pres);

        prefs_add_login(jid);

        jabber_conn.conn_status = JABBER_CONNECTED;
        jabber_conn.presence = PRESENCE_ONLINE;
    }
    else {
        if (jabber_conn.conn_status == JABBER_CONNECTED) {
            cons_bad_show("Lost connection.");
            win_disconnected();
        } else {
            cons_bad_show("Login failed.");
        }
        win_page_off();
        log_msg(CONN, "disconnected");
        xmpp_stop(ctx);
        jabber_conn.conn_status = JABBER_DISCONNECTED;
        jabber_conn.presence = PRESENCE_OFFLINE;
    }
}

static int _roster_handler(xmpp_conn_t * const conn, 
    xmpp_stanza_t * const stanza, void * const userdata)
{
    xmpp_stanza_t *query, *item;
    char *type, *name, *jid;

    type = xmpp_stanza_get_type(stanza);
    
    if (strcmp(type, "error") == 0)
        log_msg(CONN, "ERROR: query failed");
    else {
        query = xmpp_stanza_get_child_by_name(stanza, "query");
        cons_show("Roster:");

        item = xmpp_stanza_get_children(query);
        while (item != NULL) {
            name = xmpp_stanza_get_attribute(item, "name");
            jid = xmpp_stanza_get_attribute(item, "jid");

            if (name != NULL) {
                char line[strlen(name) + 2 + strlen(jid) + 1 + 1];
                sprintf(line, "%s (%s)", name, jid);
                cons_show(line);

            } else {
                char line[strlen(jid) + 1];
                sprintf(line, "%s", jid);
                cons_show(line);
            }
        
            item = xmpp_stanza_get_next(item);

            win_page_off();
        }
    }
    
    return 1;
}

static int _ping_timed_handler(xmpp_conn_t * const conn, void * const userdata)
{
    if (jabber_conn.conn_status == JABBER_CONNECTED) {
        xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
        
        xmpp_stanza_t *iq, *ping;

        iq = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(iq, "iq");
        xmpp_stanza_set_type(iq, "get");
        xmpp_stanza_set_id(iq, "c2s1");

        ping = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(ping, "ping");

        // FIXME add ping namespace to libstrophe
        xmpp_stanza_set_ns(ping, "urn:xmpp:ping");

        xmpp_stanza_add_child(iq, ping);
        xmpp_stanza_release(ping);
        xmpp_send(conn, iq);
        xmpp_stanza_release(iq);
    }

    return 1;
}

static int _jabber_presence_handler(xmpp_conn_t * const conn, 
    xmpp_stanza_t * const stanza, void * const userdata)
{
    const char *jid = xmpp_conn_get_jid(jabber_conn.conn);
    char jid_cpy[strlen(jid) + 1];
    strcpy(jid_cpy, jid);
    char *short_jid = strtok(jid_cpy, "/");
    
    char *from = xmpp_stanza_get_attribute(stanza, "from");
    char *short_from = strtok(from, "/");
    char *type = xmpp_stanza_get_attribute(stanza, "type");

    char *show_str, *status_str;
   
    xmpp_stanza_t *show = xmpp_stanza_get_child_by_name(stanza, "show");
    if (show != NULL)
        show_str = xmpp_stanza_get_text(show);
    else
        show_str = NULL;

    xmpp_stanza_t *status = xmpp_stanza_get_child_by_name(stanza, "status");
    if (status != NULL)    
        status_str = xmpp_stanza_get_text(status);
    else 
        status_str = NULL;

    if (strcmp(short_jid, short_from) !=0) {
        if (type == NULL) {// online
            gboolean result = contact_list_add(short_from, show_str, status_str);
            if (result) {
                win_contact_online(short_from, show_str, status_str);
            }
        } else {// offline
            gboolean result = contact_list_remove(short_from);
            if (result) {
                win_contact_offline(short_from, show_str, status_str);
            }
        }

        win_page_off();
    }

    return 1;
}


