/*
 * client_events.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2026 Michael Vetter <jubalh@iodoru.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#include "config.h"

#include <stdlib.h>
#include <glib.h>

#include "log.h"
#include "chatlog.h"
#include "database.h"
#include "config/preferences.h"
#include "event/common.h"
#include "plugins/plugins.h"
#include "ui/window_list.h"
#include "xmpp/chat_session.h"
#include "xmpp/session.h"
#include "xmpp/xmpp.h"

#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif

#ifdef HAVE_LIBGPGME
#include "pgp/gpg.h"
#endif

#ifdef HAVE_OMEMO
#include "omemo/omemo.h"
#endif

jabber_conn_status_t
cl_ev_connect_jid(const char* const jid, const char* const passwd, const char* const altdomain, const int port, const char* const tls_policy, const char* const auth_policy)
{
    cons_show("Connecting as %s", jid);
    return session_connect_with_details(jid, passwd, altdomain, port, tls_policy, auth_policy);
}

jabber_conn_status_t
cl_ev_connect_account(ProfAccount* account)
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
    cons_show("%s logged out successfully.", connection_get_barejid());

    ui_close_all_wins();
    ev_disconnect_cleanup();
    // on intentional disconnect reset the counter
    ev_reset_connection_counter();
}

void
cl_ev_reconnect(void)
{
    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status == JABBER_CONNECTING) {
        cons_show_error("Reconnection aborted: Connection attempt is already in progress");
        return;
    }

    cons_show("Reconnecting now.");
    if (conn_status != JABBER_DISCONNECTED && conn_status != JABBER_DISCONNECTING) {
        connection_disconnect();
        ev_disconnect_cleanup();
        // on intentional disconnect reset the counter
        ev_reset_connection_counter();
    }
    session_reconnect_now();
}

void
cl_ev_presence_send(const resource_presence_t presence_type, const int idle_secs)
{
    auto_gchar gchar* signed_status = NULL;

#ifdef HAVE_LIBGPGME
    ProfAccount* account = accounts_get_account(session_get_account_name());
    if (account->pgp_keyid) {
        signed_status = p_gpg_sign(connection_get_presence_msg(), account->pgp_keyid);
    }
    account_free(account);
#endif

    presence_send(presence_type, idle_secs, signed_status);
}

void
cl_ev_send_msg_correct(ProfChatWin* chatwin, const char* const msg, const char* const oob_url, gboolean correct_last_msg)
{
    chat_state_active(chatwin->state);

    gboolean request_receipt = prefs_get_boolean(PREF_RECEIPTS_REQUEST);
    if (request_receipt) {
        auto_char char* jid = chat_session_get_jid(chatwin->barejid);
        request_receipt = caps_jid_has_feature(jid, XMPP_FEATURE_RECEIPTS);
    }

    auto_char char* plugin_msg = plugins_pre_chat_message_send(chatwin->barejid, msg);
    auto_gchar gchar* sanitized_msg = str_xml_sanitize(plugin_msg ?: msg);
    const char* message = sanitized_msg;

    char* replace_id = NULL;
    if (correct_last_msg) {
        replace_id = chatwin->last_msg_id;
    }

    if (chatwin->is_omemo) {
#ifdef HAVE_OMEMO
        auto_char char* id = omemo_on_message_send((ProfWin*)chatwin, message, request_receipt, FALSE, replace_id);
        if (id != NULL) {
            chat_log_omemo_msg_out(chatwin->barejid, message, NULL);
            log_database_add_outgoing_chat(id, chatwin->barejid, message, replace_id, PROF_MSG_ENC_OMEMO);
            chatwin_outgoing_msg(chatwin, message, id, PROF_MSG_ENC_OMEMO, request_receipt, replace_id);
        }
#endif
    } else if (chatwin->is_ox) {
#ifdef HAVE_LIBGPGME
        // XEP-0373: OpenPGP for XMPP
        auto_char char* id = message_send_chat_ox(chatwin->barejid, message, request_receipt, replace_id);
        if (id != NULL) {
            chat_log_pgp_msg_out(chatwin->barejid, message, NULL);
            log_database_add_outgoing_chat(id, chatwin->barejid, message, replace_id, PROF_MSG_ENC_OX);
            chatwin_outgoing_msg(chatwin, message, id, PROF_MSG_ENC_OX, request_receipt, replace_id);
        }
#endif
    } else if (chatwin->pgp_send) {
#ifdef HAVE_LIBGPGME
        auto_char char* id = message_send_chat_pgp(chatwin->barejid, message, request_receipt, replace_id);
        if (id != NULL) {
            chat_log_pgp_msg_out(chatwin->barejid, message, NULL);
            log_database_add_outgoing_chat(id, chatwin->barejid, message, replace_id, PROF_MSG_ENC_PGP);
            chatwin_outgoing_msg(chatwin, message, id, PROF_MSG_ENC_PGP, request_receipt, replace_id);
        }
#endif
    } else {
        gboolean handled = FALSE;
#ifdef HAVE_LIBOTR
        handled = otr_on_message_send(chatwin, message, request_receipt, replace_id);
#endif
        if (!handled) {
            auto_char char* id = message_send_chat(chatwin->barejid, message, oob_url, request_receipt, replace_id);
            chat_log_msg_out(chatwin->barejid, message, NULL);
            log_database_add_outgoing_chat(id, chatwin->barejid, message, replace_id, PROF_MSG_ENC_NONE);
            chatwin_outgoing_msg(chatwin, message, id, PROF_MSG_ENC_NONE, request_receipt, replace_id);
        }
    }

    plugins_post_chat_message_send(chatwin->barejid, message);
    return;
}

void
cl_ev_send_msg(ProfChatWin* chatwin, const char* const msg, const char* const oob_url)
{
    cl_ev_send_msg_correct(chatwin, msg, oob_url, FALSE);
}

void
cl_ev_send_muc_msg_corrected(ProfMucWin* mucwin, const char* const msg, const char* const oob_url, gboolean correct_last_msg)
{
    auto_char char* plugin_msg = plugins_pre_room_message_send(mucwin->roomjid, msg);
    auto_gchar gchar* sanitized_msg = str_xml_sanitize(plugin_msg ?: msg);
    const char* message = sanitized_msg;

    char* replace_id = NULL;
    if (correct_last_msg) {
        replace_id = mucwin->last_msg_id;
    }

#ifdef HAVE_OMEMO
    if (mucwin->is_omemo) {
        auto_char char* id = omemo_on_message_send((ProfWin*)mucwin, message, FALSE, TRUE, replace_id);
        groupchat_log_omemo_msg_out(mucwin->roomjid, message);
        log_database_add_outgoing_muc(id, mucwin->roomjid, message, replace_id, PROF_MSG_ENC_OMEMO);
        mucwin_outgoing_msg(mucwin, message, id, PROF_MSG_ENC_OMEMO, replace_id);
    } else
#endif
    {
        auto_char char* id = message_send_groupchat(mucwin->roomjid, message, oob_url, replace_id);
        groupchat_log_msg_out(mucwin->roomjid, message);
        log_database_add_outgoing_muc(id, mucwin->roomjid, message, replace_id, PROF_MSG_ENC_NONE);
        mucwin_outgoing_msg(mucwin, message, id, PROF_MSG_ENC_NONE, replace_id);
    }

    plugins_post_room_message_send(mucwin->roomjid, message);
    return;
}

void
cl_ev_send_muc_msg(ProfMucWin* mucwin, const char* const msg, const char* const oob_url)
{
    cl_ev_send_muc_msg_corrected(mucwin, msg, oob_url, FALSE);
}

void
cl_ev_send_priv_msg(ProfPrivateWin* privwin, const char* const msg, const char* const oob_url)
{
    if (privwin->occupant_offline) {
        privwin_message_occupant_offline(privwin);
    } else if (privwin->room_left) {
        privwin_message_left_room(privwin);
    } else {
        auto_char char* plugin_msg = plugins_pre_priv_message_send(privwin->fulljid, msg);
        auto_gchar gchar* sanitized_msg = str_xml_sanitize(plugin_msg ?: msg);
        const char* message = sanitized_msg;
        auto_jid Jid* jidp = jid_create(privwin->fulljid);

        auto_char char* id = message_send_private(privwin->fulljid, message, oob_url);
        chat_log_msg_out(jidp->barejid, message, jidp->resourcepart);
        log_database_add_outgoing_muc_pm(id, privwin->fulljid, message, NULL, PROF_MSG_ENC_NONE);
        privwin_outgoing_msg(privwin, message);

        plugins_post_priv_message_send(privwin->fulljid, message);
    }
}
