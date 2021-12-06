/*
 * message.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2019 - 2021 Michael Vetter <jubalh@iodoru.org>
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

#include <strophe.h>

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
#include "xmpp/form.h"

#ifdef HAVE_OMEMO
#include "xmpp/omemo.h"
#include "omemo/omemo.h"
#endif

typedef struct p_message_handle_t
{
    ProfMessageCallback func;
    ProfMessageFreeCallback free_func;
    void* userdata;
} ProfMessageHandler;

static int _message_handler(xmpp_conn_t* const conn, xmpp_stanza_t* const stanza, void* const userdata);
static void _handle_error(xmpp_stanza_t* const stanza);
static void _handle_groupchat(xmpp_stanza_t* const stanza);
static void _handle_muc_user(xmpp_stanza_t* const stanza);
static void _handle_muc_private_message(xmpp_stanza_t* const stanza);
static void _handle_conference(xmpp_stanza_t* const stanza);
static void _handle_captcha(xmpp_stanza_t* const stanza);
static void _handle_receipt_received(xmpp_stanza_t* const stanza);
static void _handle_chat(xmpp_stanza_t* const stanza, gboolean is_mam, gboolean is_carbon, const char* result_id, GDateTime* timestamp);
static void _handle_ox_chat(xmpp_stanza_t* const stanza, ProfMessage* message, gboolean is_mam);
static xmpp_stanza_t* _handle_carbons(xmpp_stanza_t* const stanza);
static void _send_message_stanza(xmpp_stanza_t* const stanza);
static gboolean _handle_mam(xmpp_stanza_t* const stanza);
static void _handle_pubsub(xmpp_stanza_t* const stanza, xmpp_stanza_t* const event);
static gboolean _handle_form(xmpp_stanza_t* const stanza);
static gboolean _handle_jingle_message(xmpp_stanza_t* const stanza);
static gboolean _should_ignore_based_on_silence(xmpp_stanza_t* const stanza);

#ifdef HAVE_LIBGPGME
static xmpp_stanza_t* _openpgp_signcrypt(xmpp_ctx_t* ctx, const char* const to, const char* const text);
#endif // HAVE_LIBGPGME

static GHashTable* pubsub_event_handlers;

static gboolean
_handled_by_plugin(xmpp_stanza_t* const stanza)
{
    char* text;
    size_t text_size;

    xmpp_stanza_to_text(stanza, &text, &text_size);
    gboolean cont = plugins_on_message_stanza_receive(text);
    xmpp_free(connection_get_ctx(), text);

    return !cont;
}

static void
_handle_headline(xmpp_stanza_t* const stanza)
{
    xmpp_stanza_t* body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);
    if (body) {
        char* text = xmpp_stanza_get_text(body);
        if (text) {
            cons_show("Headline: %s", text);
            xmpp_free(connection_get_ctx(), text);
        }
    }
}

static void
_handle_chat_states(xmpp_stanza_t* const stanza, Jid* const jid)
{
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

static int
_message_handler(xmpp_conn_t* const conn, xmpp_stanza_t* const stanza, void* const userdata)
{
    log_debug("Message stanza handler fired");

    if (_handled_by_plugin(stanza)) {
        return 1;
    }

    // type according to RFC 6121
    const char* type = xmpp_stanza_get_type(stanza);

    if (type && g_strcmp0(type, STANZA_TYPE_ERROR) == 0) {
        _handle_error(stanza);
    } else if (type && g_strcmp0(type, STANZA_TYPE_GROUPCHAT) == 0) {
        // XEP-0045: Multi-User Chat
        _handle_groupchat(stanza);

    } else if (type && g_strcmp0(type, STANZA_TYPE_HEADLINE) == 0) {
        xmpp_stanza_t* event = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_PUBSUB_EVENT);
        // TODO: do we want to handle all pubsub here or should additionaly check for STANZA_NS_MOOD?
        if (event) {
            _handle_pubsub(stanza, event);
            return 1;
        } else {
            _handle_headline(stanza);
        }
    } else if (type == NULL || g_strcmp0(type, STANZA_TYPE_CHAT) == 0 || g_strcmp0(type, STANZA_TYPE_NORMAL) == 0) {
        // type: chat, normal (==NULL)

        // ignore all messages from JIDs that are not in roster, if 'silence' is set
        if (_should_ignore_based_on_silence(stanza)) {
            return 1;
        }

        // XEP-0353: Jingle Message Initiation
        if (_handle_jingle_message(stanza)) {
            return 1;
        }

        // XEP-0045: Multi-User Chat 8.6 Voice Requests
        if (_handle_form(stanza)) {
            return 1;
        }

        // XEP-0313: Message Archive Management
        if (_handle_mam(stanza)) {
            return 1;
        }

        // XEP-0045: Multi-User Chat - invites - presence
        xmpp_stanza_t* mucuser = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
        if (mucuser) {
            _handle_muc_user(stanza);
        }

        // XEP-0249: Direct MUC Invitations
        xmpp_stanza_t* conference = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_CONFERENCE);
        if (conference) {
            _handle_conference(stanza);
            return 1;
        }

        // XEP-0158: CAPTCHA Forms
        xmpp_stanza_t* captcha = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_CAPTCHA);
        if (captcha) {
            _handle_captcha(stanza);
            return 1;
        }

        // XEP-0184: Message Delivery Receipts
        xmpp_stanza_t* receipts = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_RECEIPTS);
        if (receipts) {
            _handle_receipt_received(stanza);
        }

        // XEP-0060: Publish-Subscribe
        xmpp_stanza_t* event = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_PUBSUB_EVENT);
        if (event) {
            _handle_pubsub(stanza, event);
            return 1;
        }

        xmpp_stanza_t* msg_stanza = stanza;
        gboolean is_carbon = FALSE;

        // XEP-0280: Message Carbons
        // Only allow `<sent xmlns='urn:xmpp:carbons:2'>` and `<received xmlns='urn:xmpp:carbons:2'>` carbons
        // Thus ignoring `<private xmlns="urn:xmpp:carbons:2"/>`
        xmpp_stanza_t* carbons = xmpp_stanza_get_child_by_name_and_ns(stanza, STANZA_NAME_SENT, STANZA_NS_CARBONS);
        if (!carbons) {
            carbons = xmpp_stanza_get_child_by_name_and_ns(stanza, STANZA_NAME_RECEIVED, STANZA_NS_CARBONS);
        }

        if (carbons) {

            // carbon must come from ourselves
            char* mybarejid = connection_get_barejid();
            const char* const stanza_from = xmpp_stanza_get_from(stanza);

            if (stanza_from) {
                if (g_strcmp0(mybarejid, stanza_from) != 0) {
                    log_warning("Invalid carbon received, from: %s", stanza_from);
                    msg_stanza = NULL;
                } else {
                    is_carbon = TRUE;
                    // returns NULL if it was a carbon that was invalid, so that we dont parse later
                    msg_stanza = _handle_carbons(carbons);
                }
            }

            free(mybarejid);
        }

        if (msg_stanza) {
            _handle_chat(msg_stanza, FALSE, is_carbon, NULL, NULL);
        }
    } else {
        // none of the allowed types
        char* text;
        size_t text_size;

        xmpp_stanza_to_text(stanza, &text, &text_size);
        log_info("Received <message> with invalid 'type': %s", text);

        xmpp_free(connection_get_ctx(), text);
    }

    return 1;
}

void
message_muc_submit_voice_approve(ProfConfWin* confwin)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();
    xmpp_stanza_t* message = stanza_create_approve_voice(ctx, NULL, confwin->roomjid, NULL, confwin->form);

    _send_message_stanza(message);
    xmpp_stanza_release(message);
}

static gboolean
_handle_form(xmpp_stanza_t* const stanza)
{
    xmpp_stanza_t* result = xmpp_stanza_get_child_by_name_and_ns(stanza, STANZA_NAME_X, STANZA_NS_DATA);
    if (!result) {
        return FALSE;
    }

    const char* const stanza_from = xmpp_stanza_get_from(stanza);
    if (!stanza_from) {
        return FALSE;
    }

    DataForm* form = form_create(result);
    ProfConfWin* confwin = (ProfConfWin*)wins_new_config(stanza_from, form, message_muc_submit_voice_approve, NULL, NULL);
    confwin_handle_configuration(confwin, form);

    return TRUE;
}

void
message_handlers_init(void)
{
    xmpp_conn_t* const conn = connection_get_conn();
    xmpp_ctx_t* const ctx = connection_get_ctx();
    xmpp_handler_add(conn, _message_handler, NULL, STANZA_NAME_MESSAGE, NULL, ctx);

    if (pubsub_event_handlers) {
        GList* keys = g_hash_table_get_keys(pubsub_event_handlers);
        GList* curr = keys;
        while (curr) {
            ProfMessageHandler* handler = g_hash_table_lookup(pubsub_event_handlers, curr->data);
            if (handler->free_func && handler->userdata) {
                handler->free_func(handler->userdata);
            }
            curr = g_list_next(curr);
        }
        g_list_free(keys);
        g_hash_table_destroy(pubsub_event_handlers);
    }

    pubsub_event_handlers = g_hash_table_new_full(g_str_hash, g_str_equal, free, free);
}

ProfMessage*
message_init(void)
{
    ProfMessage* message = malloc(sizeof(ProfMessage));

    message->from_jid = NULL;
    message->to_jid = NULL;
    message->id = NULL;
    message->originid = NULL;
    message->stanzaid = NULL;
    message->replace_id = NULL;
    message->body = NULL;
    message->encrypted = NULL;
    message->plain = NULL;
    message->enc = PROF_MSG_ENC_NONE;
    message->timestamp = NULL;
    message->trusted = true;
    message->type = PROF_MSG_TYPE_UNINITIALIZED;

    return message;
}

void
message_free(ProfMessage* message)
{
    xmpp_ctx_t* ctx = connection_get_ctx();
    if (message->from_jid) {
        jid_destroy(message->from_jid);
    }

    if (message->to_jid) {
        jid_destroy(message->to_jid);
    }

    if (message->id) {
        xmpp_free(ctx, message->id);
    }

    if (message->originid) {
        xmpp_free(ctx, message->originid);
    }

    if (message->stanzaid) {
        xmpp_free(ctx, message->stanzaid);
    }

    if (message->replace_id) {
        xmpp_free(ctx, message->replace_id);
    }

    if (message->body) {
        xmpp_free(ctx, message->body);
    }

    if (message->encrypted) {
        xmpp_free(ctx, message->encrypted);
    }

    if (message->plain) {
        free(message->plain);
    }

    if (message->timestamp) {
        g_date_time_unref(message->timestamp);
    }

    free(message);
}

void
message_handlers_clear(void)
{
    if (pubsub_event_handlers) {
        g_hash_table_remove_all(pubsub_event_handlers);
    }
}

void
message_pubsub_event_handler_add(const char* const node, ProfMessageCallback func, ProfMessageFreeCallback free_func, void* userdata)
{
    ProfMessageHandler* handler = malloc(sizeof(ProfMessageHandler));
    handler->func = func;
    handler->free_func = free_func;
    handler->userdata = userdata;

    g_hash_table_insert(pubsub_event_handlers, strdup(node), handler);
}

char*
message_send_chat(const char* const barejid, const char* const msg, const char* const oob_url, gboolean request_receipt, const char* const replace_id)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();

    char* state = chat_session_get_state(barejid);
    char* jid = chat_session_get_jid(barejid);
    char* id = connection_create_stanza_id();

    xmpp_stanza_t* message = xmpp_message_new(ctx, STANZA_TYPE_CHAT, jid, id);
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

    if (replace_id) {
        stanza_attach_correction(ctx, message, replace_id);
    }

    _send_message_stanza(message);
    xmpp_stanza_release(message);

    return id;
}

char*
message_send_chat_pgp(const char* const barejid, const char* const msg, gboolean request_receipt, const char* const replace_id)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();

    char* state = chat_session_get_state(barejid);
    char* jid = chat_session_get_jid(barejid);
    char* id = connection_create_stanza_id();

    xmpp_stanza_t* message = NULL;
#ifdef HAVE_LIBGPGME
    char* account_name = session_get_account_name();
    ProfAccount* account = accounts_get_account(account_name);
    if (account->pgp_keyid) {
        Jid* jidp = jid_create(jid);
        char* encrypted = p_gpg_encrypt(jidp->barejid, msg, account->pgp_keyid);
        if (encrypted) {
            message = xmpp_message_new(ctx, STANZA_TYPE_CHAT, jid, id);
            xmpp_message_set_body(message, "This message is encrypted (XEP-0027).");
            xmpp_stanza_t* x = xmpp_stanza_new(ctx);
            xmpp_stanza_set_name(x, STANZA_NAME_X);
            xmpp_stanza_set_ns(x, STANZA_NS_ENCRYPTED);
            xmpp_stanza_t* enc_st = xmpp_stanza_new(ctx);
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
    // ?
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

    if (replace_id) {
        stanza_attach_correction(ctx, message, replace_id);
    }

    _send_message_stanza(message);
    xmpp_stanza_release(message);

    return id;
}

// XEP-0373: OpenPGP for XMPP
char*
message_send_chat_ox(const char* const barejid, const char* const msg, gboolean request_receipt, const char* const replace_id)
{
#ifdef HAVE_LIBGPGME
    xmpp_ctx_t* const ctx = connection_get_ctx();

    char* state = chat_session_get_state(barejid);
    char* jid = chat_session_get_jid(barejid);
    char* id = connection_create_stanza_id();

    xmpp_stanza_t* message = NULL;
    Jid* jidp = jid_create(jid);

    char* account_name = session_get_account_name();
    ProfAccount* account = accounts_get_account(account_name);

    message = xmpp_message_new(ctx, STANZA_TYPE_CHAT, jid, id);
    xmpp_message_set_body(message, "This message is encrypted (XEP-0373: OpenPGP for XMPP).");

    xmpp_stanza_t* openpgp = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(openpgp, STANZA_NAME_OPENPGP);
    xmpp_stanza_set_ns(openpgp, STANZA_NS_OPENPGP_0);

    xmpp_stanza_t* signcrypt = _openpgp_signcrypt(ctx, barejid, msg);
    char* c;
    size_t s;
    xmpp_stanza_to_text(signcrypt, &c, &s);
    char* signcrypt_e = p_ox_gpg_signcrypt(account->jid, barejid, c);
    if (signcrypt_e == NULL) {
        log_error("Message not signcrypted.");
        return NULL;
    }
    // BASE64_OPENPGP_MESSAGE
    xmpp_stanza_t* base64_openpgp_message = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(base64_openpgp_message, signcrypt_e);
    xmpp_stanza_add_child(openpgp, base64_openpgp_message);
    xmpp_stanza_add_child(message, openpgp);

    xmpp_stanza_to_text(message, &c, &s);

    account_free(account);
    jid_destroy(jidp);
    free(jid);

    if (state) {
        stanza_attach_state(ctx, message, state);
    }

    if (request_receipt) {
        stanza_attach_receipt_request(ctx, message);
    }

    if (replace_id) {
        stanza_attach_correction(ctx, message, replace_id);
    }

    _send_message_stanza(message);
    xmpp_stanza_release(message);

    return id;
#endif // HAVE_LIBGPGME
    return NULL;
}

char*
message_send_chat_otr(const char* const barejid, const char* const msg, gboolean request_receipt, const char* const replace_id)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();

    char* state = chat_session_get_state(barejid);
    char* jid = chat_session_get_jid(barejid);
    char* id = connection_create_stanza_id();

    xmpp_stanza_t* message = xmpp_message_new(ctx, STANZA_TYPE_CHAT, barejid, id);
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

    if (replace_id) {
        stanza_attach_correction(ctx, message, replace_id);
    }

    _send_message_stanza(message);
    xmpp_stanza_release(message);

    return id;
}

#ifdef HAVE_OMEMO
char*
message_send_chat_omemo(const char* const jid, uint32_t sid, GList* keys,
                        const unsigned char* const iv, size_t iv_len,
                        const unsigned char* const ciphertext, size_t ciphertext_len,
                        gboolean request_receipt, gboolean muc, const char* const replace_id)
{
    char* state = chat_session_get_state(jid);
    xmpp_ctx_t* const ctx = connection_get_ctx();
    char* id;
    xmpp_stanza_t* message;
    if (muc) {
        id = connection_create_stanza_id();
        message = xmpp_message_new(ctx, STANZA_TYPE_GROUPCHAT, jid, id);
        stanza_attach_origin_id(ctx, message, id);
    } else {
        id = connection_create_stanza_id();
        message = xmpp_message_new(ctx, STANZA_TYPE_CHAT, jid, id);
    }

    xmpp_stanza_t* encrypted = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(encrypted, "encrypted");
    xmpp_stanza_set_ns(encrypted, STANZA_NS_OMEMO);

    xmpp_stanza_t* header = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(header, "header");
    char* sid_text = g_strdup_printf("%d", sid);
    log_debug("[OMEMO] Sending from device sid %s", sid_text);
    xmpp_stanza_set_attribute(header, "sid", sid_text);
    g_free(sid_text);

    GList* key_iter;
    for (key_iter = keys; key_iter != NULL; key_iter = key_iter->next) {
        omemo_key_t* key = (omemo_key_t*)key_iter->data;

        xmpp_stanza_t* key_stanza = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(key_stanza, "key");
        char* rid = g_strdup_printf("%d", key->device_id);
        log_debug("[OMEMO] Sending to device rid %s", rid == NULL ? "NULL" : rid);
        xmpp_stanza_set_attribute(key_stanza, "rid", rid);
        g_free(rid);
        if (key->prekey) {
            xmpp_stanza_set_attribute(key_stanza, "prekey", "true");
        }

        gchar* key_raw = g_base64_encode(key->data, key->length);
        xmpp_stanza_t* key_text = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(key_text, key_raw);
        g_free(key_raw);

        xmpp_stanza_add_child(key_stanza, key_text);
        xmpp_stanza_add_child(header, key_stanza);
        xmpp_stanza_release(key_text);
        xmpp_stanza_release(key_stanza);
    }

    xmpp_stanza_t* iv_stanza = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(iv_stanza, "iv");

    gchar* iv_raw = g_base64_encode(iv, iv_len);
    xmpp_stanza_t* iv_text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(iv_text, iv_raw);
    g_free(iv_raw);

    xmpp_stanza_add_child(iv_stanza, iv_text);
    xmpp_stanza_add_child(header, iv_stanza);
    xmpp_stanza_release(iv_text);
    xmpp_stanza_release(iv_stanza);

    xmpp_stanza_add_child(encrypted, header);
    xmpp_stanza_release(header);

    xmpp_stanza_t* payload = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(payload, "payload");

    gchar* ciphertext_raw = g_base64_encode(ciphertext, ciphertext_len);
    xmpp_stanza_t* payload_text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(payload_text, ciphertext_raw);
    g_free(ciphertext_raw);

    xmpp_stanza_add_child(payload, payload_text);
    xmpp_stanza_add_child(encrypted, payload);
    xmpp_stanza_release(payload_text);
    xmpp_stanza_release(payload);

    xmpp_stanza_add_child(message, encrypted);
    xmpp_stanza_release(encrypted);

    xmpp_stanza_t* body = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(body, "body");
    xmpp_stanza_t* body_text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(body_text, "You received a message encrypted with OMEMO but your client doesn't support OMEMO.");
    xmpp_stanza_add_child(body, body_text);
    xmpp_stanza_release(body_text);
    xmpp_stanza_add_child(message, body);
    xmpp_stanza_release(body);

    if (state) {
        stanza_attach_state(ctx, message, state);
    }

    stanza_attach_hints_store(ctx, message);

    if (request_receipt) {
        stanza_attach_receipt_request(ctx, message);
    }

    if (replace_id) {
        stanza_attach_correction(ctx, message, replace_id);
    }

    _send_message_stanza(message);
    xmpp_stanza_release(message);

    return id;
}
#endif

char*
message_send_private(const char* const fulljid, const char* const msg, const char* const oob_url)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();
    char* id = connection_create_stanza_id();

    xmpp_stanza_t* message = xmpp_message_new(ctx, STANZA_TYPE_CHAT, fulljid, id);
    xmpp_message_set_body(message, msg);

    if (oob_url) {
        stanza_attach_x_oob_url(ctx, message, oob_url);
    }

    _send_message_stanza(message);
    xmpp_stanza_release(message);

    return id;
}

char*
message_send_groupchat(const char* const roomjid, const char* const msg, const char* const oob_url, const char* const replace_id)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();
    char* id = connection_create_stanza_id();

    xmpp_stanza_t* message = xmpp_message_new(ctx, STANZA_TYPE_GROUPCHAT, roomjid, id);
    stanza_attach_origin_id(ctx, message, id);
    xmpp_message_set_body(message, msg);

    if (oob_url) {
        stanza_attach_x_oob_url(ctx, message, oob_url);
    }

    if (replace_id) {
        stanza_attach_correction(ctx, message, replace_id);
    }

    _send_message_stanza(message);
    xmpp_stanza_release(message);

    return id;
}

void
message_send_groupchat_subject(const char* const roomjid, const char* const subject)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();
    xmpp_stanza_t* message = stanza_create_room_subject_message(ctx, roomjid, subject);

    _send_message_stanza(message);
    xmpp_stanza_release(message);
}

void
message_send_invite(const char* const roomjid, const char* const contact,
                    const char* const reason)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();
    xmpp_stanza_t* stanza;

    muc_member_type_t member_type = muc_member_type(roomjid);
    if (member_type == MUC_MEMBER_TYPE_PUBLIC) {
        log_debug("Sending direct invite to %s, for %s", contact, roomjid);
        char* password = muc_password(roomjid);
        stanza = stanza_create_invite(ctx, roomjid, contact, reason, password);
    } else {
        log_debug("Sending mediated invite to %s, for %s", contact, roomjid);
        stanza = stanza_create_mediated_invite(ctx, roomjid, contact, reason);
    }

    _send_message_stanza(stanza);
    xmpp_stanza_release(stanza);
}

void
message_send_composing(const char* const jid)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();

    xmpp_stanza_t* stanza = stanza_create_chat_state(ctx, jid, STANZA_NAME_COMPOSING);
    _send_message_stanza(stanza);
    xmpp_stanza_release(stanza);
}

void
message_send_paused(const char* const jid)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();
    xmpp_stanza_t* stanza = stanza_create_chat_state(ctx, jid, STANZA_NAME_PAUSED);
    _send_message_stanza(stanza);
    xmpp_stanza_release(stanza);
}

void
message_send_inactive(const char* const jid)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();
    xmpp_stanza_t* stanza = stanza_create_chat_state(ctx, jid, STANZA_NAME_INACTIVE);

    _send_message_stanza(stanza);
    xmpp_stanza_release(stanza);
}

void
message_send_gone(const char* const jid)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();
    xmpp_stanza_t* stanza = stanza_create_chat_state(ctx, jid, STANZA_NAME_GONE);
    _send_message_stanza(stanza);
    xmpp_stanza_release(stanza);
}

static void
_handle_error(xmpp_stanza_t* const stanza)
{
    const char* id = xmpp_stanza_get_id(stanza);
    const char* jid = xmpp_stanza_get_from(stanza);
    xmpp_stanza_t* error_stanza = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_ERROR);
    const char* type = NULL;
    if (error_stanza) {
        type = xmpp_stanza_get_type(error_stanza);
    }

    // stanza_get_error never returns NULL
    char* err_msg = stanza_get_error_message(stanza);

    GString* log_msg = g_string_new("message stanza error received");
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
    } else {
        if (type && (strcmp(type, "cancel") == 0)) {
            Jid* jidp = jid_create(jid);
            if (jidp) {
                chat_session_remove(jidp->barejid);
                jid_destroy(jidp);
            }
        }
        ui_handle_recipient_error(jid, err_msg);
    }

    free(err_msg);
}

static void
_handle_muc_user(xmpp_stanza_t* const stanza)
{
    xmpp_ctx_t* ctx = connection_get_ctx();
    xmpp_stanza_t* xns_muc_user = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
    const char* room = xmpp_stanza_get_from(stanza);

    if (!xns_muc_user) {
        return;
    }

    if (!room) {
        log_warning("Message received with no from attribute, ignoring");
        return;
    }

    // XEP-0045
    xmpp_stanza_t* invite = xmpp_stanza_get_child_by_name(xns_muc_user, STANZA_NAME_INVITE);
    if (!invite) {
        return;
    }

    const char* invitor_jid = xmpp_stanza_get_from(invite);
    if (!invitor_jid) {
        log_warning("Chat room invite received with no from attribute");
        return;
    }

    Jid* jidp = jid_create(invitor_jid);
    if (!jidp) {
        return;
    }
    char* invitor = jidp->barejid;

    char* reason = NULL;
    xmpp_stanza_t* reason_st = xmpp_stanza_get_child_by_name(invite, STANZA_NAME_REASON);
    if (reason_st) {
        reason = xmpp_stanza_get_text(reason_st);
    }

    char* password = NULL;
    xmpp_stanza_t* password_st = xmpp_stanza_get_child_by_name(xns_muc_user, STANZA_NAME_PASSWORD);
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
_handle_conference(xmpp_stanza_t* const stanza)
{
    xmpp_stanza_t* xns_conference = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_CONFERENCE);

    if (xns_conference) {
        // XEP-0249
        const char* room = xmpp_stanza_get_attribute(xns_conference, STANZA_ATTR_JID);
        if (!room) {
            return;
        }

        const char* from = xmpp_stanza_get_from(stanza);
        if (!from) {
            log_warning("Message received with no from attribute, ignoring");
            return;
        }

        Jid* jidp = jid_create(from);
        if (!jidp) {
            return;
        }

        // reason and password are both optional
        const char* reason = xmpp_stanza_get_attribute(xns_conference, STANZA_ATTR_REASON);
        const char* password = xmpp_stanza_get_attribute(xns_conference, STANZA_ATTR_PASSWORD);

        sv_ev_room_invite(INVITE_DIRECT, jidp->barejid, room, reason, password);
        jid_destroy(jidp);
    }
}

static void
_handle_captcha(xmpp_stanza_t* const stanza)
{
    xmpp_ctx_t* ctx = connection_get_ctx();
    const char* from = xmpp_stanza_get_from(stanza);

    if (!from) {
        log_warning("Message received with no from attribute, ignoring");
        return;
    }

    // XEP-0158
    char* message = xmpp_message_get_body(stanza);
    if (!message) {
        return;
    }

    sv_ev_room_broadcast(from, message);
    xmpp_free(ctx, message);
}

static void
_handle_groupchat(xmpp_stanza_t* const stanza)
{
    xmpp_ctx_t* ctx = connection_get_ctx();

    const char* room_jid = xmpp_stanza_get_from(stanza);
    if (!room_jid) {
        return;
    }
    Jid* from_jid = jid_create(room_jid);
    if (!from_jid) {
        return;
    }

    // handle room subject
    xmpp_stanza_t* subject = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_SUBJECT);
    if (subject) {
        // subject_text is optional, can be NULL
        char* subject_text = xmpp_stanza_get_text(subject);
        sv_ev_room_subject(from_jid->barejid, from_jid->resourcepart, subject_text);
        xmpp_free(ctx, subject_text);

        jid_destroy(from_jid);
        return;
    }

    // handle room broadcasts
    if (!from_jid->resourcepart) {
        char* broadcast;
        broadcast = xmpp_message_get_body(stanza);
        if (!broadcast) {
            jid_destroy(from_jid);
            return;
        }

        sv_ev_room_broadcast(room_jid, broadcast);
        xmpp_free(ctx, broadcast);

        jid_destroy(from_jid);
        return;
    }

    if (!jid_is_valid_room_form(from_jid)) {
        log_error("Invalid room JID: %s", from_jid->str);
        jid_destroy(from_jid);
        return;
    }

    // room not active in profanity
    if (!muc_active(from_jid->barejid)) {
        log_error("Message received for inactive chat room: %s", from_jid->str);
        jid_destroy(from_jid);
        return;
    }

    ProfMessage* message = message_init();
    message->from_jid = from_jid;
    message->type = PROF_MSG_TYPE_MUC;

    const char* id = xmpp_stanza_get_id(stanza);
    if (id) {
        message->id = strdup(id);
    }

    char* stanzaid = NULL;
    xmpp_stanza_t* stanzaidst = xmpp_stanza_get_child_by_name_and_ns(stanza, STANZA_NAME_STANZA_ID, STANZA_NS_STABLE_ID);
    if (stanzaidst) {
        stanzaid = (char*)xmpp_stanza_get_attribute(stanzaidst, STANZA_ATTR_ID);
        if (stanzaid) {
            message->stanzaid = strdup(stanzaid);
        }
    }

    xmpp_stanza_t* origin = xmpp_stanza_get_child_by_name_and_ns(stanza, STANZA_NAME_ORIGIN_ID, STANZA_NS_STABLE_ID);
    if (origin) {
        char* originid = (char*)xmpp_stanza_get_attribute(origin, STANZA_ATTR_ID);
        if (originid) {
            message->originid = strdup(originid);
        }
    }

    xmpp_stanza_t* replace_id_stanza = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_LAST_MESSAGE_CORRECTION);
    if (replace_id_stanza) {
        const char* replace_id = xmpp_stanza_get_id(replace_id_stanza);
        if (replace_id) {
            message->replace_id = strdup(replace_id);
        }
    }

    message->body = xmpp_message_get_body(stanza);

    // check omemo encryption
#ifdef HAVE_OMEMO
    message->plain = omemo_receive_message(stanza, &message->trusted);
    if (message->plain != NULL) {
        message->enc = PROF_MSG_ENC_OMEMO;
    }
#endif

    if (!message->plain && !message->body) {
        log_info("Message received without body for room: %s", from_jid->str);
        goto out;
    } else if (!message->plain) {
        message->plain = strdup(message->body);
    }

    // determine if the notifications happened whilst offline (MUC history)
    message->timestamp = stanza_get_delay_from(stanza, from_jid->barejid);
    if (message->timestamp == NULL) {
        // checking the domainpart is a workaround for some prosody versions (gh#1190)
        message->timestamp = stanza_get_delay_from(stanza, from_jid->domainpart);
    }

    bool is_muc_history = FALSE;
    if (message->timestamp != NULL) {
        is_muc_history = TRUE;
        g_date_time_unref(message->timestamp);
        message->timestamp = NULL;
    }

    // we want to display the oldest delay
    message->timestamp = stanza_get_oldest_delay(stanza);

    // now this has nothing to do with MUC history
    // it's just setting the time to the received time so upon displaying we can use this time
    // for example in win_println_incoming_muc_msg()
    if (!message->timestamp) {
        message->timestamp = g_date_time_new_now_local();
    }

    if (is_muc_history) {
        sv_ev_room_history(message);
    } else {
        sv_ev_room_message(message);
    }

out:
    message_free(message);
}

static void
_message_send_receipt(const char* const fulljid, const char* const message_id)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();

    char* id = connection_create_stanza_id();
    xmpp_stanza_t* message = xmpp_message_new(ctx, NULL, fulljid, id);
    free(id);

    xmpp_stanza_t* receipt = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(receipt, "received");
    xmpp_stanza_set_ns(receipt, STANZA_NS_RECEIPTS);
    xmpp_stanza_set_id(receipt, message_id);

    xmpp_stanza_add_child(message, receipt);
    xmpp_stanza_release(receipt);

    _send_message_stanza(message);
    xmpp_stanza_release(message);
}

static void
_handle_receipt_received(xmpp_stanza_t* const stanza)
{
    xmpp_stanza_t* receipt = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_RECEIPTS);
    if (receipt) {
        const char* name = xmpp_stanza_get_name(receipt);
        if ((name == NULL) || (g_strcmp0(name, "received") != 0)) {
            return;
        }

        const char* id = xmpp_stanza_get_id(receipt);
        if (!id) {
            return;
        }

        const char* fulljid = xmpp_stanza_get_from(stanza);
        if (!fulljid) {
            return;
        }

        Jid* jidp = jid_create(fulljid);
        if (!jidp) {
            return;
        }

        sv_ev_message_receipt(jidp->barejid, id);
        jid_destroy(jidp);
    }
}

static void
_receipt_request_handler(xmpp_stanza_t* const stanza)
{
    if (!prefs_get_boolean(PREF_RECEIPTS_SEND)) {
        return;
    }

    const char* id = xmpp_stanza_get_id(stanza);
    if (!id) {
        return;
    }

    xmpp_stanza_t* receipts = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_RECEIPTS);
    if (!receipts) {
        return;
    }

    const char* receipts_name = xmpp_stanza_get_name(receipts);
    if ((receipts_name == NULL) || (g_strcmp0(receipts_name, "request") != 0)) {
        return;
    }

    const gchar* from = xmpp_stanza_get_from(stanza);
    if (from) {
        Jid* jid = jid_create(from);
        if (jid) {
            _message_send_receipt(jid->fulljid, id);
            jid_destroy(jid);
        }
    }
}

static void
_handle_muc_private_message(xmpp_stanza_t* const stanza)
{
    // standard chat message, use jid without resource
    ProfMessage* message = message_init();
    message->type = PROF_MSG_TYPE_MUCPM;

    const gchar* from = xmpp_stanza_get_from(stanza);
    if (!from) {
        goto out;
    }

    message->from_jid = jid_create(from);
    if (!message->from_jid) {
        goto out;
    }

    // message stanza id
    const char* id = xmpp_stanza_get_id(stanza);
    if (id) {
        message->id = strdup(id);
    }

    // check omemo encryption
#ifdef HAVE_OMEMO
    message->plain = omemo_receive_message(stanza, &message->trusted);
    if (message->plain != NULL) {
        message->enc = PROF_MSG_ENC_OMEMO;
    }
#endif

    message->timestamp = stanza_get_delay(stanza);
    message->body = xmpp_message_get_body(stanza);

    if (!message->plain && !message->body) {
        log_info("Message received without body from: %s", message->from_jid->str);
        goto out;
    } else if (!message->plain) {
        message->plain = strdup(message->body);
    }

    if (message->timestamp) {
        sv_ev_delayed_private_message(message);
    } else {
        message->timestamp = g_date_time_new_now_local();

        sv_ev_incoming_private_message(message);
    }

out:
    message_free(message);
}

static xmpp_stanza_t*
_handle_carbons(xmpp_stanza_t* const stanza)
{
    const char* name = xmpp_stanza_get_name(stanza);
    if (!name) {
        log_error("Unable to retrieve stanza name for Carbon");
        return NULL;
    }

    /* TODO: private shouldn't arrive at the client, should it?
    if (g_strcmp0(name, "private") == 0) {
        log_info("Carbon received with private element.");
    }
    */

    if ((g_strcmp0(name, STANZA_NAME_RECEIVED) != 0) && (g_strcmp0(name, STANZA_NAME_SENT) != 0)) {
        log_warning("Carbon received with unrecognised stanza name: %s", name);
        return NULL;
    }

    xmpp_stanza_t* forwarded = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_FORWARD);
    if (!forwarded) {
        log_warning("Carbon received with no forwarded element");
        return NULL;
    }

    xmpp_stanza_t* message_stanza = xmpp_stanza_get_child_by_name(forwarded, STANZA_NAME_MESSAGE);
    if (!message_stanza) {
        log_warning("Carbon received with no message element");
        return NULL;
    }

    return message_stanza;
}

