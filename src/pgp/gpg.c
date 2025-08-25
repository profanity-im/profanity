/*
 * gpg.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2012 - 2019 James Booth <boothj5@gmail.com>
 * Copyright (C) 2018 - 2025 Michael Vetter <jubalh@iodoru.org>
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
#include "pgp/gpg.h"
#include "config/files.h"
#include "tools/autocomplete.h"
#include "ui/ui.h"

#define PGP_SIGNATURE_HEADER  "-----BEGIN PGP SIGNATURE-----"
#define PGP_SIGNATURE_FOOTER  "-----END PGP SIGNATURE-----"
#define PGP_MESSAGE_HEADER    "-----BEGIN PGP MESSAGE-----"
#define PGP_MESSAGE_FOOTER    "-----END PGP MESSAGE-----"
#define PGP_PUBLIC_KEY_HEADER "-----BEGIN PGP PUBLIC KEY BLOCK-----"
#define PGP_PUBLIC_KEY_FOOTER "-----END PGP PUBLIC KEY BLOCK-----"

static const char* libversion = NULL;
static GHashTable* pubkeys;

static prof_keyfile_t pubkeys_prof_keyfile;
static GKeyFile* pubkeyfile;

static char* passphrase;
static char* passphrase_attempt;

static Autocomplete key_ac;

static char* _remove_header_footer(char* str, const char* const footer);
static char* _add_header_footer(const char* const str, const char* const header, const char* const footer);
static char* _gpgme_data_to_char(gpgme_data_t data);
static void _save_pubkeys(void);
static ProfPGPKey* _gpgme_key_to_ProfPGPKey(gpgme_key_t key);
static const char* _gpgme_key_get_email(gpgme_key_t key);

void
_p_gpg_free_pubkeyid(ProfPGPPubKeyId* pubkeyid)
{
    if (pubkeyid) {
        free(pubkeyid->id);
    }
    free(pubkeyid);
}

static gpgme_error_t*
_p_gpg_passphrase_cb(void* hook, const char* uid_hint, const char* passphrase_info, int prev_was_bad, int fd)
{
    if (passphrase) {
        gpgme_io_write(fd, passphrase, strlen(passphrase));
    } else {
        GString* pass_term = g_string_new("");

        auto_char char* password = ui_ask_pgp_passphrase(uid_hint, prev_was_bad);
        if (password) {
            g_string_append(pass_term, password);
        }

        g_string_append(pass_term, "\n");
        if (passphrase_attempt) {
            free(passphrase_attempt);
        }
        passphrase_attempt = g_string_free(pass_term, FALSE);

        gpgme_io_write(fd, passphrase_attempt, strlen(passphrase_attempt));
    }

    return 0;
}

static void
_p_gpg_close(void)
{
    if (pubkeys) {
        g_hash_table_destroy(pubkeys);
        pubkeys = NULL;
    }

    free_keyfile(&pubkeys_prof_keyfile);
    pubkeyfile = NULL;

    autocomplete_free(key_ac);
    key_ac = NULL;

    if (passphrase) {
        free(passphrase);
        passphrase = NULL;
    }

    if (passphrase_attempt) {
        free(passphrase_attempt);
        passphrase_attempt = NULL;
    }
}

void
p_gpg_init(void)
{
    libversion = gpgme_check_version(NULL);
    log_debug("GPG: Found gpgme version: %s", libversion);
    gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));

    prof_add_shutdown_routine(_p_gpg_close);

    pubkeys = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)_p_gpg_free_pubkeyid);

    key_ac = autocomplete_new();
    GHashTable* keys = p_gpg_list_keys();
    p_gpg_free_keys(keys);

    passphrase = NULL;
    passphrase_attempt = NULL;
}

void
p_gpg_on_connect(const char* const barejid)
{
    gchar* pubsloc = files_file_in_account_data_path(DIR_PGP, barejid, "pubkeys");
    if (!pubsloc) {
        log_error("Could not create directory for account %s.", barejid);
        cons_show_error("Could not create directory for account %s.", barejid);
        return;
    }
    load_custom_keyfile(&pubkeys_prof_keyfile, pubsloc);
    pubkeyfile = pubkeys_prof_keyfile.keyfile;

    // load each keyid
    gsize len = 0;
    auto_gcharv gchar** jids = g_key_file_get_groups(pubkeyfile, &len);

    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);

    if (error) {
        log_error("GPG: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return;
    }

    for (int i = 0; i < len; i++) {
        GError* gerr = NULL;
        gchar* jid = jids[i];
        auto_gchar gchar* keyid = g_key_file_get_string(pubkeyfile, jid, "keyid", &gerr);
        if (gerr) {
            log_error("Error loading PGP key id for %s", jid);
            g_error_free(gerr);
        } else {
            gpgme_key_t key = NULL;
            error = gpgme_get_key(ctx, keyid, &key, 0);
            if (error || key == NULL) {
                log_warning("GPG: Failed to get key for %s: %s %s", jid, gpgme_strsource(error), gpgme_strerror(error));
                continue;
            }

            ProfPGPPubKeyId* pubkeyid = malloc(sizeof(ProfPGPPubKeyId));
            pubkeyid->id = strdup(keyid);
            pubkeyid->received = FALSE;
            g_hash_table_replace(pubkeys, strdup(jid), pubkeyid);
            gpgme_key_unref(key);
        }
    }

    gpgme_release(ctx);

    _save_pubkeys();
}

void
p_gpg_on_disconnect(void)
{
    _p_gpg_close();
    pubkeys = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)_p_gpg_free_pubkeyid);
    key_ac = autocomplete_new();
}

gboolean
p_gpg_addkey(const char* const jid, const char* const keyid)
{
    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);
    if (error) {
        log_error("GPG: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return FALSE;
    }

    gpgme_key_t key = NULL;
    error = gpgme_get_key(ctx, keyid, &key, 0);
    gpgme_release(ctx);

    if (error || key == NULL) {
        log_error("GPG: Failed to get key. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return FALSE;
    }

    // save to public key file
    g_key_file_set_string(pubkeyfile, jid, "keyid", keyid);
    _save_pubkeys();

    // update in memory pubkeys list
    ProfPGPPubKeyId* pubkeyid = malloc(sizeof(ProfPGPPubKeyId));
    pubkeyid->id = strdup(keyid);
    pubkeyid->received = FALSE;
    g_hash_table_replace(pubkeys, strdup(jid), pubkeyid);
    gpgme_key_unref(key);

    return TRUE;
}

ProfPGPKey*
p_gpg_key_new(void)
{
    ProfPGPKey* p_pgpkey = malloc(sizeof(ProfPGPKey));
    p_pgpkey->id = NULL;
    p_pgpkey->name = NULL;
    p_pgpkey->fp = NULL;
    p_pgpkey->encrypt = FALSE;
    p_pgpkey->sign = FALSE;
    p_pgpkey->certify = FALSE;
    p_pgpkey->authenticate = FALSE;
    p_pgpkey->secret = FALSE;

    return p_pgpkey;
}

void
p_gpg_free_key(ProfPGPKey* key)
{
    if (key) {
        free(key->id);
        free(key->name);
        free(key->fp);
        free(key);
    }
}

/**
 * Retrieve a list of GPG keys and create a hash table of ProfPGPKey objects.
 *
 * This function utilizes the GPGME library to retrieve both public and secret keys.
 * It iterates over the keys and their subkeys to populate a hash table with ProfPGPKey objects.
 * The key name is used as the key in the hash table, and the ProfPGPKey object is the corresponding value.
 *
 * @return A newly created GHashTable* containing ProfPGPKey objects, with the key name as the key.
 *         Returns NULL if an error occurs during key retrieval or if memory allocation fails.
 *
 * @note The returned hash table should be released using p_gpg_free_keys()
 *       when they are no longer needed to avoid memory leaks.
 *
 * @note This function may perform additional operations, such as autocomplete, related to the retrieved keys.
 */
