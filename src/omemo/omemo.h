/*
 * omemo.h
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 Paul Fariello <paul@fariello.eu>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later WITH OpenSSL-exception
 */
#include <glib.h>
#include <gcrypt.h>

#include "ui/ui.h"
#include "config/account.h"

#define OMEMO_ERR_UNSUPPORTED_CRYPTO -10000
#define OMEMO_ERR_GCRYPT             -20000

#define OMEMO_AESGCM_NONCE_LENGTH AES128_GCM_IV_LENGTH
#define OMEMO_AESGCM_KEY_LENGTH   32
#define OMEMO_AESGCM_URL_SCHEME   "aesgcm"

typedef enum {
    PROF_OMEMOPOLICY_MANUAL,
    PROF_OMEMOPOLICY_AUTOMATIC,
    PROF_OMEMOPOLICY_ALWAYS
} prof_omemopolicy_t;

typedef struct omemo_key
{
    unsigned char* data;
    size_t length;
    gboolean prekey;
    uint32_t device_id;
    uint32_t id;
} omemo_key_t;

void omemo_init(void);
void omemo_on_connect(ProfAccount* account);
void omemo_on_disconnect(void);
void omemo_generate_crypto_materials(ProfAccount* account);
void omemo_key_free(omemo_key_t* key);
void omemo_publish_crypto_materials(void);

uint32_t omemo_device_id(void);
void omemo_identity_key(unsigned char** output, size_t* length);
void omemo_signed_prekey(unsigned char** output, size_t* length);
void omemo_signed_prekey_signature(unsigned char** output, size_t* length);
void omemo_prekeys(GList** prekeys, GList** ids, GList** lengths);
void omemo_set_device_list(const char* const jid, GList* device_list);
GKeyFile* omemo_identity_keyfile(void);
void omemo_identity_keyfile_save(void);
GKeyFile* omemo_trust_keyfile(void);
void omemo_trust_keyfile_save(void);
GKeyFile* omemo_sessions_keyfile(void);
void omemo_sessions_keyfile_save(void);
char* omemo_format_fingerprint(const char* const fingerprint);
char* omemo_own_fingerprint(gboolean formatted);
void omemo_trust(const char* const jid, const char* const fingerprint);
void omemo_untrust(const char* const jid, const char* const fingerprint);
GList* omemo_known_device_identities(const char* const jid);
gboolean omemo_is_trusted_identity(const char* const jid, const char* const fingerprint);
gboolean omemo_is_jid_trusted(const char* const jid);
GList* omemo_get_jid_untrusted_fingerprints(const char* const jid);
gboolean omemo_is_device_active(const char* const jid, const char* const fingerprint);
char* omemo_fingerprint_autocomplete(const char* const search_str, gboolean previous, void* context);
void omemo_fingerprint_autocomplete_reset(void);
gboolean omemo_automatic_start(const char* const recipient);

void omemo_start_sessions(void);
void omemo_start_session(const char* const barejid);
void omemo_start_muc_sessions(const char* const roomjid);
void omemo_start_device_session(const char* const jid, uint32_t device_id, GList* prekeys, uint32_t signed_prekey_id, const unsigned char* const signed_prekey, size_t signed_prekey_len, const unsigned char* const signature, size_t signature_len, const unsigned char* const identity_key, size_t identity_key_len);

gboolean omemo_loaded(void);

char* omemo_on_message_send(ProfWin* win, const char* const message, gboolean request_receipt, gboolean muc, const char* const replace_id);
char* omemo_on_message_recv(const char* const from, uint32_t sid, const unsigned char* const iv, size_t iv_len, GList* keys, const unsigned char* const payload, size_t payload_len, gboolean muc, gboolean* trusted, omemo_error_t* error) __attribute__((nonnull(9, 10)));

char* omemo_encrypt_file(FILE* in, FILE* out, off_t file_size, int* gcry_res);
gcry_error_t omemo_decrypt_file(FILE* in, FILE* out, off_t file_size, const char* fragment);
void omemo_free(void* a);
int omemo_parse_aesgcm_url(const char* aesgcm_url, char** https_url, char** fragment);

char* omemo_qrcode_str();