static void
_handle_chat(xmpp_stanza_t* const stanza, gboolean is_mam, gboolean is_carbon, const char* result_id, GDateTime* timestamp)
{
    // some clients send the mucuser namespace with private messages
    // if the namespace exists, and the stanza contains a body element, assume its a private message
    // otherwise exit the handler
    xmpp_stanza_t* mucuser = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_MUC_USER);
    xmpp_stanza_t* body = xmpp_stanza_get_child_by_name(stanza, STANZA_NAME_BODY);
    if (mucuser && body == NULL) {
        return;
    }

    const gchar* from = xmpp_stanza_get_from(stanza);
    if (!from) {
        return;
    }
    Jid* jid = jid_create(from);
    if (!jid) {
        return;
    }

    // private message from chat room use full jid (room/nick)
    if (muc_active(jid->barejid)) {
        _handle_muc_private_message(stanza);
        jid_destroy(jid);
        return;
    }

    // standard chat message, use jid without resource
    ProfMessage* message = message_init();
    message->is_mam = is_mam;
    message->from_jid = jid;
    const gchar* to = xmpp_stanza_get_to(stanza);
    if (to) {
        message->to_jid = jid_create(to);
    } else if (is_carbon) {
        // happens when receive a carbon of a self sent message
        // really? maybe some servers do this, but it's not required.
        message->to_jid = jid_create(from);
    }

    if (mucuser) {
        message->type = PROF_MSG_TYPE_MUCPM;
    } else {
        message->type = PROF_MSG_TYPE_CHAT;
    }

    // message stanza id
    const char* id = xmpp_stanza_get_id(stanza);
    if (id) {
        message->id = strdup(id);
    }

    if (is_mam) {
        // MAM has XEP-0359 stanza-id as <result id="">
        if (result_id) {
            message->stanzaid = strdup(result_id);
        } else {
            log_warning("MAM received with no result id");
        }
    } else {
        // live messages use XEP-0359 <stanza-id>
        char* stanzaid = NULL;
        xmpp_stanza_t* stanzaidst = xmpp_stanza_get_child_by_name_and_ns(stanza, STANZA_NAME_STANZA_ID, STANZA_NS_STABLE_ID);
        if (stanzaidst) {
            stanzaid = (char*)xmpp_stanza_get_attribute(stanzaidst, STANZA_ATTR_ID);
            if (stanzaid) {
                message->stanzaid = strdup(stanzaid);
            }
        }
    }

    // replace id for XEP-0308: Last Message Correction
    xmpp_stanza_t* replace_id_stanza = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_LAST_MESSAGE_CORRECTION);
    if (replace_id_stanza) {
        const char* replace_id = xmpp_stanza_get_id(replace_id_stanza);
        if (replace_id) {
            message->replace_id = strdup(replace_id);
        }
    }

    if (timestamp) {
        // timestamp provided outside like in a <forwarded> by MAM
        message->timestamp = timestamp;
    } else {
        // timestamp in the message stanza or use time of receival (now)
        message->timestamp = stanza_get_delay(stanza);
        if (!message->timestamp) {
            message->timestamp = g_date_time_new_now_local();
        }
    }

    if (body) {
        message->body = xmpp_stanza_get_text(body);
    }

