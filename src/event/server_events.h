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

void sv_ev_login_account_success(char* account_name, gboolean secured);
void sv_ev_lost_connection(void);
void sv_ev_failed_login(void);
void sv_ev_room_invite(jabber_invite_t invite_type,
                       const char* const invitor, const char* const room,
                       const char* const reason, const char* const password);
void sv_ev_room_broadcast(const char* const room_jid, const char* const message);
void sv_ev_room_subject(const char* const room, const char* const nick, const char* const subject);
void sv_ev_room_history(ProfMessage* message);
void sv_ev_room_message(ProfMessage* message);
void sv_ev_incoming_message(ProfMessage* message);
void sv_ev_incoming_private_message(ProfMessage* message);
void sv_ev_delayed_private_message(ProfMessage* message);
void sv_ev_typing(char* barejid, char* resource);
void sv_ev_paused(char* barejid, char* resource);
void sv_ev_inactive(char* barejid, char* resource);
void sv_ev_activity(const char* barejid, const char* resource, gboolean send_states);
void sv_ev_gone(const char* const barejid, const char* const resource);
void sv_ev_subscription(const char* from, jabber_subscr_t type);
void sv_ev_message_receipt(const char* const barejid, const char* const id);
void sv_ev_contact_offline(char* contact, char* resource, char* status);
void sv_ev_contact_online(char* contact, Resource* resource, GDateTime* last_activity, char* pgpkey);
void sv_ev_leave_room(const char* const room);
void sv_ev_room_destroy(const char* const room);
void sv_ev_room_occupant_offline(const char* const room, const char* const nick,
                                 const char* const show, const char* const status);
void sv_ev_room_destroyed(const char* const room, const char* const new_jid, const char* const password,
                          const char* const reason);
void sv_ev_room_kicked(const char* const room, const char* const actor, const char* const reason);
void sv_ev_room_occupent_kicked(const char* const room, const char* const nick, const char* const actor,
                                const char* const reason);
void sv_ev_room_banned(const char* const room, const char* const actor, const char* const reason);
void sv_ev_room_occupent_banned(const char* const room, const char* const nick, const char* const actor,
                                const char* const reason);
void sv_ev_outgoing_carbon(ProfMessage* message);
void sv_ev_incoming_carbon(ProfMessage* message);
void sv_ev_xmpp_stanza(const char* const msg);
void sv_ev_muc_self_online(const char* const room, const char* const nick, gboolean config_required,
                           const char* const role, const char* const affiliation, const char* const actor, const char* const reason,
                           const char* const jid, const char* const show, const char* const status);
void sv_ev_muc_occupant_online(const char* const room, const char* const nick, const char* const jid,
                               const char* const role, const char* const affiliation, const char* const actor, const char* const reason,
                               const char* const show_str, const char* const status_str);
void sv_ev_roster_update(const char* const barejid, const char* const name,
                         GSList* groups, const char* const subscription, gboolean pending_out);
void sv_ev_roster_received(void);
void sv_ev_connection_features_received(void);
int sv_ev_certfail(const char* const errormsg, const TLSCertificate* cert);
void sv_ev_lastactivity_response(const char* const from, const int seconds, const char* const msg);
void sv_ev_bookmark_autojoin(Bookmark* bookmark);

#endif
