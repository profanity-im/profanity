/*
 * xmpp.h
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2025 Michael Vetter <jubalh@iodoru.org>
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

#ifndef XMPP_XMPP_H
#define XMPP_XMPP_H

#include <stdint.h>

#include "config.h"

#include <strophe.h>

#include "config/accounts.h"
#include "config/tlscerts.h"
#include "tools/autocomplete.h"
#include "tools/http_upload.h"
#include "xmpp/contact.h"
#include "xmpp/jid.h"

#define JABBER_PRIORITY_MIN -128
#define JABBER_PRIORITY_MAX 127

#define XMPP_FEATURE_PING                        "urn:xmpp:ping"
#define XMPP_FEATURE_BLOCKING                    "urn:xmpp:blocking"
#define XMPP_FEATURE_RECEIPTS                    "urn:xmpp:receipts"
#define XMPP_FEATURE_LASTACTIVITY                "jabber:iq:last"
#define XMPP_FEATURE_MUC                         "http://jabber.org/protocol/muc"
#define XMPP_FEATURE_COMMANDS                    "http://jabber.org/protocol/commands"
#define XMPP_FEATURE_OMEMO_DEVICELIST_NOTIFY     "eu.siacs.conversations.axolotl.devicelist+notify"
#define XMPP_FEATURE_PUBSUB                      "http://jabber.org/protocol/pubsub"
#define XMPP_FEATURE_PUBSUB_PUBLISH_OPTIONS      "http://jabber.org/protocol/pubsub#publish-options"
#define XMPP_FEATURE_USER_AVATAR_METADATA_NOTIFY "urn:xmpp:avatar:metadata+notify"
#define XMPP_FEATURE_LAST_MESSAGE_CORRECTION     "urn:xmpp:message-correct:0"
#define XMPP_FEATURE_MAM2                        "urn:xmpp:mam:2"
#define XMPP_FEATURE_MAM2_EXTENDED               "urn:xmpp:mam:2#extended"
#define XMPP_FEATURE_SPAM_REPORTING              "urn:xmpp:reporting:1"

typedef enum {
    JABBER_CONNECTING,
    JABBER_CONNECTED,
    JABBER_DISCONNECTING,
    JABBER_DISCONNECTED,
    JABBER_RAW_CONNECTING,
    JABBER_RAW_CONNECTED,
    JABBER_RECONNECT
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

typedef enum {
    BLOCKED_NO_REPORT,
    BLOCKED_REPORT_ABUSE,
    BLOCKED_REPORT_SPAM
} blocked_report;

typedef struct bookmark_t
{
    char* barejid;
    char* nick;
    char* password;
    char* name;
    gboolean autojoin;
    int ext_gajim_minimize; // 0 - non existent, 1 - true, 2 - false
} Bookmark;

typedef struct disco_identity_t
{
    char* name;
    char* type;
    char* category;
} DiscoIdentity;

typedef struct software_version_t
{
    char* software;
    char* software_version;
    char* os;
    char* os_version;
} SoftwareVersion;

typedef struct entity_capabilities_t
{
    DiscoIdentity* identity;
    SoftwareVersion* software_version;
    GSList* features;
} EntityCapabilities;

typedef struct disco_item_t
{
    char* jid;
    char* name;
} DiscoItem;

typedef enum {
    PROF_MSG_ENC_NONE,
    PROF_MSG_ENC_OTR,
    PROF_MSG_ENC_PGP,
    PROF_MSG_ENC_OMEMO,
    PROF_MSG_ENC_OX
} prof_enc_t;

typedef enum {
    PROF_MSG_TYPE_UNINITIALIZED,
    // regular 1:1 chat
    PROF_MSG_TYPE_CHAT,
    // groupchats to whole group
    PROF_MSG_TYPE_MUC,
    // groupchat private message
    PROF_MSG_TYPE_MUCPM
} prof_msg_type_t;

typedef struct prof_message_t
{
    Jid* from_jid;
    Jid* to_jid;
    /* regular <message id=""> */
    char* id;
    /* </origin-id> XEP-0359 */
    char* originid;
    /* <replace id> XEP-0308 LMC */
    char* replace_id;
    /* stanza-id from XEP 0359. Used for MAM. archive_id in our database (see database.c)
     * coming in as <stanza-id> for live messages
     * coming in as <result id=""> for MAM messages*/
    char* stanzaid;
    /* The raw body from xmpp message, either plaintext or OTR encrypted text */
    char* body;
    /* The encrypted message as for PGP */
    char* encrypted;
    /* The message that will be printed on screen and logs */
    char* plain;
    GDateTime* timestamp;
    prof_enc_t enc;
    gboolean trusted;
    gboolean is_mam;
    prof_msg_type_t type;
} ProfMessage;

