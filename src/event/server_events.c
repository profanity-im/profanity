/*
 * server_events.c
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

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "profanity.h"
#include "log.h"
#include "config/preferences.h"
#include "config/tlscerts.h"
#include "config/account.h"
#include "config/scripts.h"
#include "event/client_events.h"
#include "plugins/plugins.h"
#include "ui/window_list.h"
#include "xmpp/muc.h"
#include "xmpp/chat_session.h"
#include "xmpp/roster_list.h"

#ifdef HAVE_LIBOTR
#include "otr/otr.h"
#endif

#ifdef HAVE_LIBGPGME
#include "pgp/gpg.h"
#endif

#include "ui/ui.h"

void
sv_ev_login_account_success(char *account_name, gboolean secured)
{
    ProfAccount *account = accounts_get_account(account_name);

    roster_create();

#ifdef HAVE_LIBOTR
    otr_on_connect(account);
#endif

#ifdef HAVE_LIBGPGME
    p_gpg_on_connect(account->jid);
#endif

    ui_handle_login_account_success(account, secured);

    // attempt to rejoin rooms with passwords
    GList *rooms = muc_rooms();
    GList *curr = rooms;
    while (curr) {
        char *password = muc_password(curr->data);
        if (password) {
            char *nick = muc_nick(curr->data);
            presence_join_room(curr->data, nick, password);
        }
        curr = g_list_next(curr);
    }
    g_list_free(rooms);

    log_info("%s logged in successfully", account->jid);

    if (account->startscript) {
        scripts_exec(account->startscript);
    }

    account_free(account);
}

void
sv_ev_roster_received(void)
{
    if (prefs_get_boolean(PREF_ROSTER)) {
        ui_show_roster();
    }

    char *account_name = session_get_account_name();

#ifdef HAVE_LIBGPGME
    // check pgp key valid if specified
    ProfAccount *account = accounts_get_account(account_name);
    if (account && account->pgp_keyid) {
        char *err_str = NULL;
        if (!p_gpg_valid_key(account->pgp_keyid, &err_str)) {
            cons_show_error("Invalid PGP key ID specified: %s, %s", account->pgp_keyid, err_str);
        }
        free(err_str);
    }
    account_free(account);
#endif

    // send initial presence
    resource_presence_t conn_presence = accounts_get_login_presence(account_name);
    char *last_activity_str = accounts_get_last_activity(account_name);
    if (last_activity_str) {
        GDateTime *nowdt = g_date_time_new_now_utc();

        GTimeVal lasttv;
        gboolean res = g_time_val_from_iso8601(last_activity_str, &lasttv);
        if (res) {
            GDateTime *lastdt = g_date_time_new_from_timeval_utc(&lasttv);
            GTimeSpan diff_micros = g_date_time_difference(nowdt, lastdt);
            int diff_secs = (diff_micros / 1000) / 1000;
            if (prefs_get_boolean(PREF_LASTACTIVITY)) {
                connection_set_presence_msg(NULL);
                cl_ev_presence_send(conn_presence, diff_secs);
            } else {
                connection_set_presence_msg(NULL);
                cl_ev_presence_send(conn_presence, 0);
            }
            g_date_time_unref(lastdt);
        } else {
            connection_set_presence_msg(NULL);
            cl_ev_presence_send(conn_presence, 0);
        }

        free(last_activity_str);
        g_date_time_unref(nowdt);
    } else {
        connection_set_presence_msg(NULL);
        cl_ev_presence_send(conn_presence, 0);
    }

    const char *fulljid = connection_get_fulljid();
    plugins_on_connect(account_name, fulljid);
}

void
sv_ev_lost_connection(void)
{
    cons_show_error("Lost connection.");

#ifdef HAVE_LIBOTR
    GSList *recipients = wins_get_chat_recipients();
    GSList *curr = recipients;
    while (curr) {
        char *barejid = curr->data;
        ProfChatWin *chatwin = wins_get_chat(barejid);
        if (chatwin && otr_is_secure(barejid)) {
            chatwin_otr_unsecured(chatwin);
            otr_end_session(barejid);
        }
        curr = g_slist_next(curr);
    }
    if (recipients) {
        g_slist_free(recipients);
    }
#endif

    muc_invites_clear();
    chat_sessions_clear();
    ui_disconnected();
    roster_destroy();
#ifdef HAVE_LIBGPGME
    p_gpg_on_disconnect();
#endif
}

void
sv_ev_failed_login(void)
{
    cons_show_error("Login failed.");
    log_info("Login failed");
    tlscerts_clear_current();
}

void
sv_ev_room_invite(jabber_invite_t invite_type, const char *const invitor, const char *const room,
    const char *const reason, const char *const password)
{
    if (!muc_active(room) && !muc_invites_contain(room)) {
        cons_show_room_invite(invitor, room, reason);
        muc_invites_add(room, password);
    }
}

void
sv_ev_room_broadcast(const char *const room_jid, const char *const message)
{
    if (muc_roster_complete(room_jid)) {
        ProfMucWin *mucwin = wins_get_muc(room_jid);
        if (mucwin) {
            mucwin_broadcast(mucwin, message);
        }
    } else {
        muc_pending_broadcasts_add(room_jid, message);
    }
}

void
sv_ev_room_subject(const char *const room, const char *const nick, const char *const subject)
{
    muc_set_subject(room, subject);
    ProfMucWin *mucwin = wins_get_muc(room);
    if (mucwin && muc_roster_complete(room)) {
        mucwin_subject(mucwin, nick, subject);
    }
}

void
sv_ev_room_history(const char *const room_jid, const char *const nick,
    GDateTime *timestamp, const char *const message)
{
    ProfMucWin *mucwin = wins_get_muc(room_jid);
    if (mucwin) {
        mucwin_history(mucwin, nick, timestamp, message);
    }
}

void
sv_ev_room_message(const char *const room_jid, const char *const nick, const char *const message)
{
    if (prefs_get_boolean(PREF_GRLOG)) {
        Jid *jid = jid_create(connection_get_fulljid());
        groupchat_log_chat(jid->barejid, room_jid, nick, message);
        jid_destroy(jid);
    }

    ProfMucWin *mucwin = wins_get_muc(room_jid);
    if (!mucwin) {
        return;
    }

    char *new_message = plugins_pre_room_message_display(room_jid, nick, message);
    char *mynick = muc_nick(mucwin->roomjid);

    gboolean whole_word = prefs_get_boolean(PREF_NOTIFY_MENTION_WHOLE_WORD);
    gboolean case_sensitive = prefs_get_boolean(PREF_NOTIFY_MENTION_CASE_SENSITIVE);
    char *message_search = case_sensitive ? strdup(new_message) : g_utf8_strdown(new_message, -1);
    char *mynick_search = case_sensitive ? strdup(mynick) : g_utf8_strdown(mynick, -1);

    GSList *mentions = NULL;
    mentions = prof_occurrences(mynick_search, message_search, 0, whole_word, &mentions);
    gboolean mention = g_slist_length(mentions) > 0;
    g_free(message_search);
    g_free(mynick_search);

    GList *triggers = prefs_message_get_triggers(new_message);

    mucwin_message(mucwin, nick, new_message, mentions, triggers);

    g_slist_free(mentions);

    ProfWin *window = (ProfWin*)mucwin;
    int num = wins_get_num(window);
    gboolean is_current = FALSE;

    // currently in groupchat window
    if (wins_is_current(window)) {
        is_current = TRUE;
        status_bar_active(num);

        if ((g_strcmp0(mynick, nick) != 0) && (prefs_get_boolean(PREF_BEEP))) {
            beep();
        }

    // not currently on groupchat window
    } else {
        status_bar_new(num);

        if ((g_strcmp0(mynick, nick) != 0) && (prefs_get_boolean(PREF_FLASH))) {
            flash();
        }

        cons_show_incoming_room_message(nick, mucwin->roomjid, num, mention, triggers, mucwin->unread);

        mucwin->unread++;

        if (mention) {
            mucwin->unread_mentions = TRUE;
        }
        if (triggers) {
            mucwin->unread_triggers = TRUE;
        }
    }

    if (prefs_do_room_notify(is_current, mucwin->roomjid, mynick, nick, new_message, mention, triggers != NULL)) {
        Jid *jidp = jid_create(mucwin->roomjid);
        notify_room_message(nick, jidp->localpart, num, new_message);
        jid_destroy(jidp);
    }

    if (triggers) {
        g_list_free_full(triggers, free);
    }

    rosterwin_roster();

    plugins_post_room_message_display(room_jid, nick, new_message);
    free(new_message);
}

void
sv_ev_incoming_private_message(const char *const fulljid, char *message)
{
    char *plugin_message =  plugins_pre_priv_message_display(fulljid, message);

    ProfPrivateWin *privatewin = wins_get_private(fulljid);
    if (privatewin == NULL) {
        ProfWin *window = wins_new_private(fulljid);
        privatewin = (ProfPrivateWin*)window;
    }
    privwin_incoming_msg(privatewin, plugin_message, NULL);

    plugins_post_priv_message_display(fulljid, plugin_message);

    free(plugin_message);
    rosterwin_roster();
}

void
sv_ev_delayed_private_message(const char *const fulljid, char *message, GDateTime *timestamp)
{
    char *new_message = plugins_pre_priv_message_display(fulljid, message);

    ProfPrivateWin *privatewin = wins_get_private(fulljid);
    if (privatewin == NULL) {
        ProfWin *window = wins_new_private(fulljid);
        privatewin = (ProfPrivateWin*)window;
    }
    privwin_incoming_msg(privatewin, new_message, timestamp);

    plugins_post_priv_message_display(fulljid, new_message);

    free(new_message);
}

void
sv_ev_outgoing_carbon(char *barejid, char *message, char *pgp_message)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (!chatwin) {
        chatwin = chatwin_new(barejid);
    }

    chat_state_active(chatwin->state);

#ifdef HAVE_LIBGPGME
    if (pgp_message) {
        char *decrypted = p_gpg_decrypt(pgp_message);
        if (decrypted) {
            chatwin_outgoing_carbon(chatwin, decrypted, PROF_MSG_PGP);
        } else {
            chatwin_outgoing_carbon(chatwin, message, PROF_MSG_PLAIN);
        }
    } else {
        chatwin_outgoing_carbon(chatwin, message, PROF_MSG_PLAIN);
    }
#else
    chatwin_outgoing_carbon(chatwin, message, PROF_MSG_PLAIN);
#endif
}

#ifdef HAVE_LIBGPGME
static void
_sv_ev_incoming_pgp(ProfChatWin *chatwin, gboolean new_win, char *barejid, char *resource, char *message, char *pgp_message, GDateTime *timestamp)
{
    char *decrypted = p_gpg_decrypt(pgp_message);
    if (decrypted) {
        chatwin_incoming_msg(chatwin, resource, decrypted, timestamp, new_win, PROF_MSG_PGP);
        chat_log_pgp_msg_in(barejid, decrypted, timestamp);
        chatwin->pgp_recv = TRUE;
        p_gpg_free_decrypted(decrypted);
    } else {
        chatwin_incoming_msg(chatwin, resource, message, timestamp, new_win, PROF_MSG_PLAIN);
        chat_log_msg_in(barejid, message, timestamp);
        chatwin->pgp_recv = FALSE;
    }
}
#endif

#ifdef HAVE_LIBOTR
static void
_sv_ev_incoming_otr(ProfChatWin *chatwin, gboolean new_win, char *barejid, char *resource, char *message, GDateTime *timestamp)
{
    gboolean decrypted = FALSE;
    char *otr_res = otr_on_message_recv(barejid, resource, message, &decrypted);
    if (otr_res) {
        if (decrypted) {
            chatwin_incoming_msg(chatwin, resource, otr_res, timestamp, new_win, PROF_MSG_OTR);
            chatwin->pgp_send = FALSE;
        } else {
            chatwin_incoming_msg(chatwin, resource, otr_res, timestamp, new_win, PROF_MSG_PLAIN);
        }
        chat_log_otr_msg_in(barejid, otr_res, decrypted, timestamp);
        otr_free_message(otr_res);
        chatwin->pgp_recv = FALSE;
    }
}
#endif

static void
_sv_ev_incoming_plain(ProfChatWin *chatwin, gboolean new_win, char *barejid, char *resource, char *message, GDateTime *timestamp)
{
    chatwin_incoming_msg(chatwin, resource, message, timestamp, new_win, PROF_MSG_PLAIN);
    chat_log_msg_in(barejid, message, timestamp);
    chatwin->pgp_recv = FALSE;
}

void
sv_ev_incoming_message(char *barejid, char *resource, char *message, char *pgp_message, GDateTime *timestamp)
{
    gboolean new_win = FALSE;
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (!chatwin) {
        ProfWin *window = wins_new_chat(barejid);
        chatwin = (ProfChatWin*)window;
        new_win = TRUE;
    }

// OTR suported, PGP supported
#ifdef HAVE_LIBOTR
#ifdef HAVE_LIBGPGME
    if (pgp_message) {
        if (chatwin->is_otr) {
            win_println((ProfWin*)chatwin, THEME_DEFAULT, '-', "PGP encrypted message received whilst in OTR session.");
        } else { // PROF_ENC_NONE, PROF_ENC_PGP
            _sv_ev_incoming_pgp(chatwin, new_win, barejid, resource, message, pgp_message, timestamp);
        }
    } else {
        _sv_ev_incoming_otr(chatwin, new_win, barejid, resource, message, timestamp);
    }
    rosterwin_roster();
    return;
#endif
#endif

// OTR supported, PGP unsupported
#ifdef HAVE_LIBOTR
#ifndef HAVE_LIBGPGME
    _sv_ev_incoming_otr(chatwin, new_win, barejid, resource, message, timestamp);
    rosterwin_roster();
    return;
#endif
#endif

// OTR unsupported, PGP supported
#ifndef HAVE_LIBOTR
#ifdef HAVE_LIBGPGME
    if (pgp_message) {
        _sv_ev_incoming_pgp(chatwin, new_win, barejid, resource, message, pgp_message, timestamp);
    } else {
        _sv_ev_incoming_plain(chatwin, new_win, barejid, resource, message, timestamp);
    }
    rosterwin_roster();
    return;
#endif
#endif

// OTR unsupported, PGP unsupported
#ifndef HAVE_LIBOTR
#ifndef HAVE_LIBGPGME
    _sv_ev_incoming_plain(chatwin, new_win, barejid, resource, message, timestamp);
    rosterwin_roster();
    return;
#endif
#endif
}

void
sv_ev_incoming_carbon(char *barejid, char *resource, char *message, char *pgp_message)
{
    gboolean new_win = FALSE;
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (!chatwin) {
        ProfWin *window = wins_new_chat(barejid);
        chatwin = (ProfChatWin*)window;
        new_win = TRUE;
    }

#ifdef HAVE_LIBGPGME
    if (pgp_message) {
        _sv_ev_incoming_pgp(chatwin, new_win, barejid, resource, message, pgp_message, NULL);
    } else {
        _sv_ev_incoming_plain(chatwin, new_win, barejid, resource, message, NULL);
    }
#else
    _sv_ev_incoming_plain(chatwin, new_win, barejid, resource, message, NULL);
#endif
    rosterwin_roster();
}

void
sv_ev_message_receipt(const char *const barejid, const char *const id)
{
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (!chatwin)
        return;

    chatwin_receipt_received(chatwin, id);
}

void
sv_ev_typing(char *barejid, char *resource)
{
    ui_contact_typing(barejid, resource);
    if (wins_chat_exists(barejid)) {
        chat_session_recipient_typing(barejid, resource);
    }
}

void
sv_ev_paused(char *barejid, char *resource)
{
    if (wins_chat_exists(barejid)) {
        chat_session_recipient_paused(barejid, resource);
    }
}

void
sv_ev_inactive(char *barejid, char *resource)
{
    if (wins_chat_exists(barejid)) {
        chat_session_recipient_inactive(barejid, resource);
    }
}

void
sv_ev_gone(const char *const barejid, const char *const resource)
{
    if (barejid && resource) {
        gboolean show_message = TRUE;

        ProfChatWin *chatwin = wins_get_chat(barejid);
        if (chatwin) {
            ChatSession *session = chat_session_get(barejid);
            if (session && g_strcmp0(session->resource, resource) != 0) {
                show_message = FALSE;
            }
            if (show_message) {
                chatwin_recipient_gone(chatwin);
            }
        }
    }

    if (wins_chat_exists(barejid)) {
        chat_session_recipient_gone(barejid, resource);
    }
}

void
sv_ev_activity(const char *const barejid, const char *const resource, gboolean send_states)
{
    if (wins_chat_exists(barejid)) {
        chat_session_recipient_active(barejid, resource, send_states);
    }
}

void
sv_ev_subscription(const char *barejid, jabber_subscr_t type)
{
    switch (type) {
    case PRESENCE_SUBSCRIBE:
        /* TODO: auto-subscribe if needed */
        cons_show("Received authorization request from %s", barejid);
        log_info("Received authorization request from %s", barejid);
        ui_print_system_msg_from_recipient(barejid, "Authorization request, type '/sub allow' to accept or '/sub deny' to reject");
        if (prefs_get_boolean(PREF_NOTIFY_SUB)) {
            notify_subscription(barejid);
        }
        break;
    case PRESENCE_SUBSCRIBED:
        cons_show("Subscription received from %s", barejid);
        log_info("Subscription received from %s", barejid);
        ui_print_system_msg_from_recipient(barejid, "Subscribed");
        break;
    case PRESENCE_UNSUBSCRIBED:
        cons_show("%s deleted subscription", barejid);
        log_info("%s deleted subscription", barejid);
        ui_print_system_msg_from_recipient(barejid, "Unsubscribed");
        break;
    default:
        /* unknown type */
        break;
    }
}

