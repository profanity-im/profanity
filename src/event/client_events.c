/*
 * client_events.c
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

#include "config.h"

#include <stdlib.h>
#include <glib.h>

#include "log.h"
#include "config/preferences.h"
#include "plugins/plugins.h"
#include "ui/window_list.h"
#include "ui/ui.h"
#include "xmpp/xmpp.h"
#include "xmpp/roster_list.h"
#include "xmpp/chat_session.h"

#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif

#ifdef HAVE_LIBGPGME
#include "pgp/gpg.h"
#endif

jabber_conn_status_t
cl_ev_connect_jid(const char *const jid, const char *const passwd, const char *const altdomain, const int port, const char *const tls_policy)
{
    cons_show("Connecting as %s", jid);
    return session_connect_with_details(jid, passwd, altdomain, port, tls_policy);
}

jabber_conn_status_t
cl_ev_connect_account(ProfAccount *account)
{
    if (account->resource) {
        cons_show("Connecting with account %s as %s/%s", account->name, account->jid, account->resource);
    } else if (g_strcmp0(account->name, account->jid) == 0) {
        cons_show("Connecting with account %s", account->name);
    } else {
        cons_show("Connecting with account %s as %s", account->name, account->jid);
    }

    return session_connect_with_account(account);
}

void
cl_ev_disconnect(void)
{
    const char *jid = connection_get_fulljid();
    Jid *jidp = jid_create(jid);
    cons_show("%s logged out successfully.", jidp->barejid);
    jid_destroy(jidp);

    ui_disconnected();
    ui_close_all_wins();
    session_disconnect();
    roster_destroy();
    muc_invites_clear();
    chat_sessions_clear();
    tlscerts_clear_current();
#ifdef HAVE_LIBGPGME
    p_gpg_on_disconnect();
#endif
}

void
cl_ev_presence_send(const resource_presence_t presence_type, const int idle_secs)
{
    char *signed_status = NULL;

#ifdef HAVE_LIBGPGME
    char *account_name = session_get_account_name();
    ProfAccount *account = accounts_get_account(account_name);
    if (account->pgp_keyid) {
        char *msg = connection_get_presence_msg();
        signed_status = p_gpg_sign(msg, account->pgp_keyid);
    }
    account_free(account);
#endif

    presence_send(presence_type, idle_secs, signed_status);

    free(signed_status);
}

void
cl_ev_send_msg(ProfChatWin *chatwin, const char *const msg, const char *const oob_url)
{
    chat_state_active(chatwin->state);

    gboolean request_receipt = FALSE;
    if (prefs_get_boolean(PREF_RECEIPTS_REQUEST)) {
        char *session_jid = chat_session_get_jid(chatwin->barejid);
        if (session_jid) {
            Jid *session_jidp = jid_create(session_jid);
            if (session_jidp && session_jidp->resourcepart) {
                if (caps_jid_has_feature(session_jid, XMPP_FEATURE_RECEIPTS)) {
                    request_receipt = TRUE;
                }
            }
            jid_destroy(session_jidp);
            free(session_jid);
        }
    }

    char *plugin_msg = plugins_pre_chat_message_send(chatwin->barejid, msg);
    if (plugin_msg == NULL) {
        return;
    }

// OTR suported, PGP supported
#ifdef HAVE_LIBOTR
#ifdef HAVE_LIBGPGME
    if (chatwin->pgp_send) {
        char *id = message_send_chat_pgp(chatwin->barejid, plugin_msg, request_receipt);
        chat_log_pgp_msg_out(chatwin->barejid, plugin_msg);
        chatwin_outgoing_msg(chatwin, plugin_msg, id, PROF_MSG_PGP, request_receipt);
        free(id);
    } else {
        gboolean handled = otr_on_message_send(chatwin, plugin_msg, request_receipt);
        if (!handled) {
            char *id = message_send_chat(chatwin->barejid, plugin_msg, oob_url, request_receipt);
            chat_log_msg_out(chatwin->barejid, plugin_msg);
            chatwin_outgoing_msg(chatwin, plugin_msg, id, PROF_MSG_PLAIN, request_receipt);
            free(id);
        }
    }

    plugins_post_chat_message_send(chatwin->barejid, plugin_msg);
    free(plugin_msg);
    return;
#endif
#endif

// OTR supported, PGP unsupported
#ifdef HAVE_LIBOTR
#ifndef HAVE_LIBGPGME
    gboolean handled = otr_on_message_send(chatwin, plugin_msg, request_receipt);
    if (!handled) {
        char *id = message_send_chat(chatwin->barejid, plugin_msg, oob_url, request_receipt);
        chat_log_msg_out(chatwin->barejid, plugin_msg);
        chatwin_outgoing_msg(chatwin, plugin_msg, id, PROF_MSG_PLAIN, request_receipt);
        free(id);
    }

    plugins_post_chat_message_send(chatwin->barejid, plugin_msg);
    free(plugin_msg);
    return;
#endif
#endif

// OTR unsupported, PGP supported
#ifndef HAVE_LIBOTR
#ifdef HAVE_LIBGPGME
    if (chatwin->pgp_send) {
        char *id = message_send_chat_pgp(chatwin->barejid, plugin_msg, request_receipt);
        chat_log_pgp_msg_out(chatwin->barejid, plugin_msg);
        chatwin_outgoing_msg(chatwin, plugin_msg, id, PROF_MSG_PGP, request_receipt);
        free(id);
    } else {
        char *id = message_send_chat(chatwin->barejid, plugin_msg, oob_url, request_receipt);
        chat_log_msg_out(chatwin->barejid, plugin_msg);
        chatwin_outgoing_msg(chatwin, plugin_msg, id, PROF_MSG_PLAIN, request_receipt);
        free(id);
    }

    plugins_post_chat_message_send(chatwin->barejid, plugin_msg);
    free(plugin_msg);
    return;
#endif
#endif

// OTR unsupported, PGP unsupported
#ifndef HAVE_LIBOTR
#ifndef HAVE_LIBGPGME
    char *id = message_send_chat(chatwin->barejid, plugin_msg, oob_url, request_receipt);
    chat_log_msg_out(chatwin->barejid, plugin_msg);
    chatwin_outgoing_msg(chatwin, plugin_msg, id, PROF_MSG_PLAIN, request_receipt);
    free(id);

    plugins_post_chat_message_send(chatwin->barejid, plugin_msg);
    free(plugin_msg);
    return;
#endif
#endif
}

void
cl_ev_send_muc_msg(ProfMucWin *mucwin, const char *const msg, const char *const oob_url)
{
    char *plugin_msg = plugins_pre_room_message_send(mucwin->roomjid, msg);
    if (plugin_msg == NULL) {
        return;
    }

    message_send_groupchat(mucwin->roomjid, plugin_msg, oob_url);

    plugins_post_room_message_send(mucwin->roomjid, plugin_msg);
    free(plugin_msg);
}

void
cl_ev_send_priv_msg(ProfPrivateWin *privwin, const char *const msg, const char *const oob_url)
{
    if (privwin->occupant_offline) {
        privwin_message_occupant_offline(privwin);
    } else if (privwin->room_left) {
        privwin_message_left_room(privwin);
    } else {
        char *plugin_msg = plugins_pre_priv_message_send(privwin->fulljid, msg);

        message_send_private(privwin->fulljid, plugin_msg, oob_url);
        privwin_outgoing_msg(privwin, plugin_msg);

        plugins_post_priv_message_send(privwin->fulljid, plugin_msg);
        free(plugin_msg);
    }
}
