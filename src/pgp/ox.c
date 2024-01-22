/*
 * ox.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2020 Stefan Kropp <stefan@debxwoody.de>
 * Copyright (C) 2020 - 2024 Michael Vetter <jubalh@iodoru.org>
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

#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gpgme.h>

#include "log.h"
#include "common.h"
#include "pgp/ox.h"
#include "config/files.h"
#include "ui/ui.h"

static gpgme_key_t _ox_key_lookup(const char* const barejid, gboolean secret_only);
static gboolean _ox_key_is_usable(gpgme_key_t key, const char* const barejid, gboolean secret);

/*!
 * \brief Public keys with XMPP-URI.
 *
 * This function will look for all public key with a XMPP-URI as UID.
 *
 * @returns GHashTable* with GString* xmpp-uri and ProfPGPKey* value. Empty
 * hash, if there is no key. NULL in case of error.
 *
 */
GHashTable*
ox_gpg_public_keys(void)
{
    gpgme_error_t error;
    GHashTable* result = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)p_gpg_free_key);

    gpgme_ctx_t ctx;
    error = gpgme_new(&ctx);

    if (error) {
        log_error("OX: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

    error = gpgme_op_keylist_start(ctx, NULL, 0); // all public keys
    if (error == GPG_ERR_NO_ERROR) {
        gpgme_key_t key;
        error = gpgme_op_keylist_next(ctx, &key);
        if (error != GPG_ERR_EOF && error != GPG_ERR_NO_ERROR) {
            log_error("OX: gpgme_op_keylist_next %s %s", gpgme_strsource(error), gpgme_strerror(error));
            g_hash_table_destroy(result);
            return NULL;
        }
        while (!error) {
            // Looking for XMPP URI UID
            gpgme_user_id_t uid = key->uids;
            gpgme_user_id_t xmppid = NULL;
            while (!xmppid && uid) {
                if (uid->name && strlen(uid->name) >= 10) {
                    if (strstr(uid->name, "xmpp:") == uid->name) {
                        xmppid = uid;
                    }
                }
                uid = uid->next;
            }

            if (xmppid) {
                // Build Key information about all subkey
                gpgme_subkey_t sub = key->subkeys;

                ProfPGPKey* p_pgpkey = p_gpg_key_new();
                p_pgpkey->id = strdup(sub->keyid);
                p_pgpkey->name = strdup(xmppid->uid);
                p_pgpkey->fp = strdup(sub->fpr);
                if (sub->can_encrypt)
                    p_pgpkey->encrypt = TRUE;
                if (sub->can_authenticate)
                    p_pgpkey->authenticate = TRUE;
                if (sub->can_certify)
                    p_pgpkey->certify = TRUE;
                if (sub->can_sign)
                    p_pgpkey->sign = TRUE;

                sub = sub->next;
                while (sub) {
                    if (sub->can_encrypt)
                        p_pgpkey->encrypt = TRUE;
                    if (sub->can_authenticate)
                        p_pgpkey->authenticate = TRUE;
                    if (sub->can_certify)
                        p_pgpkey->certify = TRUE;
                    if (sub->can_sign)
                        p_pgpkey->sign = TRUE;

                    sub = sub->next;
                }

                g_hash_table_insert(result, strdup(p_pgpkey->name), p_pgpkey);
            }
            gpgme_key_unref(key);
            error = gpgme_op_keylist_next(ctx, &key);
        }
    }
    gpgme_release(ctx);

    return result;
}

char*
p_ox_gpg_signcrypt(const char* const sender_barejid, const char* const recipient_barejid, const char* const message)
{
    setlocale(LC_ALL, "");
    gpgme_check_version(NULL);
    gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
    gpgme_ctx_t ctx;

    gpgme_error_t error = gpgme_new(&ctx);
    if (GPG_ERR_NO_ERROR != error) {
        log_error("OX: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

    error = gpgme_set_protocol(ctx, GPGME_PROTOCOL_OPENPGP);
    if (error != 0) {
        log_error("OX: Signcrypt error: %s", gpgme_strerror(error));
    }

    gpgme_set_armor(ctx, 0);
    gpgme_set_textmode(ctx, 0);
    gpgme_set_offline(ctx, 1);
    gpgme_set_keylist_mode(ctx, GPGME_KEYLIST_MODE_LOCAL);
    if (error != 0) {
        log_error("OX: Signcrypt error: %s", gpgme_strerror(error));
    }

    gpgme_key_t recp[3];
    recp[0] = NULL,
    recp[1] = NULL;

    char* xmpp_jid_me = alloca((strlen(sender_barejid) + 6) * sizeof(char));
    char* xmpp_jid_recipient = alloca((strlen(recipient_barejid) + 6) * sizeof(char));

    strcpy(xmpp_jid_me, "xmpp:");
    strcpy(xmpp_jid_recipient, "xmpp:");
    strcat(xmpp_jid_me, sender_barejid);
    strcat(xmpp_jid_recipient, recipient_barejid);

    gpgme_signers_clear(ctx);

    // lookup own key
    recp[0] = _ox_key_lookup(sender_barejid, TRUE);
    if (error != 0) {
        cons_show_error("Can't find OX key for %s", xmpp_jid_me);
        log_error("OX: Key not found for %s. Error: %s", xmpp_jid_me, gpgme_strerror(error));
        return NULL;
    }

    error = gpgme_signers_add(ctx, recp[0]);
    if (error != 0) {
        log_error("OX: gpgme_signers_add %s. Error: %s", xmpp_jid_me, gpgme_strerror(error));
        return NULL;
    }

    // lookup key of recipient
    recp[1] = _ox_key_lookup(recipient_barejid, FALSE);
    if (error != 0) {
        cons_show_error("Can't find OX key for %s", xmpp_jid_recipient);
        log_error("OX: Key not found for %s. Error: %s", xmpp_jid_recipient, gpgme_strerror(error));
        return NULL;
    }

    recp[2] = NULL;
    log_debug("OX: %s <%s>", recp[0]->uids->name, recp[0]->uids->email);
    log_debug("OX: %s <%s>", recp[1]->uids->name, recp[1]->uids->email);

    gpgme_encrypt_flags_t flags = 0;

    gpgme_data_t plain;
    gpgme_data_t cipher;

    error = gpgme_data_new(&plain);
    if (error != 0) {
        log_error("OX: %s", gpgme_strerror(error));
        return NULL;
    }

    error = gpgme_data_new_from_mem(&plain, message, strlen(message), 0);
    if (error != 0) {
        log_error("OX: %s", gpgme_strerror(error));
        return NULL;
    }
    error = gpgme_data_new(&cipher);
    if (error != 0) {
        log_error("OX: %s", gpgme_strerror(error));
        return NULL;
    }

    error = gpgme_op_encrypt_sign(ctx, recp, flags, plain, cipher);
    if (error != 0) {
        log_error("OX: %s", gpgme_strerror(error));
        return NULL;
    }

    size_t len;
    char* cipher_str = gpgme_data_release_and_get_mem(cipher, &len);
    char* result = g_base64_encode((unsigned char*)cipher_str, len);
    gpgme_key_release(recp[0]);
    gpgme_key_release(recp[1]);
    gpgme_release(ctx);
    return result;
}

gboolean
ox_is_private_key_available(const char* const barejid)
{
    g_assert(barejid);
    gboolean result = FALSE;

    gpgme_key_t key = _ox_key_lookup(barejid, TRUE);
    if (key) {
        if (_ox_key_is_usable(key, barejid, TRUE)) {
            result = TRUE;
        }
        gpgme_key_unref(key);
    }

    return result;
}

gboolean
ox_is_public_key_available(const char* const barejid)
{
    g_assert(barejid);
    gboolean result = FALSE;
    gpgme_key_t key = _ox_key_lookup(barejid, FALSE);
    if (key) {
        if (_ox_key_is_usable(key, barejid, FALSE)) {
            result = TRUE;
        }
        gpgme_key_unref(key);
    }
    return result;
}

static gpgme_key_t
_ox_key_lookup(const char* const barejid, gboolean secret_only)
{
    g_assert(barejid);
    log_debug("OX: Looking for %s key: %s", secret_only == TRUE ? "Private" : "Public", barejid);
    gpgme_key_t key = NULL;
    gpgme_error_t error;

    gpgme_ctx_t ctx;
    error = gpgme_new(&ctx);

    if (error) {
        log_error("OX: gpgme_new failed: %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

    error = gpgme_op_keylist_start(ctx, NULL, secret_only);
    if (error == GPG_ERR_NO_ERROR) {
        error = gpgme_op_keylist_next(ctx, &key);
        if (error != GPG_ERR_EOF && error != GPG_ERR_NO_ERROR) {
            log_error("OX: gpgme_op_keylist_next %s %s", gpgme_strsource(error), gpgme_strerror(error));
            return NULL;
        }

        GString* xmppuri = g_string_new("xmpp:");
        g_string_append(xmppuri, barejid);

        while (!error) {
            // Looking for XMPP URI UID
            gpgme_user_id_t uid = key->uids;

            while (uid) {
                if (uid->name && strlen(uid->name) >= 10) {
                    if (g_strcmp0(uid->name, xmppuri->str) == 0) {
                        gpgme_release(ctx);
                        return key;
                    }
                }
                uid = uid->next;
            }
            gpgme_key_unref(key);
            error = gpgme_op_keylist_next(ctx, &key);
        }
    }
    gpgme_release(ctx);

    return key;
}

static gboolean
_ox_key_is_usable(gpgme_key_t key, const char* const barejid, gboolean secret)
{
    gboolean result = TRUE;

    if (key->revoked || key->expired || key->disabled) {
        cons_show_error("%s's key is revoked, expired or disabled", barejid);
        log_info("OX:  %s's key is revoked, expired or disabled", barejid);
        result = FALSE;
    }

    // This might be a nice features but AFAIK is not defined in the XEP.
    // If we add this we need to expand our documentation on how to set the
    // trust leven in gpg. I'll add an example to this commit body.
    /*
    if (key->owner_trust < GPGME_VALIDITY_MARGINAL) {
        cons_show_error(" %s's key is has a trust level lower than marginal", barejid);
        log_info("OX: Owner trust of %s's key is < GPGME_VALIDITY_MARGINAL", barejid);
        result = FALSE;
    }
    */

    return result;
}

/*!
 * @brief XMPP-OX: Decrypt OX Message.
 *
 * @param base64  base64_encode OpenPGP message.
 *
 * @result decrypt XMPP OX Message NULL terminated C-String
 */
char*
p_ox_gpg_decrypt(char* base64)
{
    // if there is no private key avaibale,
    // we don't try do decrypt
    if (!ox_is_private_key_available(connection_get_barejid())) {
        return NULL;
    }
    setlocale(LC_ALL, "");
    gpgme_check_version(NULL);
    gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);

    if (GPG_ERR_NO_ERROR != error) {
        log_error("OX: gpgme_new failed: %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

    error = gpgme_set_protocol(ctx, GPGME_PROTOCOL_OPENPGP);
    if (error != 0) {
        log_error("OX: %s", gpgme_strerror(error));
    }

    gpgme_set_armor(ctx, 0);
    gpgme_set_textmode(ctx, 0);
    gpgme_set_offline(ctx, 1);
    gpgme_set_keylist_mode(ctx, GPGME_KEYLIST_MODE_LOCAL);
    if (error != 0) {
        log_error("OX: %s", gpgme_strerror(error));
    }

    gpgme_data_t plain = NULL;
    gpgme_data_t cipher = NULL;

    gsize s;
    guchar* encrypted = g_base64_decode(base64, &s);
    error = gpgme_data_new_from_mem(&cipher, (char*)encrypted, s, 0);
    if (error != 0) {
        log_error("OX: gpgme_data_new_from_mem: %s", gpgme_strerror(error));
        return NULL;
    }

    error = gpgme_data_new(&plain);
    if (error != 0) {
        log_error("OX: %s", gpgme_strerror(error));
        return NULL;
    }

    error = gpgme_op_decrypt_verify(ctx, cipher, plain);
    if (error != 0) {
        log_error("OX: gpgme_op_decrypt: %s", gpgme_strerror(error));
        error = gpgme_op_decrypt(ctx, cipher, plain);
        if (error != 0) {
            return NULL;
        }
    }

    size_t len;
    char* plain_str = gpgme_data_release_and_get_mem(plain, &len);
    char* result = NULL;
    if (plain_str) {
        result = strndup(plain_str, len);
        gpgme_free(plain_str);
    }

    return result;
}

/*!
 * \brief Read public key from file.
 *
 * This function is used the read a public key from a file.
 *
 * This function is used to read a key and push it on PEP. There are some checks
 * in this function:
 *
 * Key is not
 * - gkey->revoked
 * - gkey->expired
 * - gkey->disabled
 * - gkey->invalid
 * - gkey->secret
 *
 * Only one key in the file.
 *
 * \param filename filename to read the file.
 * \param key result with base64 encode key or NULL
 * \param fp result with the fingerprint or NULL
 *
 */
void
p_ox_gpg_readkey(const char* const filename, char** key, char** fp)
{
    log_info("PX: Read OpenPGP Key from file %s", filename);

    GError* error = NULL;
    gchar* data = NULL;
    gsize size = -1;

    gboolean success = g_file_get_contents(filename,
                                           &data,
                                           &size,
                                           &error);
    if (success) {
        setlocale(LC_ALL, "");
        gpgme_check_version(NULL);
        gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
        gpgme_ctx_t ctx;
        gpgme_error_t error = gpgme_new(&ctx);

        if (GPG_ERR_NO_ERROR != error) {
            log_error("OX: Read OpenPGP key from file: gpgme_new failed: %s", gpgme_strerror(error));
            return;
        }

        error = gpgme_set_protocol(ctx, GPGME_PROTOCOL_OPENPGP);
        if (error != GPG_ERR_NO_ERROR) {
            log_error("OX: Read OpenPGP key from file: set GPGME_PROTOCOL_OPENPGP:  %s", gpgme_strerror(error));
            return;
        }

        gpgme_set_armor(ctx, 0);
        gpgme_set_textmode(ctx, 0);
        gpgme_set_offline(ctx, 1);
        gpgme_set_keylist_mode(ctx, GPGME_KEYLIST_MODE_LOCAL);

        gpgme_data_t gpgme_data = NULL;
        error = gpgme_data_new(&gpgme_data);
        if (error != GPG_ERR_NO_ERROR) {
            log_error("OX: Read OpenPGP key from file: gpgme_data_new %s", gpgme_strerror(error));
            return;
        }

        error = gpgme_data_new_from_mem(&gpgme_data, (char*)data, size, 0);
        if (error != GPG_ERR_NO_ERROR) {
            log_error("OX: Read OpenPGP key from file: gpgme_data_new_from_mem %s", gpgme_strerror(error));
            return;
        }
        error = gpgme_op_keylist_from_data_start(ctx, gpgme_data, 0);
        if (error != GPG_ERR_NO_ERROR) {
            log_error("OX: Read OpenPGP key from file: gpgme_op_keylist_from_data_start %s", gpgme_strerror(error));
            return;
        }
        gpgme_key_t gkey;
        error = gpgme_op_keylist_next(ctx, &gkey);
        if (error != GPG_ERR_NO_ERROR) {
            log_error("OX: Read OpenPGP key from file: gpgme_op_keylist_next %s", gpgme_strerror(error));
            return;
        }

        gpgme_key_t end;
        error = gpgme_op_keylist_next(ctx, &end);
        if (error == GPG_ERR_NO_ERROR) {
            log_error("OX: Read OpenPGP key from file: ambiguous key");
            return;
        }

        if (gkey->revoked || gkey->expired || gkey->disabled || gkey->invalid || gkey->secret) {
            log_error("OX: Read OpenPGP key from file: Key is not valid");
            return;
        }

        gchar* keybase64 = g_base64_encode((const guchar*)data, size);

        *key = strdup(keybase64);
        *fp = strdup(gkey->fpr);
    } else {
        log_error("OX: Read OpenPGP key from file: Unable to read file: %s", error->message);
    }
}

gboolean
p_ox_gpg_import(char* base64_public_key)
{
    gsize size = -1;
    guchar* key = g_base64_decode(base64_public_key, &size);

    setlocale(LC_ALL, "");
    gpgme_check_version(NULL);
    gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));
    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);

    if (GPG_ERR_NO_ERROR != error) {
        log_error("OX: Read OpenPGP key from file: gpgme_new failed: %s", gpgme_strerror(error));
        return FALSE;
    }

    error = gpgme_set_protocol(ctx, GPGME_PROTOCOL_OPENPGP);
    if (error != GPG_ERR_NO_ERROR) {
        log_error("OX: Read OpenPGP key from file: set GPGME_PROTOCOL_OPENPGP:  %s", gpgme_strerror(error));
        return FALSE;
    }

    gpgme_set_armor(ctx, 0);
    gpgme_set_textmode(ctx, 0);
    gpgme_set_offline(ctx, 1);
    gpgme_set_keylist_mode(ctx, GPGME_KEYLIST_MODE_LOCAL);

    gpgme_data_t gpgme_data = NULL;
    error = gpgme_data_new(&gpgme_data);
    if (error != GPG_ERR_NO_ERROR) {
        log_error("OX: Read OpenPGP key from file: gpgme_data_new %s", gpgme_strerror(error));
        return FALSE;
    }

    gpgme_data_new_from_mem(&gpgme_data, (gchar*)key, size, 0);
    error = gpgme_op_import(ctx, gpgme_data);
    if (error != GPG_ERR_NO_ERROR) {
        log_error("OX: Failed to import key");
    }

    return TRUE;
}