void session_init(void);
jabber_conn_status_t session_connect_with_details(const char* const jid, const char* const passwd,
                                                  const char* const altdomain, const int port, const char* const tls_policy, const char* const auth_policy);
jabber_conn_status_t session_connect_with_account(const ProfAccount* const account);

void session_disconnect(void);
void session_process_events(void);
const char* session_get_account_name(void);
void session_reconnect_now(void);

void connection_disconnect(void);
jabber_conn_status_t connection_get_status(void);
const char* connection_get_presence_msg(void);
void connection_set_presence_msg(const char* const message);
const char* connection_get_fulljid(void);
const Jid* connection_get_jid(void);
const char* connection_get_barejid(void);
gboolean equals_our_barejid(const char* cmp);
const char* connection_get_user(void);
char* connection_create_uuid(void);
void connection_free_uuid(char* uuid);
TLSCertificate* connection_get_tls_peer_cert(void);
gboolean connection_is_secured(void);
gboolean connection_send_stanza(const char* const stanza);
GList* connection_get_available_resources(void);
int connection_count_available_resources(void);
gboolean connection_supports(const char* const feature);
const char* connection_jid_for_feature(const char* const feature);

const char* connection_get_profanity_identifier(void);

char* message_send_chat(const char* const barejid, const char* const msg, const char* const oob_url, gboolean request_receipt, const char* const replace_id);
char* message_send_chat_otr(const char* const barejid, const char* const msg, gboolean request_receipt, const char* const replace_id);
char* message_send_chat_pgp(const char* const barejid, const char* const msg, gboolean request_receipt, const char* const replace_id);
// XEP-0373: OpenPGP for XMPP
char* message_send_chat_ox(const char* const barejid, const char* const msg, gboolean request_receipt, const char* const replace_id);
char* message_send_chat_omemo(const char* const jid, uint32_t sid, GList* keys, const unsigned char* const iv, size_t iv_len, const unsigned char* const ciphertext, size_t ciphertext_len, gboolean request_receipt, gboolean muc, const char* const replace_id);
char* message_send_private(const char* const fulljid, const char* const msg, const char* const oob_url);
char* message_send_groupchat(const char* const roomjid, const char* const msg, const char* const oob_url, const char* const replace_id);
void message_send_groupchat_subject(const char* const roomjid, const char* const subject);
void message_send_inactive(const char* const jid);
void message_send_composing(const char* const jid);
void message_send_paused(const char* const jid);
void message_send_gone(const char* const jid);
void message_send_invite(const char* const room, const char* const contact, const char* const reason);
void message_request_voice(const char* const roomjid);

bool message_is_sent_by_us(const ProfMessage* const message, bool checkOID);

void presence_subscription(const char* const jid, const jabber_subscr_t action);
GList* presence_get_subscription_requests(void);
gint presence_sub_request_count(void);
void presence_reset_sub_request_search(void);
char* presence_sub_request_find(const char* const search_str, gboolean previous, void* context);
void presence_join_room(const char* const room, const char* const nick, const char* const passwd);
void presence_change_room_nick(const char* const room, const char* const nick);
void presence_leave_chat_room(const char* const room_jid);
void presence_send(resource_presence_t status, int idle, char* signed_status);
gboolean presence_sub_request_exists(const char* const bare_jid);

