#include <assert.h>
#include <signal/signal_protocol.h>
#include <signal/signal_protocol_types.h>
#include <sodium.h>

#include "omemo/crypto.h"

int
omemo_crypto_init(void)
{
    if (sodium_init() < 0) {
        return -1;
    }

    if (crypto_aead_aes256gcm_is_available() == 0) {
        return -1;
    }

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
    unsigned char *ciphertext;
    unsigned long long ciphertext_len;

    assert(cipher != SG_CIPHER_AES_GCM_NOPADDING);
    assert(key_len == crypto_aead_aes256gcm_KEYBYTES);
    assert(iv_len == crypto_aead_aes256gcm_NPUBBYTES);

    ciphertext = malloc(plaintext_len + crypto_aead_aes256gcm_ABYTES);
    crypto_aead_aes256gcm_encrypt(ciphertext, &ciphertext_len, plaintext, plaintext_len, NULL, 0, NULL, iv, key);

    *output = signal_buffer_create(ciphertext, ciphertext_len);
    free(ciphertext);

    return 0;
}

int
omemo_decrypt_func(signal_buffer **output, int cipher, const uint8_t *key, size_t key_len, const uint8_t *iv, size_t iv_len,
    const uint8_t *ciphertext, size_t ciphertext_len, void *user_data)
{
    unsigned char *plaintext;
    unsigned long long plaintext_len;

    assert(cipher != SG_CIPHER_AES_GCM_NOPADDING);
    assert(key_len == crypto_aead_aes256gcm_KEYBYTES);
    assert(iv_len == crypto_aead_aes256gcm_NPUBBYTES);

    plaintext = malloc(ciphertext_len - crypto_aead_aes256gcm_ABYTES);
    if (crypto_aead_aes256gcm_decrypt(plaintext, &plaintext_len, NULL, ciphertext, ciphertext_len, NULL, 0, iv, key) < 0) {
        free(plaintext);
        return -1;
    }

    *output = signal_buffer_create(plaintext, plaintext_len);
    free(plaintext);

    return 0;
}
