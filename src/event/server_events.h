/*
 * server_events.h
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

#ifndef SERVER_EVENTS_H
#define SERVER_EVENTS_H

#include "xmpp/xmpp.h"

void srv_login_account_success(char *account_name);
void srv_lost_connection(void);
void srv_failed_login(void);
void srv_software_version_result(const char * const jid, const char * const  presence,
    const char * const name, const char * const version, const char * const os);
void srv_disco_info(const char *from, GSList *identities, GSList *features);
void srv_disco_info_error(const char * const from, const char * const error);
void srv_room_list(GSList *rooms, const char *conference_node);
void srv_disco_items(GSList *items, const char *jid);
void srv_room_invite(jabber_invite_t invite_type,
    const char * const invitor, const char * const room,
    const char * const reason, const char * const password);
void srv_room_broadcast(const char *const room_jid,
    const char * const message);
void srv_room_subject(const char * const room, const char * const nick, const char * const subject);
void srv_room_history(const char * const room_jid, const char * const nick,
    GTimeVal tv_stamp, const char * const message);
void srv_room_message(const char * const room_jid, const char * const nick,
    const char * const message);
void srv_room_join_error(const char * const room, const char * const err);
void srv_room_info_error(const char * const room, const char * const error);
void srv_room_disco_info(const char * const room, GSList *identities, GSList *features, gboolean display);
void srv_room_affiliation_list_result_error(const char * const room, const char * const affiliation,
    const char * const error);
void srv_room_affiliation_list(const char * const room, const char * const affiliation, GSList *jids);
void srv_room_affiliation_set_error(const char * const room, const char * const jid, const char * const affiliation,
    const char * const error);
void srv_room_role_list_result_error(const char * const from, const char * const role, const char * const error);
void srv_room_role_list(const char * const from, const char * const role, GSList *nicks);
void srv_room_role_set_error(const char * const room, const char * const nick, const char * const role,
    const char * const error);
void srv_room_kick_result_error(const char * const room, const char * const nick, const char * const error);
void srv_incoming_message(char *barejid, char *resource, char *message);
void srv_incoming_private_message(char *fulljid, char *message);
void srv_delayed_message(char *fulljid, char *message, GTimeVal tv_stamp);
void srv_delayed_private_message(char *fulljid, char *message, GTimeVal tv_stamp);
void srv_typing(char *barejid, char *resource);
void srv_paused(char *barejid, char *resource);
void srv_inactive(char *barejid, char *resource);
void srv_activity(char *barejid, char *resource, gboolean send_states);
void srv_gone(const char * const barejid, const char * const resource);
void srv_subscription(const char *from, jabber_subscr_t type);
void srv_message_receipt(char *barejid, char *id);
void srv_contact_offline(char *contact, char *resource, char *status);
void srv_contact_online(char *contact, Resource *resource,
    GDateTime *last_activity);
void srv_leave_room(const char * const room);
void srv_room_destroy(const char * const room);
void srv_room_occupant_offline(const char * const room, const char * const nick,
    const char * const show, const char * const status);
void srv_room_destroyed(const char * const room, const char * const new_jid, const char * const password,
    const char * const reason);
void srv_room_kicked(const char * const room, const char * const actor, const char * const reason);
void srv_room_occupent_kicked(const char * const room, const char * const nick, const char * const actor,
    const char * const reason);
void srv_room_banned(const char * const room, const char * const actor, const char * const reason);
void srv_room_occupent_banned(const char * const room, const char * const nick, const char * const actor,
    const char * const reason);
void srv_group_add(const char * const contact,
    const char * const group);
void srv_group_remove(const char * const contact,
    const char * const group);
void srv_roster_remove(const char * const barejid);
void srv_roster_add(const char * const barejid, const char * const name);
void srv_autoping_cancel(void);
void srv_carbon(char *barejid, char *message);
void srv_message_error(const char * const from, const char * const type,
    const char * const err_msg);
void srv_presence_error(const char *from, const char * const type,
    const char *err_msg);
void srv_xmpp_stanza(const char * const msg);
void srv_ping_result(const char * const from, int millis);
void srv_ping_error_result(const char * const from, const char * const error);
void srv_room_configure(const char * const room, DataForm *form);
void srv_room_configuration_form_error(const char * const from, const char * const message);
void srv_room_config_submit_result(const char * const room);
void srv_room_config_submit_result_error(const char * const room, const char * const message);
void srv_muc_self_online(const char * const room, const char * const nick, gboolean config_required,
    const char * const role, const char * const affiliation, const char * const actor, const char * const reason,
    const char * const jid, const char * const show, const char * const status);
void srv_muc_occupant_online(const char * const room, const char * const nick, const char * const jid,
    const char * const role, const char * const affiliation, const char * const actor, const char * const reason,
    const char * const show_str, const char * const status_str);
void srv_roster_update(const char * const barejid, const char * const name,
    GSList *groups, const char * const subscription, gboolean pending_out);
void srv_roster_received(void);
void srv_enable_carbons_error(const char * const error);
void srv_disable_carbons_error(const char * const error);

#endif
