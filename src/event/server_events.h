/*
 * server_events.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */

#ifndef EVENT_SERVER_EVENTS_H
#define EVENT_SERVER_EVENTS_H

#include "xmpp/xmpp.h"
#include "xmpp/message.h"

void sv_ev_login_account_success(const gchar* const account_name, gboolean secured);
void sv_ev_lost_connection(void);
void sv_ev_failed_login(void);
void sv_ev_room_invite(jabber_invite_t invite_type,
                       const gchar* const invitor, const gchar* const room,
                       const gchar* const reason, const gchar* const password);
void sv_ev_room_broadcast(const gchar* const room_jid, const gchar* const message);
void sv_ev_room_subject(const gchar* const room, const gchar* const nick, const gchar* const subject);
void sv_ev_room_history(ProfMessage* message);
void sv_ev_room_message(ProfMessage* message);
void sv_ev_incoming_message(ProfMessage* message);
void sv_ev_incoming_private_message(ProfMessage* message);
void sv_ev_delayed_private_message(ProfMessage* message);
void sv_ev_typing(const gchar* const barejid, const gchar* const resource);
void sv_ev_paused(const gchar* const barejid, const gchar* const resource);
void sv_ev_inactive(const gchar* const barejid, const gchar* const resource);
void sv_ev_activity(const gchar* const barejid, const gchar* const resource, gboolean send_states);
void sv_ev_gone(const gchar* const barejid, const gchar* const resource);
void sv_ev_subscription(const gchar* const from, jabber_subscr_t type);
void sv_ev_message_receipt(const gchar* const barejid, const gchar* const id);
void sv_ev_contact_offline(const gchar* const contact, const gchar* const resource, const gchar* const status);
void sv_ev_contact_online(const gchar* const contact, Resource* resource, GDateTime* last_activity, const gchar* const pgpkey);
void sv_ev_leave_room(const gchar* const room);
void sv_ev_room_destroy(const gchar* const room);
void sv_ev_room_occupant_offline(const gchar* const room, const gchar* const nick,
                                 const gchar* const show, const gchar* const status);
void sv_ev_room_destroyed(const gchar* const room, const gchar* const new_jid, const gchar* const password,
                          const gchar* const reason);
void sv_ev_room_kicked(const gchar* const room, const gchar* const actor, const gchar* const reason);
void sv_ev_room_occupent_kicked(const gchar* const room, const gchar* const nick, const gchar* const actor,
                                const gchar* const reason);
void sv_ev_room_banned(const gchar* const room, const gchar* const actor, const gchar* const reason);
void sv_ev_room_occupent_banned(const gchar* const room, const gchar* const nick, const gchar* const actor,
                                const gchar* const reason);
void sv_ev_outgoing_carbon(ProfMessage* message);
void sv_ev_incoming_carbon(ProfMessage* message);
void sv_ev_xmpp_stanza(const gchar* const msg);
void sv_ev_muc_self_online(const gchar* const room, const gchar* const nick, gboolean config_required,
                           const gchar* const role, const gchar* const affiliation, const gchar* const actor, const gchar* const reason,
                           const gchar* const jid, const gchar* const show, const gchar* const status);
void sv_ev_muc_occupant_online(const gchar* const room, const gchar* const nick, const gchar* const jid,
                               const gchar* const role, const gchar* const affiliation, const gchar* const actor, const gchar* const reason,
                               const gchar* const show_str, const gchar* const status_str);
void sv_ev_roster_update(const gchar* const barejid, const gchar* const name,
                         GSList* groups, const gchar* const subscription, gboolean pending_out);
void sv_ev_roster_received(void);
void sv_ev_connection_features_received(void);
int sv_ev_certfail(const gchar* const errormsg, const TLSCertificate* cert);
void sv_ev_lastactivity_response(const gchar* const from, const int seconds, const gchar* const msg);
void sv_ev_bookmark_autojoin(Bookmark* bookmark);
void autojoin_queue_clear(void);

#endif