#ifdef HAVE_OMEMO
    // check omemo encryption
    message->plain = omemo_receive_message(stanza, &message->trusted);
    if (message->plain != NULL) {
        message->enc = PROF_MSG_ENC_OMEMO;
    }
#endif

    xmpp_stanza_t* encrypted = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_ENCRYPTED);
    if (encrypted) {
        message->encrypted = xmpp_stanza_get_text(encrypted);
    }

    xmpp_stanza_t* ox = xmpp_stanza_get_child_by_ns(stanza, STANZA_NS_OPENPGP_0);
    if (ox) {
        _handle_ox_chat(stanza, message, FALSE);
    }

    if (message->plain || message->body || message->encrypted) {
        if (is_carbon) {
            char* mybarejid = connection_get_barejid();

            // if we are the recipient, treat as standard incoming message
            if (g_strcmp0(mybarejid, message->to_jid->barejid) == 0) {
                sv_ev_incoming_carbon(message);
                // else treat as a sent message
            } else {
                sv_ev_outgoing_carbon(message);
            }

            free(mybarejid);
        } else {
            sv_ev_incoming_message(message);
            _receipt_request_handler(stanza);
        }
    }

    // 0085 works only with resource
    if (jid->resourcepart) {
        // XEP-0085: Chat State Notifications
        _handle_chat_states(stanza, jid);
    }

    message_free(message);
}

