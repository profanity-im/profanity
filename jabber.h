#ifndef JABBER_H
#define JABBER_H

#include <strophe/strophe.h>

// message handlers
int in_message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
    void * const userdata);

// connection handler
void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status,
          const int error, xmpp_stream_error_t * const stream_error,
          void * const userdata);

#endif