void
sv_ev_contact_offline(char *barejid, char *resource, char *status)
{
    gboolean updated = roster_contact_offline(barejid, resource, status);

    if (resource && updated) {
        plugins_on_contact_offline(barejid, resource, status);
        ui_contact_offline(barejid, resource, status);
    }

#ifdef HAVE_LIBOTR
    ProfChatWin *chatwin = wins_get_chat(barejid);
    if (chatwin && otr_is_secure(barejid)) {
        chatwin_otr_unsecured(chatwin);
        otr_end_session(chatwin->barejid);
    }
#endif

    rosterwin_roster();
    chat_session_remove(barejid);
}

void
sv_ev_contact_online(char *barejid, Resource *resource, GDateTime *last_activity, char *pgpsig)
{
    gboolean updated = roster_update_presence(barejid, resource, last_activity);

    if (updated) {
        plugins_on_contact_presence(barejid, resource->name, string_from_resource_presence(resource->presence),
            resource->status, resource->priority);
        ui_contact_online(barejid, resource, last_activity);
    }

#ifdef HAVE_LIBGPGME
    if (pgpsig) {
        p_gpg_verify(barejid, pgpsig);
    }
#endif

    rosterwin_roster();
    chat_session_remove(barejid);
}

void
sv_ev_leave_room(const char *const room)
{
    muc_leave(room);
    ui_leave_room(room);
}

