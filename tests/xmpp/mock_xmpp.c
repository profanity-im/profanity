/*
 * mock_xmpp.c
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
static void _jabber_init(const int disable_tls) {}
void (*jabber_init)(const int disable_tls) = _jabber_init;

static jabber_conn_status_t _jabber_connect_with_details(const char * const jid,
    const char * const passwd, const char * const altdomain)
{
    return JABBER_DISCONNECTED;
}
jabber_conn_status_t (*jabber_connect_with_details)(const char * const,
    const char * const, const char * const) = _jabber_connect_with_details;

static jabber_conn_status_t _jabber_connect_with_account(const ProfAccount * const account)
{
    return JABBER_DISCONNECTED;
}
jabber_conn_status_t (*jabber_connect_with_account)(const ProfAccount * const) = _jabber_connect_with_account;

static void _jabber_disconnect(void) {}
void (*jabber_disconnect)(void) = _jabber_disconnect;

static void _jabber_shutdown(void) {}
void (*jabber_shutdown)(void) = _jabber_shutdown;

static void _jabber_process_events(void) {}
void (*jabber_process_events)(void) = _jabber_process_events;

static const char * _jabber_get_fulljid(void)
{
    return NULL;
}
const char * (*jabber_get_fulljid)(void) = _jabber_get_fulljid;

static const char * _jabber_get_domain(void)
{
    return NULL;
}
const char * (*jabber_get_domain)(void) = _jabber_get_domain;

static jabber_conn_status_t _jabber_get_connection_status(void)
{
    return JABBER_DISCONNECTED;
}
jabber_conn_status_t (*jabber_get_connection_status)(void) = _jabber_get_connection_status;

static char * _jabber_get_presence_message(void)
{
    return NULL;
}
char * (*jabber_get_presence_message)(void) = _jabber_get_presence_message;

static void _jabber_set_autoping(int seconds) {}
void (*jabber_set_autoping)(int) = _jabber_set_autoping;

static char * _jabber_get_account_name(void)
{
    return NULL;
}
char * (*jabber_get_account_name)(void) = _jabber_get_account_name;

static GList * _jabber_get_available_resources(void)
{
    return NULL;
}
GList * (*jabber_get_available_resources)(void) = _jabber_get_available_resources;

// message functions
static void _message_send(const char * const msg, const char * const recipient) {}
void (*message_send)(const char * const, const char * const) = _message_send;

static void _message_send_groupchat(const char * const msg, const char * const recipient) {}
void (*message_send_groupchat)(const char * const, const char * const) = _message_send_groupchat;

static void _message_send_inactive(const char * const recipient) {}
void (*message_send_inactive)(const char * const) = _message_send_inactive;

static void _message_send_composing(const char * const recipient) {}
void (*message_send_composing)(const char * const) = _message_send_composing;

static void _message_send_paused(const char * const recipient) {}
void (*message_send_paused)(const char * const) = _message_send_paused;

static void _message_send_gone(const char * const recipient) {}
void (*message_send_gone)(const char * const) = _message_send_gone;

static void _message_send_invite(const char * const room, const char * const contact,
    const char * const reason) {}
void (*message_send_invite)(const char * const, const char * const,
    const char * const) = _message_send_invite;

static void _message_send_duck(const char * const query) {}
void (*message_send_duck)(const char * const) = _message_send_duck;

// presence functions
static void _presence_subscription(const char * const jid, const jabber_subscr_t action) {}
void (*presence_subscription)(const char * const, const jabber_subscr_t) = _presence_subscription;

static GSList* _presence_get_subscription_requests(void)
{
    return NULL;
}
GSList* (*presence_get_subscription_requests)(void) = _presence_get_subscription_requests;

static gint _presence_sub_request_count(void)
{
    return 0;
}
gint (*presence_sub_request_count)(void) = _presence_sub_request_count;

static void _presence_reset_sub_request_search(void) {}
void (*presence_reset_sub_request_search)(void) = _presence_reset_sub_request_search;

static char * _presence_sub_request_find(char * search_str)
{
    return NULL;
}
char * (*presence_sub_request_find)(char *) = _presence_sub_request_find;

static void _presence_join_room(Jid *jid) {}
void (*presence_join_room)(Jid *) = _presence_join_room;

static void _presence_change_room_nick(const char * const room, const char * const nick) {}
void (*presence_change_room_nick)(const char * const, const char * const) = _presence_change_room_nick;

static void _presence_leave_chat_room(const char * const room_jid) {}
void (*presence_leave_chat_room)(const char * const) = _presence_leave_chat_room;

static void _presence_update(resource_presence_t status, const char * const msg,
    int idle) {}
void (*presence_update)(resource_presence_t, const char * const,
    int) = _presence_update;

static gboolean _presence_sub_request_exists(const char * const bare_jid)
{
    return FALSE;
}
gboolean (*presence_sub_request_exists)(const char * const bare_jid) = _presence_sub_request_exists;

// iq functions
static void _iq_send_software_version(const char * const fulljid) {}
void (*iq_send_software_version)(const char * const) = _iq_send_software_version;

static void _iq_room_list_request(gchar *conferencejid) {}
void (*iq_room_list_request)(gchar *) = _iq_room_list_request;

static void _iq_disco_info_request(gchar *jid) {}
void (*iq_disco_info_request)(gchar *) = _iq_disco_info_request;

static void _iq_disco_items_request(gchar *jid) {}
void (*iq_disco_items_request)(gchar *) = _iq_disco_items_request;

// caps functions
static Capabilities* _caps_get(const char * const caps_str)
{
    return NULL;
}
Capabilities* (*caps_get)(const char * const) = _caps_get;

static void _caps_close(void) {}
void (*caps_close)(void) = _caps_close;

static void _bookmark_add(const char *jid, const char *nick, gboolean autojoin) {}
void (*bookmark_add)(const char *, const char *, gboolean) = _bookmark_add;

static void _bookmark_remove(const char *jid, gboolean autojoin) {}
void (*bookmark_remove)(const char *, gboolean) = _bookmark_remove;

static const GList* _bookmark_get_list(void) 
{
    return NULL;
}
const GList* (*bookmark_get_list)(void) = _bookmark_get_list;

static char* _bookmark_find(char *search_str)
{
    return NULL;
}
char* (*bookmark_find)(char *) = _bookmark_find;

static void _bookmark_autocomplete_reset(void) {}
void (*bookmark_autocomplete_reset)(void) = _bookmark_autocomplete_reset;

static void _roster_send_name_change(const char * const barejid, const char * const new_name, GSList *groups) {}
void (*roster_send_name_change)(const char * const, const char * const, GSList *) = _roster_send_name_change;

static void _roster_send_add_to_group(const char * const group, PContact contact) {}
void (*roster_send_add_to_group)(const char * const, PContact) = _roster_send_add_to_group;

static void _roster_send_remove_from_group(const char * const group, PContact contact) {}
void (*roster_send_remove_from_group)(const char * const, PContact) = _roster_send_remove_from_group;

static void _roster_add_new(const char * const barejid, const char * const name) {}
void (*roster_add_new)(const char * const, const char * const) = _roster_add_new;

static void _roster_send_remove(const char * const barejid) {}
void (*roster_send_remove)(const char * const) = _roster_send_remove;
