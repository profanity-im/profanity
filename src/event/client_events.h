/*
 * client_events.h
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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
 * along with Profanity.  If not, see <https://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give permission to
 * link the code of portions of this program with the OpenSSL library under
 * certain conditions as described in each individual source file, and
 * distribute linked combinations including the two.
 *
 * You must obey the GNU General Public License in all respects for all of the
 * code used other than OpenSSL. If you modify file(s) with this exception, you
 * may extend this exception to your version of the file(s), but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version. If you delete this exception statement from all
 * source files in the program, then also delete it here.
 *
 */

#ifndef EVENT_CLIENT_EVENTS_H
#define EVENT_CLIENT_EVENTS_H

#include "xmpp/xmpp.h"

jabber_conn_status_t cl_ev_connect_jid(const char *const jid, const char *const passwd, const char *const altdomain, const int port, const char *const tls_policy);
jabber_conn_status_t cl_ev_connect_account(ProfAccount *account);

void cl_ev_disconnect(void);

void cl_ev_presence_send(const resource_presence_t presence_type, const int idle_secs);

void cl_ev_send_msg(ProfChatWin *chatwin, const char *const msg, const char *const oob_url);
void cl_ev_send_muc_msg(ProfMucWin *mucwin, const char *const msg, const char *const oob_url);
void cl_ev_send_priv_msg(ProfPrivateWin *privwin, const char *const msg, const char *const oob_url);

#endif