void
sv_ev_room_destroy(const char *const room)
{
    muc_leave(room);
    ui_room_destroy(room);
}

void
sv_ev_room_destroyed(const char *const room, const char *const new_jid, const char *const password,
    const char *const reason)
{
    muc_leave(room);
    ui_room_destroyed(room, reason, new_jid, password);
}

void
sv_ev_room_kicked(const char *const room, const char *const actor, const char *const reason)
{
    muc_leave(room);
    ui_room_kicked(room, actor, reason);
}

void
sv_ev_room_banned(const char *const room, const char *const actor, const char *const reason)
{
    muc_leave(room);
    ui_room_banned(room, actor, reason);
}

void
sv_ev_room_occupant_offline(const char *const room, const char *const nick,
    const char *const show, const char *const status)
{
    muc_roster_remove(room, nick);

    char *muc_status_pref = prefs_get_string(PREF_STATUSES_MUC);
    ProfMucWin *mucwin = wins_get_muc(room);
    if (mucwin && (g_strcmp0(muc_status_pref, "none") != 0)) {
        mucwin_occupant_offline(mucwin, nick);
    }
    prefs_free_string(muc_status_pref);

    Jid *jidp = jid_create_from_bare_and_resource(room, nick);
    ProfPrivateWin *privwin = wins_get_private(jidp->fulljid);
    jid_destroy(jidp);
    if (privwin != NULL) {
        privwin_occupant_offline(privwin);
    }

    occupantswin_occupants(room);
    rosterwin_roster();
}

