/*
 * client_events.c
 *
 * Copyright (C) 2012 - 2015 James Booth <boothj5@gmail.com>
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

#include <glib.h>

#include "config.h"
#include "log.h"
#include "ui/ui.h"
#include "window_list.h"
#include "xmpp/xmpp.h"
#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif

jabber_conn_status_t
cl_ev_connect_jid(const char * const jid, const char * const passwd, const char * const altdomain, const int port)
{
    cons_show("Connecting as %s", jid);
    return jabber_connect_with_details(jid, passwd, altdomain, port);
}

jabber_conn_status_t
cl_ev_connect_account(ProfAccount *account)
{
    char *jid = account_create_full_jid(account);
    cons_show("Connecting with account %s as %s", account->name, jid);
    free(jid);

    return jabber_connect_with_account(account);
}

void
cl_ev_presence_send(const resource_presence_t presence_type, const char * const msg, const int idle)
{
    presence_send(presence_type, msg, idle);
}

void
cl_ev_send_msg(ProfChatWin *chatwin, const char * const msg)
{
    chat_state_active(chatwin->state);

#ifdef HAVE_LIBOTR
    otr_on_message_send(chatwin, msg);
#else
    char *id = message_send_chat(chatwin->barejid, msg);
    chat_log_msg_out(chatwin->barejid, msg);
    ui_outgoing_chat_msg(chatwin, msg, id);
    free(id);
#endif
}

void
cl_ev_send_muc_msg(ProfMucWin *mucwin, const char * const msg)
{
    message_send_groupchat(mucwin->roomjid, msg);
}

void
cl_ev_send_priv_msg(ProfPrivateWin *privwin, const char * const msg)
{
    message_send_private(privwin->fulljid, msg);
    ui_outgoing_private_msg(privwin, msg);
}
