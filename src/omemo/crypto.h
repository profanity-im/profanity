/*
 * crypto.h
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
#include <stdio.h>
#include <stdbool.h>
#include <omemo/signal_protocol_types.h>
#include <gcrypt.h>

#define AES128_GCM_KEY_LENGTH 16
#define AES128_GCM_IV_LENGTH  12
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
int omemo_random_func(uint8_t* data, size_t len, void* user_data);

/**
 * Callback for an HMAC-SHA256 implementation.
 * This function shall initialize an HMAC context with the provided key.
 *
 * @param hmac_context private HMAC context pointer
 * @param key pointer to the key
 * @param key_len length of the key
 * @return 0 on success, negative on failure
 */
int omemo_hmac_sha256_init_func(void** hmac_context, const uint8_t* key, size_t key_len, void* user_data);

/**
 * Callback for an HMAC-SHA256 implementation.
 * This function shall update the HMAC context with the provided data
 *
 * @param hmac_context private HMAC context pointer
 * @param data pointer to the data
 * @param data_len length of the data
 * @return 0 on success, negative on failure
 */
int omemo_hmac_sha256_update_func(void* hmac_context, const uint8_t* data, size_t data_len, void* user_data);

/**
 * Callback for an HMAC-SHA256 implementation.
 * This function shall finalize an HMAC calculation and populate the output
 * buffer with the result.
 *
 * @param hmac_context private HMAC context pointer
 * @param output buffer to be allocated and populated with the result
 * @return 0 on success, negative on failure
 */
int omemo_hmac_sha256_final_func(void* hmac_context, signal_buffer** output, void* user_data);

/**
 * Callback for an HMAC-SHA256 implementation.
 * This function shall free the private context allocated in
 * hmac_sha256_init_func.
 *
 * @param hmac_context private HMAC context pointer
 */
void omemo_hmac_sha256_cleanup_func(void* hmac_context, void* user_data);

/**
 * Callback for a SHA512 message digest implementation.
 * This function shall initialize a digest context.
 *
 * @param digest_context private digest context pointer
 * @return 0 on success, negative on failure
 */
int omemo_sha512_digest_init_func(void** digest_context, void* user_data);

/**
 * Callback for a SHA512 message digest implementation.
 * This function shall update the digest context with the provided data.
 *
 * @param digest_context private digest context pointer
 * @param data pointer to the data
 * @param data_len length of the data
 * @return 0 on success, negative on failure
 */
int omemo_sha512_digest_update_func(void* digest_context, const uint8_t* data, size_t data_len, void* user_data);

/**
 * Callback for a SHA512 message digest implementation.
 * This function shall finalize the digest calculation, populate the
 * output buffer with the result, and prepare the context for reuse.
 *
 * @param digest_context private digest context pointer
 * @param output buffer to be allocated and populated with the result
 * @return 0 on success, negative on failure
 */
int omemo_sha512_digest_final_func(void* digest_context, signal_buffer** output, void* user_data);

/**
 * Callback for a SHA512 message digest implementation.
 * This function shall free the private context allocated in
 * sha512_digest_init_func.
 *
 * @param digest_context private digest context pointer
 */
void omemo_sha512_digest_cleanup_func(void* digest_context, void* user_data);

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
int omemo_encrypt_func(signal_buffer** output,
                       int cipher,
                       const uint8_t* key, size_t key_len,
                       const uint8_t* iv, size_t iv_len,
                       const uint8_t* plaintext, size_t plaintext_len,
                       void* user_data);

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
int omemo_decrypt_func(signal_buffer** output,
                       int cipher,
                       const uint8_t* key, size_t key_len,
                       const uint8_t* iv, size_t iv_len,
                       const uint8_t* ciphertext, size_t ciphertext_len,
                       void* user_data);

int aes128gcm_encrypt(unsigned char* ciphertext, size_t* ciphertext_len,
                      unsigned char* tag, size_t* tag_len,
                      const unsigned char* const plaintext, size_t plaintext_len,
                      const unsigned char* const iv, const unsigned char* const key);

int aes128gcm_decrypt(unsigned char* plaintext,
                      size_t* plaintext_len, const unsigned char* const ciphertext,
                      size_t ciphertext_len, const unsigned char* const iv, size_t iv_len,
                      const unsigned char* const key, const unsigned char* const tag);

gcry_error_t aes256gcm_crypt_file(FILE* in, FILE* out, off_t file_size,
                                  unsigned char key[], unsigned char nonce[], bool encrypt);

char* aes256gcm_create_secure_fragment(unsigned char* key,
                                       unsigned char* nonce);