void
sv_ev_room_occupent_kicked(const char *const room, const char *const nick, const char *const actor,
    const char *const reason)
{
    muc_roster_remove(room, nick);
    ProfMucWin *mucwin = wins_get_muc(room);
    if (mucwin) {
        mucwin_occupant_kicked(mucwin, nick, actor, reason);
    }

    Jid *jidp = jid_create_from_bare_and_resource(room, nick);
    ProfPrivateWin *privwin = wins_get_private(jidp->fulljid);
    jid_destroy(jidp);
    if (privwin != NULL) {
        privwin_occupant_kicked(privwin, actor, reason);
    }

    occupantswin_occupants(room);
    rosterwin_roster();
}

void
sv_ev_room_occupent_banned(const char *const room, const char *const nick, const char *const actor,
    const char *const reason)
{
    muc_roster_remove(room, nick);
    ProfMucWin *mucwin = wins_get_muc(room);
    if (mucwin) {
        mucwin_occupant_banned(mucwin, nick, actor, reason);
    }

    Jid *jidp = jid_create_from_bare_and_resource(room, nick);
    ProfPrivateWin *privwin = wins_get_private(jidp->fulljid);
    jid_destroy(jidp);
    if (privwin != NULL) {
        privwin_occupant_banned(privwin, actor, reason);
    }

    occupantswin_occupants(room);
    rosterwin_roster();
}

