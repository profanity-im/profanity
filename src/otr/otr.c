/*
 * otr.c
 *
 * Copyright (C) 2012 - 2014 James Booth <boothj5@gmail.com>
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

#include <libotr/proto.h>
#include <libotr/privkey.h>
#include <libotr/message.h>
#include <libotr/sm.h>
#include <glib.h>

#include "otr/otr.h"
#include "otr/otrlib.h"
#include "log.h"
#include "roster_list.h"
#include "contact.h"
#include "ui/ui.h"
#include "config/preferences.h"
#include "chat_session.h"

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

OtrlMessageAppOps *
otr_messageops(void)
{
    return &ops;
}

GHashTable *
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
cb_is_logged_in(void *opdata, const char *accountname,
    const char *protocol, const char *recipient)
{
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
    gboolean send_state = chat_session_on_message_send(recipient);
    message_send_chat(recipient, NULL, message, send_state);
}

static void
cb_write_fingerprints(void *opdata)
{
    gcry_error_t err = 0;

    gchar *data_home = xdg_get_data_home();
    GString *basedir = g_string_new(data_home);
    free(data_home);

    gchar *account_dir = str_replace(jid, "@", "_at_");
    g_string_append(basedir, "/profanity/otr/");
    g_string_append(basedir, account_dir);
    g_string_append(basedir, "/");
    free(account_dir);

    GString *fpsfilename = g_string_new(basedir->str);
    g_string_append(fpsfilename, "fingerprints.txt");
    err = otrl_privkey_write_fingerprints(user_state, fpsfilename->str);
    if (!err == GPG_ERR_NO_ERROR) {
        log_error("Failed to write fingerprints file");
        cons_show_error("Failed to create fingerprints file");
    }
    g_string_free(basedir, TRUE);
    g_string_free(fpsfilename, TRUE);
}

static void
cb_gone_secure(void *opdata, ConnContext *context)
{
    ui_gone_secure(context->username, otr_is_trusted(context->username));
}

char *
otr_libotr_version(void)
{
    return OTRL_VERSION;
}

char *
otr_start_query(void)
{
    return otrlib_start_query();
}

void
otr_init(void)
{
    log_info("Initialising OTR");
    OTRL_INIT;

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
    if (jid != NULL) {
        free(jid);
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
    if (jid != NULL) {
        free(jid);
    }
    jid = strdup(account->jid);
    log_info("Loading OTR key for %s", jid);

    gchar *data_home = xdg_get_data_home();
    GString *basedir = g_string_new(data_home);
    free(data_home);

    gchar *account_dir = str_replace(jid, "@", "_at_");
    g_string_append(basedir, "/profanity/otr/");
    g_string_append(basedir, account_dir);
    g_string_append(basedir, "/");
    free(account_dir);

    if (!mkdir_recursive(basedir->str)) {
        log_error("Could not create %s for account %s.", basedir->str, jid);
        cons_show_error("Could not create %s for account %s.", basedir->str, jid);
        g_string_free(basedir, TRUE);
        return;
    }

    user_state = otrl_userstate_create();

    gcry_error_t err = 0;

    GString *keysfilename = g_string_new(basedir->str);
    g_string_append(keysfilename, "keys.txt");
    if (!g_file_test(keysfilename->str, G_FILE_TEST_IS_REGULAR)) {
        log_info("No private key file found %s", keysfilename->str);
        data_loaded = FALSE;
    } else {
        log_info("Loading OTR private key %s", keysfilename->str);
        err = otrl_privkey_read(user_state, keysfilename->str);
        if (!err == GPG_ERR_NO_ERROR) {
            g_string_free(basedir, TRUE);
            g_string_free(keysfilename, TRUE);
            log_error("Failed to load private key");
            return;
        } else {
            log_info("Loaded private key");
            data_loaded = TRUE;
        }
    }

    GString *fpsfilename = g_string_new(basedir->str);
    g_string_append(fpsfilename, "fingerprints.txt");
    if (!g_file_test(fpsfilename->str, G_FILE_TEST_IS_REGULAR)) {
        log_info("No fingerprints file found %s", fpsfilename->str);
        data_loaded = FALSE;
    } else {
        log_info("Loading fingerprints %s", fpsfilename->str);
        err = otrl_privkey_read_fingerprints(user_state, fpsfilename->str, NULL, NULL);
        if (!err == GPG_ERR_NO_ERROR) {
            g_string_free(basedir, TRUE);
            g_string_free(keysfilename, TRUE);
            g_string_free(fpsfilename, TRUE);
            log_error("Failed to load fingerprints");
            return;
        } else {
            log_info("Loaded fingerprints");
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

void
otr_keygen(ProfAccount *account)
{
    if (data_loaded) {
        cons_show("OTR key already generated.");
        return;
    }

    if (jid != NULL) {
        free(jid);
    }
    jid = strdup(account->jid);
    log_info("Generating OTR key for %s", jid);

    gchar *data_home = xdg_get_data_home();
    GString *basedir = g_string_new(data_home);
    free(data_home);

    gchar *account_dir = str_replace(jid, "@", "_at_");
    g_string_append(basedir, "/profanity/otr/");
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
    if (!err == GPG_ERR_NO_ERROR) {
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
    if (!err == GPG_ERR_NO_ERROR) {
        g_string_free(basedir, TRUE);
        g_string_free(keysfilename, TRUE);
        log_error("Failed to create fingerprints file");
        cons_show_error("Failed to create fingerprints file");
        return;
    }
    log_info("Fingerprints file created");

    err = otrl_privkey_read(user_state, keysfilename->str);
    if (!err == GPG_ERR_NO_ERROR) {
        g_string_free(basedir, TRUE);
        g_string_free(keysfilename, TRUE);
        log_error("Failed to load private key");
        data_loaded = FALSE;
        return;
    }

    err = otrl_privkey_read_fingerprints(user_state, fpsfilename->str, NULL, NULL);
    if (!err == GPG_ERR_NO_ERROR) {
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

gboolean
otr_is_secure(const char * const recipient)
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
otr_is_trusted(const char * const recipient)
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
otr_trust(const char * const recipient)
{
    ConnContext *context = otrlib_context_find(user_state, recipient, jid);

    if (context == NULL) {
        return;
    }

    if (context->msgstate != OTRL_MSGSTATE_ENCRYPTED) {
        return;
    }

    if (context->active_fingerprint) {
        if (context->active_fingerprint->trust != NULL) {
            free(context->active_fingerprint->trust);
        }
        context->active_fingerprint->trust = strdup("trusted");
        cb_write_fingerprints(NULL);
    }

    return;
}

void
otr_untrust(const char * const recipient)
{
    ConnContext *context = otrlib_context_find(user_state, recipient, jid);

    if (context == NULL) {
        return;
    }

    if (context->msgstate != OTRL_MSGSTATE_ENCRYPTED) {
        return;
    }

    if (context->active_fingerprint) {
        if (context->active_fingerprint->trust != NULL) {
            free(context->active_fingerprint->trust);
        }
        context->active_fingerprint->trust = NULL;
        cb_write_fingerprints(NULL);
    }

    return;
}

void
otr_smp_secret(const char * const recipient, const char *secret)
{
    ConnContext *context = otrlib_context_find(user_state, recipient, jid);

    if (context == NULL) {
        return;
    }

    if (context->msgstate != OTRL_MSGSTATE_ENCRYPTED) {
        return;
    }

    // if recipient initiated SMP, send response, else initialise
    if (g_hash_table_contains(smp_initiators, recipient)) {
        otrl_message_respond_smp(user_state, &ops, NULL, context, (const unsigned char*)secret, strlen(secret));
        ui_otr_authenticating(recipient);
        g_hash_table_remove(smp_initiators, context->username);
    } else {
        otrl_message_initiate_smp(user_state, &ops, NULL, context, (const unsigned char*)secret, strlen(secret));
        ui_otr_authetication_waiting(recipient);
    }
}

void
otr_smp_question(const char * const recipient, const char *question, const char *answer)
{
    ConnContext *context = otrlib_context_find(user_state, recipient, jid);

    if (context == NULL) {
        return;
    }

    if (context->msgstate != OTRL_MSGSTATE_ENCRYPTED) {
        return;
    }

    otrl_message_initiate_smp_q(user_state, &ops, NULL, context, question, (const unsigned char*)answer, strlen(answer));
    ui_otr_authetication_waiting(recipient);
}

void
otr_smp_answer(const char * const recipient, const char *answer)
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
otr_end_session(const char * const recipient)
{
    otrlib_end_session(user_state, recipient, jid, &ops);
}

char *
otr_get_my_fingerprint(void)
{
    char fingerprint[45];
    otrl_privkey_fingerprint(user_state, fingerprint, jid, "xmpp");
    char *result = strdup(fingerprint);

    return result;
}

char *
otr_get_their_fingerprint(const char * const recipient)
{
    ConnContext *context = otrlib_context_find(user_state, recipient, jid);

    if (context != NULL) {
        Fingerprint *fingerprint = context->active_fingerprint;
        char readable[45];
        otrl_privkey_hash_to_human(readable, fingerprint->fingerprint);
        return strdup(readable);
    } else {
        return NULL;
    }
}

prof_otrpolicy_t
otr_get_policy(const char * const recipient)
{
    ProfAccount *account = accounts_get_account(jabber_get_account_name());
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
    if (account->otr_policy != NULL) {
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

char *
otr_encrypt_message(const char * const to, const char * const message)
{
    char *newmessage = NULL;
    gcry_error_t err = otrlib_encrypt_message(user_state, &ops, jid, to, message, &newmessage);

    if (err != 0) {
        return NULL;
    } else {
        return newmessage;
    }
}

char *
otr_decrypt_message(const char * const from, const char * const message, gboolean *was_decrypted)
{
    char *decrypted = NULL;
    OtrlTLV *tlvs = NULL;

    int result = otrlib_decrypt_message(user_state, &ops, jid, from, message, &decrypted, &tlvs);

    // internal libotr message
    if (result == 1) {
        ConnContext *context = otrlib_context_find(user_state, from, jid);

        // common tlv handling
        OtrlTLV *tlv = otrl_tlv_find(tlvs, OTRL_TLV_DISCONNECTED);
        if (tlv) {

            if (context != NULL) {
                otrl_context_force_plaintext(context);
                ui_gone_insecure(from);
            }
        }

        // library version specific tlv handling
        otrlib_handle_tlvs(user_state, &ops, context, tlvs, smp_initiators);

        return NULL;

    // message was decrypted, return to user
    } else if (decrypted != NULL) {
        *was_decrypted = TRUE;
        return decrypted;

    // normal non OTR message
    } else {
        *was_decrypted = FALSE;
        return strdup(message);
    }
}

void
otr_free_message(char *message)
{
    otrl_message_free(message);
}