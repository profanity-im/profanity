/*
 * stanza.h
 *
 * Copyright (C) 2012 - 20172017 James Booth <boothj5@gmail.com>
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

#ifndef XMPP_STANZA_H
#define XMPP_STANZA_H

#include "config.h"

#ifdef HAVE_LIBMESODE
#include <mesode.h>
#endif

#ifdef HAVE_LIBSTROPHE
#include <strophe.h>
#endif

#include <xmpp/xmpp.h>

#define STANZA_NAME_ACTIVE "active"
#define STANZA_NAME_INACTIVE "inactive"
#define STANZA_NAME_COMPOSING "composing"
#define STANZA_NAME_PAUSED "paused"
#define STANZA_NAME_GONE "gone"

#define STANZA_NAME_MESSAGE "message"
#define STANZA_NAME_BODY "body"
#define STANZA_NAME_BLOCK "block"
#define STANZA_NAME_UNBLOCK "unblock"
#define STANZA_NAME_BLOCKLIST "blocklist"
#define STANZA_NAME_PRESENCE "presence"
#define STANZA_NAME_PRIORITY "priority"
#define STANZA_NAME_X "x"
#define STANZA_NAME_SHOW "show"
#define STANZA_NAME_STATUS "status"
#define STANZA_NAME_IQ "iq"
#define STANZA_NAME_QUERY "query"
#define STANZA_NAME_REQUEST "request"
#define STANZA_NAME_DELAY "delay"
#define STANZA_NAME_ERROR "error"
#define STANZA_NAME_PING "ping"
#define STANZA_NAME_TEXT "text"
#define STANZA_NAME_SUBJECT "subject"
#define STANZA_NAME_ITEM "item"
#define STANZA_NAME_ITEMS "items"
#define STANZA_NAME_C "c"
#define STANZA_NAME_IDENTITY "identity"
#define STANZA_NAME_FEATURE "feature"
#define STANZA_NAME_INVITE "invite"
#define STANZA_NAME_REASON "reason"
#define STANZA_NAME_GROUP "group"
#define STANZA_NAME_PUBSUB "pubsub"
#define STANZA_NAME_PUBLISH "publish"
#define STANZA_NAME_PUBLISH_OPTIONS "publish-options"
#define STANZA_NAME_FIELD "field"
#define STANZA_NAME_STORAGE "storage"
#define STANZA_NAME_NICK "nick"
#define STANZA_NAME_PASSWORD "password"
#define STANZA_NAME_CONFERENCE "conference"
#define STANZA_NAME_VALUE "value"
#define STANZA_NAME_DESTROY "destroy"
#define STANZA_NAME_ACTOR "actor"
#define STANZA_NAME_ENABLE "enable"
#define STANZA_NAME_DISABLE "disable"
#define STANZA_NAME_FILENAME "filename"
#define STANZA_NAME_SIZE "size"
#define STANZA_NAME_CONTENT_TYPE "content-type"
#define STANZA_NAME_SLOT "slot"
#define STANZA_NAME_PUT "put"
#define STANZA_NAME_GET "get"
#define STANZA_NAME_URL "url"

// error conditions
#define STANZA_NAME_BAD_REQUEST "bad-request"
#define STANZA_NAME_CONFLICT "conflict"
#define STANZA_NAME_FEATURE_NOT_IMPLEMENTED "feature-not-implemented"
#define STANZA_NAME_FORBIDDEN "forbidden"
#define STANZA_NAME_GONE "gone"
#define STANZA_NAME_INTERNAL_SERVER_ERROR "internal-server-error"
#define STANZA_NAME_ITEM_NOT_FOUND "item-not-found"
#define STANZA_NAME_JID_MALFORMED "jid-malformed"
#define STANZA_NAME_NOT_ACCEPTABLE "not-acceptable"
#define STANZA_NAME_NOT_ALLOWED "not-allowed"
#define STANZA_NAME_NOT_AUTHORISED "not-authorised"
#define STANZA_NAME_POLICY_VIOLATION "policy-violation"
#define STANZA_NAME_RECIPIENT_UNAVAILABLE "recipient-unavailable"
#define STANZA_NAME_REDIRECT "redirect"
#define STANZA_NAME_REGISTRATION_REQUIRED "registration-required"
#define STANZA_NAME_REMOTE_SERVER_NOT_FOUND "remote-server-not-found"
#define STANZA_NAME_REMOTE_SERVER_TIMEOUT "remote-server-timeout"
#define STANZA_NAME_RESOURCE_CONSTRAINT "resource-constraint"
#define STANZA_NAME_SERVICE_UNAVAILABLE "service-unavailable"
#define STANZA_NAME_SUBSCRIPTION_REQUIRED "subscription-required"
#define STANZA_NAME_UNDEFINED_CONDITION "undefined-condition"
#define STANZA_NAME_UNEXPECTED_REQUEST "unexpected-request"

#define STANZA_TYPE_CHAT "chat"
#define STANZA_TYPE_GROUPCHAT "groupchat"
#define STANZA_TYPE_UNAVAILABLE "unavailable"
#define STANZA_TYPE_SUBSCRIBE "subscribe"
#define STANZA_TYPE_SUBSCRIBED "subscribed"
#define STANZA_TYPE_UNSUBSCRIBED "unsubscribed"
#define STANZA_TYPE_GET "get"
#define STANZA_TYPE_SET "set"
#define STANZA_TYPE_ERROR "error"
#define STANZA_TYPE_RESULT "result"

#define STANZA_ATTR_TO "to"
#define STANZA_ATTR_FROM "from"
#define STANZA_ATTR_STAMP "stamp"
#define STANZA_ATTR_TYPE "type"
#define STANZA_ATTR_CODE "code"
#define STANZA_ATTR_JID "jid"
#define STANZA_ATTR_NAME "name"
#define STANZA_ATTR_SUBSCRIPTION "subscription"
#define STANZA_ATTR_XMLNS "xmlns"
#define STANZA_ATTR_NICK "nick"
#define STANZA_ATTR_ASK "ask"
#define STANZA_ATTR_ID "id"
#define STANZA_ATTR_SECONDS "seconds"
#define STANZA_ATTR_NODE "node"
#define STANZA_ATTR_VER "ver"
#define STANZA_ATTR_VAR "var"
#define STANZA_ATTR_HASH "hash"
#define STANZA_ATTR_CATEGORY "category"
#define STANZA_ATTR_REASON "reason"
#define STANZA_ATTR_AUTOJOIN "autojoin"
#define STANZA_ATTR_PASSWORD "password"

#define STANZA_TEXT_AWAY "away"
#define STANZA_TEXT_DND "dnd"
#define STANZA_TEXT_CHAT "chat"
#define STANZA_TEXT_XA "xa"
#define STANZA_TEXT_ONLINE "online"

#define STANZA_NS_STANZAS "urn:ietf:params:xml:ns:xmpp-stanzas"
#define STANZA_NS_CHATSTATES "http://jabber.org/protocol/chatstates"
#define STANZA_NS_MUC "http://jabber.org/protocol/muc"
#define STANZA_NS_MUC_USER "http://jabber.org/protocol/muc#user"
#define STANZA_NS_MUC_OWNER "http://jabber.org/protocol/muc#owner"
#define STANZA_NS_MUC_ADMIN "http://jabber.org/protocol/muc#admin"
#define STANZA_NS_CAPS "http://jabber.org/protocol/caps"
#define STANZA_NS_PING "urn:xmpp:ping"
#define STANZA_NS_LASTACTIVITY "jabber:iq:last"
#define STANZA_NS_DATA "jabber:x:data"
#define STANZA_NS_VERSION "jabber:iq:version"
#define STANZA_NS_CONFERENCE "jabber:x:conference"
#define STANZA_NS_CAPTCHA "urn:xmpp:captcha"
#define STANZA_NS_PUBSUB "http://jabber.org/protocol/pubsub"
#define STANZA_NS_CARBONS "urn:xmpp:carbons:2"
#define STANZA_NS_HINTS "urn:xmpp:hints"
#define STANZA_NS_FORWARD "urn:xmpp:forward:0"
#define STANZA_NS_RECEIPTS "urn:xmpp:receipts"
#define STANZA_NS_SIGNED "jabber:x:signed"
#define STANZA_NS_ENCRYPTED "jabber:x:encrypted"
#define STANZA_NS_HTTP_UPLOAD "urn:xmpp:http:upload"
#define STANZA_NS_X_OOB "jabber:x:oob"
#define STANZA_NS_BLOCKING "urn:xmpp:blocking"

#define STANZA_DATAFORM_SOFTWARE "urn:xmpp:dataforms:softwareinfo"

typedef struct caps_stanza_t {
    char *hash;
    char *node;
    char *ver;
} XMPPCaps;

typedef struct presence_stanza_t {
    Jid *jid;
    char *show;
    char *status;
    int priority;
    GDateTime *last_activity;
} XMPPPresence;

typedef enum {
    STANZA_PARSE_ERROR_NO_FROM,
    STANZA_PARSE_ERROR_INVALID_FROM
} stanza_parse_error_t;

xmpp_stanza_t* stanza_create_bookmarks_storage_request(xmpp_ctx_t *ctx);

xmpp_stanza_t* stanza_create_blocked_list_request(xmpp_ctx_t *ctx);

xmpp_stanza_t* stanza_create_http_upload_request(xmpp_ctx_t *ctx, const char *const id, const char *const jid, HTTPUpload *upload);

xmpp_stanza_t* stanza_enable_carbons(xmpp_ctx_t *ctx);

xmpp_stanza_t* stanza_disable_carbons(xmpp_ctx_t *ctx);

xmpp_stanza_t* stanza_create_chat_state(xmpp_ctx_t *ctx,
    const char *const fulljid, const char *const state);

xmpp_stanza_t* stanza_attach_state(xmpp_ctx_t *ctx, xmpp_stanza_t *stanza, const char *const state);
xmpp_stanza_t* stanza_attach_carbons_private(xmpp_ctx_t *ctx, xmpp_stanza_t *stanza);
xmpp_stanza_t* stanza_attach_hints_no_copy(xmpp_ctx_t *ctx, xmpp_stanza_t *stanza);
xmpp_stanza_t* stanza_attach_hints_no_store(xmpp_ctx_t *ctx, xmpp_stanza_t *stanza);
xmpp_stanza_t* stanza_attach_receipt_request(xmpp_ctx_t *ctx, xmpp_stanza_t *stanza);
xmpp_stanza_t* stanza_attach_x_oob_url(xmpp_ctx_t *ctx, xmpp_stanza_t *stanza, const char *const url);

xmpp_stanza_t* stanza_create_room_join_presence(xmpp_ctx_t *const ctx,
    const char *const full_room_jid, const char *const passwd);

xmpp_stanza_t* stanza_create_room_newnick_presence(xmpp_ctx_t *ctx,
    const char *const full_room_jid);

xmpp_stanza_t* stanza_create_room_leave_presence(xmpp_ctx_t *ctx,
    const char *const room, const char *const nick);

xmpp_stanza_t* stanza_create_roster_iq(xmpp_ctx_t *ctx);
xmpp_stanza_t* stanza_create_ping_iq(xmpp_ctx_t *ctx, const char *const target);
xmpp_stanza_t* stanza_create_disco_info_iq(xmpp_ctx_t *ctx, const char *const id,
    const char *const to, const char *const node);

xmpp_stanza_t* stanza_create_last_activity_iq(xmpp_ctx_t *ctx, const char *const id,
    const char *const to);

xmpp_stanza_t* stanza_create_invite(xmpp_ctx_t *ctx, const char *const room,
    const char *const contact, const char *const reason, const char *const password);
xmpp_stanza_t* stanza_create_mediated_invite(xmpp_ctx_t *ctx, const char *const room,
    const char *const contact, const char *const reason);

gboolean stanza_contains_chat_state(xmpp_stanza_t *stanza);

GDateTime* stanza_get_delay(xmpp_stanza_t *const stanza);

gboolean stanza_is_muc_presence(xmpp_stanza_t *const stanza);
gboolean stanza_is_muc_self_presence(xmpp_stanza_t *const stanza,
    const char *const self_jid);
gboolean stanza_is_room_nick_change(xmpp_stanza_t *const stanza);
gboolean stanza_muc_requires_config(xmpp_stanza_t *const stanza);

const char* stanza_get_new_nick(xmpp_stanza_t *const stanza);
xmpp_stanza_t* stanza_create_instant_room_request_iq(xmpp_ctx_t *ctx, const char *const room_jid);
xmpp_stanza_t* stanza_create_instant_room_destroy_iq(xmpp_ctx_t *ctx, const char *const room_jid);
xmpp_stanza_t* stanza_create_room_config_request_iq(xmpp_ctx_t *ctx, const char *const room_jid);
xmpp_stanza_t* stanza_create_room_config_cancel_iq(xmpp_ctx_t *ctx, const char *const room_jid);
xmpp_stanza_t* stanza_create_room_config_submit_iq(xmpp_ctx_t *ctx, const char *const room, DataForm *form);
xmpp_stanza_t* stanza_create_room_affiliation_list_iq(xmpp_ctx_t *ctx, const char *const room,
    const char *const affiliation);
xmpp_stanza_t* stanza_create_room_affiliation_set_iq(xmpp_ctx_t *ctx, const char *const room, const char *const jid,
    const char *const affiliation, const char *const reason);
xmpp_stanza_t* stanza_create_room_role_set_iq(xmpp_ctx_t *const ctx, const char *const room, const char *const jid,
    const char *const role, const char *const reason);
xmpp_stanza_t* stanza_create_room_role_list_iq(xmpp_ctx_t *ctx, const char *const room, const char *const role);

xmpp_stanza_t* stanza_create_room_subject_message(xmpp_ctx_t *ctx, const char *const room, const char *const subject);
xmpp_stanza_t* stanza_create_room_kick_iq(xmpp_ctx_t *const ctx, const char *const room, const char *const nick,
    const char *const reason);

int stanza_get_idle_time(xmpp_stanza_t *const stanza);

void stanza_attach_priority(xmpp_ctx_t *const ctx, xmpp_stanza_t *const presence, const int pri);
void stanza_attach_last_activity(xmpp_ctx_t *const ctx, xmpp_stanza_t *const presence, const int idle);
void stanza_attach_caps(xmpp_ctx_t *const ctx, xmpp_stanza_t *const presence);
void stanza_attach_show(xmpp_ctx_t *const ctx, xmpp_stanza_t *const presence, const char *const show);
void stanza_attach_status(xmpp_ctx_t *const ctx, xmpp_stanza_t *const presence, const char *const status);

xmpp_stanza_t* stanza_create_caps_query_element(xmpp_ctx_t *ctx);
char* stanza_create_caps_sha1_from_query(xmpp_stanza_t *const query);
EntityCapabilities* stanza_create_caps_from_query_element(xmpp_stanza_t *query);

const char* stanza_get_presence_string_from_type(resource_presence_t presence_type);
xmpp_stanza_t* stanza_create_software_version_iq(xmpp_ctx_t *ctx, const char *const fulljid);
xmpp_stanza_t* stanza_create_disco_items_iq(xmpp_ctx_t *ctx, const char *const id, const char *const jid);

char* stanza_get_status(xmpp_stanza_t *stanza, char *def);
char* stanza_get_show(xmpp_stanza_t *stanza, char *def);

xmpp_stanza_t* stanza_create_roster_set(xmpp_ctx_t *ctx, const char *const id, const char *const jid,
    const char *const handle, GSList *groups);
xmpp_stanza_t* stanza_create_roster_remove_set(xmpp_ctx_t *ctx, const char *const barejid);

char* stanza_get_error_message(xmpp_stanza_t *const stanza);

GSList* stanza_get_status_codes_by_ns(xmpp_stanza_t *const stanza, char *ns);
gboolean stanza_room_destroyed(xmpp_stanza_t *stanza);
const char* stanza_get_muc_destroy_alternative_room(xmpp_stanza_t *stanza);
char* stanza_get_muc_destroy_alternative_password(xmpp_stanza_t *stanza);
char* stanza_get_muc_destroy_reason(xmpp_stanza_t *stanza);
const char* stanza_get_actor(xmpp_stanza_t *stanza);
char* stanza_get_reason(xmpp_stanza_t *stanza);

Resource* stanza_resource_from_presence(XMPPPresence *presence);
XMPPPresence* stanza_parse_presence(xmpp_stanza_t *stanza, int *err);
void stanza_free_presence(XMPPPresence *presence);

char* stanza_text_strdup(xmpp_stanza_t *stanza);

XMPPCaps* stanza_parse_caps(xmpp_stanza_t *const stanza);
void stanza_free_caps(XMPPCaps *caps);

#endif