void
sv_ev_roster_update(const char *const barejid, const char *const name,
    GSList *groups, const char *const subscription, gboolean pending_out)
{
    roster_update(barejid, name, groups, subscription, pending_out);
    rosterwin_roster();
}

void
sv_ev_xmpp_stanza(const char *const msg)
{
    ProfXMLWin *xmlwin = wins_get_xmlconsole();
    if (xmlwin) {
        xmlwin_show(xmlwin, msg);
    }
}

void
sv_ev_muc_self_online(const char *const room, const char *const nick, gboolean config_required,
    const char *const role, const char *const affiliation, const char *const actor, const char *const reason,
    const char *const jid, const char *const show, const char *const status)
{
    muc_roster_add(room, nick, jid, role, affiliation, show, status);
    char *old_role = muc_role_str(room);
    char *old_affiliation = muc_affiliation_str(room);
    muc_set_role(room, role);
    muc_set_affiliation(room, affiliation);

    // handle self nick change
    if (muc_nick_change_pending(room)) {
        muc_nick_change_complete(room, nick);
        ProfMucWin *mucwin = wins_get_muc(room);
        if (mucwin) {
            mucwin_nick_change(mucwin, nick);
        }

    // handle roster complete
    } else if (!muc_roster_complete(room)) {
        if (muc_autojoin(room)) {
            ui_room_join(room, FALSE);
        } else {
            ui_room_join(room, TRUE);
        }

        iq_room_info_request(room, FALSE);

        if (muc_invites_contain(room)) {
            if (prefs_get_boolean(PREF_BOOKMARK_INVITE) && !bookmark_exists(room)) {
                bookmark_add(room, nick, muc_invite_password(room), "on");
            }
            muc_invites_remove(room);
        }

        muc_roster_set_complete(room);

        // show roster if occupants list disabled by default
        ProfMucWin *mucwin = wins_get_muc(room);
        if (mucwin && !prefs_get_boolean(PREF_OCCUPANTS)) {
            GList *occupants = muc_roster(room);
            mucwin_roster(mucwin, occupants, NULL);
            g_list_free(occupants);
        }

        char *subject = muc_subject(room);
        if (mucwin && subject) {
            mucwin_subject(mucwin, NULL, subject);
        }

        GList *pending_broadcasts = muc_pending_broadcasts(room);
        if (mucwin && pending_broadcasts) {
            GList *curr = pending_broadcasts;
            while (curr) {
                mucwin_broadcast(mucwin, curr->data);
                curr = g_list_next(curr);
            }
        }

        // room configuration required
        if (config_required) {
            muc_set_requires_config(room, TRUE);
            if (mucwin) {
                mucwin_requires_config(mucwin);
            }
        }

        rosterwin_roster();

    // check for change in role/affiliation
    } else {
        ProfMucWin *mucwin = wins_get_muc(room);
        if (mucwin && prefs_get_boolean(PREF_MUC_PRIVILEGES)) {
            // both changed
            if ((g_strcmp0(role, old_role) != 0) && (g_strcmp0(affiliation, old_affiliation) != 0)) {
                mucwin_role_and_affiliation_change(mucwin, role, affiliation, actor, reason);

            // role changed
            } else if (g_strcmp0(role, old_role) != 0) {
                mucwin_role_change(mucwin, role, actor, reason);

            // affiliation changed
            } else if (g_strcmp0(affiliation, old_affiliation) != 0) {
                mucwin_affiliation_change(mucwin, affiliation, actor, reason);
            }
        }
    }

    occupantswin_occupants(room);
}