/*!
 * @brief Handle incoming XMMP-OX chat message.
 */
static void
_handle_ox_chat(xmpp_stanza_t* const stanza, ProfMessage* message, gboolean is_mam)
{
    message->enc = PROF_MSG_ENC_OX;

#ifdef HAVE_LIBGPGME
    xmpp_stanza_t* ox = xmpp_stanza_get_child_by_name_and_ns(stanza, "openpgp", STANZA_NS_OPENPGP_0);
    if (ox) {
        message->plain = p_ox_gpg_decrypt(xmpp_stanza_get_text(ox));
        if (message->plain) {
            xmpp_stanza_t* x = xmpp_stanza_new_from_string(connection_get_ctx(), message->plain);
            if (x) {
                xmpp_stanza_t* p = xmpp_stanza_get_child_by_name(x, "payload");
                if (!p) {
                    log_warning("OX Stanza - no Payload");
                    message->plain = "OX error: No payload found";
                    return;
                }
                xmpp_stanza_t* b = xmpp_stanza_get_child_by_name(p, "body");
                if (!b) {
                    log_warning("OX Stanza - no body");
                    message->plain = "OX error: No paylod body found";
                    return;
                }
                message->plain = xmpp_stanza_get_text(b);
                message->encrypted = xmpp_stanza_get_text(ox);
                if (message->plain == NULL) {
                    message->plain = xmpp_stanza_get_text(stanza);
                }
            } else {
                message->plain = "Unable to decrypt OX message (XEP-0373: OpenPGP for XMPP)";
                log_warning("OX Stanza text to stanza failed");
            }
        } else {
            message->plain = "Unable to decrypt OX message (XEP-0373: OpenPGP for XMPP)";
        }
    } else {
        message->plain = "OX stanza without openpgp name";
        log_warning("OX Stanza without openpgp stanza");
    }
#endif // HAVE_LIBGPGME
}

