/*
 * connection.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef XMPP_CONNECTION_H
#define XMPP_CONNECTION_H

#include "xmpp/xmpp.h"

#define CON_RAND_ID_LEN 15

void connection_init(void);
void connection_shutdown(void);
void connection_check_events(void);

jabber_conn_status_t connection_connect(const char* const fulljid, const char* const passwd, const char* const altdomain, int port,
                                        const char* const tls_policy, const char* const auth_policy);
jabber_conn_status_t connection_register(const char* const altdomain, int port, const char* const tls_policy,
                                         const char* const username, const char* const password);
void connection_set_disconnected(void);

void connection_set_priority(const int priority);
void connection_set_priority(int priority);
void connection_set_disco_items(GSList* items);

xmpp_conn_t* connection_get_conn(void);
xmpp_ctx_t* connection_get_ctx(void);
const char* connection_get_domain(void);
void connection_request_features(void);
void connection_features_received(const char* const jid);
GHashTable* connection_get_features(const char* const jid);

void connection_clear_data(void);

void connection_add_available_resource(Resource* resource);
void connection_remove_available_resource(const char* const resource);

char* connection_create_stanza_id(void);

#endif