GHashTable*
p_gpg_list_keys(void)
{
    gpgme_error_t error;
    GHashTable* result = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)p_gpg_free_key);

    gpgme_ctx_t ctx;
    error = gpgme_new(&ctx);
    if (error) {
        log_error("GPG: Could not create GPGME context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        g_hash_table_destroy(result);
        return NULL;
    }

    error = gpgme_op_keylist_start(ctx, NULL, 0);
    if (error == GPG_ERR_NO_ERROR) {
        gpgme_key_t key;

        error = gpgme_op_keylist_next(ctx, &key);
        while (!error) {
            ProfPGPKey* p_pgpkey = _gpgme_key_to_ProfPGPKey(key);
            if (p_pgpkey != NULL) {
                g_hash_table_insert(result, strdup(p_pgpkey->name), p_pgpkey);
            }

            gpgme_key_unref(key);
            error = gpgme_op_keylist_next(ctx, &key);
        }
    }

    error = gpgme_op_keylist_start(ctx, NULL, 1);
    if (error == GPG_ERR_NO_ERROR) {
        gpgme_key_t key;
        error = gpgme_op_keylist_next(ctx, &key);
        while (!error) {
            gpgme_subkey_t sub = key->subkeys;
            while (sub) {
                if (sub->secret) {
                    ProfPGPKey* p_pgpkey = g_hash_table_lookup(result, key->uids->uid);
                    if (p_pgpkey) {
                        p_pgpkey->secret = TRUE;
                    }
                }
                sub = sub->next;
            }

            gpgme_key_unref(key);
            error = gpgme_op_keylist_next(ctx, &key);
        }
    }

    gpgme_release(ctx);

    // TODO: move autocomplete in other place
    autocomplete_clear(key_ac);
    GList* ids = g_hash_table_get_keys(result);
    GList* curr = ids;
    while (curr) {
        ProfPGPKey* key = g_hash_table_lookup(result, curr->data);
        autocomplete_add(key_ac, key->id);
        curr = curr->next;
    }
    g_list_free(ids);

    return result;
}