static gboolean
_handle_mam(xmpp_stanza_t* const stanza)
{
    xmpp_stanza_t* result = xmpp_stanza_get_child_by_name_and_ns(stanza, STANZA_NAME_RESULT, STANZA_NS_MAM2);
    if (!result) {
        return FALSE;
    }

    xmpp_stanza_t* forwarded = xmpp_stanza_get_child_by_ns(result, STANZA_NS_FORWARD);
    if (!forwarded) {
        log_warning("MAM received with no forwarded element");
        return FALSE;
    }

    // <result xmlns='urn:xmpp:mam:2' queryid='f27' id='5d398-28273-f7382'>
    // same as <stanza-id> from XEP-0359 for live messages
    const char* result_id = xmpp_stanza_get_id(result);

    GDateTime* timestamp = stanza_get_delay_from(forwarded, NULL);

    xmpp_stanza_t* message_stanza = xmpp_stanza_get_child_by_ns(forwarded, "jabber:client");

    _handle_chat(message_stanza, TRUE, FALSE, result_id, timestamp);

    return TRUE;
}

static void
_handle_pubsub(xmpp_stanza_t* const stanza, xmpp_stanza_t* const event)
{
    xmpp_stanza_t* child = xmpp_stanza_get_children(event);
    if (child) {
        const char* node = xmpp_stanza_get_attribute(child, STANZA_ATTR_NODE);
        if (node) {
            ProfMessageHandler* handler = g_hash_table_lookup(pubsub_event_handlers, node);
            if (handler) {
                int keep = handler->func(stanza, handler->userdata);
                if (!keep) {
                    g_hash_table_remove(pubsub_event_handlers, node);
                }
            }
        }
    }
}