void
sv_ev_muc_occupant_online(const char *const room, const char *const nick, const char *const jid,
    const char *const role, const char *const affiliation, const char *const actor, const char *const reason,
    const char *const show, const char *const status)
{
    Occupant *occupant = muc_roster_item(room, nick);

    const char *old_role = NULL;
    const char *old_affiliation = NULL;
    if (occupant) {
        old_role = muc_occupant_role_str(occupant);
        old_affiliation = muc_occupant_affiliation_str(occupant);
    }

    gboolean updated = muc_roster_add(room, nick, jid, role, affiliation, show, status);

    // not yet finished joining room
    if (!muc_roster_complete(room)) {
        return;
    }

    // handle nickname change
    char *old_nick = muc_roster_nick_change_complete(room, nick);
    if (old_nick) {
        ProfMucWin *mucwin = wins_get_muc(room);
        if (mucwin) {
            mucwin_occupant_nick_change(mucwin, old_nick, nick);
        }
        wins_private_nick_change(mucwin->roomjid, old_nick, nick);
        free(old_nick);

        occupantswin_occupants(room);
        rosterwin_roster();
        return;
    }

    // joined room
    if (!occupant) {
        char *muc_status_pref = prefs_get_string(PREF_STATUSES_MUC);
        ProfMucWin *mucwin = wins_get_muc(room);
        if (mucwin && g_strcmp0(muc_status_pref, "none") != 0) {
            mucwin_occupant_online(mucwin, nick, role, affiliation, show, status);
        }
        prefs_free_string(muc_status_pref);

        Jid *jidp = jid_create_from_bare_and_resource(mucwin->roomjid, nick);
        ProfPrivateWin *privwin = wins_get_private(jidp->fulljid);
        jid_destroy(jidp);
        if (privwin) {
            privwin_occupant_online(privwin);
        }

        occupantswin_occupants(room);
        rosterwin_roster();
        return;
    }

    // presence updated
    if (updated) {
        char *muc_status_pref = prefs_get_string(PREF_STATUSES_MUC);
        ProfMucWin *mucwin = wins_get_muc(room);
        if (mucwin && (g_strcmp0(muc_status_pref, "all") == 0)) {
            mucwin_occupant_presence(mucwin, nick, show, status);
        }
        prefs_free_string(muc_status_pref);
        occupantswin_occupants(room);

    // presence unchanged, check for role/affiliation change
    } else {
        ProfMucWin *mucwin = wins_get_muc(room);
        if (mucwin && prefs_get_boolean(PREF_MUC_PRIVILEGES)) {
            // both changed
            if ((g_strcmp0(role, old_role) != 0) && (g_strcmp0(affiliation, old_affiliation) != 0)) {
                mucwin_occupant_role_and_affiliation_change(mucwin, nick, role, affiliation, actor, reason);

            // role changed
            } else if (g_strcmp0(role, old_role) != 0) {
                mucwin_occupant_role_change(mucwin, nick, role, actor, reason);

            // affiliation changed
            } else if (g_strcmp0(affiliation, old_affiliation) != 0) {
                mucwin_occupant_affiliation_change(mucwin, nick, affiliation, actor, reason);
            }
        }
        occupantswin_occupants(room);
    }

    rosterwin_roster();
}

