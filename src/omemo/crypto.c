/*
 * crypto.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 Paul Fariello <paul@fariello.eu>
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
#include <assert.h>
#include <signal/signal_protocol.h>
#include <signal/signal_protocol_types.h>
#include <gcrypt.h>

#include "log.h"
#include "omemo/omemo.h"
#include "omemo/crypto.h"

int
omemo_crypto_init(void)
{
    if (!gcry_check_version(GCRYPT_VERSION)) {
        return -1;
    }

    gcry_control(GCRYCTL_SUSPEND_SECMEM_WARN);

    gcry_control(GCRYCTL_INIT_SECMEM, 16384, 0);

    gcry_control(GCRYCTL_RESUME_SECMEM_WARN);

    gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

    /* Ask for a first random buffer to ensure CSPRNG is initialized.
     * Thus we control the memleak produced by gcrypt initialization and we can
     * suppress it without having false negatives */
    gcry_free(gcry_random_bytes_secure(1, GCRY_VERY_STRONG_RANDOM));

    return 0;
}

int
omemo_random_func(uint8_t* data, size_t len, void* user_data)
{
    gcry_randomize(data, len, GCRY_VERY_STRONG_RANDOM);
    return 0;
}

int
omemo_hmac_sha256_init_func(void** hmac_context, const uint8_t* key, size_t key_len, void* user_data)
{
    gcry_error_t res;
    gcry_mac_hd_t hd;

    res = gcry_mac_open(&hd, GCRY_MAC_HMAC_SHA256, 0, NULL);
    if (res != GPG_ERR_NO_ERROR) {
        log_error("OMEMO: %s", gcry_strerror(res));
        return OMEMO_ERR_GCRYPT;
    }

    *hmac_context = hd;
    res = gcry_mac_setkey(hd, key, key_len);
    if (res != GPG_ERR_NO_ERROR) {
        log_error("OMEMO: %s", gcry_strerror(res));
        return OMEMO_ERR_GCRYPT;
    }

    return 0;
}

int
omemo_hmac_sha256_update_func(void* hmac_context, const uint8_t* data, size_t data_len, void* user_data)
{
    gcry_error_t res;

    res = gcry_mac_write(hmac_context, data, data_len);
    if (res != GPG_ERR_NO_ERROR) {
        log_error("OMEMO: %s", gcry_strerror(res));
        return OMEMO_ERR_GCRYPT;
    }

    return 0;
}

int
omemo_hmac_sha256_final_func(void* hmac_context, signal_buffer** output, void* user_data)
{
    gcry_error_t res;
    size_t mac_len = 32;
    unsigned char out[mac_len];

    res = gcry_mac_read(hmac_context, out, &mac_len);
    if (res != GPG_ERR_NO_ERROR) {
        log_error("OMEMO: %s", gcry_strerror(res));
        return OMEMO_ERR_GCRYPT;
    }

    *output = signal_buffer_create(out, mac_len);
    return 0;
}

void
omemo_hmac_sha256_cleanup_func(void* hmac_context, void* user_data)
{
    gcry_mac_close(hmac_context);
}

int
omemo_sha512_digest_init_func(void** digest_context, void* user_data)
{
    gcry_error_t res;
    gcry_md_hd_t hd;

    res = gcry_md_open(&hd, GCRY_MD_SHA512, 0);
    if (res != GPG_ERR_NO_ERROR) {
        log_error("OMEMO: %s", gcry_strerror(res));
        return OMEMO_ERR_GCRYPT;
    }

    *digest_context = hd;

    return 0;
}

int
omemo_sha512_digest_update_func(void* digest_context, const uint8_t* data, size_t data_len, void* user_data)
{
    gcry_md_write(digest_context, data, data_len);

    return 0;
}

int
omemo_sha512_digest_final_func(void* digest_context, signal_buffer** output, void* user_data)
{
    gcry_error_t res;
    unsigned char out[64];

    res = gcry_md_extract(digest_context, GCRY_MD_SHA512, out, 64);
    if (res != GPG_ERR_NO_ERROR) {
        log_error("OMEMO: %s", gcry_strerror(res));
        return OMEMO_ERR_GCRYPT;
    }

    *output = signal_buffer_create(out, 64);
    return 0;
}

void
omemo_sha512_digest_cleanup_func(void* digest_context, void* user_data)
{
    gcry_md_close(digest_context);
}

int
omemo_encrypt_func(signal_buffer** output, int cipher, const uint8_t* key, size_t key_len, const uint8_t* iv, size_t iv_len,
                   const uint8_t* plaintext, size_t plaintext_len, void* user_data)
{
    gcry_cipher_hd_t hd;
    unsigned char* padded_plaintext;
    unsigned char* ciphertext;
    size_t ciphertext_len;
    int mode;
    int algo;
    uint8_t padding = 0;

    switch (key_len) {
    case 32:
        algo = GCRY_CIPHER_AES256;
        break;
    default:
        return OMEMO_ERR_UNSUPPORTED_CRYPTO;
    }

    switch (cipher) {
    case SG_CIPHER_AES_CBC_PKCS5:
        mode = GCRY_CIPHER_MODE_CBC;
        break;
    default:
        return OMEMO_ERR_UNSUPPORTED_CRYPTO;
    }

    gcry_cipher_open(&hd, algo, mode, GCRY_CIPHER_SECURE);

    gcry_cipher_setkey(hd, key, key_len);

    switch (cipher) {
    case SG_CIPHER_AES_CBC_PKCS5:
        gcry_cipher_setiv(hd, iv, iv_len);
        padding = 16 - (plaintext_len % 16);
        break;
    default:
        assert(FALSE);
    }

    padded_plaintext = malloc(plaintext_len + padding);
    memcpy(padded_plaintext, plaintext, plaintext_len);
    memset(padded_plaintext + plaintext_len, padding, padding);

    ciphertext_len = plaintext_len + padding;
    ciphertext = malloc(ciphertext_len);
    gcry_cipher_encrypt(hd, ciphertext, ciphertext_len, padded_plaintext, plaintext_len + padding);

    *output = signal_buffer_create(ciphertext, ciphertext_len);
    free(padded_plaintext);
    free(ciphertext);

    gcry_cipher_close(hd);

    return SG_SUCCESS;
}