void
p_gpg_free_keys(GHashTable* keys)
{
    g_hash_table_destroy(keys);
}

GHashTable*
p_gpg_pubkeys(void)
{
    return pubkeys;
}

const char*
p_gpg_libver(void)
{
    if (libversion == NULL) {
        libversion = gpgme_check_version(NULL);
    }
    return libversion;
}

gboolean
p_gpg_valid_key(const char* const keyid, char** err_str)
{
    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);
    if (error) {
        log_error("GPG: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        if (err_str) {
            *err_str = strdup(gpgme_strerror(error));
        }
        return FALSE;
    }

    gpgme_key_t key = NULL;
    error = gpgme_get_key(ctx, keyid, &key, 1);

    if (error || key == NULL) {
        log_error("GPG: Failed to get key. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        if (err_str) {
            *err_str = strdup(error ? gpgme_strerror(error) : "gpgme didn't return any error, but it didn't return a key");
        }
        gpgme_release(ctx);
        return FALSE;
    }

    gpgme_release(ctx);
    gpgme_key_unref(key);
    return TRUE;
}

gboolean
p_gpg_available(const char* const barejid)
{
    char* pubkey = g_hash_table_lookup(pubkeys, barejid);
    return (pubkey != NULL);
}

void
p_gpg_verify(const char* const barejid, const char* const sign)
{
    if (!sign) {
        return;
    }

    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);

    if (error) {
        log_error("GPG: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return;
    }

    auto_char char* sign_with_header_footer = _add_header_footer(sign, PGP_SIGNATURE_HEADER, PGP_SIGNATURE_FOOTER);
    gpgme_data_t sign_data;
    gpgme_data_new_from_mem(&sign_data, sign_with_header_footer, strlen(sign_with_header_footer), 1);

    gpgme_data_t plain_data;
    gpgme_data_new(&plain_data);

    error = gpgme_op_verify(ctx, sign_data, NULL, plain_data);
    gpgme_data_release(sign_data);
    gpgme_data_release(plain_data);

    if (error) {
        log_error("GPG: Failed to verify. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        gpgme_release(ctx);
        return;
    }

    gpgme_verify_result_t result = gpgme_op_verify_result(ctx);
    if (result) {
        if (result->signatures) {
            gpgme_key_t key = NULL;
            error = gpgme_get_key(ctx, result->signatures->fpr, &key, 0);
            if (error) {
                log_debug("Could not find PGP key with ID %s for %s", result->signatures->fpr, barejid);
            } else {
                log_debug("Fingerprint found for %s: %s ", barejid, key->subkeys->fpr);
                ProfPGPPubKeyId* pubkeyid = malloc(sizeof(ProfPGPPubKeyId));
                pubkeyid->id = strdup(key->subkeys->keyid);
                pubkeyid->received = TRUE;
                g_hash_table_replace(pubkeys, strdup(barejid), pubkeyid);
            }

            gpgme_key_unref(key);
        }
    }

    gpgme_release(ctx);
}

char*
p_gpg_sign(const char* const str, const char* const fp)
{
    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);
    if (error) {
        log_error("GPG: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

    gpgme_set_passphrase_cb(ctx, (gpgme_passphrase_cb_t)_p_gpg_passphrase_cb, NULL);

    gpgme_key_t key = NULL;
    error = gpgme_get_key(ctx, fp, &key, 1);

    if (error || key == NULL) {
        log_error("GPG: Failed to get key. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        gpgme_release(ctx);
        return NULL;
    }

    gpgme_signers_clear(ctx);
    error = gpgme_signers_add(ctx, key);
    gpgme_key_unref(key);

    if (error) {
        log_error("GPG: Failed to load signer. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        gpgme_release(ctx);
        return NULL;
    }

    auto_char char* str_or_empty = NULL;
    if (str) {
        str_or_empty = strdup(str);
    } else {
        str_or_empty = strdup("");
    }
    gpgme_data_t str_data;
    gpgme_data_new_from_mem(&str_data, str_or_empty, strlen(str_or_empty), 1);

    gpgme_data_t signed_data;
    gpgme_data_new(&signed_data);

    gpgme_set_armor(ctx, 1);
    error = gpgme_op_sign(ctx, str_data, signed_data, GPGME_SIG_MODE_DETACH);
    gpgme_data_release(str_data);
    gpgme_release(ctx);

    if (error) {
        log_error("GPG: Failed to sign string. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        gpgme_data_release(signed_data);
        return NULL;
    }

    char* result = NULL;

    size_t len = 0;
    char* signed_str = gpgme_data_release_and_get_mem(signed_data, &len);
    if (signed_str) {
        GString* signed_gstr = g_string_new("");
        g_string_append_len(signed_gstr, signed_str, len);
        result = _remove_header_footer(signed_gstr->str, PGP_SIGNATURE_FOOTER);
        g_string_free(signed_gstr, TRUE);
        gpgme_free(signed_str);
    }

    if (passphrase_attempt) {
        passphrase = strdup(passphrase_attempt);
    }

    return result;
}

char*
p_gpg_encrypt(const char* const barejid, const char* const message, const char* const fp)
{
    ProfPGPPubKeyId* pubkeyid = g_hash_table_lookup(pubkeys, barejid);
    if (!pubkeyid) {
        return NULL;
    }
    if (!pubkeyid->id) {
        return NULL;
    }

    gpgme_key_t keys[3];

    keys[0] = NULL;
    keys[1] = NULL;
    keys[2] = NULL;

    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);
    if (error) {
        log_error("GPG: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

    gpgme_key_t receiver_key;
    error = gpgme_get_key(ctx, pubkeyid->id, &receiver_key, 0);
    if (error || receiver_key == NULL) {
        log_error("GPG: Failed to get receiver_key. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        gpgme_release(ctx);
        return NULL;
    }
    keys[0] = receiver_key;

    gpgme_key_t sender_key = NULL;
    error = gpgme_get_key(ctx, fp, &sender_key, 0);
    if (error || sender_key == NULL) {
        log_error("GPG: Failed to get sender_key. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        gpgme_release(ctx);
        return NULL;
    }
    keys[1] = sender_key;

    gpgme_data_t plain;
    gpgme_data_new_from_mem(&plain, message, strlen(message), 1);

    gpgme_data_t cipher;
    gpgme_data_new(&cipher);

    gpgme_set_armor(ctx, 1);
    error = gpgme_op_encrypt(ctx, keys, GPGME_ENCRYPT_ALWAYS_TRUST, plain, cipher);
    gpgme_data_release(plain);
    gpgme_release(ctx);
    gpgme_key_unref(receiver_key);
    gpgme_key_unref(sender_key);

    if (error) {
        log_error("GPG: Failed to encrypt message. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

    size_t len;
    char* cipher_str = gpgme_data_release_and_get_mem(cipher, &len);

    char* result = NULL;
    if (cipher_str) {
        GString* cipher_gstr = g_string_new("");
        g_string_append_len(cipher_gstr, cipher_str, len);
        result = _remove_header_footer(cipher_gstr->str, PGP_MESSAGE_FOOTER);
        g_string_free(cipher_gstr, TRUE);
        gpgme_free(cipher_str);
    }

    return result;
}

char*
p_gpg_decrypt(const char* const cipher)
{
    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);

    if (error) {
        log_error("GPG: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

    gpgme_set_passphrase_cb(ctx, (gpgme_passphrase_cb_t)_p_gpg_passphrase_cb, NULL);

    auto_char char* cipher_with_headers = _add_header_footer(cipher, PGP_MESSAGE_HEADER, PGP_MESSAGE_FOOTER);
    gpgme_data_t cipher_data;
    gpgme_data_new_from_mem(&cipher_data, cipher_with_headers, strlen(cipher_with_headers), 1);

    gpgme_data_t plain_data;
    gpgme_data_new(&plain_data);

    error = gpgme_op_decrypt(ctx, cipher_data, plain_data);
    gpgme_data_release(cipher_data);

    if (error) {
        log_error("GPG: Failed to encrypt message. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        gpgme_data_release(plain_data);
        gpgme_release(ctx);
        return NULL;
    }

    gpgme_decrypt_result_t res = gpgme_op_decrypt_result(ctx);
    if (res) {
        GString* recipients_str = g_string_new("");
        gpgme_recipient_t recipient = res->recipients;
        while (recipient) {
            gpgme_key_t key;
            error = gpgme_get_key(ctx, recipient->keyid, &key, 1);

            if (!error && key) {
                const char* addr = _gpgme_key_get_email(key);
                if (addr) {
                    g_string_append(recipients_str, addr);
                }
                gpgme_key_unref(key);
            }

            if (recipient->next) {
                g_string_append(recipients_str, ", ");
            }

            recipient = recipient->next;
        }

        log_debug("GPG: Decrypted message for recipients: %s", recipients_str->str);
        g_string_free(recipients_str, TRUE);
    }
    gpgme_release(ctx);

    if (passphrase_attempt) {
        passphrase = strdup(passphrase_attempt);
    }

    return _gpgme_data_to_char(plain_data);
}

void
p_gpg_free_decrypted(char* decrypted)
{
    g_free(decrypted);
}

char*
p_gpg_autocomplete_key(const char* const search_str, gboolean previous, void* context)
{
    return autocomplete_complete(key_ac, search_str, TRUE, previous);
}

void
p_gpg_autocomplete_key_reset(void)
{
    autocomplete_reset(key_ac);
}

char*
p_gpg_format_fp_str(char* fp)
{
    if (!fp) {
        return NULL;
    }

    GString* format = g_string_new("");
    int len = strlen(fp);
    for (int i = 0; i < len; i++) {
        g_string_append_c(format, fp[i]);
        if (((i + 1) % 4 == 0) && (i + 1 < len)) {
            g_string_append_c(format, ' ');
        }
    }

    return g_string_free(format, FALSE);
}

/**
 * Returns the public key data for the given key ID.
 *
 * @param keyid The key ID for which to retrieve the public key data.
 *              If the key ID is empty or NULL, returns NULL.
 * @return The public key data as a null-terminated char* string allocated using malloc.
 *         The returned string should be freed by the caller.
 *         Returns NULL on error, and errors are written to the error log.
 */
char*
p_gpg_get_pubkey(const char* keyid)
{
    if (!keyid || *keyid == '\0') {
        return NULL;
    }

    gpgme_ctx_t context = NULL;
    gpgme_error_t error = GPG_ERR_NO_ERROR;
    gpgme_data_t data = NULL;

    error = gpgme_new(&context);
    if (error != GPG_ERR_NO_ERROR) {
        log_error("GPG: Failed to create gpgme context. %s", gpgme_strerror(error));
        goto cleanup;
    }

    error = gpgme_data_new(&data);
    if (error != GPG_ERR_NO_ERROR || data == NULL) {
        log_error("GPG: Failed to create new gpgme data. %s", gpgme_strerror(error));
        goto cleanup;
    }

    gpgme_set_armor(context, 1);

    error = gpgme_op_export(context, keyid, GPGME_EXPORT_MODE_MINIMAL, data);
    if (error != GPG_ERR_NO_ERROR) {
        log_error("GPG: Failed to export public key. %s", gpgme_strerror(error));
        goto cleanup;
    }

cleanup:
    gpgme_release(context);
    return _gpgme_data_to_char(data);
}

/**
 * Validate that the provided buffer has the format of an armored public key.
 *
 * This function only briefly checks for presence of armored header and footer.
 *
 * @param buffer The buffer containing the key data.
 * @return TRUE if the buffer has the expected header and footer of an armored public key, FALSE otherwise.
 */
gboolean
p_gpg_is_public_key_format(const char* buffer)
{
    if (buffer == NULL || buffer[0] == '\0') {
        return false;
    }

    const char* headerPos = strstr(buffer, PGP_PUBLIC_KEY_HEADER);
    if (headerPos == NULL) {
        return false;
    }

    const char* footerPos = strstr(buffer, PGP_PUBLIC_KEY_FOOTER);

    return (footerPos != NULL && footerPos > headerPos);
}

/**
 * Imports a PGP public key(s) from a buffer.
 *
 * @param buffer The buffer containing the PGP key data.
 * @return A pointer to the first imported ProfPGPKey structure, or NULL if an error occurs.
 *
 * @note The caller is responsible for freeing the memory of the returned ProfPGPKey structure
 *       by calling p_gpg_free_key() to avoid resource leaks.
 */
ProfPGPKey*
p_gpg_import_pubkey(const char* buffer)
{
    gpgme_ctx_t ctx;
    ProfPGPKey* result = NULL;
    gpgme_error_t error = gpgme_new(&ctx);
    if (error != GPG_ERR_NO_ERROR) {
        log_error("GPG: Error creating GPGME context");
        goto out;
    }

    gpgme_data_t key_data;
    error = gpgme_data_new_from_mem(&key_data, buffer, strlen(buffer), 1);
    if (error != GPG_ERR_NO_ERROR) {
        log_error("GPG: Error creating GPGME data from buffer");
        goto out;
    }

    error = gpgme_op_import(ctx, key_data);

    gpgme_data_release(key_data);

    if (error != GPG_ERR_NO_ERROR) {
        log_error("GPG: Error importing key data");
        goto out;
    }

    gpgme_import_result_t import_result = gpgme_op_import_result(ctx);
    gpgme_import_status_t status = import_result->imports;
    gboolean is_valid = (status && status->result == GPG_ERR_NO_ERROR);

    if (!is_valid) {
        log_error("GPG: Error importing PGP key (%s).", status ? gpgme_strerror(status->result) : "Invalid import status.");
        goto out;
    }

    gpgme_key_t key = NULL;
    error = gpgme_get_key(ctx, status->fpr, &key, 0);
    if (error != GPG_ERR_NO_ERROR) {
        log_error("GPG: Unable to find imported PGP key (%s).", gpgme_strerror(error));
        goto out;
    }

    result = _gpgme_key_to_ProfPGPKey(key);
    gpgme_key_release(key);

out:
    gpgme_release(ctx);
    return result;
}

/**
 * Converts a GPGME key struct to a ProfPGPKey struct.
 *
 * @param key The GPGME key struct to convert.
 * @return A newly allocated ProfPGPKey struct populated with the converted data,
 *         or NULL if an error occurs.
 *
 * @note The caller is responsible for freeing the memory of the returned ProfPGPKey structure
 *       by calling p_gpg_free_key() to avoid resource leaks.
 */
static ProfPGPKey*
_gpgme_key_to_ProfPGPKey(gpgme_key_t key)
{
    if (key == NULL || key->uids == NULL
        || key->subkeys == NULL || key->uids->uid == NULL) {
        return NULL;
    }

    ProfPGPKey* p_pgpkey = p_gpg_key_new();
    gpgme_subkey_t sub = key->subkeys;
    p_pgpkey->id = strdup(sub->keyid);
    p_pgpkey->name = strdup(key->uids->uid);
    p_pgpkey->fp = strdup(sub->fpr);

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
    return p_pgpkey;
}

/**
 * Extract the first email address from a gpgme_key_t object.
 * This function provides backwards compatibility for both old and new GPGME versions.
 * - GPGME < 2.0.0: Uses gpgme_key_get_string_attr (if available)
 * - GPGME >= 2.0.0: Uses modern key->uids->email API
 *
 * @param key The gpgme_key_t object to extract email from.
 * @return The first email address found in the key's user IDs, or NULL if none found.
 *         The returned string should not be freed as it points to internal gpgme memory.
 */
static const char*
_gpgme_key_get_email(gpgme_key_t key)
{
    if (!key) {
        return NULL;
    }

#ifdef GPGME_ATTR_EMAIL
    /* Use deprecated function if available (GPGME < 2.0.0) */
    return gpgme_key_get_string_attr(key, GPGME_ATTR_EMAIL, NULL, 0);
#else
    /* Use modern API for GPGME >= 2.0.0 */
    gpgme_user_id_t uid = key->uids;
    while (uid) {
        if (uid->email && strlen(uid->email) > 0) {
            return uid->email;
        }
        uid = uid->next;
    }

    return NULL;
#endif
}

/**
 * Convert a gpgme_data_t object to a null-terminated char* string.
 *
 * @param data The gpgme_data_t object to convert.
 * @return The converted string allocated using malloc, which should be freed by the caller.
 *         If an error occurs or the data is empty, NULL is returned and errors are written to the error log.
 */
static char*
_gpgme_data_to_char(gpgme_data_t data)
{
    size_t buffer_size = 0;
    char* gpgme_buffer = gpgme_data_release_and_get_mem(data, &buffer_size);

    if (!gpgme_buffer) {
        log_error("GPG: Unable to extract gpgmedata.");
        return NULL;
    }

    char* buffer = malloc(buffer_size + 1);
    memcpy(buffer, gpgme_buffer, buffer_size);
    buffer[buffer_size] = '\0';
    gpgme_free(gpgme_buffer);

    return buffer;
}

static char*
_remove_header_footer(char* str, const char* const footer)
{
    int pos = 0;
    int newlines = 0;

    while (newlines < 2) {
        if (str[pos] == '\n') {
            newlines++;
        }
        pos++;

        if (str[pos] == '\0') {
            return NULL;
        }
    }

    char* stripped = strdup(&str[pos]);
    char* footer_start = g_strrstr(stripped, footer);
    footer_start[0] = '\0';

    return stripped;
}

static char*
_add_header_footer(const char* const str, const char* const header, const char* const footer)
{
    return g_strdup_printf("%s\n\n%s\n%s", header, str, footer);
}

static void
_save_pubkeys(void)
{
    save_keyfile(&pubkeys_prof_keyfile);
}