int
sv_ev_certfail(const char *const errormsg, TLSCertificate *cert)
{
    // check profanity trusted certs
    if (tlscerts_exists(cert->fingerprint)) {
        return 1;
    }

    // check current cert
    char *current_fp = tlscerts_get_current();
    if (current_fp && g_strcmp0(current_fp, cert->fingerprint) == 0) {
        return 1;
    }

    cons_show("");
    cons_show_error("TLS certificate verification failed: %s", errormsg);
    cons_show_tlscert(cert);
    cons_show("");
    cons_show("Use '/tls allow' to accept this certificate.");
    cons_show("Use '/tls always' to accept this certificate permanently.");
    cons_show("Use '/tls deny' to reject this certificate.");
    cons_show("");
    ui_update();

    char *cmd = ui_get_line();

    while ((g_strcmp0(cmd, "/tls allow") != 0)
                && (g_strcmp0(cmd, "/tls always") != 0)
                && (g_strcmp0(cmd, "/tls deny") != 0)
                && (g_strcmp0(cmd, "/quit") != 0)) {
        cons_show("Use '/tls allow' to accept this certificate.");
        cons_show("Use '/tls always' to accept this certificate permanently.");
        cons_show("Use '/tls deny' to reject this certificate.");
        cons_show("");
        ui_update();
        free(cmd);
        cmd = ui_get_line();
    }

    if (g_strcmp0(cmd, "/tls allow") == 0) {
        cons_show("Continuing with connection.");
        tlscerts_set_current(cert->fingerprint);
        free(cmd);
        return 1;
    } else if (g_strcmp0(cmd, "/tls always") == 0) {
        cons_show("Adding %s to trusted certificates.", cert->fingerprint);
        if (!tlscerts_exists(cert->fingerprint)) {
            tlscerts_add(cert);
        }
        free(cmd);
        return 1;
    } else if (g_strcmp0(cmd, "/quit") == 0) {
        prof_set_quit();
        free(cmd);
        return 0;
    } else {
        cons_show("Aborting connection.");
        free(cmd);
        return 0;
    }
}

