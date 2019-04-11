#include <signal/signal_protocol_types.h>

#define AES128_GCM_KEY_LENGTH 16
#define AES128_GCM_IV_LENGTH 16
#define AES128_GCM_TAG_LENGTH 16

int omemo_crypto_init(void);
/**
* Callback for a secure random number generator.
* This function shall fill the provided buffer with random bytes.
*
* @param data pointer to the output buffer
* @param len size of the output buffer
* @return 0 on success, negative on failure
*/
int omemo_random_func(uint8_t *data, size_t len, void *user_data);

/**
* Callback for an HMAC-SHA256 implementation.
* This function shall initialize an HMAC context with the provided key.
*
* @param hmac_context private HMAC context pointer
* @param key pointer to the key
* @param key_len length of the key
* @return 0 on success, negative on failure
*/
int omemo_hmac_sha256_init_func(void **hmac_context, const uint8_t *key, size_t key_len, void *user_data);

/**
* Callback for an HMAC-SHA256 implementation.
* This function shall update the HMAC context with the provided data
*
* @param hmac_context private HMAC context pointer
* @param data pointer to the data
* @param data_len length of the data
* @return 0 on success, negative on failure
*/
int omemo_hmac_sha256_update_func(void *hmac_context, const uint8_t *data, size_t data_len, void *user_data);

/**
* Callback for an HMAC-SHA256 implementation.
* This function shall finalize an HMAC calculation and populate the output
* buffer with the result.
*
* @param hmac_context private HMAC context pointer
* @param output buffer to be allocated and populated with the result
* @return 0 on success, negative on failure
*/
int omemo_hmac_sha256_final_func(void *hmac_context, signal_buffer **output, void *user_data);

/**
* Callback for an HMAC-SHA256 implementation.
* This function shall free the private context allocated in
* hmac_sha256_init_func.
*
* @param hmac_context private HMAC context pointer
*/
void omemo_hmac_sha256_cleanup_func(void *hmac_context, void *user_data);

/**
* Callback for a SHA512 message digest implementation.
* This function shall initialize a digest context.
*
* @param digest_context private digest context pointer
* @return 0 on success, negative on failure
*/
int omemo_sha512_digest_init_func(void **digest_context, void *user_data);

/**
* Callback for a SHA512 message digest implementation.
* This function shall update the digest context with the provided data.
*
* @param digest_context private digest context pointer
* @param data pointer to the data
* @param data_len length of the data
* @return 0 on success, negative on failure
*/
int omemo_sha512_digest_update_func(void *digest_context, const uint8_t *data, size_t data_len, void *user_data);

/**
* Callback for a SHA512 message digest implementation.
* This function shall finalize the digest calculation, populate the
* output buffer with the result, and prepare the context for reuse.
*
* @param digest_context private digest context pointer
* @param output buffer to be allocated and populated with the result
* @return 0 on success, negative on failure
*/
int omemo_sha512_digest_final_func(void *digest_context, signal_buffer **output, void *user_data);

/**
* Callback for a SHA512 message digest implementation.
* This function shall free the private context allocated in
* sha512_digest_init_func.
*
* @param digest_context private digest context pointer
*/
void omemo_sha512_digest_cleanup_func(void *digest_context, void *user_data);

/**
* Callback for an AES encryption implementation.
*
* @param output buffer to be allocated and populated with the ciphertext
* @param cipher specific cipher variant to use, either SG_CIPHER_AES_CTR_NOPADDING or SG_CIPHER_AES_CBC_PKCS5
* @param key the encryption key
* @param key_len length of the encryption key
* @param iv the initialization vector
* @param iv_len length of the initialization vector
* @param plaintext the plaintext to encrypt
* @param plaintext_len length of the plaintext
* @return 0 on success, negative on failure
*/
int omemo_encrypt_func(signal_buffer **output,
    int cipher,
    const uint8_t *key, size_t key_len,
    const uint8_t *iv, size_t iv_len,
    const uint8_t *plaintext, size_t plaintext_len,
    void *user_data);

/**
* Callback for an AES decryption implementation.
*
* @param output buffer to be allocated and populated with the plaintext
* @param cipher specific cipher variant to use, either SG_CIPHER_AES_CTR_NOPADDING or SG_CIPHER_AES_CBC_PKCS5
* @param key the encryption key
* @param key_len length of the encryption key
* @param iv the initialization vector
* @param iv_len length of the initialization vector
* @param ciphertext the ciphertext to decrypt
* @param ciphertext_len length of the ciphertext
* @return 0 on success, negative on failure
*/
int omemo_decrypt_func(signal_buffer **output,
    int cipher,
    const uint8_t *key, size_t key_len,
    const uint8_t *iv, size_t iv_len,
    const uint8_t *ciphertext, size_t ciphertext_len,
    void *user_data);

int aes128gcm_encrypt(unsigned char *ciphertext, size_t *ciphertext_len,
    unsigned char *tag, size_t *tag_len,
    const unsigned char *const plaintext, size_t plaintext_len,
    const unsigned char *const iv, const unsigned char *const key);

int aes128gcm_decrypt(unsigned char *plaintext,
    size_t *plaintext_len, const unsigned char *const ciphertext,
    size_t ciphertext_len, const unsigned char *const iv,
    const unsigned char *const key, const unsigned char *const tag);
