/*
 * otr.c
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
#include "ui/ui.h"

static OtrlUserState user_state;
static OtrlMessageAppOps ops;
static char *jid;
static gboolean data_loaded;

// ops callbacks
static OtrlPolicy
cb_policy(void *opdata, ConnContext *context)
{
//    cons_debug("cb_policy");
    return OTRL_POLICY_DEFAULT ;
}

static void
cb_create_privkey(void *opdata, const char *accountname,
    const char *protocol)
{
//    cons_debug("cb_create_privkey accountname: %s, protocol: %s", accountname, protocol);
}

static int
cb_is_logged_in(void *opdata, const char *accountname,
    const char *protocol, const char *recipient)
{
//    cons_debug("cb_is_logged_in: account: %s, protocol: %s, recipient: %s",
//        accountname, protocol, recipient);
    return -1;
}

static void
cb_inject_message(void *opdata, const char *accountname,
    const char *protocol, const char *recipient, const char *message)
{
//    cons_debug("cb_inject_message: account: %s, protocol, %s, recipient: %s, message: %s",
//        accountname, protocol, recipient, message);
    message_send(message, recipient);
}

static void
cb_notify(void *opdata, OtrlNotifyLevel level,
    const char *accountname, const char *protocol, const char *username,
    const char *title, const char *primary, const char *secondary)
{
//    cons_debug("cb_notify");
}

static int
cb_display_otr_message(void *opdata, const char *accountname,
    const char *protocol, const char *username, const char *msg)
{
    cons_show_error("%s", msg);
    return 0;
}

static const char *
cb_protocol_name(void *opdata, const char *protocol)
{
//    cons_debug("cb_protocol_name: %s", protocol);
    return "xmpp";
}

static void
cb_new_fingerprint(void *opdata, OtrlUserState us, const char *accountname,
    const char *protocol, const char *username, unsigned char fingerprint[20])
{
//    cons_debug("cb_new_fingerprint: account: %s, protocol: %s, username: %s",
//        accountname, protocol, username);
}

static void
cb_protocol_name_free(void *opdata, const char *protocol_name)
{
//    cons_debug("cb_protocol_name_free: %s", protocol_name);
}

static void
cb_update_context_list(void *opdata)
{
//    cons_debug("cb_update_context_list");
}

static void
cb_write_fingerprints(void *opdata)
{
//    cons_debug("cb_write_fingerprints");
}

static void
cb_gone_secure(void *opdata, ConnContext *context)
{
//    cons_debug("cb_gone_secure");
}

static void
cb_gone_insecure(void *opdata, ConnContext *context)
{
//    cons_debug("cb_gone_insecure");
}

static void
cb_still_secure(void *opdata, ConnContext *context, int is_reply)
{
//    cons_debug("cb_still_secure: is_reply = %d", is_reply);
}

static void
cb_log_message(void *opdata, const char *message)
{
//    cons_debug("cb_log_message: %s", message);
}

void
otr_init(void)
{
    log_info("Initialising OTR");
    OTRL_INIT;

    ops.policy = cb_policy;
    ops.create_privkey = cb_create_privkey;
    ops.is_logged_in = cb_is_logged_in;
    ops.inject_message = cb_inject_message;
    ops.notify = cb_notify;
    ops.display_otr_message = cb_display_otr_message;
    ops.update_context_list = cb_update_context_list;
    ops.protocol_name = cb_protocol_name;
    ops.protocol_name_free = cb_protocol_name_free;
    ops.new_fingerprint = cb_new_fingerprint;
    ops.write_fingerprints = cb_write_fingerprints;
    ops.gone_secure = cb_gone_secure;
    ops.gone_insecure = cb_gone_insecure;
    ops.still_secure = cb_still_secure;
    ops.log_message = cb_log_message;

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

    if (!mkdir_recursive(basedir->str)) {
        g_string_free(basedir, TRUE);
        log_error("Could not create %s for account %s.", basedir->str, jid);
        cons_show_error("Could not create %s for account %s.", basedir->str, jid);
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

    if (!mkdir_recursive(basedir->str)) {
        g_string_free(basedir, TRUE);
        log_error("Could not create %s for account %s.", basedir->str, jid);
        cons_show_error("Could not create %s for account %s.", basedir->str, jid);
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

char *
otr_get_fingerprint(void)
{
    char fingerprint[45];
    otrl_privkey_fingerprint(user_state, fingerprint, jid, "xmpp");
    char *result = strdup(fingerprint);

    return result;
}

char *
otr_encrypt_message(const char * const to, const char * const message)
{
    cons_debug("Encrypting message: %s", message);
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
//        cons_debug("Error encrypting, result: %s", newmessage);
        return NULL;
    } else {
        cons_debug("Encrypted message: %s", newmessage);
        return newmessage;
    }
}

char *
otr_decrypt_message(const char * const from, const char * const message)
{
    cons_debug("Decrypting message: %s", message);
    char *decrypted = NULL;
    int result = otrl_message_receiving(user_state, &ops, NULL, jid, "xmpp", from, message, &decrypted, 0, NULL, NULL);

    // internal libotr message, ignore
    if (result == 1) {
        return NULL;

    // message was decrypted, return to user
    } else if (decrypted != NULL) {
        cons_debug("Decrypted message: %s", decrypted);
        return decrypted;

    // normal non OTR message
    } else {
        return strdup(message);
    }
}

void
otr_free_message(char *message)
{
    otrl_message_free(message);
}
