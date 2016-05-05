/*
 * xmpp.h
 *
 * Copyright (C) 2012 - 2016 James Booth <boothj5@gmail.com>
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

#ifndef XMPP_XMPP_H
#define XMPP_XMPP_H

#include "config.h"

#ifdef HAVE_LIBMESODE
#include <mesode.h>
#endif
#ifdef HAVE_LIBSTROPHE
#include <strophe.h>
#endif

#include "config/accounts.h"
#include "config/tlscerts.h"
#include "contact.h"
#include "jid.h"
#include "tools/autocomplete.h"
#include "tools/http_upload.h"

#define JABBER_PRIORITY_MIN -128
#define JABBER_PRIORITY_MAX 127

#define XMPP_FEATURE_BLOCKING "urn:xmpp:blocking"

typedef enum {
    JABBER_UNDEFINED,
    JABBER_STARTED,
    JABBER_CONNECTING,
    JABBER_CONNECTED,
    JABBER_DISCONNECTING,
    JABBER_DISCONNECTED
} jabber_conn_status_t;

typedef enum {
    PRESENCE_SUBSCRIBE,
    PRESENCE_SUBSCRIBED,
    PRESENCE_UNSUBSCRIBED
} jabber_subscr_t;

typedef enum {
    INVITE_DIRECT,
    INVITE_MEDIATED
} jabber_invite_t;

typedef struct bookmark_t {
    char *jid;
    char *nick;
    char *password;
    gboolean autojoin;
} Bookmark;

typedef struct capabilities_t {
    char *category;
    char *type;
    char *name;
    char *software;
    char *software_version;
    char *os;
    char *os_version;
    GSList *features;
} Capabilities;

typedef struct disco_item_t {
    char *jid;
    char *name;
} DiscoItem;

typedef struct disco_identity_t {
    char *name;
    char *type;
    char *category;
} DiscoIdentity;

typedef struct disco_info_t {
    char *item;
    GHashTable *features;
} DiscoInfo;

void jabber_init(void);
jabber_conn_status_t jabber_connect_with_details(const char *const jid, const char *const passwd,
    const char *const altdomain, const int port, const char *const tls_policy);
jabber_conn_status_t jabber_connect_with_account(const ProfAccount *const account);
void jabber_disconnect(void);
void jabber_shutdown(void);
void jabber_process_events(int millis);
const char* jabber_get_fulljid(void);
jabber_conn_status_t connection_get_status(void);
char *connection_get_presence_msg(void);
char* jabber_get_account_name(void);
GList* jabber_get_available_resources(void);
char* jabber_create_uuid(void);
void jabber_free_uuid(char *uuid);
#ifdef HAVE_LIBMESODE
TLSCertificate* jabber_get_tls_peer_cert(void);
#endif
gboolean jabber_conn_is_secured(void);
gboolean jabber_send_stanza(const char *const stanza);
gboolean jabber_service_supports(const char *const feature);

char* message_send_chat(const char *const barejid, const char *const msg, const char *const oob_url);
char* message_send_chat_otr(const char *const barejid, const char *const msg);
char* message_send_chat_pgp(const char *const barejid, const char *const msg);
void message_send_private(const char *const fulljid, const char *const msg, const char *const oob_url);
void message_send_groupchat(const char *const roomjid, const char *const msg, const char *const oob_url);
void message_send_groupchat_subject(const char *const roomjid, const char *const subject);
void message_send_inactive(const char *const jid);
void message_send_composing(const char *const jid);
void message_send_paused(const char *const jid);
void message_send_gone(const char *const jid);
void message_send_invite(const char *const room, const char *const contact, const char *const reason);

void presence_subscription(const char *const jid, const jabber_subscr_t action);
GSList* presence_get_subscription_requests(void);
gint presence_sub_request_count(void);
void presence_reset_sub_request_search(void);
char* presence_sub_request_find(const char *const search_str);
void presence_join_room(const char *const room, const char *const nick, const char *const passwd);
void presence_change_room_nick(const char *const room, const char *const nick);
void presence_leave_chat_room(const char *const room_jid);
void presence_send(resource_presence_t status, const char *const msg, int idle, char *signed_status);
gboolean presence_sub_request_exists(const char *const bare_jid);

void iq_enable_carbons(void);
void iq_disable_carbons(void);
void iq_send_software_version(const char *const fulljid);
void iq_room_list_request(gchar *conferencejid);
void iq_disco_info_request(gchar *jid);
void iq_disco_items_request(gchar *jid);
void iq_last_activity_request(gchar *jid);
void iq_set_autoping(int seconds);
void iq_confirm_instant_room(const char *const room_jid);
void iq_destroy_room(const char *const room_jid);
void iq_request_room_config_form(const char *const room_jid);
void iq_submit_room_config(const char *const room, DataForm *form);
void iq_room_config_cancel(const char *const room_jid);
void iq_send_ping(const char *const target);
void iq_room_info_request(const char *const room, gboolean display_result);
void iq_room_affiliation_list(const char *const room, char *affiliation);
void iq_room_affiliation_set(const char *const room, const char *const jid, char *affiliation,
    const char *const reason);
void iq_room_kick_occupant(const char *const room, const char *const nick, const char *const reason);
void iq_room_role_set(const char *const room, const char *const nick, char *role, const char *const reason);
void iq_room_role_list(const char * const room, char *role);
void iq_autoping_check(void);
void iq_http_upload_request(HTTPUpload *upload);

Capabilities* caps_lookup(const char *const jid);
void caps_close(void);
void caps_destroy(Capabilities *caps);
void caps_reset_ver(void);

gboolean bookmark_add(const char *jid, const char *nick, const char *password, const char *autojoin_str);
gboolean bookmark_update(const char *jid, const char *nick, const char *password, const char *autojoin_str);
gboolean bookmark_remove(const char *jid);
gboolean bookmark_join(const char *jid);
const GList* bookmark_get_list(void);
char* bookmark_find(const char *const search_str);
void bookmark_autocomplete_reset(void);

void roster_send_name_change(const char *const barejid, const char *const new_name, GSList *groups);
void roster_send_add_to_group(const char *const group, PContact contact);
void roster_send_remove_from_group(const char *const group, PContact contact);
void roster_send_add_new(const char *const barejid, const char *const name);
void roster_send_remove(const char *const barejid);

GList* blocked_list(void);
gboolean blocked_add(char *jid);
gboolean blocked_remove(char *jid);
char* blocked_ac_find(const char *const search_str);
void blocked_ac_reset(void);

void form_destroy(DataForm *form);
void form_set_value(DataForm *form, const char *const tag, char *value);
gboolean form_add_unique_value(DataForm *form, const char *const tag, char *value);
void form_add_value(DataForm *form, const char *const tag, char *value);
gboolean form_remove_value(DataForm *form, const char *const tag, char *value);
gboolean form_remove_text_multi_value(DataForm *form, const char *const tag, int index);
gboolean form_tag_exists(DataForm *form, const char *const tag);
form_field_type_t form_get_field_type(DataForm *form, const char *const tag);
gboolean form_field_contains_option(DataForm *form, const char *const tag, char *value);
int form_get_value_count(DataForm *form, const char *const tag);
FormField* form_get_field_by_tag(DataForm *form, const char *const tag);
Autocomplete form_get_value_ac(DataForm *form, const char *const tag);
void form_reset_autocompleters(DataForm *form);

#endif
