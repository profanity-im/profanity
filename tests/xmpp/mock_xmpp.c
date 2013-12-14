/*
 * xmpp.h
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

#include <glib.h>
#include <setjmp.h>
#include <cmocka.h>

#include "xmpp/xmpp.h"

// connection functions
void jabber_init(const int disable_tls) {}

jabber_conn_status_t jabber_connect_with_details(const char * const jid,
    const char * const passwd, const char * const altdomain)
{
    return (jabber_conn_status_t)mock();
}

jabber_conn_status_t jabber_connect_with_account(const ProfAccount * const account)
{
    return (jabber_conn_status_t)mock();
}

void jabber_disconnect(void) {}
void jabber_shutdown(void) {}
void jabber_process_events(void) {}
const char * jabber_get_fulljid(void)
{
    return (const char *)mock();
}
const char * jabber_get_domain(void)
{
    return (const char *)mock();
}

jabber_conn_status_t jabber_get_connection_status(void)
{
    return (jabber_conn_status_t)mock();
}

char * jabber_get_presence_message(void)
{
    return (char *)mock();
}
void jabber_set_autoping(int seconds) {}

char* jabber_get_account_name(void)
{
    return (char *)mock();
}

GList * jabber_get_available_resources(void)
{
    return (GList *)mock();
}

// message functions
void message_send(const char * const msg, const char * const recipient) {}
void message_send_groupchat(const char * const msg, const char * const recipient) {}
void message_send_inactive(const char * const recipient) {}
void message_send_composing(const char * const recipient) {}
void message_send_paused(const char * const recipient) {}
void message_send_gone(const char * const recipient) {}
void message_send_invite(const char * const room, const char * const contact,
    const char * const reason) {}
void message_send_duck(const char * const query) {}

// presence functions
void presence_subscription(const char * const jid, const jabber_subscr_t action) {}

GSList* presence_get_subscription_requests(void)
{
    return (GSList *)mock();
}

gint presence_sub_request_count(void)
{
    return (gint)mock();
}

void presence_reset_sub_request_search(void) {}

char * presence_sub_request_find(char * search_str)
{
    return (char *)mock();
}

void presence_join_room(Jid *jid) {}
void presence_change_room_nick(const char * const room, const char * const nick) {}
void presence_leave_chat_room(const char * const room_jid) {}
void presence_update(resource_presence_t status, const char * const msg,
    int idle) {}
gboolean presence_sub_request_exists(const char * const bare_jid)
{
    return (gboolean)mock();
}

// iq functions
void iq_send_software_version(const char * const fulljid) {}
void iq_room_list_request(gchar *conferencejid) {}
void iq_disco_info_request(gchar *jid) {}
void iq_disco_items_request(gchar *jid) {}

// caps functions
Capabilities* caps_get(const char * const caps_str)
{
    return (Capabilities *)mock();
}

void caps_close(void) {}

void bookmark_add(const char *jid, const char *nick, gboolean autojoin) {}
void bookmark_remove(const char *jid, gboolean autojoin) {}

const GList *bookmark_get_list(void)
{
    return (const GList *)mock();
}

char *bookmark_find(char *search_str)
{
    return (char *)mock();
}

void bookmark_autocomplete_reset(void) {}

void roster_send_name_change(const char * const barejid, const char * const new_name, GSList *groups) {}
void roster_send_add_to_group(const char * const group, PContact contact) {}
void roster_send_remove_from_group(const char * const group, PContact contact) {}
void roster_add_new(const char * const barejid, const char * const name) {}
void roster_send_remove(const char * const barejid) {}