static void
_send_message_stanza(xmpp_stanza_t* const stanza)
{
    char* text;
    size_t text_size;
    xmpp_stanza_to_text(stanza, &text, &text_size);

    xmpp_conn_t* conn = connection_get_conn();
    char* plugin_text = plugins_on_message_stanza_send(text);
    if (plugin_text) {
        xmpp_send_raw_string(conn, "%s", plugin_text);
        free(plugin_text);
    } else {
        xmpp_send_raw_string(conn, "%s", text);
    }
    xmpp_free(connection_get_ctx(), text);
}

/* ckeckOID = true: check origin-id
 * checkOID = false: check regular id
 */
bool
message_is_sent_by_us(const ProfMessage* const message, bool checkOID)
{
    bool ret = FALSE;

    // we check the </origin-id> for this we calculate a hash into it so we can detect
    // whether this client sent it. See connection_create_stanza_id()
    if (message) {
        char* tmp_id = NULL;

        if (checkOID && message->originid != NULL) {
            tmp_id = message->originid;
        } else if (!checkOID && message->id != NULL) {
            tmp_id = message->id;
        }

        if (tmp_id != NULL) {
            gsize tmp_len = strlen(tmp_id);

            // our client sents at CON_RAND_ID_LEN + identifier
            if (tmp_len > CON_RAND_ID_LEN) {
                char* uuid = g_strndup(tmp_id, CON_RAND_ID_LEN);
                const char* prof_identifier = connection_get_profanity_identifier();

                gchar* hmac = g_compute_hmac_for_string(G_CHECKSUM_SHA1,
                                                        (guchar*)prof_identifier, strlen(prof_identifier),
                                                        uuid, strlen(uuid));

                if (g_strcmp0(&tmp_id[CON_RAND_ID_LEN], hmac) == 0) {
                    ret = TRUE;
                }

                g_free(uuid);
                g_free(hmac);
            }
        }
    }

    return ret;
}

