/*
 * server_events.h
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

#ifndef EVENT_SERVER_EVENTS_H
#define EVENT_SERVER_EVENTS_H

#include "xmpp/xmpp.h"

void sv_ev_login_account_success(char *account_name, gboolean secured);
void sv_ev_lost_connection(void);
void sv_ev_failed_login(void);
void sv_ev_room_invite(jabber_invite_t invite_type,
    const char *const invitor, const char *const room,
    const char *const reason, const char *const password);
void sv_ev_room_broadcast(const char *const room_jid, const char *const message);
void sv_ev_room_subject(const char *const room, const char *const nick, const char *const subject);
void sv_ev_room_history(const char *const room_jid, const char *const nick,
    GDateTime *timestamp, const char *const message);
void sv_ev_room_message(const char *const room_jid, const char *const nick,
    const char *const message);
void sv_ev_incoming_message(char *barejid, char *resource, char *message, char *pgp_message, GDateTime *timestamp);
void sv_ev_incoming_private_message(const char *const fulljid, char *message);
void sv_ev_delayed_private_message(const char *const fulljid, char *message, GDateTime *timestamp);
void sv_ev_typing(char *barejid, char *resource);
void sv_ev_paused(char *barejid, char *resource);
void sv_ev_inactive(char *barejid, char *resource);
void sv_ev_activity(char *barejid, char *resource, gboolean send_states);
void sv_ev_gone(const char *const barejid, const char *const resource);
void sv_ev_subscription(const char *from, jabber_subscr_t type);
void sv_ev_message_receipt(const char *const barejid, const char *const id);
void sv_ev_contact_offline(char *contact, char *resource, char *status);
void sv_ev_contact_online(char *contact, Resource *resource, GDateTime *last_activity, char *pgpkey);
void sv_ev_leave_room(const char *const room);
void sv_ev_room_destroy(const char *const room);
void sv_ev_room_occupant_offline(const char *const room, const char *const nick,
    const char *const show, const char *const status);
void sv_ev_room_destroyed(const char *const room, const char *const new_jid, const char *const password,
    const char *const reason);
void sv_ev_room_kicked(const char *const room, const char *const actor, const char *const reason);
void sv_ev_room_occupent_kicked(const char *const room, const char *const nick, const char *const actor,
    const char *const reason);
void sv_ev_room_banned(const char *const room, const char *const actor, const char *const reason);
void sv_ev_room_occupent_banned(const char *const room, const char *const nick, const char *const actor,
    const char *const reason);
void sv_ev_outgoing_carbon(char *barejid, char *message, char *pgp_message);
void sv_ev_incoming_carbon(char *barejid, char *resource, char *message, char *pgp_message);
void sv_ev_xmpp_stanza(const char *const msg);
void sv_ev_muc_self_online(const char *const room, const char *const nick, gboolean config_required,
    const char *const role, const char *const affiliation, const char *const actor, const char *const reason,
    const char *const jid, const char *const show, const char *const status);
void sv_ev_muc_occupant_online(const char *const room, const char *const nick, const char *const jid,
    const char *const role, const char *const affiliation, const char *const actor, const char *const reason,
    const char *const show_str, const char *const status_str);
void sv_ev_roster_update(const char *const barejid, const char *const name,
    GSList *groups, const char *const subscription, gboolean pending_out);
void sv_ev_roster_received(void);
int sv_ev_certfail(const char *const errormsg, TLSCertificate *cert);
void sv_ev_lastactivity_response(const char *const from, const int seconds, const char *const msg);
void sv_ev_bookmark_autojoin(Bookmark *bookmark);

#endif
