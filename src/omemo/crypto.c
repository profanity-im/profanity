#include <assert.h>
#include <signal/signal_protocol.h>
#include <signal/signal_protocol_types.h>
#include <sodium.h>
#include <gcrypt.h>

#include "omemo/omemo.h"
#include "omemo/crypto.h"

int
omemo_crypto_init(void)
{
    if (sodium_init() < 0) {
        return -1;
    }

    if (!gcry_check_version(GCRYPT_VERSION)) {
        return -1;
    }

    gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

    return 0;
}

int
omemo_random_func(uint8_t *data, size_t len, void *user_data)
{
    randombytes_buf(data, len);
    return 0;
}

int
omemo_hmac_sha256_init_func(void **hmac_context, const uint8_t *key, size_t key_len, void *user_data)
{
    *hmac_context = sodium_malloc(sizeof(crypto_auth_hmacsha256_state));
    return crypto_auth_hmacsha256_init(*hmac_context, key, key_len);
}

int
omemo_hmac_sha256_update_func(void *hmac_context, const uint8_t *data, size_t data_len, void *user_data)
{
    return crypto_auth_hmacsha256_update(hmac_context, data, data_len);
}

int
omemo_hmac_sha256_final_func(void *hmac_context, signal_buffer **output, void *user_data)
{
    int ret;
    unsigned char out[crypto_auth_hmacsha256_BYTES];

    if ((ret = crypto_auth_hmacsha256_final(hmac_context, out)) != 0) {
        return ret;
    }

    *output = signal_buffer_create(out, crypto_auth_hmacsha256_BYTES);
    return 0;
}

void
omemo_hmac_sha256_cleanup_func(void *hmac_context, void *user_data)
{
    sodium_free(hmac_context);
}

int
omemo_sha512_digest_init_func(void **digest_context, void *user_data)
{
    *digest_context = sodium_malloc(sizeof(crypto_hash_sha512_state));
    return crypto_hash_sha512_init(*digest_context);
}

int
omemo_sha512_digest_update_func(void *digest_context, const uint8_t *data, size_t data_len, void *user_data)
{
    return crypto_hash_sha512_update(digest_context, data, data_len);
}

int
omemo_sha512_digest_final_func(void *digest_context, signal_buffer **output, void *user_data)
{
    int ret;
    unsigned char out[crypto_hash_sha512_BYTES];

    if ((ret = crypto_hash_sha512_final(digest_context, out)) != 0) {
        return ret;
    }

    *output = signal_buffer_create(out, crypto_hash_sha512_BYTES);
    return 0;
}

void
omemo_sha512_digest_cleanup_func(void *digest_context, void *user_data)
{
    sodium_free(digest_context);
}

int
omemo_encrypt_func(signal_buffer **output, int cipher, const uint8_t *key, size_t key_len, const uint8_t *iv, size_t iv_len,
    const uint8_t *plaintext, size_t plaintext_len, void *user_data)
{
    gcry_cipher_hd_t hd;
    unsigned char *padded_plaintext;
    unsigned char *ciphertext;
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
omemo_decrypt_func(signal_buffer **output, int cipher, const uint8_t *key, size_t key_len, const uint8_t *iv, size_t iv_len,
    const uint8_t *ciphertext, size_t ciphertext_len, void *user_data)
{
    int ret = SG_SUCCESS;
    gcry_cipher_hd_t hd;
    unsigned char *plaintext;
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
aes128gcm_encrypt(unsigned char *ciphertext, size_t *ciphertext_len, const unsigned char *const plaintext, size_t plaintext_len, const unsigned char *const iv, const unsigned char *const key)
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

    res = gcry_cipher_gettag(hd, ciphertext + plaintext_len, AES128_GCM_TAG_LENGTH);
    if (res != GPG_ERR_NO_ERROR) {
        goto out;
    }

out:
    gcry_cipher_close(hd);
    return res;
}

int
aes128gcm_decrypt(unsigned char *plaintext, size_t *plaintext_len, const unsigned char *const ciphertext, size_t ciphertext_len, const unsigned char *const iv, const unsigned char *const key)
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

    res = gcry_cipher_decrypt(hd, plaintext, *plaintext_len, ciphertext, ciphertext_len);
    if (res != GPG_ERR_NO_ERROR) {
        goto out;
    }
    //res = gcry_cipher_checktag(hd, ciphertext + ciphertext_len - AES128_GCM_TAG_LENGTH, AES128_GCM_TAG_LENGTH);
    //if (res != GPG_ERR_NO_ERROR) {
    //    goto out;
    //}

out:
    gcry_cipher_close(hd);
    return res;
}