int
omemo_decrypt_func(signal_buffer** output, int cipher, const uint8_t* key, size_t key_len, const uint8_t* iv, size_t iv_len,
                   const uint8_t* ciphertext, size_t ciphertext_len, void* user_data)
{
    int ret = SG_SUCCESS;
    gcry_cipher_hd_t hd;
    unsigned char* plaintext;
    size_t plaintext_len;
    int mode;
    int algo;
    uint8_t padding = 0;

    switch (key_len) {
    case 32:
        algo = GCRY_CIPHER_AES256;
        break;
    default:
        return OMEMO_ERR_UNSUPPORTED_CRYPTO;
    }

    switch (cipher) {
    case SG_CIPHER_AES_CBC_PKCS5:
        mode = GCRY_CIPHER_MODE_CBC;
        break;
    default:
        return OMEMO_ERR_UNSUPPORTED_CRYPTO;
    }

    gcry_cipher_open(&hd, algo, mode, GCRY_CIPHER_SECURE);

    gcry_cipher_setkey(hd, key, key_len);

    switch (cipher) {
    case SG_CIPHER_AES_CBC_PKCS5:
        gcry_cipher_setiv(hd, iv, iv_len);
        break;
    default:
        assert(FALSE);
    }

    plaintext_len = ciphertext_len;
    plaintext = malloc(plaintext_len);
    gcry_cipher_decrypt(hd, plaintext, plaintext_len, ciphertext, ciphertext_len);

    switch (cipher) {
    case SG_CIPHER_AES_CBC_PKCS5:
        padding = plaintext[plaintext_len - 1];
        break;
    default:
        assert(FALSE);
    }

    int i;
    for (i = 0; i < padding; i++) {
        if (plaintext[plaintext_len - 1 - i] != padding) {
            ret = SG_ERR_UNKNOWN;
            goto out;
        }
    }

    *output = signal_buffer_create(plaintext, plaintext_len - padding);

out:
    free(plaintext);

    gcry_cipher_close(hd);

    return ret;
}

int
aes128gcm_encrypt(unsigned char* ciphertext, size_t* ciphertext_len, unsigned char* tag, size_t* tag_len, const unsigned char* const plaintext, size_t plaintext_len, const unsigned char* const iv, const unsigned char* const key)
{
    gcry_error_t res;
    gcry_cipher_hd_t hd;

    res = gcry_cipher_open(&hd, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_GCM, GCRY_CIPHER_SECURE);
    if (res != GPG_ERR_NO_ERROR) {
        goto out;
    }
    res = gcry_cipher_setkey(hd, key, AES128_GCM_KEY_LENGTH);
    if (res != GPG_ERR_NO_ERROR) {
        goto out;
    }
    res = gcry_cipher_setiv(hd, iv, AES128_GCM_IV_LENGTH);
    if (res != GPG_ERR_NO_ERROR) {
        goto out;
    }

    res = gcry_cipher_encrypt(hd, ciphertext, *ciphertext_len, plaintext, plaintext_len);
    if (res != GPG_ERR_NO_ERROR) {
        goto out;
    }

    res = gcry_cipher_gettag(hd, tag, *tag_len);
    if (res != GPG_ERR_NO_ERROR) {
        goto out;
    }

out:
    gcry_cipher_close(hd);
    return res;
}

int
aes128gcm_decrypt(unsigned char* plaintext, size_t* plaintext_len, const unsigned char* const ciphertext, size_t ciphertext_len, const unsigned char* const iv, size_t iv_len, const unsigned char* const key, const unsigned char* const tag)
{
    gcry_error_t res;
    gcry_cipher_hd_t hd;

    res = gcry_cipher_open(&hd, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_GCM, GCRY_CIPHER_SECURE);
    if (res != GPG_ERR_NO_ERROR) {
        goto out;
    }

    res = gcry_cipher_setkey(hd, key, AES128_GCM_KEY_LENGTH);
    if (res != GPG_ERR_NO_ERROR) {
        goto out;
    }

    res = gcry_cipher_setiv(hd, iv, iv_len);
    if (res != GPG_ERR_NO_ERROR) {
        goto out;
    }

    res = gcry_cipher_decrypt(hd, plaintext, *plaintext_len, ciphertext, ciphertext_len);
    if (res != GPG_ERR_NO_ERROR) {
        goto out;
    }

    res = gcry_cipher_checktag(hd, tag, AES128_GCM_TAG_LENGTH);
    if (res != GPG_ERR_NO_ERROR) {
        goto out;
    }

out:
    gcry_cipher_close(hd);
    return res;
}