#ifdef HAVE_LIBGPGME
static xmpp_stanza_t*
_openpgp_signcrypt(xmpp_ctx_t* ctx, const char* const to, const char* const text)
{
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    char buf[255];
    strftime(buf, sizeof(buf), "%FT%T%z", tm);
    int randnr = rand() % 5;
    char rpad_data[randnr];

    for (int i = 0; i < randnr - 1; i++) {
        rpad_data[i] = 'c';
    }
    rpad_data[randnr - 1] = '\0';

    // signcrypt
    xmpp_stanza_t* signcrypt = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(signcrypt, "signcrypt");
    xmpp_stanza_set_ns(signcrypt, "urn:xmpp:openpgp:0");
    // to
    xmpp_stanza_t* s_to = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(s_to, "to");
    xmpp_stanza_set_attribute(s_to, "jid", to);
    // time
    xmpp_stanza_t* time = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(time, "time");
    xmpp_stanza_set_attribute(time, "stamp", buf);
    xmpp_stanza_set_name(time, "time");
    // rpad
    xmpp_stanza_t* rpad = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(rpad, "rpad");
    xmpp_stanza_t* rpad_text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(rpad_text, rpad_data);
    // payload
    xmpp_stanza_t* payload = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(payload, "payload");
    // body
    xmpp_stanza_t* body = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(body, "body");
    xmpp_stanza_set_ns(body, "jabber:client");
    // text
    xmpp_stanza_t* body_text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(body_text, text);
    xmpp_stanza_add_child(signcrypt, s_to);
    xmpp_stanza_add_child(signcrypt, time);
    xmpp_stanza_add_child(signcrypt, rpad);
    xmpp_stanza_add_child(rpad, rpad_text);
    xmpp_stanza_add_child(signcrypt, payload);
    xmpp_stanza_add_child(payload, body);
    xmpp_stanza_add_child(body, body_text);

    xmpp_stanza_release(body_text);
    xmpp_stanza_release(body);
    xmpp_stanza_release(rpad_text);
    xmpp_stanza_release(rpad);
    xmpp_stanza_release(time);
    xmpp_stanza_release(s_to);

    return signcrypt;
}
#endif // HAVE_LIBGPGME

