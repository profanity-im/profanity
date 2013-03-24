/*
 * profanity.h
 *
 * Copyright (C) 2012, 2013 James Booth <boothj5@gmail.com>
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
 */

#ifndef PROFANITY_H
#define PROFANITY_H

#include "resource.h"
#include "xmpp/xmpp.h"

void prof_run(const int disable_tls, char *log_level);

void prof_handle_login_success(const char *jid, const char *altdomain);
void prof_handle_login_account_success(char *account_name);
void prof_handle_lost_connection(void);
void prof_handle_disconnect(const char * const jid);
void prof_handle_failed_login(void);
void prof_handle_typing(char *from);
void prof_handle_contact_online(char *contact, Resource *resource,
    GDateTime *last_activity);
void prof_handle_contact_offline(char *contact, char *show, char *status);
void prof_handle_incoming_message(char *from, char *message, gboolean priv);
void prof_handle_delayed_message(char *from, char *message, GTimeVal tv_stamp,
    gboolean priv);
void prof_handle_error_message(const char *from, const char *err_msg);
void prof_handle_subscription(const char *from, jabber_subscr_t type);
void prof_handle_roster(GSList *roster);
void prof_handle_gone(const char * const from);
void prof_handle_room_history(const char * const room_jid,
    const char * const nick, GTimeVal tv_stamp, const char * const message);
void prof_handle_room_message(const char * const room_jid, const char * const nick,
    const char * const message);
void prof_handle_room_subject(const char * const room_jid,
    const char * const subject);
void prof_handle_room_roster_complete(const char * const room);
void prof_handle_room_member_online(const char * const room,
    const char * const nick, const char * const show, const char * const status,
    const char * const caps_str);
void prof_handle_room_member_offline(const char * const room,
    const char * const nick, const char * const show, const char * const status);
void prof_handle_room_member_presence(const char * const room,
    const char * const nick, const char * const show,
    const char * const status, const char * const caps_str);
void prof_handle_leave_room(const char * const room);
void prof_handle_room_member_nick_change(const char * const room,
    const char * const old_nick, const char * const nick);
void prof_handle_room_nick_change(const char * const room,
    const char * const nick);
void prof_handle_room_broadcast(const char *const room_jid,
    const char * const message);
void prof_handle_room_invite(jabber_invite_t invite_type,
    const char * const invitor, const char * const room,
    const char * const reason);
void prof_handle_idle(void);
void prof_handle_activity(void);
void prof_handle_version_result(const char * const jid,
    const char * const presence, const char * const name,
    const char * const version, const char * const os);
void prof_handle_room_list(GSList *rooms, const char *conference_node);
void prof_handle_disco_items(GSList *items, const char *jid);
void prof_handle_disco_info(const char *from, GSList *identities,
    GSList *features);

#endif