void
sv_ev_lastactivity_response(const char *const from, const int seconds, const char *const msg)
{
    Jid *jidp = jid_create(from);

    if (!jidp) {
        return;
    }

    GDateTime *now = g_date_time_new_now_local();
    GDateTime *active = g_date_time_add_seconds(now, 0 - seconds);

    gchar *date_fmt = NULL;
    char *time_pref = prefs_get_string(PREF_TIME_LASTACTIVITY);
    date_fmt = g_date_time_format(active, time_pref);
    prefs_free_string(time_pref);
    assert(date_fmt != NULL);

    // full jid - last activity
    if (jidp->resourcepart) {
        if (seconds == 0) {
            if (msg) {
                cons_show("%s currently active, status: %s", from, msg);
            } else {
                cons_show("%s currently active", from);
            }
        } else {
            if (msg) {
                cons_show("%s last active %s, status: %s", from, date_fmt, msg);
            } else {
                cons_show("%s last active %s", from, date_fmt);
            }
        }

    // barejid - last logged in
    } else if (jidp->localpart) {
        if (seconds == 0) {
            if (msg) {
                cons_show("%s currently logged in, status: %s", from, msg);
            } else {
                cons_show("%s currently logged in", from);
            }
        } else {
            if (msg) {
                cons_show("%s last logged in %s, status: %s", from, date_fmt, msg);
            } else {
                cons_show("%s last logged in %s", from, date_fmt);
            }
        }

    // domain only - uptime
    } else {
        int left = seconds;
        int days = seconds / 86400;
        left = left - days * 86400;
        int hours = left / 3600;
        left = left - hours * 3600;
        int minutes = left / 60;
        left = left - minutes * 60;
        int seconds = left;

        cons_show("%s up since %s, uptime %d days, %d hrs, %d mins, %d secs", from, date_fmt, days, hours, minutes, seconds);
    }

    g_date_time_unref(now);
    g_date_time_unref(active);
    g_free(date_fmt);
    jid_destroy(jidp);
}

void
sv_ev_bookmark_autojoin(Bookmark *bookmark)
{
    char *nick = NULL;
    if (bookmark->nick) {
        nick = strdup(bookmark->nick);
    } else {
        char *account_name = session_get_account_name();
        ProfAccount *account = accounts_get_account(account_name);
        nick = strdup(account->muc_nick);
        account_free(account);
    }

    log_debug("Autojoin %s with nick=%s", bookmark->barejid, nick);
    if (!muc_active(bookmark->barejid)) {
        presence_join_room(bookmark->barejid, nick, bookmark->password);
        muc_join(bookmark->barejid, nick, bookmark->password, TRUE);
    }

    free(nick);
}