void iq_enable_carbons(void);
void iq_disable_carbons(void);
void iq_send_software_version(const char* const fulljid);
void iq_rooms_cache_clear(void);
void iq_handlers_remove_win(ProfWin* window);
void iq_handlers_clear(void);
void iq_room_list_request(const char* conferencejid, char* filter);
void iq_disco_info_request(const char* jid);
void iq_disco_items_request(const char* jid);
void iq_last_activity_request(const char* jid);
void iq_set_autoping(int seconds);
void iq_confirm_instant_room(const char* const room_jid);
void iq_destroy_room(const char* const room_jid);
void iq_request_room_config_form(const char* const room_jid);
void iq_submit_room_config(ProfConfWin* confwin);
void iq_room_config_cancel(ProfConfWin* confwin);
void iq_send_ping(const char* const target);
void iq_room_info_request(const char* const room, gboolean display_result);
void iq_room_affiliation_list(const char* const room, char* affiliation, bool show);
void iq_room_affiliation_set(const char* const room, const char* const jid, char* affiliation,
                             const char* const reason);
void iq_room_kick_occupant(const char* const room, const char* const nick, const char* const reason);
void iq_room_role_set(const char* const room, const char* const nick, char* role, const char* const reason);
void iq_room_role_list(const char* const room, char* role);
void iq_autoping_timer_cancel(void);
void iq_autoping_check(void);
void iq_http_upload_request(HTTPUpload* upload);
void iq_command_list(const char* const target);
void iq_command_exec(const char* const target, const char* const command);
void iq_mam_request(ProfChatWin* win, GDateTime* enddate);
void iq_mam_request_older(ProfChatWin* win);
void iq_register_change_password(const char* const user, const char* const password);
void iq_muc_register_nick(const char* const roomjid);

void autoping_timer_extend(void);

EntityCapabilities* caps_lookup(const char* const jid);
void caps_destroy(EntityCapabilities* caps);
void caps_reset_ver(void);
void caps_add_feature(char* feature);
void caps_remove_feature(char* feature);
gboolean caps_jid_has_feature(const char* const jid, const char* const feature);

gboolean bookmark_add(const char* jid, const char* nick, const char* password, const char* autojoin_str, const char* name);
gboolean bookmark_update(const char* jid, const char* nick, const char* password, const char* autojoin_str, const char* name);
gboolean bookmark_remove(const char* jid);
gboolean bookmark_join(const char* jid);
GList* bookmark_get_list(void);
Bookmark* bookmark_get_by_jid(const char* jid);
char* bookmark_find(const char* const search_str, gboolean previous, void* context);
void bookmark_autocomplete_reset(void);
gboolean bookmark_exists(const char* const room);

void roster_send_name_change(const char* const barejid, const char* const new_name, GSList* groups);
void roster_send_add_to_group(const char* const group, PContact contact);
void roster_send_remove_from_group(const char* const group, PContact contact);
void roster_send_add_new(const char* const barejid, const char* const name);
void roster_send_remove(const char* const barejid);

GList* blocked_list(void);
gboolean blocked_add(char* jid, blocked_report reportkind, const char* const message);
gboolean blocked_remove(char* jid);
char* blocked_ac_find(const char* const search_str, gboolean previous, void* context);
void blocked_ac_reset(void);

void form_destroy(DataForm* form);
void form_set_value(DataForm* form, const char* const tag, char* value);
gboolean form_add_unique_value(DataForm* form, const char* const tag, char* value);
void form_add_value(DataForm* form, const char* const tag, char* value);
gboolean form_remove_value(DataForm* form, const char* const tag, char* value);
gboolean form_remove_text_multi_value(DataForm* form, const char* const tag, int index);
gboolean form_tag_exists(DataForm* form, const char* const tag);
form_field_type_t form_get_field_type(DataForm* form, const char* const tag);
gboolean form_field_contains_option(DataForm* form, const char* const tag, char* value);
int form_get_value_count(DataForm* form, const char* const tag);
FormField* form_get_field_by_tag(DataForm* form, const char* const tag);
Autocomplete form_get_value_ac(DataForm* form, const char* const tag);
void form_reset_autocompleters(DataForm* form);

void publish_user_mood(const char* const mood, const char* const text);

gchar* get_display_name(const ProfMessage* const message, int* flags);
#endif
