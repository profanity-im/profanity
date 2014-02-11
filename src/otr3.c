/*
 * otr3.c
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

#include <libotr/proto.h>
#include <libotr/privkey.h>
#include <libotr/message.h>
#include <glib.h>

#include "otr.h"
#include "log.h"
#include "roster_list.h"
#include "contact.h"
#include "ui/ui.h"

static OtrlUserState user_state;
static OtrlMessageAppOps ops;
static char *jid;
static gboolean data_loaded;

// ops callbacks
static OtrlPolicy
cb_policy(void *opdata, ConnContext *context)
{
    return OTRL_POLICY_ALLOW_V1 | OTRL_POLICY_ALLOW_V2 ;
}

static int
cb_is_logged_in(void *opdata, const char *accountname,
    const char *protocol, const char *recipient)
{
    PContact contact = roster_get_contact(recipient);
    if (g_strcmp0(p_contact_presence(contact), "offline") == 0) {
        return 0;
    } else {
        return 1;
    }
}

static void
cb_inject_message(void *opdata, const char *accountname,
    const char *protocol, const char *recipient, const char *message)
{
    message_send(message, recipient);
}

static int
cb_display_otr_message(void *opdata, const char *accountname,
    const char *protocol, const char *username, const char *msg)
{
    cons_show_error("%s", msg);
    return 0;
}

static void
cb_write_fingerprints(void *opdata)
{
    gcry_error_t err = 0;
    gchar *data_home = xdg_get_data_home();
    gchar *account_dir = str_replace(jid, "@", "_at_");

    GString *basedir = g_string_new(data_home);
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

void
otr_init(void)
{
    log_info("Initialising OTR");
    OTRL_INIT;

    ops.policy = cb_policy;
    ops.is_logged_in = cb_is_logged_in;
    ops.inject_message = cb_inject_message;
    ops.display_otr_message = cb_display_otr_message;
    ops.write_fingerprints = cb_write_fingerprints;
    ops.gone_secure = cb_gone_secure;

    data_loaded = FALSE;
}

void
otr_on_connect(ProfAccount *account)
{
    jid = strdup(account->jid);
    log_info("Loading OTR key for %s", jid);

    gchar *data_home = xdg_get_data_home();
    gchar *account_dir = str_replace(jid, "@", "_at_");

    GString *basedir = g_string_new(data_home);
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

    jid = strdup(account->jid);
    log_info("Generating OTR key for %s", jid);

    jid = strdup(account->jid);

    gchar *data_home = xdg_get_data_home();
    gchar *account_dir = str_replace(jid, "@", "_at_");

    GString *basedir = g_string_new(data_home);
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
    ui_current_page_off();
    ui_refresh();
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
    ConnContext *context = otrl_context_find(user_state, recipient, jid, "xmpp",
        0, NULL, NULL, NULL);

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
    ConnContext *context = otrl_context_find(user_state, recipient, jid, "xmpp",
        0, NULL, NULL, NULL);

    if (context == NULL) {
        return FALSE;
    }

    if (context->msgstate != OTRL_MSGSTATE_ENCRYPTED) {
        return TRUE;
    }

    if (context->active_fingerprint &&
                g_strcmp0(context->active_fingerprint->trust, "trusted") == 0) {
        return TRUE;
    }

    return FALSE;
}

void
otr_trust(const char * const recipient)
{
    ConnContext *context = otrl_context_find(user_state, recipient, jid, "xmpp",
        0, NULL, NULL, NULL);

    if (context == NULL) {
        return;
    }

    if (context->msgstate != OTRL_MSGSTATE_ENCRYPTED) {
        return;
    }

    if (context->active_fingerprint) {
        context->active_fingerprint->trust = "trusted";
        cb_write_fingerprints(NULL);
    }

    return;
}

void
otr_untrust(const char * const recipient)
{
    ConnContext *context = otrl_context_find(user_state, recipient, jid, "xmpp",
        0, NULL, NULL, NULL);

    if (context == NULL) {
        return;
    }

    if (context->msgstate != OTRL_MSGSTATE_ENCRYPTED) {
        return;
    }

    if (context->active_fingerprint) {
        context->active_fingerprint->trust = NULL;
        cb_write_fingerprints(NULL);
    }

    return;
}

void
otr_end_session(const char * const recipient)
{
    ConnContext *context = otrl_context_find(user_state, recipient, jid, "xmpp",
        0, NULL, NULL, NULL);

    if (context != NULL) {
        otrl_message_disconnect(user_state, &ops, NULL, jid, "xmpp", recipient);
    }
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
    ConnContext *context = otrl_context_find(user_state, recipient, jid, "xmpp",
        0, NULL, NULL, NULL);

    if (context != NULL) {
        Fingerprint *fingerprint = context->active_fingerprint;
        char readable[45];
        otrl_privkey_hash_to_human(readable, fingerprint->fingerprint);
        return strdup(readable);
    } else {
        return NULL;
    }
}

char *
otr_encrypt_message(const char * const to, const char * const message)
{
    gcry_error_t err;
    char *newmessage = NULL;

    err = otrl_message_sending(
        user_state,
        &ops,
        NULL,
        jid,
        "xmpp",
        to,
        message,
        0,
        &newmessage,
        NULL,
        NULL);
    if (!err == GPG_ERR_NO_ERROR) {
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
    OtrlTLV *tlv = NULL;
    int result = otrl_message_receiving(user_state, &ops, NULL, jid, "xmpp", from, message, &decrypted, &tlvs, NULL, NULL);

    // internal libotr message
    if (result == 1) {
        tlv = otrl_tlv_find(tlvs, OTRL_TLV_DISCONNECTED);
        if (tlv) {
            ConnContext *context = otrl_context_find(user_state, from, jid, "xmpp",
                0, NULL, NULL, NULL);

            if (context != NULL) {
                otrl_context_force_plaintext(context);
                ui_gone_insecure(from);
            }
        }
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
