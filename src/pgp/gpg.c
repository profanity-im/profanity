/*
 * gpg.c
 *
 * Copyright (C) 2012 - 2015 James Booth <boothj5@gmail.com>
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

#include "config.h"

#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <gpgme.h>

#include "pgp/gpg.h"
#include "log.h"
#include "common.h"

#define PGP_SIGNATURE_HEADER "-----BEGIN PGP SIGNATURE-----"
#define PGP_SIGNATURE_FOOTER "-----END PGP SIGNATURE-----"
#define PGP_MESSAGE_HEADER "-----BEGIN PGP MESSAGE-----"
#define PGP_MESSAGE_FOOTER "-----END PGP MESSAGE-----"

static const char *libversion;
static GHashTable *fingerprints;

static gchar *fpsloc;
static GKeyFile *fpskeyfile;

static char* _remove_header_footer(char *str, const char * const footer);
static char* _add_header_footer(const char * const str, const char * const header, const char * const footer);
static void _save_fps(void);

void
p_gpg_init(void)
{
    libversion = gpgme_check_version(NULL);
    log_debug("GPG: Found gpgme version: %s", libversion);
    gpgme_set_locale(NULL, LC_CTYPE, setlocale(LC_CTYPE, NULL));

    fingerprints = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
}

void
p_gpg_close(void)
{
    if (fingerprints) {
        g_hash_table_destroy(fingerprints);
        fingerprints = NULL;
    }

    if (fpskeyfile) {
        g_key_file_free(fpskeyfile);
        fpskeyfile = NULL;
    }

    free(fpsloc);
    fpsloc = NULL;
}

void
p_gpg_on_connect(const char * const barejid)
{
    gchar *data_home = xdg_get_data_home();
    GString *fpsfile = g_string_new(data_home);
    free(data_home);

    gchar *account_dir = str_replace(barejid, "@", "_at_");
    g_string_append(fpsfile, "/profanity/pgp/");
    g_string_append(fpsfile, account_dir);
    free(account_dir);

    // mkdir if doesn't exist for account
    errno = 0;
    int res = g_mkdir_with_parents(fpsfile->str, S_IRWXU);
    if (res == -1) {
        char *errmsg = strerror(errno);
        if (errmsg) {
            log_error("Error creating directory: %s, %s", fpsfile->str, errmsg);
        } else {
            log_error("Error creating directory: %s", fpsfile->str);
        }
    }

    // create or read fingerprints keyfile
    g_string_append(fpsfile, "/fingerprints");
    fpsloc = fpsfile->str;
    g_string_free(fpsfile, FALSE);

    if (g_file_test(fpsloc, G_FILE_TEST_EXISTS)) {
        g_chmod(fpsloc, S_IRUSR | S_IWUSR);
    }

    fpskeyfile = g_key_file_new();
    g_key_file_load_from_file(fpskeyfile, fpsloc, G_KEY_FILE_KEEP_COMMENTS, NULL);

    // load each keyid
    gsize len = 0;
    gchar **jids = g_key_file_get_groups(fpskeyfile, &len);

    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);

    if (error) {
        log_error("GPG: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        g_strfreev(jids);
        return;
    }

    int i = 0;
    for (i = 0; i < len; i++) {
        GError *gerr = NULL;
        gchar *jid = jids[i];
        gchar *keyid = g_key_file_get_string(fpskeyfile, jid, "keyid", &gerr);
        if (gerr) {
            log_error("Error loading PGP key id for %s", jid);
            g_error_free(gerr);
            g_free(keyid);
        } else {
            gpgme_key_t key = NULL;
            error = gpgme_get_key(ctx, keyid, &key, 1);
            g_free(keyid);
            if (error || key == NULL) {
                log_warning("GPG: Failed to get key for %s: %s %s", jid, gpgme_strsource(error), gpgme_strerror(error));
                continue;
            }

            g_hash_table_replace(fingerprints, strdup(jid), strdup(key->subkeys->fpr));
            gpgme_key_unref(key);
        }
    }

    gpgme_release(ctx);
    g_strfreev(jids);

    _save_fps();
}

void
p_gpg_on_disconnect(void)
{
    if (fingerprints) {
        g_hash_table_destroy(fingerprints);
        fingerprints = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    }

    if (fpskeyfile) {
        g_key_file_free(fpskeyfile);
        fpskeyfile = NULL;
    }

    free(fpsloc);
    fpsloc = NULL;
}

gboolean
p_gpg_addkey(const char * const jid, const char * const keyid)
{
    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);
    if (error) {
        log_error("GPG: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return FALSE;
    }

    gpgme_key_t key = NULL;
    error = gpgme_get_key(ctx, keyid, &key, 1);
    gpgme_release(ctx);

    if (error || key == NULL) {
        log_error("GPG: Failed to get key. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return FALSE;
    }

    // save to ID keyfile
    g_key_file_set_string(fpskeyfile, jid, "keyid", keyid);
    _save_fps();

    // update in memory fingerprint list
    g_hash_table_replace(fingerprints, strdup(jid), strdup(key->subkeys->fpr));
    gpgme_key_unref(key);

    return TRUE;
}

GSList *
p_gpg_list_keys(void)
{
    gpgme_error_t error;
    GSList *result = NULL;

    gpgme_ctx_t ctx;
    error = gpgme_new(&ctx);

    if (error) {
        log_error("GPG: Could not list keys. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

    error = gpgme_op_keylist_start(ctx, NULL, 1);
    if (error == GPG_ERR_NO_ERROR) {
        while (!error) {
            gpgme_key_t key;
            error = gpgme_op_keylist_next(ctx, &key);

            if (error) {
                break;
            }

            ProfPGPKey *p_pgpkey = malloc(sizeof(ProfPGPKey));
            p_pgpkey->id = strdup(key->subkeys->keyid);
            p_pgpkey->name = strdup(key->uids->uid);
            p_pgpkey->fp = strdup(key->subkeys->fpr);

            result = g_slist_append(result, p_pgpkey);

            gpgme_key_unref(key);
        }
    } else {
        log_error("GPG: Could not list keys. %s %s", gpgme_strsource(error), gpgme_strerror(error));
    }

    gpgme_release(ctx);

    return result;
}

GHashTable *
p_gpg_fingerprints(void)
{
    return fingerprints;
}

const char*
p_gpg_libver(void)
{
    return libversion;
}

void
p_gpg_free_key(ProfPGPKey *key)
{
    if (key) {
        free(key->id);
        free(key->name);
        free(key->fp);
        free(key);
    }
}

gboolean
p_gpg_valid_key(const char * const keyid)
{
    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);
    if (error) {
        log_error("GPG: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return FALSE;
    }

    gpgme_key_t key = NULL;
    error = gpgme_get_key(ctx, keyid, &key, 1);

    if (error || key == NULL) {
        log_error("GPG: Failed to get key. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        gpgme_release(ctx);
        return FALSE;
    }

    if (key) {
        gpgme_release(ctx);
        gpgme_key_unref(key);
        return TRUE;
    }

    gpgme_release(ctx);
    return FALSE;
}

gboolean
p_gpg_available(const char * const barejid)
{
    char *fp = g_hash_table_lookup(fingerprints, barejid);
    return (fp != NULL);
}

void
p_gpg_verify(const char * const barejid, const char *const sign)
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

    char *sign_with_header_footer = _add_header_footer(sign, PGP_SIGNATURE_HEADER, PGP_SIGNATURE_FOOTER);
    gpgme_data_t sign_data;
    gpgme_data_new_from_mem(&sign_data, sign_with_header_footer, strlen(sign_with_header_footer), 1);
    free(sign_with_header_footer);

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
            log_debug("Fingerprint found for %s: %s ", barejid, result->signatures->fpr);
            g_hash_table_replace(fingerprints, strdup(barejid), strdup(result->signatures->fpr));
        }
    }

    gpgme_release(ctx);
}

char*
p_gpg_sign(const char * const str, const char * const fp)
{
    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);
    if (error) {
        log_error("GPG: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

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

    char *str_or_empty = NULL;
    if (str) {
        str_or_empty = strdup(str);
    } else {
        str_or_empty = strdup("");
    }
    gpgme_data_t str_data;
    gpgme_data_new_from_mem(&str_data, str_or_empty, strlen(str_or_empty), 1);
    free(str_or_empty);

    gpgme_data_t signed_data;
    gpgme_data_new(&signed_data);

    gpgme_set_armor(ctx,1);
    error = gpgme_op_sign(ctx, str_data, signed_data, GPGME_SIG_MODE_DETACH);
    gpgme_data_release(str_data);
    gpgme_release(ctx);

    if (error) {
        log_error("GPG: Failed to sign string. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        gpgme_data_release(signed_data);
        return NULL;
    }

    char *result = NULL;

    size_t len = 0;
    char *signed_str = gpgme_data_release_and_get_mem(signed_data, &len);
    if (signed_str) {
        GString *signed_gstr = g_string_new("");
        g_string_append_len(signed_gstr, signed_str, len);
        result = _remove_header_footer(signed_gstr->str, PGP_SIGNATURE_FOOTER);
        g_string_free(signed_gstr, TRUE);
        gpgme_free(signed_str);
    }

    return result;
}

char *
p_gpg_encrypt(const char * const barejid, const char * const message)
{
    char *fp = g_hash_table_lookup(fingerprints, barejid);

    if (!fp) {
        return NULL;
    }

    gpgme_key_t keys[2];

    keys[0] = NULL;
    keys[1] = NULL;

    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);
    if (error) {
        log_error("GPG: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

    gpgme_key_t key;
    error = gpgme_get_key(ctx, fp, &key, 0);

    if (error || key == NULL) {
        log_error("GPG: Failed to get key. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        gpgme_release(ctx);
        return NULL;
    }

    keys[0] = key;

    gpgme_data_t plain;
    gpgme_data_new_from_mem(&plain, message, strlen(message), 1);

    gpgme_data_t cipher;
    gpgme_data_new(&cipher);

    gpgme_set_armor(ctx, 1);
    error = gpgme_op_encrypt(ctx, keys, GPGME_ENCRYPT_ALWAYS_TRUST, plain, cipher);
    gpgme_data_release(plain);
    gpgme_release(ctx);
    gpgme_key_unref(key);

    if (error) {
        log_error("GPG: Failed to encrypt message. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

    size_t len;
    char *cipher_str = gpgme_data_release_and_get_mem(cipher, &len);

    char *result = NULL;
    if (cipher_str) {
        GString *cipher_gstr = g_string_new("");
        g_string_append_len(cipher_gstr, cipher_str, len);
        result = _remove_header_footer(cipher_gstr->str, PGP_MESSAGE_FOOTER);
        g_string_free(cipher_gstr, TRUE);
        gpgme_free(cipher_str);
    }

    return result;
}

char *
p_gpg_decrypt(const char * const cipher)
{
    gpgme_ctx_t ctx;
    gpgme_error_t error = gpgme_new(&ctx);

    if (error) {
        log_error("GPG: Failed to create gpgme context. %s %s", gpgme_strsource(error), gpgme_strerror(error));
        return NULL;
    }

    char *cipher_with_headers = _add_header_footer(cipher, PGP_MESSAGE_HEADER, PGP_MESSAGE_FOOTER);
    gpgme_data_t cipher_data;
    gpgme_data_new_from_mem(&cipher_data, cipher_with_headers, strlen(cipher_with_headers), 1);
    free(cipher_with_headers);

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
        gpgme_recipient_t recipient = res->recipients;
        if (recipient) {
            gpgme_key_t key;
            error = gpgme_get_key(ctx, recipient->keyid, &key, 0);

            if (!error && key) {
                const char *addr = gpgme_key_get_string_attr(key, GPGME_ATTR_EMAIL, NULL, 0);
                if (addr) {
                    log_debug("GPG: Decrypted message for recipient: %s", addr);
                }
                gpgme_key_unref(key);
            }
        }
    }
    gpgme_release(ctx);

    size_t len = 0;
    char *plain_str = gpgme_data_release_and_get_mem(plain_data, &len);
    char *result = NULL;
    if (plain_str) {
        plain_str[len] = 0;
        result = g_strdup(plain_str);
    }
    gpgme_free(plain_str);

    return result;
}

void
p_gpg_free_decrypted(char *decrypted)
{
    g_free(decrypted);
}

static char*
_remove_header_footer(char *str, const char * const footer)
{
    int pos = 0;
    int newlines = 0;

    while (newlines < 3) {
        if (str[pos] == '\n') {
            newlines++;
        }
        pos++;

        if (str[pos] == '\0') {
            return NULL;
        }
    }

    char *stripped = strdup(&str[pos]);
    char *footer_start = g_strrstr(stripped, footer);
    footer_start[0] = '\0';

    return stripped;
}

static char*
_add_header_footer(const char * const str, const char * const header, const char * const footer)
{
    GString *result_str = g_string_new("");

    g_string_append(result_str, header);
    g_string_append(result_str, "\n\n");
    g_string_append(result_str, str);
    g_string_append(result_str, "\n");
    g_string_append(result_str, footer);

    char *result = result_str->str;
    g_string_free(result_str, FALSE);

    return result;
}

static void
_save_fps(void)
{
    gsize g_data_size;
    gchar *g_fps_data = g_key_file_to_data(fpskeyfile, &g_data_size, NULL);
    g_file_set_contents(fpsloc, g_fps_data, g_data_size, NULL);
    g_chmod(fpsloc, S_IRUSR | S_IWUSR);
    g_free(g_fps_data);
}
