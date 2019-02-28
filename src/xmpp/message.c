/*
 * message.c
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
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

#include "config.h"

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LIBMESODE
#include <mesode.h>
#endif

#ifdef HAVE_LIBSTROPHE
#include <strophe.h>
#endif

#include "profanity.h"
#include "log.h"
#include "config/preferences.h"
#include "event/server_events.h"
#include "pgp/gpg.h"
#include "plugins/plugins.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/chat_session.h"
#include "xmpp/muc.h"
#include "xmpp/session.h"
#include "xmpp/message.h"
#include "xmpp/roster.h"
#include "xmpp/roster_list.h"
#include "xmpp/stanza.h"
#include "xmpp/connection.h"
#include "xmpp/xmpp.h"

#ifdef HAVE_OMEMO
#include "omemo/omemo.h"
#endif

typedef struct p_message_handle_t {
    ProfMessageCallback func;
    ProfMessageFreeCallback free_func;
    void *userdata;
} ProfMessageHandler;

static int _message_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata);

static void _handle_error(xmpp_stanza_t *const stanza);
static void _handle_groupchat(xmpp_stanza_t *const stanza);
static void _handel_muc_user(xmpp_stanza_t *const stanza);
static void _handle_conference(xmpp_stanza_t *const stanza);
static void _handle_captcha(xmpp_stanza_t *const stanza);
static void _handle_receipt_received(xmpp_stanza_t *const stanza);
static void _handle_chat(xmpp_stanza_t *const stanza);
static void _handle_omemo(xmpp_stanza_t *const stanza);

static void _send_message_stanza(xmpp_stanza_t *const stanza);

static GHashTable *pubsub_event_handlers;

static int
_message_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata)
{
    log_debug("Message stanza handler fired");

    char *text;
    size_t text_size;
    xmpp_stanza_to_text(stanza, &text, &text_size);
    gboolean cont = plugins_on_message_stanza_receive(text);
    xmpp_free(connection_get_ctx(), text);
    if (!cont) {
        return 1;
    }

    const char *type = xmpp_stanza_get_type(stanza);

    if (g_strcmp0(type, STANZA_TYPE_ERROR) == 0) {
        _handle_error(stanza);
    }

    if (g_strcmp0(type, STANZA_TYPE_GROUPCHAT) == 0) {
        _handle_groupchat(stanza);
    }

    xmpp_stanza_t *mucuser = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
    if (mucuser) {
        _handel_muc_user(stanza);
    }

    xmpp_stanza_t *conference = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_CONFERENCE);
    if (conference) {
        _handle_conference(stanza);
    }

    xmpp_stanza_t *captcha = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_CAPTCHA);
    if (captcha) {
        _handle_captcha(stanza);
    }

    xmpp_stanza_t *receipts = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_RECEIPTS);
    if (receipts) {
        _handle_receipt_received(stanza);
    }

    xmpp_stanza_t *event = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_PUBSUB_EVENT);
    if (event) {
        xmpp_stanza_t *child = xmpp_stanza_get_children(event);
        if (child) {
            const char *node = xmpp_stanza_get_attribute(child, STANZA_ATTR_NODE);
            if (node) {
                ProfMessageHandler *handler = g_hash_table_lookup(pubsub_event_handlers, node);
                if (handler) {
                    int keep = handler->func(stanza, handler->userdata);
                    if (!keep) {
                        free(handler);
                        g_hash_table_remove(pubsub_event_handlers, node);
                    }
                }
            }
        }
    }

#ifdef HAVE_OMEMO
    xmpp_stanza_t *omemo = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_OMEMO);
    if (omemo) {
        _handle_omemo(stanza);
    }
#endif

    _handle_chat(stanza);

    return 1;
}

void
message_handlers_init(void)
{
    xmpp_conn_t * const conn = connection_get_conn();
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_handler_add(conn, _message_handler, NULL, STANZA_NAME_MESSAGE, NULL, ctx);

    if (pubsub_event_handlers) {
        GList *keys = g_hash_table_get_keys(pubsub_event_handlers);
        GList *curr = keys;
        while (curr) {
            ProfMessageHandler *handler = g_hash_table_lookup(pubsub_event_handlers, curr->data);
            if (handler->free_func && handler->userdata) {
                handler->free_func(handler->userdata);
            }
            curr = g_list_next(curr);
        }
        g_list_free(keys);
        g_hash_table_destroy(pubsub_event_handlers);
    }

    pubsub_event_handlers = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
}

void
message_pubsub_event_handler_add(const char *const node, ProfMessageCallback func, ProfMessageFreeCallback free_func, void *userdata)
{
    ProfMessageHandler *handler = malloc(sizeof(ProfMessageHandler));
    handler->func = func;
    handler->free_func = free_func;
    handler->userdata = userdata;

    g_hash_table_insert(pubsub_event_handlers, strdup(node), handler);
}

char*
message_send_chat(const char *const barejid, const char *const msg, const char *const oob_url,
    gboolean request_receipt)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();

    char *state = chat_session_get_state(barejid);
    char *jid = chat_session_get_jid(barejid);
    char *id = connection_create_stanza_id("msg");

    xmpp_stanza_t *message = xmpp_message_new(ctx, STANZA_TYPE_CHAT, jid, id);
    xmpp_message_set_body(message, msg);
    free(jid);

    if (state) {
        stanza_attach_state(ctx, message, state);
    }

    if (oob_url) {
        stanza_attach_x_oob_url(ctx, message, oob_url);
    }

    if (request_receipt) {
        stanza_attach_receipt_request(ctx, message);
    }

    _send_message_stanza(message);
    xmpp_stanza_release(message);

    return id;
}

char*
message_send_chat_pgp(const char *const barejid, const char *const msg, gboolean request_receipt)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();

    char *state = chat_session_get_state(barejid);
    char *jid = chat_session_get_jid(barejid);
    char *id = connection_create_stanza_id("msg");

    xmpp_stanza_t *message = NULL;
#ifdef HAVE_LIBGPGME
    char *account_name = session_get_account_name();
    ProfAccount *account = accounts_get_account(account_name);
    if (account->pgp_keyid) {
        Jid *jidp = jid_create(jid);
        char *encrypted = p_gpg_encrypt(jidp->barejid, msg, account->pgp_keyid);
        if (encrypted) {
            message = xmpp_message_new(ctx, STANZA_TYPE_CHAT, jid, id);
            xmpp_message_set_body(message, "This message is encrypted (XEP-0027).");
            xmpp_stanza_t *x = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(x, STANZA_NAME_X);
            xmpp_stanza_set_ns(x, STANZA_NS_ENCRYPTED);
            xmpp_stanza_t *enc_st = xmpp_stanza_new(ctx);
            xmpp_stanza_set_text(enc_st, encrypted);
            xmpp_stanza_add_child(x, enc_st);
            xmpp_stanza_release(enc_st);
            xmpp_stanza_add_child(message, x);
            xmpp_stanza_release(x);
            free(encrypted);
        } else {
            message = xmpp_message_new(ctx, STANZA_TYPE_CHAT, jid, id);
            xmpp_message_set_body(message, msg);
        }
        jid_destroy(jidp);
    } else {
        message = xmpp_message_new(ctx, STANZA_TYPE_CHAT, jid, id);
        xmpp_message_set_body(message, msg);
    }
    account_free(account);
#else
    message = xmpp_message_new(ctx, STANZA_TYPE_CHAT, jid, id);
    xmpp_message_set_body(message, msg);
#endif
    free(jid);

    if (state) {
        stanza_attach_state(ctx, message, state);
    }

    if (request_receipt) {
        stanza_attach_receipt_request(ctx, message);
    }

    _send_message_stanza(message);
    xmpp_stanza_release(message);

    return id;
}

char*
message_send_chat_otr(const char *const barejid, const char *const msg, gboolean request_receipt)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();

    char *state = chat_session_get_state(barejid);
    char *jid = chat_session_get_jid(barejid);
    char *id = connection_create_stanza_id("msg");

    xmpp_stanza_t *message = xmpp_message_new(ctx, STANZA_TYPE_CHAT, barejid, id);
    xmpp_message_set_body(message, msg);

    free(jid);

    if (state) {
        stanza_attach_state(ctx, message, state);
    }

    stanza_attach_carbons_private(ctx, message);
    stanza_attach_hints_no_copy(ctx, message);
    stanza_attach_hints_no_store(ctx, message);

    if (request_receipt) {
        stanza_attach_receipt_request(ctx, message);
    }

    _send_message_stanza(message);
    xmpp_stanza_release(message);

    return id;
}

#ifdef HAVE_OMEMO
char*
message_send_chat_omemo(const char *const jid, uint32_t sid, GList *keys,
    const unsigned char *const iv, size_t iv_len,
    const unsigned char *const ciphertext, size_t ciphertext_len,
    gboolean request_receipt)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    char *id = connection_create_stanza_id("msg");

    xmpp_stanza_t *message = xmpp_message_new(ctx, STANZA_TYPE_CHAT, jid, id);

    xmpp_stanza_t *encrypted = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(encrypted, "encrypted");
    xmpp_stanza_set_ns(encrypted, STANZA_NS_OMEMO);

    xmpp_stanza_t *header = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(header, "header");
    char *sid_text = g_strdup_printf("%d", sid);
    xmpp_stanza_set_attribute(header, "sid", sid_text);
    g_free(sid_text);

    GList *key_iter;
    for (key_iter = keys; key_iter != NULL; key_iter = key_iter->next) {
        omemo_key_t *key = (omemo_key_t *)key_iter->data;

        xmpp_stanza_t *key_stanza = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(key_stanza, "key");
        char *rid = g_strdup_printf("%d", key->device_id);
        xmpp_stanza_set_attribute(key_stanza, "rid", rid);
        g_free(rid);
        if (key->prekey) {
            xmpp_stanza_set_attribute(key_stanza, "prekey", "true");
        }

        gchar *key_raw = g_base64_encode(key->data, key->length);
        xmpp_stanza_t *key_text = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(key_text, key_raw);
        g_free(key_raw);

        xmpp_stanza_add_child(key_stanza, key_text);
        xmpp_stanza_add_child(header, key_stanza);
        xmpp_stanza_release(key_text);
        xmpp_stanza_release(key_stanza);
    }

    xmpp_stanza_t *iv_stanza = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iv_stanza, "iv");

    gchar *iv_raw = g_base64_encode(iv, iv_len);
    xmpp_stanza_t *iv_text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(iv_text, iv_raw);
    g_free(iv_raw);

    xmpp_stanza_add_child(iv_stanza, iv_text);
    xmpp_stanza_add_child(header, iv_stanza);
    xmpp_stanza_release(iv_text);
    xmpp_stanza_release(iv_stanza);

    xmpp_stanza_add_child(encrypted, header);
    xmpp_stanza_release(header);

    xmpp_stanza_t *payload = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(payload, "payload");

    gchar *ciphertext_raw = g_base64_encode(ciphertext, ciphertext_len);
    xmpp_stanza_t *payload_text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(payload_text, ciphertext_raw);
    g_free(ciphertext_raw);

    xmpp_stanza_add_child(payload, payload_text);
    xmpp_stanza_add_child(encrypted, payload);
    xmpp_stanza_release(payload_text);
    xmpp_stanza_release(payload);

    xmpp_stanza_add_child(message, encrypted);
    xmpp_stanza_release(encrypted);

    xmpp_stanza_t *body = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(body, "body");
    xmpp_stanza_t *body_text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(body_text, "You received a message encrypted with OMEMO but your client doesn't support OMEMO.");
    xmpp_stanza_add_child(body, body_text);
    xmpp_stanza_release(body_text);
    xmpp_stanza_add_child(message, body);

    stanza_attach_carbons_private(ctx, message);
    stanza_attach_hints_no_copy(ctx, message);
    stanza_attach_hints_no_store(ctx, message);

    if (request_receipt) {
        stanza_attach_receipt_request(ctx, message);
    }

    _send_message_stanza(message);
    xmpp_stanza_release(message);

    return id;
}
#endif

void
message_send_private(const char *const fulljid, const char *const msg, const char *const oob_url)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    char *id = connection_create_stanza_id("prv");

    xmpp_stanza_t *message = xmpp_message_new(ctx, STANZA_TYPE_CHAT, fulljid, id);
    xmpp_message_set_body(message, msg);

    free(id);

    if (oob_url) {
        stanza_attach_x_oob_url(ctx, message, oob_url);
    }

    _send_message_stanza(message);
    xmpp_stanza_release(message);
}

void
message_send_groupchat(const char *const roomjid, const char *const msg, const char *const oob_url)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    char *id = connection_create_stanza_id("muc");

    xmpp_stanza_t *message = xmpp_message_new(ctx, STANZA_TYPE_GROUPCHAT, roomjid, id);
    xmpp_message_set_body(message, msg);

    free(id);

    if (oob_url) {
        stanza_attach_x_oob_url(ctx, message, oob_url);
    }

    _send_message_stanza(message);
    xmpp_stanza_release(message);
}

void
message_send_groupchat_subject(const char *const roomjid, const char *const subject)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *message = stanza_create_room_subject_message(ctx, roomjid, subject);

    _send_message_stanza(message);
    xmpp_stanza_release(message);
}

void
message_send_invite(const char *const roomjid, const char *const contact,
    const char *const reason)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *stanza;

    muc_member_type_t member_type = muc_member_type(roomjid);
    if (member_type == MUC_MEMBER_TYPE_PUBLIC) {
        log_debug("Sending direct invite to %s, for %s", contact, roomjid);
        char *password = muc_password(roomjid);
        stanza = stanza_create_invite(ctx, roomjid, contact, reason, password);
    } else {
        log_debug("Sending mediated invite to %s, for %s", contact, roomjid);
        stanza = stanza_create_mediated_invite(ctx, roomjid, contact, reason);
    }

    _send_message_stanza(stanza);
    xmpp_stanza_release(stanza);
}

void
message_send_composing(const char *const jid)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();

    xmpp_stanza_t *stanza = stanza_create_chat_state(ctx, jid, STANZA_NAME_COMPOSING);
    _send_message_stanza(stanza);
    xmpp_stanza_release(stanza);

}

void
message_send_paused(const char *const jid)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *stanza = stanza_create_chat_state(ctx, jid, STANZA_NAME_PAUSED);
    _send_message_stanza(stanza);
    xmpp_stanza_release(stanza);
}

void
message_send_inactive(const char *const jid)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *stanza = stanza_create_chat_state(ctx, jid, STANZA_NAME_INACTIVE);

    _send_message_stanza(stanza);
    xmpp_stanza_release(stanza);
}

void
message_send_gone(const char *const jid)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    xmpp_stanza_t *stanza = stanza_create_chat_state(ctx, jid, STANZA_NAME_GONE);
    _send_message_stanza(stanza);
    xmpp_stanza_release(stanza);
}

static void
_handle_error(xmpp_stanza_t *const stanza)
{
    const char *id = xmpp_stanza_get_id(stanza);
    const char *jid = xmpp_stanza_get_from(stanza);
    xmpp_stanza_t *error_stanza = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_ERROR);
    const char *type = NULL;
    if (error_stanza) {
        type = xmpp_stanza_get_type(error_stanza);
    }

    // stanza_get_error never returns NULL
    char *err_msg = stanza_get_error_message(stanza);

    GString *log_msg = g_string_new("message stanza error received");
    if (id) {
        g_string_append(log_msg, " id=");
        g_string_append(log_msg, id);
    }
    if (jid) {
        g_string_append(log_msg, " from=");
        g_string_append(log_msg, jid);
    }
    if (type) {
        g_string_append(log_msg, " type=");
        g_string_append(log_msg, type);
    }
    g_string_append(log_msg, " error=");
    g_string_append(log_msg, err_msg);

    log_info(log_msg->str);

    g_string_free(log_msg, TRUE);

    if (!jid) {
        ui_handle_error(err_msg);
    } else if (type && (strcmp(type, "cancel") == 0)) {
        log_info("Recipient %s not found: %s", jid, err_msg);
        Jid *jidp = jid_create(jid);
        chat_session_remove(jidp->barejid);
        jid_destroy(jidp);
    } else {
        ui_handle_recipient_error(jid, err_msg);
    }

    free(err_msg);
}

static void
_handel_muc_user(xmpp_stanza_t *const stanza)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_stanza_t *xns_muc_user = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
    const char *room = xmpp_stanza_get_from(stanza);

    if (!room) {
        log_warning("Message received with no from attribute, ignoring");
        return;
    }

    // XEP-0045
    xmpp_stanza_t *invite = xmpp_stanza_get_child_by_name(xns_muc_user, STANZA_NAME_INVITE);
    if (!invite) {
        return;
    }

    const char *invitor_jid = xmpp_stanza_get_from(invite);
    if (!invitor_jid) {
        log_warning("Chat room invite received with no from attribute");
        return;
    }

    Jid *jidp = jid_create(invitor_jid);
    if (!jidp) {
        return;
    }
    char *invitor = jidp->barejid;

    char *reason = NULL;
    xmpp_stanza_t *reason_st = xmpp_stanza_get_child_by_name(invite, STANZA_NAME_REASON);
    if (reason_st) {
        reason = xmpp_stanza_get_text(reason_st);
    }

    char *password = NULL;
    xmpp_stanza_t *password_st = xmpp_stanza_get_child_by_name(xns_muc_user, STANZA_NAME_PASSWORD);
    if (password_st) {
        password = xmpp_stanza_get_text(password_st);
    }

    sv_ev_room_invite(INVITE_MEDIATED, invitor, room, reason, password);
    jid_destroy(jidp);
    if (reason) {
        xmpp_free(ctx, reason);
    }
    if (password) {
        xmpp_free(ctx, password);
    }
}

static void
_handle_conference(xmpp_stanza_t *const stanza)
{
    xmpp_stanza_t *xns_conference = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_CONFERENCE);

    const char *from = xmpp_stanza_get_from(stanza);
    if (!from) {
        log_warning("Message received with no from attribute, ignoring");
        return;
    }

    Jid *jidp = jid_create(from);
    if (!jidp) {
        return;
    }

    // XEP-0249
    const char *room = xmpp_stanza_get_attribute(xns_conference, STANZA_ATTR_JID);
    if (!room) {
        jid_destroy(jidp);
        return;
    }

    const char *reason = xmpp_stanza_get_attribute(xns_conference, STANZA_ATTR_REASON);
    const char *password = xmpp_stanza_get_attribute(xns_conference, STANZA_ATTR_PASSWORD);

    sv_ev_room_invite(INVITE_DIRECT, jidp->barejid, room, reason, password);
    jid_destroy(jidp);
}

static void
_handle_captcha(xmpp_stanza_t *const stanza)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    const char *from = xmpp_stanza_get_from(stanza);

    if (!from) {
        log_warning("Message received with no from attribute, ignoring");
        return;
    }

    // XEP-0158
    char *message = xmpp_message_get_body(stanza);
    if (!message) {
        return;
    }

    sv_ev_room_broadcast(from, message);
    xmpp_free(ctx, message);
}

static void
_handle_groupchat(xmpp_stanza_t *const stanza)
{
    xmpp_ctx_t *ctx = connection_get_ctx();
    char *message = NULL;
    const char *room_jid = xmpp_stanza_get_from(stanza);
    Jid *jid = jid_create(room_jid);

    // handle room subject
    xmpp_stanza_t *subject = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_SUBJECT);
    if (subject) {
        message = xmpp_stanza_get_text(subject);
        sv_ev_room_subject(jid->barejid, jid->resourcepart, message);
        xmpp_free(ctx, message);

        jid_destroy(jid);
        return;
    }

    // handle room broadcasts
    if (!jid->resourcepart) {
        message = xmpp_message_get_body(stanza);
        if (!message) {
            jid_destroy(jid);
            return;
        }

        sv_ev_room_broadcast(room_jid, message);
        xmpp_free(ctx, message);

        jid_destroy(jid);
        return;
    }

    if (!jid_is_valid_room_form(jid)) {
        log_error("Invalid room JID: %s", jid->str);
        jid_destroy(jid);
        return;
    }

    // room not active in profanity
    if (!muc_active(jid->barejid)) {
        log_error("Message received for inactive chat room: %s", jid->str);
        jid_destroy(jid);
        return;
    }

    message = xmpp_message_get_body(stanza);
    if (!message) {
        jid_destroy(jid);
        return;
    }

    // determine if the notifications happened whilst offline
    GDateTime *timestamp = stanza_get_delay(stanza);
    if (timestamp) {
        sv_ev_room_history(jid->barejid, jid->resourcepart, timestamp, message);
        g_date_time_unref(timestamp);
    } else {
        sv_ev_room_message(jid->barejid, jid->resourcepart, message);
    }

    xmpp_free(ctx, message);
    jid_destroy(jid);
}

void
_message_send_receipt(const char *const fulljid, const char *const message_id)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();

    char *id = connection_create_stanza_id("receipt");
    xmpp_stanza_t *message = xmpp_message_new(ctx, NULL, fulljid, id);
    free(id);

    xmpp_stanza_t *receipt = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(receipt, "received");
    xmpp_stanza_set_ns(receipt, STANZA_NS_RECEIPTS);
    xmpp_stanza_set_id(receipt, message_id);

    xmpp_stanza_add_child(message, receipt);
    xmpp_stanza_release(receipt);

    _send_message_stanza(message);
    xmpp_stanza_release(message);
}

static void
_handle_receipt_received(xmpp_stanza_t *const stanza)
{
    xmpp_stanza_t *receipt = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_RECEIPTS);
    const char *name = xmpp_stanza_get_name(receipt);
    if (g_strcmp0(name, "received") != 0) {
        return;
    }

    const char *id = xmpp_stanza_get_id(receipt);
    if (!id) {
        return;
    }

    const char *fulljid = xmpp_stanza_get_from(stanza);
    if (!fulljid) {
        return;
    }

    Jid *jidp = jid_create(fulljid);
    sv_ev_message_receipt(jidp->barejid, id);
    jid_destroy(jidp);
}

void
_receipt_request_handler(xmpp_stanza_t *const stanza)
{
    if (!prefs_get_boolean(PREF_RECEIPTS_SEND)) {
        return;
    }

    const char *id = xmpp_stanza_get_id(stanza);
    if (!id) {
        return;
    }

    xmpp_stanza_t *receipts = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_RECEIPTS);
    if (!receipts) {
        return;
    }

    const char *receipts_name = xmpp_stanza_get_name(receipts);
    if (g_strcmp0(receipts_name, "request") != 0) {
        return;
    }

    const gchar *from = xmpp_stanza_get_from(stanza);
    Jid *jid = jid_create(from);
    _message_send_receipt(jid->fulljid, id);
    jid_destroy(jid);
}

void
_private_chat_handler(xmpp_stanza_t *const stanza, const char *const fulljid)
{
    char *message = xmpp_message_get_body(stanza);
    if (!message) {
        return;
    }

    GDateTime *timestamp = stanza_get_delay(stanza);
    if (timestamp) {
        sv_ev_delayed_private_message(fulljid, message, timestamp);
        g_date_time_unref(timestamp);
    } else {
        sv_ev_incoming_private_message(fulljid, message);
    }

    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_free(ctx, message);
}

static gboolean
_handle_carbons(xmpp_stanza_t *const stanza)
{
    xmpp_stanza_t *carbons = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_CARBONS);
    if (!carbons) {
        return FALSE;
    }

    const char *name = xmpp_stanza_get_name(carbons);
    if (!name) {
        log_error("Unable to retrieve stanza name for Carbon");
        return TRUE;
    }

    if (g_strcmp0(name, "private") == 0) {
        log_info("Carbon received with private element.");
        return FALSE;
    }

    if ((g_strcmp0(name, "received") != 0) && (g_strcmp0(name, "sent") != 0)) {
        log_warning("Carbon received with unrecognised stanza name: %s", name);
        return TRUE;
    }

    xmpp_stanza_t *forwarded = xmpp_stanza_get_child_by_ns(carbons, STANZA_NS_FORWARD);
    if (!forwarded) {
        log_warning("Carbon received with no forwarded element");
        return TRUE;
    }

    xmpp_stanza_t *message = xmpp_stanza_get_child_by_name(forwarded, STANZA_NAME_MESSAGE);
    if (!message) {
        log_warning("Carbon received with no message element");
        return TRUE;
    }

    char *message_txt = xmpp_message_get_body(message);
    if (!message_txt) {
        log_warning("Carbon received with no message.");
        return TRUE;
    }

    Jid *my_jid = jid_create(connection_get_fulljid());
    const char *const stanza_from = xmpp_stanza_get_from(stanza);
    if (g_strcmp0(my_jid->barejid, stanza_from) != 0) {
        log_warning("Invalid carbon received, from: %s", stanza_from);
        return TRUE;
    }

    const gchar *to = xmpp_stanza_get_to(message);
    const gchar *from = xmpp_stanza_get_from(message);

    // happens when receive a carbon of a self sent message
    if (!to) to = from;

    Jid *jid_from = jid_create(from);
    Jid *jid_to = jid_create(to);

    // check for pgp encrypted message
    char *enc_message = NULL;
    xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(message, STANZA_NS_ENCRYPTED);
    if (x) {
        enc_message = xmpp_stanza_get_text(x);
    }

    // if we are the recipient, treat as standard incoming message
    if (g_strcmp0(my_jid->barejid, jid_to->barejid) == 0) {
        sv_ev_incoming_carbon(jid_from->barejid, jid_from->resourcepart, message_txt, enc_message);

    // else treat as a sent message
    } else {
        sv_ev_outgoing_carbon(jid_to->barejid, message_txt, enc_message);
    }

    xmpp_ctx_t *ctx = connection_get_ctx();
    xmpp_free(ctx, message_txt);
    xmpp_free(ctx, enc_message);

    jid_destroy(jid_from);
    jid_destroy(jid_to);
    jid_destroy(my_jid);

    return TRUE;
}

static void
_handle_chat(xmpp_stanza_t *const stanza)
{
    // ignore if type not chat or absent
    const char *type = xmpp_stanza_get_type(stanza);
    if (!(g_strcmp0(type, "chat") == 0 || type == NULL)) {
        return;
    }

    // check if carbon message
    gboolean res = _handle_carbons(stanza);
    if (res) {
        return;
    }

    // ignore handled namespaces
    xmpp_stanza_t *conf = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_CONFERENCE);
    xmpp_stanza_t *captcha = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_CAPTCHA);
    xmpp_stanza_t *omemo = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_OMEMO);
    if (conf || captcha || omemo) {
        return;
    }

    // some clients send the mucuser namespace with private messages
    // if the namespace exists, and the stanza contains a body element, assume its a private message
    // otherwise exit the handler
    xmpp_stanza_t *mucuser = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
    xmpp_stanza_t *body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);
    if (mucuser && body == NULL) {
        return;
    }

    const gchar *from = xmpp_stanza_get_from(stanza);
    Jid *jid = jid_create(from);

    // private message from chat room use full jid (room/nick)
    if (muc_active(jid->barejid)) {
        _private_chat_handler(stanza, jid->fulljid);
        jid_destroy(jid);
        return;
    }

    // standard chat message, use jid without resource
    xmpp_ctx_t *ctx = connection_get_ctx();
    GDateTime *timestamp = stanza_get_delay(stanza);
    if (body) {
        char *message = xmpp_stanza_get_text(body);
        if (message) {
            char *enc_message = NULL;
            xmpp_stanza_t *x = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_ENCRYPTED);
            if (x) {
                enc_message = xmpp_stanza_get_text(x);
            }
            sv_ev_incoming_message(jid->barejid, jid->resourcepart, message, enc_message, timestamp);
            xmpp_free(ctx, enc_message);

            _receipt_request_handler(stanza);

            xmpp_free(ctx, message);
        }
    }

    // handle chat sessions and states
    if (!timestamp && jid->resourcepart) {
        gboolean gone = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_GONE) != NULL;
        gboolean typing = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_COMPOSING) != NULL;
        gboolean paused = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_PAUSED) != NULL;
        gboolean inactive = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_INACTIVE) != NULL;
        if (gone) {
            sv_ev_gone(jid->barejid, jid->resourcepart);
        } else if (typing) {
            sv_ev_typing(jid->barejid, jid->resourcepart);
        } else if (paused) {
            sv_ev_paused(jid->barejid, jid->resourcepart);
        } else if (inactive) {
            sv_ev_inactive(jid->barejid, jid->resourcepart);
        } else if (stanza_contains_chat_state(stanza)) {
            sv_ev_activity(jid->barejid, jid->resourcepart, TRUE);
        } else {
            sv_ev_activity(jid->barejid, jid->resourcepart, FALSE);
        }
    }

    if (timestamp) g_date_time_unref(timestamp);
    jid_destroy(jid);
}

static void
_handle_omemo(xmpp_stanza_t *const stanza)
{
    xmpp_stanza_t *encrypted = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_OMEMO);
    if (!encrypted) {
        return;
    }

    xmpp_stanza_t *header = xmpp_stanza_get_child_by_name(encrypted, "header");
    if (!header) {
        return;
    }

    const char *sid_text = xmpp_stanza_get_attribute(header, "sid");
    if (!sid_text) {
        return;
    }
    uint32_t sid = strtoul(sid_text, NULL, 10);

    xmpp_stanza_t *iv = xmpp_stanza_get_child_by_name(header, "iv");
    if (!iv) {
        return;
    }
    const char *iv_text = xmpp_stanza_get_text(iv);
    if (!iv_text) {
        return;
    }
    size_t iv_len;
    const unsigned char *iv_raw = g_base64_decode(iv_text, &iv_len);

    xmpp_stanza_t *payload = xmpp_stanza_get_child_by_name(encrypted, "payload");
    if (!payload) {
        return;
    }
    const char *payload_text = xmpp_stanza_get_text(payload);
    if (!payload_text) {
        return;
    }
    size_t payload_len;
    const unsigned char *payload_raw = g_base64_decode(payload_text, &payload_len);

    GList *keys = NULL;
    xmpp_stanza_t *key_stanza;
    for (key_stanza = xmpp_stanza_get_children(header); key_stanza != NULL; key_stanza = xmpp_stanza_get_next(key_stanza)) {
        if (g_strcmp0(xmpp_stanza_get_name(key_stanza), "key") != 0) {
            continue;
        }

        omemo_key_t *key = malloc(sizeof(omemo_key_t));
        const char *key_text = xmpp_stanza_get_text(key_stanza);
        if (!key_text) {
            goto skip;
        }


        const char *rid_text = xmpp_stanza_get_attribute(key_stanza, "rid");
        key->device_id = strtoul(rid_text, NULL, 10);
        if (!key->device_id) {
            goto skip;
        }
        key->data = g_base64_decode(key_text, &key->length);
        key->prekey = g_strcmp0(xmpp_stanza_get_attribute(key_stanza, "prekey"), "true") == 0;
        keys = g_list_append(keys, key);
        continue;

skip:
        free(key);
    }

    const char *from = xmpp_stanza_get_from(stanza);
    Jid *jid = jid_create(from);
    GDateTime *timestamp = stanza_get_delay(stanza);

    char *plaintext = omemo_on_message_recv(jid->barejid, sid, iv_raw, iv_len, keys, payload_raw, payload_len);
    if (!plaintext) {
        goto out;
    }

    gboolean new_win = FALSE;
    ProfChatWin *chatwin = wins_get_chat(jid->barejid);
    if (!chatwin) {
        ProfWin *window = wins_new_chat(jid->barejid);
        chatwin = (ProfChatWin*)window;
        new_win = TRUE;
    }

    chat_log_omemo_msg_in(jid->barejid, plaintext, timestamp);
    chatwin_incoming_msg(chatwin, jid->resourcepart, plaintext, timestamp, new_win, PROF_MSG_OMEMO);

out:
    jid_destroy(jid);
    if (timestamp) g_date_time_unref(timestamp);
}

static void
_send_message_stanza(xmpp_stanza_t *const stanza)
{
    char *text;
    size_t text_size;
    xmpp_stanza_to_text(stanza, &text, &text_size);

    xmpp_conn_t *conn = connection_get_conn();
    char *plugin_text = plugins_on_message_stanza_send(text);
    if (plugin_text) {
        xmpp_send_raw_string(conn, "%s", plugin_text);
        free(plugin_text);
    } else {
        xmpp_send_raw_string(conn, "%s", text);
    }
    xmpp_free(connection_get_ctx(), text);
}
