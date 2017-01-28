/*
 * otr.c
 *
 * Copyright (C) 2012 - 2017 James Booth <boothj5@gmail.com>
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

#include <libotr/proto.h>
#include <libotr/privkey.h>
#include <libotr/message.h>
#include <libotr/sm.h>
#include <glib.h>

#include "log.h"
#include "config/preferences.h"
#include "config/files.h"
#include "otr/otr.h"
#include "otr/otrlib.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/chat_session.h"
#include "xmpp/roster_list.h"
#include "xmpp/contact.h"
#include "xmpp/xmpp.h"

#define PRESENCE_ONLINE 1
#define PRESENCE_OFFLINE 0
#define PRESENCE_UNKNOWN -1

static OtrlUserState user_state;
static OtrlMessageAppOps ops;
static char *jid;
static gboolean data_loaded;
static GHashTable *smp_initiators;

OtrlUserState
otr_userstate(void)
{
    return user_state;
}

OtrlMessageAppOps*
otr_messageops(void)
{
    return &ops;
}

GHashTable*
otr_smpinitators(void)
{
    return smp_initiators;
}

// ops callbacks
static OtrlPolicy
cb_policy(void *opdata, ConnContext *context)
{
    return otrlib_policy();
}

static int
cb_is_logged_in(void *opdata, const char *accountname, const char *protocol, const char *recipient)
{
    jabber_conn_status_t conn_status = connection_get_status();
    if (conn_status != JABBER_CONNECTED) {
        return PRESENCE_OFFLINE;
    }

    PContact contact = roster_get_contact(recipient);

    // not in roster
    if (contact == NULL) {
        return PRESENCE_ONLINE;
    }

    // not subscribed
    if (p_contact_subscribed(contact) == FALSE) {
        return PRESENCE_ONLINE;
    }

    // subscribed
    if (g_strcmp0(p_contact_presence(contact), "offline") == 0) {
        return PRESENCE_OFFLINE;
    } else {
        return PRESENCE_ONLINE;
    }
}

static void
cb_inject_message(void *opdata, const char *accountname,
    const char *protocol, const char *recipient, const char *message)
{
    char *id = message_send_chat_otr(recipient, message, FALSE);
    free(id);
}

static void
cb_write_fingerprints(void *opdata)
{
    gcry_error_t err = 0;

    char *otrdir = files_get_data_path(DIR_OTR);
    GString *basedir = g_string_new(otrdir);
    free(otrdir);
    gchar *account_dir = str_replace(jid, "@", "_at_");
    g_string_append(basedir, "/");
    g_string_append(basedir, account_dir);
    g_string_append(basedir, "/");
    free(account_dir);

    GString *fpsfilename = g_string_new(basedir->str);
    g_string_append(fpsfilename, "fingerprints.txt");
    err = otrl_privkey_write_fingerprints(user_state, fpsfilename->str);
    if (err != GPG_ERR_NO_ERROR) {
        log_error("Failed to write fingerprints file");
        cons_show_error("Failed to create fingerprints file");
    }
    g_string_free(basedir, TRUE);
    g_string_free(fpsfilename, TRUE);
}

static void
cb_gone_secure(void *opdata, ConnContext *context)
{
    ProfChatWin *chatwin = wins_get_chat(context->username);
    if (!chatwin) {
        chatwin = (ProfChatWin*) wins_new_chat(context->username);
    }

    chatwin_otr_secured(chatwin, otr_is_trusted(context->username));
}

char*
otr_libotr_version(void)
{
    return OTRL_VERSION;
}

char*
otr_start_query(void)
{
    return otrlib_start_query();
}

void
otr_init(void)
{
    log_info("Initialising OTR");
    OTRL_INIT;

    jid = NULL;

    ops.policy = cb_policy;
    ops.is_logged_in = cb_is_logged_in;
    ops.inject_message = cb_inject_message;
    ops.write_fingerprints = cb_write_fingerprints;
    ops.gone_secure = cb_gone_secure;

    otrlib_init_ops(&ops);
    otrlib_init_timer();
    smp_initiators = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    data_loaded = FALSE;
}

void
otr_shutdown(void)
{
    if (jid) {
        free(jid);
        jid = NULL;
    }
}

void
otr_poll(void)
{
    otrlib_poll();
}

void
otr_on_connect(ProfAccount *account)
{
    if (jid) {
        free(jid);
    }
    jid = strdup(account->jid);
    log_info("Loading OTR key for %s", jid);

    char *otrdir = files_get_data_path(DIR_OTR);
    GString *basedir = g_string_new(otrdir);
    free(otrdir);
    gchar *account_dir = str_replace(jid, "@", "_at_");
    g_string_append(basedir, "/");
    g_string_append(basedir, account_dir);
    g_string_append(basedir, "/");
    free(account_dir);

    if (!mkdir_recursive(basedir->str)) {
        log_error("Could not create %s for account %s.", basedir->str, jid);
        cons_show_error("Could not create %s for account %s.", basedir->str, jid);
        g_string_free(basedir, TRUE);
        return;
    }

    if (user_state) {
        otrl_userstate_free(user_state);
    }
    user_state = otrl_userstate_create();

    gcry_error_t err = 0;

    GString *keysfilename = g_string_new(basedir->str);
    g_string_append(keysfilename, "keys.txt");
    if (!g_file_test(keysfilename->str, G_FILE_TEST_IS_REGULAR)) {
        log_info("No OTR private key file found %s", keysfilename->str);
        data_loaded = FALSE;
    } else {
        log_info("Loading OTR private key %s", keysfilename->str);
        err = otrl_privkey_read(user_state, keysfilename->str);
        if (err != GPG_ERR_NO_ERROR) {
            log_warning("Failed to read OTR private key file: %s", keysfilename->str);
            cons_show_error("Failed to read OTR private key file: %s", keysfilename->str);
            g_string_free(basedir, TRUE);
            g_string_free(keysfilename, TRUE);
            return;
        }

        OtrlPrivKey* privkey = otrl_privkey_find(user_state, jid, "xmpp");
        if (!privkey) {
            log_warning("No OTR private key found for account \"%s\", protocol \"xmpp\" in file: %s", jid, keysfilename->str);
            cons_show_error("No OTR private key found for account \"%s\", protocol \"xmpp\" in file: %s", jid, keysfilename->str);
            g_string_free(basedir, TRUE);
            g_string_free(keysfilename, TRUE);
            return;
        }
        log_info("Loaded OTR private key");
        data_loaded = TRUE;
    }

    GString *fpsfilename = g_string_new(basedir->str);
    g_string_append(fpsfilename, "fingerprints.txt");
    if (!g_file_test(fpsfilename->str, G_FILE_TEST_IS_REGULAR)) {
        log_info("No OTR fingerprints file found %s", fpsfilename->str);
        data_loaded = FALSE;
    } else {
        log_info("Loading OTR fingerprints %s", fpsfilename->str);
        err = otrl_privkey_read_fingerprints(user_state, fpsfilename->str, NULL, NULL);
        if (err != GPG_ERR_NO_ERROR) {
            log_error("Failed to load OTR fingerprints file: %s", fpsfilename->str);
            g_string_free(basedir, TRUE);
            g_string_free(keysfilename, TRUE);
            g_string_free(fpsfilename, TRUE);
            return;
        } else {
            log_info("Loaded OTR fingerprints");
            data_loaded = TRUE;
        }
    }

    if (data_loaded) {
        cons_show("Loaded OTR private key for %s", jid);
    }

    g_string_free(basedir, TRUE);
    g_string_free(keysfilename, TRUE);
    g_string_free(fpsfilename, TRUE);
    return;
}

char*
otr_on_message_recv(const char *const barejid, const char *const resource, const char *const message, gboolean *decrypted)
{
    prof_otrpolicy_t policy = otr_get_policy(barejid);
    char *whitespace_base = strstr(message, OTRL_MESSAGE_TAG_BASE);

    //check for OTR whitespace (opportunistic or always)
    if (policy == PROF_OTRPOLICY_OPPORTUNISTIC || policy == PROF_OTRPOLICY_ALWAYS) {
        if (whitespace_base) {
            if (strstr(message, OTRL_MESSAGE_TAG_V2) || strstr(message, OTRL_MESSAGE_TAG_V1)) {
                // Remove whitespace pattern for proper display in UI
                // Handle both BASE+TAGV1/2(16+8) and BASE+TAGV1+TAGV2(16+8+8)
                int tag_length = 24;
                if (strstr(message, OTRL_MESSAGE_TAG_V2) && strstr(message, OTRL_MESSAGE_TAG_V1)) {
                    tag_length = 32;
                }
                memmove(whitespace_base, whitespace_base+tag_length, tag_length);
                char *otr_query_message = otr_start_query();
                cons_show("OTR Whitespace pattern detected. Attempting to start OTR session...");
                char *id = message_send_chat_otr(barejid, otr_query_message, FALSE);
                free(id);
            }
        }
    }

    char *newmessage = otr_decrypt_message(barejid, message, decrypted);
    if (!newmessage) { // internal OTR message
        return NULL;
    }

    if (policy == PROF_OTRPOLICY_ALWAYS && *decrypted == FALSE && !whitespace_base) {
        char *otr_query_message = otr_start_query();
        cons_show("Attempting to start OTR session...");
        char *id = message_send_chat_otr(barejid, otr_query_message, FALSE);
        free(id);
    }

    return newmessage;
}

gboolean
otr_on_message_send(ProfChatWin *chatwin, const char *const message, gboolean request_receipt)
{
    char *id = NULL;
    prof_otrpolicy_t policy = otr_get_policy(chatwin->barejid);

    // Send encrypted message
    if (otr_is_secure(chatwin->barejid)) {
        char *encrypted = otr_encrypt_message(chatwin->barejid, message);
        if (encrypted) {
            id = message_send_chat_otr(chatwin->barejid, encrypted, request_receipt);
            chat_log_otr_msg_out(chatwin->barejid, message);
            chatwin_outgoing_msg(chatwin, message, id, PROF_MSG_OTR, request_receipt);
            otr_free_message(encrypted);
            free(id);
            return TRUE;
        } else {
            win_println((ProfWin*)chatwin, THEME_ERROR, '-', "%s", "Failed to encrypt and send message.");
            return TRUE;
        }
    }

    // show error if not secure and policy always
    if (policy == PROF_OTRPOLICY_ALWAYS) {
        win_println((ProfWin*)chatwin, THEME_ERROR, '-', "%s", "Failed to send message. OTR policy set to: always");
        return TRUE;
    }

    // tag and send for policy opportunistic
    if (policy == PROF_OTRPOLICY_OPPORTUNISTIC) {
        char *otr_tagged_msg = otr_tag_message(message);
        id = message_send_chat_otr(chatwin->barejid, otr_tagged_msg, request_receipt);
        chatwin_outgoing_msg(chatwin, message, id, PROF_MSG_PLAIN, request_receipt);
        chat_log_msg_out(chatwin->barejid, message);
        free(otr_tagged_msg);
        free(id);
        return TRUE;
    }

    return FALSE;
}

void
otr_keygen(ProfAccount *account)
{
    if (data_loaded) {
        cons_show("OTR key already generated.");
        return;
    }

    if (jid) {
        free(jid);
    }
    jid = strdup(account->jid);
    log_info("Generating OTR key for %s", jid);

    char *otrdir = files_get_data_path(DIR_OTR);
    GString *basedir = g_string_new(otrdir);
    free(otrdir);
    gchar *account_dir = str_replace(jid, "@", "_at_");
    g_string_append(basedir, "/");
    g_string_append(basedir, account_dir);
    g_string_append(basedir, "/");
    free(account_dir);

    if (!mkdir_recursive(basedir->str)) {
        log_error("Could not create %s for account %s.", basedir->str, jid);
        cons_show_error("Could not create %s for account %s.", basedir->str, jid);
        g_string_free(basedir, TRUE);
        return;
    }

    gcry_error_t err = 0;

    GString *keysfilename = g_string_new(basedir->str);
    g_string_append(keysfilename, "keys.txt");
    log_debug("Generating private key file %s for %s", keysfilename->str, jid);
    cons_show("Generating private key, this may take some time.");
    cons_show("Moving the mouse randomly around the screen may speed up the process!");
    ui_update();
    err = otrl_privkey_generate(user_state, keysfilename->str, account->jid, "xmpp");
    if (err != GPG_ERR_NO_ERROR) {
        g_string_free(basedir, TRUE);
        g_string_free(keysfilename, TRUE);
        log_error("Failed to generate private key");
        cons_show_error("Failed to generate private key");
        return;
    }
    log_info("Private key generated");
    cons_show("");
    cons_show("Private key generation complete.");

    GString *fpsfilename = g_string_new(basedir->str);
    g_string_append(fpsfilename, "fingerprints.txt");
    log_debug("Generating fingerprints file %s for %s", fpsfilename->str, jid);
    err = otrl_privkey_write_fingerprints(user_state, fpsfilename->str);
    if (err != GPG_ERR_NO_ERROR) {
        g_string_free(basedir, TRUE);
        g_string_free(keysfilename, TRUE);
        log_error("Failed to create fingerprints file");
        cons_show_error("Failed to create fingerprints file");
        return;
    }
    log_info("Fingerprints file created");

    err = otrl_privkey_read(user_state, keysfilename->str);
    if (err != GPG_ERR_NO_ERROR) {
        g_string_free(basedir, TRUE);
        g_string_free(keysfilename, TRUE);
        log_error("Failed to load private key");
        data_loaded = FALSE;
        return;
    }

    err = otrl_privkey_read_fingerprints(user_state, fpsfilename->str, NULL, NULL);
    if (err != GPG_ERR_NO_ERROR) {
        g_string_free(basedir, TRUE);
        g_string_free(keysfilename, TRUE);
        log_error("Failed to load fingerprints");
        data_loaded = FALSE;
        return;
    }

    data_loaded = TRUE;

    g_string_free(basedir, TRUE);
    g_string_free(keysfilename, TRUE);
    g_string_free(fpsfilename, TRUE);
    return;
}

gboolean
otr_key_loaded(void)
{
    return data_loaded;
}

char*
otr_tag_message(const char *const msg)
{
    GString *otr_message = g_string_new(msg);
    g_string_append(otr_message, OTRL_MESSAGE_TAG_BASE);
    g_string_append(otr_message, OTRL_MESSAGE_TAG_V2);
    char *result = otr_message->str;
    g_string_free(otr_message, FALSE);

    return result;
}

gboolean
otr_is_secure(const char *const recipient)
{
    ConnContext *context = otrlib_context_find(user_state, recipient, jid);

    if (context == NULL) {
        return FALSE;
    }

    if (context->msgstate != OTRL_MSGSTATE_ENCRYPTED) {
        return FALSE;
    } else {
        return TRUE;
    }
}

gboolean
otr_is_trusted(const char *const recipient)
{
    ConnContext *context = otrlib_context_find(user_state, recipient, jid);

    if (context == NULL) {
        return FALSE;
    }

    if (context->msgstate != OTRL_MSGSTATE_ENCRYPTED) {
        return TRUE;
    }

    if (context->active_fingerprint) {
        if (context->active_fingerprint->trust == NULL) {
            return FALSE;
        } else if (context->active_fingerprint->trust[0] == '\0') {
            return FALSE;
        } else {
            return TRUE;
        }
    }

    return FALSE;
}

void
otr_trust(const char *const recipient)
{
    ConnContext *context = otrlib_context_find(user_state, recipient, jid);

    if (context == NULL) {
        return;
    }

    if (context->msgstate != OTRL_MSGSTATE_ENCRYPTED) {
        return;
    }

    if (context->active_fingerprint) {
        if (context->active_fingerprint->trust) {
            free(context->active_fingerprint->trust);
        }
        context->active_fingerprint->trust = strdup("trusted");
        cb_write_fingerprints(NULL);
    }

    return;
}

void
otr_untrust(const char *const recipient)
{
    ConnContext *context = otrlib_context_find(user_state, recipient, jid);

    if (context == NULL) {
        return;
    }

    if (context->msgstate != OTRL_MSGSTATE_ENCRYPTED) {
        return;
    }

    if (context->active_fingerprint) {
        if (context->active_fingerprint->trust) {
            free(context->active_fingerprint->trust);
        }
        context->active_fingerprint->trust = NULL;
        cb_write_fingerprints(NULL);
    }

    return;
}

void
otr_smp_secret(const char *const recipient, const char *secret)
{
    ConnContext *context = otrlib_context_find(user_state, recipient, jid);

    if (context == NULL) {
        return;
    }

    if (context->msgstate != OTRL_MSGSTATE_ENCRYPTED) {
        return;
    }

    // if recipient initiated SMP, send response, else initialise
    ProfChatWin *chatwin = wins_get_chat(recipient);
    if (g_hash_table_contains(smp_initiators, recipient)) {
        otrl_message_respond_smp(user_state, &ops, NULL, context, (const unsigned char*)secret, strlen(secret));
        if (chatwin) {
            chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_AUTH, NULL);
        }
        g_hash_table_remove(smp_initiators, context->username);
    } else {
        otrl_message_initiate_smp(user_state, &ops, NULL, context, (const unsigned char*)secret, strlen(secret));
        if (chatwin) {
            chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_AUTH_WAIT, NULL);
        }
    }
}

void
otr_smp_question(const char *const recipient, const char *question, const char *answer)
{
    ConnContext *context = otrlib_context_find(user_state, recipient, jid);

    if (context == NULL) {
        return;
    }

    if (context->msgstate != OTRL_MSGSTATE_ENCRYPTED) {
        return;
    }

    otrl_message_initiate_smp_q(user_state, &ops, NULL, context, question, (const unsigned char*)answer, strlen(answer));
    ProfChatWin *chatwin = wins_get_chat(recipient);
    if (chatwin) {
        chatwin_otr_smp_event(chatwin, PROF_OTR_SMP_AUTH_WAIT, NULL);
    }
}

void
otr_smp_answer(const char *const recipient, const char *answer)
{
    ConnContext *context = otrlib_context_find(user_state, recipient, jid);

    if (context == NULL) {
        return;
    }

    if (context->msgstate != OTRL_MSGSTATE_ENCRYPTED) {
        return;
    }

    // if recipient initiated SMP, send response, else initialise
    otrl_message_respond_smp(user_state, &ops, NULL, context, (const unsigned char*)answer, strlen(answer));
}

void
otr_end_session(const char *const recipient)
{
    otrlib_end_session(user_state, recipient, jid, &ops);
}

char*
otr_get_my_fingerprint(void)
{
    char fingerprint[45];
    otrl_privkey_fingerprint(user_state, fingerprint, jid, "xmpp");
    char *result = strdup(fingerprint);

    return result;
}

char*
otr_get_their_fingerprint(const char *const recipient)
{
    ConnContext *context = otrlib_context_find(user_state, recipient, jid);

    if (context) {
        Fingerprint *fingerprint = context->active_fingerprint;
        char readable[45];
        otrl_privkey_hash_to_human(readable, fingerprint->fingerprint);
        return strdup(readable);
    } else {
        return NULL;
    }
}

prof_otrpolicy_t
otr_get_policy(const char *const recipient)
{
    char *account_name = session_get_account_name();
    ProfAccount *account = accounts_get_account(account_name);
    // check contact specific setting
    if (g_list_find_custom(account->otr_manual, recipient, (GCompareFunc)g_strcmp0)) {
        account_free(account);
        return PROF_OTRPOLICY_MANUAL;
    }
    if (g_list_find_custom(account->otr_opportunistic, recipient, (GCompareFunc)g_strcmp0)) {
        account_free(account);
        return PROF_OTRPOLICY_OPPORTUNISTIC;
    }
    if (g_list_find_custom(account->otr_always, recipient, (GCompareFunc)g_strcmp0)) {
        account_free(account);
        return PROF_OTRPOLICY_ALWAYS;
    }

    // check default account setting
    if (account->otr_policy) {
        prof_otrpolicy_t result;
        if (g_strcmp0(account->otr_policy, "manual") == 0) {
            result = PROF_OTRPOLICY_MANUAL;
        }
        if (g_strcmp0(account->otr_policy, "opportunistic") == 0) {
            result = PROF_OTRPOLICY_OPPORTUNISTIC;
        }
        if (g_strcmp0(account->otr_policy, "always") == 0) {
            result = PROF_OTRPOLICY_ALWAYS;
        }
        account_free(account);
        return result;
    }
    account_free(account);

    // check global setting
    char *pref_otr_policy = prefs_get_string(PREF_OTR_POLICY);

    // pref defaults to manual
    prof_otrpolicy_t result = PROF_OTRPOLICY_MANUAL;

    if (strcmp(pref_otr_policy, "opportunistic") == 0) {
        result = PROF_OTRPOLICY_OPPORTUNISTIC;
    } else if (strcmp(pref_otr_policy, "always") == 0) {
        result = PROF_OTRPOLICY_ALWAYS;
    }

    prefs_free_string(pref_otr_policy);

    return result;
}

char*
otr_encrypt_message(const char *const to, const char *const message)
{
    char *newmessage = NULL;
    gcry_error_t err = otrlib_encrypt_message(user_state, &ops, jid, to, message, &newmessage);

    if (err != 0) {
        return NULL;
    } else {
        return newmessage;
    }
}

static void
_otr_tlv_free(OtrlTLV *tlvs)
{
    if (tlvs) {
        otrl_tlv_free(tlvs);
    }
}

char*
otr_decrypt_message(const char *const from, const char *const message, gboolean *decrypted)
{
    char *newmessage = NULL;
    OtrlTLV *tlvs = NULL;

    int result = otrlib_decrypt_message(user_state, &ops, jid, from, message, &newmessage, &tlvs);

    // internal libotr message
    if (result == 1) {
        ConnContext *context = otrlib_context_find(user_state, from, jid);

        // common tlv handling
        OtrlTLV *tlv = otrl_tlv_find(tlvs, OTRL_TLV_DISCONNECTED);
        if (tlv) {
            if (context) {
                otrl_context_force_plaintext(context);
                ProfChatWin *chatwin = wins_get_chat(from);
                if (chatwin) {
                    chatwin_otr_unsecured(chatwin);
                }
            }
        }

        // library version specific tlv handling
        otrlib_handle_tlvs(user_state, &ops, context, tlvs, smp_initiators);
        _otr_tlv_free(tlvs);

        return NULL;

    // message was processed, return to user
    } else if (newmessage) {
        _otr_tlv_free(tlvs);
        if (g_str_has_prefix(message, "?OTR:")) {
            *decrypted = TRUE;
        }
        return newmessage;

    // normal non OTR message
    } else {
        _otr_tlv_free(tlvs);
        *decrypted = FALSE;
        return strdup(message);
    }
}

void
otr_free_message(char *message)
{
    otrl_message_free(message);
}
