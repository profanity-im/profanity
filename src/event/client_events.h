/*
 * client_events.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef EVENT_CLIENT_EVENTS_H
#define EVENT_CLIENT_EVENTS_H

#include "xmpp/xmpp.h"

jabber_conn_status_t cl_ev_connect_jid(const char* const jid, const char* const passwd, const char* const altdomain, const int port, const char* const tls_policy, const char* const auth_policy);
jabber_conn_status_t cl_ev_connect_account(ProfAccount* account);

void cl_ev_disconnect(void);
void cl_ev_reconnect(void);

void cl_ev_presence_send(const resource_presence_t presence_type, const int idle_secs);

void cl_ev_send_msg_correct(ProfChatWin* chatwin, const char* const msg, const char* const oob_url, gboolean correct_last_msg);
void cl_ev_send_msg(ProfChatWin* chatwin, const char* const msg, const char* const oob_url);
void cl_ev_send_muc_msg_corrected(ProfMucWin* mucwin, const char* const msg, const char* const oob_url, gboolean correct_last_msg);
void cl_ev_send_muc_msg(ProfMucWin* mucwin, const char* const msg, const char* const oob_url);
void cl_ev_send_priv_msg(ProfPrivateWin* privwin, const char* const msg, const char* const oob_url);

#endif