void
message_request_voice(const char* const roomjid)
{
    xmpp_ctx_t* const ctx = connection_get_ctx();
    xmpp_stanza_t* stanza = stanza_request_voice(ctx, roomjid);

    log_debug("Requesting voice in %s", roomjid);

    _send_message_stanza(stanza);
    xmpp_stanza_release(stanza);
}

static gboolean
_handle_jingle_message(xmpp_stanza_t* const stanza)
{
    xmpp_stanza_t* propose = xmpp_stanza_get_child_by_name_and_ns(stanza, STANZA_NAME_PROPOSE, STANZA_NS_JINGLE_MESSAGE);

    if (propose) {
        xmpp_stanza_t* description = xmpp_stanza_get_child_by_ns(propose, STANZA_NS_JINGLE_RTP);
        if (description) {
            const char* const from = xmpp_stanza_get_from(stanza);
            cons_show("Ring ring: %s is trying to call you", from);
            cons_alert(NULL);
            return TRUE;
        }
    }
    return FALSE;
}

static gboolean
_should_ignore_based_on_silence(xmpp_stanza_t* const stanza)
{
    if (prefs_get_boolean(PREF_SILENCE_NON_ROSTER)) {
        const char* const from = xmpp_stanza_get_from(stanza);
        Jid* from_jid = jid_create(from);
        PContact contact = roster_get_contact(from_jid->barejid);
        jid_destroy(from_jid);
        if (!contact) {
            log_debug("[Silence] Ignoring message from: %s", from);
            return TRUE;
        }
    }
    return FALSE;
}
