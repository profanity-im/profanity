/* roster.c
** libstrophe XMPP client library -- handler example
**
** Copyright (C) 2005-2009 Collecta, Inc. 
**
**  This software is provided AS-IS with no warranty, either express
**  or implied.
**
**  This software is distributed under license and may not be copied,
**  modified or distributed except as expressly authorized under the
**  terms of the license contained in the file LICENSE.txt in this
**  distribution.
*/

/* This example demonstrates basic handler functions by printing out
** the user's roster.
*/

#include <stdio.h>
#include <string.h>

#include <strophe/strophe.h>

int handle_reply(xmpp_conn_t * const conn,
		 xmpp_stanza_t * const stanza,
		 void * const userdata)
{
    xmpp_stanza_t *query, *item;
    char *type, *name;

    type = xmpp_stanza_get_type(stanza);
    if (strcmp(type, "error") == 0)
	fprintf(stderr, "ERROR: query failed\n");
    else {
	query = xmpp_stanza_get_child_by_name(stanza, "query");
	printf("Roster:\n");
	for (item = xmpp_stanza_get_children(query); item; 
	     item = xmpp_stanza_get_next(item))
	    if ((name = xmpp_stanza_get_attribute(item, "name")))
		printf("\t %s (%s) sub=%s\n", 
		       name,
		       xmpp_stanza_get_attribute(item, "jid"),
		       xmpp_stanza_get_attribute(item, "subscription"));
	    else
		printf("\t %s sub=%s\n",
		       xmpp_stanza_get_attribute(item, "jid"),
		       xmpp_stanza_get_attribute(item, "subscription"));
	printf("END OF LIST\n");
    }

    /* disconnect */
    xmpp_disconnect(conn);

    return 0;
}

void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, 
		  const int error, xmpp_stream_error_t * const stream_error,
		  void * const userdata)
{
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    xmpp_stanza_t *iq, *query;

    if (status == XMPP_CONN_CONNECT) {
	fprintf(stderr, "DEBUG: connected\n");
	
	/* create iq stanza for request */
	iq = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(iq, "iq");
	xmpp_stanza_set_type(iq, "get");
	xmpp_stanza_set_id(iq, "roster1");

	query = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(query, "query");
	xmpp_stanza_set_ns(query, XMPP_NS_ROSTER);

	xmpp_stanza_add_child(iq, query);

	/* we can release the stanza since it belongs to iq now */
	xmpp_stanza_release(query);

	/* set up reply handler */
	xmpp_id_handler_add(conn, handle_reply, "roster1", ctx);

	/* send out the stanza */
	xmpp_send(conn, iq);

	/* release the stanza */
	xmpp_stanza_release(iq);
    } else {
	fprintf(stderr, "DEBUG: disconnected\n");
	xmpp_stop(ctx);
    }
}

int main(int argc, char **argv)
{
    xmpp_ctx_t *ctx;
    xmpp_conn_t *conn;

    if (argc != 3) {
	fprintf(stderr, "Usage: roster <jid> <pass>\n\n");
	return 1;
    }

    /* initialize lib */
    xmpp_initialize();

    /* create a context */
    ctx = xmpp_ctx_new(NULL, NULL);

    /* create a connection */
    conn = xmpp_conn_new(ctx);

    /* setup authentication information */
    xmpp_conn_set_jid(conn, argv[1]);
    xmpp_conn_set_pass(conn, argv[2]);

    /* initiate connection */
    xmpp_connect_client(conn, NULL, 0, conn_handler, ctx);

    /* start the event loop */
    xmpp_run(ctx);

    /* release our connection and context */
    xmpp_conn_release(conn);
    xmpp_ctx_free(ctx);

    /* shutdown lib */
    xmpp_shutdown();

    return 0;
}
