/*
 * omemo.c
 * vim: expandtab:ts=4:sts=4:sw=4
 *
 * Copyright (C) 2019 Paul Fariello <paul@fariello.eu>
 * Copyright (C) 2019 - 2021 Michael Vetter <jubalh@iodoru.org>
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

#include <sys/time.h>
#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <glib.h>
#include <pthread.h>
#include <signal/key_helper.h>
#include <signal/protocol.h>
#include <signal/signal_protocol.h>
#include <signal/session_builder.h>
#include <signal/session_cipher.h>

#include "config/account.h"
#include "config/files.h"
#include "config/preferences.h"
#include "log.h"
#include "omemo/crypto.h"
#include "omemo/omemo.h"
#include "omemo/store.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/connection.h"
#include "xmpp/muc.h"
#include "xmpp/omemo.h"
#include "xmpp/roster_list.h"
#include "xmpp/xmpp.h"

#define AESGCM_URL_NONCE_LEN (2 * OMEMO_AESGCM_NONCE_LENGTH)
#define AESGCM_URL_KEY_LEN   (2 * OMEMO_AESGCM_KEY_LENGTH)

static gboolean loaded = FALSE;

static void _generate_pre_keys(int count);
static void _generate_signed_pre_key(void);
static gboolean _load_identity(void);
static void _load_trust(void);
static void _load_sessions(void);
static void _load_known_devices(void);
static void _lock(void* user_data);
static void _unlock(void* user_data);
static void _omemo_log(int level, const char* message, size_t len, void* user_data);
static gboolean _handle_own_device_list(const char* const jid, GList* device_list);
static gboolean _handle_device_list_start_session(const char* const jid, GList* device_list);
static char* _omemo_fingerprint(ec_public_key* identity, gboolean formatted);
static unsigned char* _omemo_fingerprint_decode(const char* const fingerprint, size_t* len);
static char* _omemo_unformat_fingerprint(const char* const fingerprint_formatted);
static void _cache_device_identity(const char* const jid, uint32_t device_id, ec_public_key* identity);
static void _g_hash_table_free(GHashTable* hash_table);

typedef gboolean (*OmemoDeviceListHandler)(const char* const jid, GList* device_list);

struct omemo_context_t
{
    pthread_mutexattr_t attr;
    pthread_mutex_t lock;
    signal_context* signal;
    uint32_t device_id;
    GHashTable* device_list;
    GHashTable* device_list_handler;
    ratchet_identity_key_pair* identity_key_pair;
    uint32_t registration_id;
    uint32_t signed_pre_key_id;
    signal_protocol_store_context* store;
    GHashTable* session_store;
    GHashTable* pre_key_store;
    GHashTable* signed_pre_key_store;
    identity_key_store_t identity_key_store;
    GString* identity_filename;
    GKeyFile* identity_keyfile;
    GString* trust_filename;
    GKeyFile* trust_keyfile;
    GString* sessions_filename;
    GKeyFile* sessions_keyfile;
    GHashTable* known_devices;
    GString* known_devices_filename;
    GKeyFile* known_devices_keyfile;
    GHashTable* fingerprint_ac;
};

static omemo_context omemo_ctx;

void
omemo_init(void)
{
    log_info("[OMEMO] initialising");
    if (omemo_crypto_init() != 0) {
        cons_show("Error initializing OMEMO crypto: gcry_check_version() failed");
    }

    pthread_mutexattr_init(&omemo_ctx.attr);
    pthread_mutexattr_settype(&omemo_ctx.attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&omemo_ctx.lock, &omemo_ctx.attr);

    omemo_ctx.fingerprint_ac = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)autocomplete_free);
}

void
omemo_close(void)
{
    if (omemo_ctx.fingerprint_ac) {
        g_hash_table_destroy(omemo_ctx.fingerprint_ac);
        omemo_ctx.fingerprint_ac = NULL;
    }
}

/*!
 * \brief Initialize a global context for signal after connect
 *
 * Before using the library, a libsignal-protocol-c client needs to initialize
 * a global context.
 *
 */

void
omemo_on_connect(ProfAccount* account)
{
    loaded = FALSE;
    GError* error = NULL;

    if (signal_context_create(&omemo_ctx.signal, &omemo_ctx) != 0) {
        cons_show("Error initializing OMEMO context");
        return;
    }

    if (signal_context_set_log_function(omemo_ctx.signal, _omemo_log) != 0) {
        cons_show("Error initializing OMEMO log");
    }

    signal_crypto_provider crypto_provider = {
        .random_func = omemo_random_func,
        .hmac_sha256_init_func = omemo_hmac_sha256_init_func,
        .hmac_sha256_update_func = omemo_hmac_sha256_update_func,
        .hmac_sha256_final_func = omemo_hmac_sha256_final_func,
        .hmac_sha256_cleanup_func = omemo_hmac_sha256_cleanup_func,
        .sha512_digest_init_func = omemo_sha512_digest_init_func,
        .sha512_digest_update_func = omemo_sha512_digest_update_func,
        .sha512_digest_final_func = omemo_sha512_digest_final_func,
        .sha512_digest_cleanup_func = omemo_sha512_digest_cleanup_func,
        .encrypt_func = omemo_encrypt_func,
        .decrypt_func = omemo_decrypt_func,
        .user_data = NULL
    };

    if (signal_context_set_crypto_provider(omemo_ctx.signal, &crypto_provider) != 0) {
        cons_show("Error initializing OMEMO crypto: unable to set crypto provider");
        return;
    }

    signal_context_set_locking_functions(omemo_ctx.signal, _lock, _unlock);

    signal_protocol_store_context_create(&omemo_ctx.store, omemo_ctx.signal);

    omemo_ctx.session_store = session_store_new();
    signal_protocol_session_store session_store = {
        .load_session_func = load_session,
        .get_sub_device_sessions_func = get_sub_device_sessions,
        .store_session_func = store_session,
        .contains_session_func = contains_session,
        .delete_session_func = delete_session,
        .delete_all_sessions_func = delete_all_sessions,
        .destroy_func = NULL,
        .user_data = omemo_ctx.session_store
    };
    signal_protocol_store_context_set_session_store(omemo_ctx.store, &session_store);

    omemo_ctx.pre_key_store = pre_key_store_new();
    signal_protocol_pre_key_store pre_key_store = {
        .load_pre_key = load_pre_key,
        .store_pre_key = store_pre_key,
        .contains_pre_key = contains_pre_key,
        .remove_pre_key = remove_pre_key,
        .destroy_func = NULL,
        .user_data = omemo_ctx.pre_key_store
    };
    signal_protocol_store_context_set_pre_key_store(omemo_ctx.store, &pre_key_store);

    omemo_ctx.signed_pre_key_store = signed_pre_key_store_new();
    signal_protocol_signed_pre_key_store signed_pre_key_store = {
        .load_signed_pre_key = load_signed_pre_key,
        .store_signed_pre_key = store_signed_pre_key,
        .contains_signed_pre_key = contains_signed_pre_key,
        .remove_signed_pre_key = remove_signed_pre_key,
        .destroy_func = NULL,
        .user_data = omemo_ctx.signed_pre_key_store
    };
    signal_protocol_store_context_set_signed_pre_key_store(omemo_ctx.store, &signed_pre_key_store);

    identity_key_store_new(&omemo_ctx.identity_key_store);
    signal_protocol_identity_key_store identity_key_store = {
        .get_identity_key_pair = get_identity_key_pair,
        .get_local_registration_id = get_local_registration_id,
        .save_identity = save_identity,
        .is_trusted_identity = is_trusted_identity,
        .destroy_func = NULL,
        .user_data = &omemo_ctx.identity_key_store
    };
    signal_protocol_store_context_set_identity_key_store(omemo_ctx.store, &identity_key_store);

    omemo_ctx.device_list = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)g_list_free);
    omemo_ctx.device_list_handler = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
    omemo_ctx.known_devices = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)_g_hash_table_free);

    gchar* omemo_dir = files_get_account_data_path(DIR_OMEMO, account->jid);

    omemo_ctx.identity_filename = g_string_new(omemo_dir);
    g_string_append(omemo_ctx.identity_filename, "/identity.txt");
    omemo_ctx.trust_filename = g_string_new(omemo_dir);
    g_string_append(omemo_ctx.trust_filename, "/trust.txt");
    omemo_ctx.sessions_filename = g_string_new(omemo_dir);
    g_string_append(omemo_ctx.sessions_filename, "/sessions.txt");
    omemo_ctx.known_devices_filename = g_string_new(omemo_dir);
    g_string_append(omemo_ctx.known_devices_filename, "/known_devices.txt");

    errno = 0;
    int res = g_mkdir_with_parents(omemo_dir, S_IRWXU);
    if (res == -1) {
        const char* errmsg = strerror(errno);
        if (errmsg) {
            log_error("[OMEMO] error creating directory: %s, %s", omemo_dir, errmsg);
        } else {
            log_error("[OMEMO] creating directory: %s", omemo_dir);
        }
    }

    g_free(omemo_dir);

    // Loading OMEMO files

    omemo_ctx.identity_keyfile = g_key_file_new();
    omemo_ctx.trust_keyfile = g_key_file_new();
    omemo_ctx.sessions_keyfile = g_key_file_new();
    omemo_ctx.known_devices_keyfile = g_key_file_new();

    if (g_key_file_load_from_file(omemo_ctx.identity_keyfile, omemo_ctx.identity_filename->str, G_KEY_FILE_KEEP_COMMENTS, &error)) {
        if (!_load_identity()) {
            return;
        }
    } else if (error->code != G_FILE_ERROR_NOENT) {
        log_warning("[OMEMO] error loading identity from: %s, %s", omemo_ctx.identity_filename->str, error->message);
        g_error_free(error);
        return;
    }

    error = NULL;
    if (g_key_file_load_from_file(omemo_ctx.trust_keyfile, omemo_ctx.trust_filename->str, G_KEY_FILE_KEEP_COMMENTS, &error)) {
        _load_trust();
    } else if (error->code != G_FILE_ERROR_NOENT) {
        log_warning("[OMEMO] error loading trust from: %s, %s", omemo_ctx.trust_filename->str, error->message);
        g_error_free(error);
    }

    error = NULL;
    if (g_key_file_load_from_file(omemo_ctx.sessions_keyfile, omemo_ctx.sessions_filename->str, G_KEY_FILE_KEEP_COMMENTS, &error)) {
        _load_sessions();
    } else if (error->code != G_FILE_ERROR_NOENT) {
        log_warning("[OMEMO] error loading sessions from: %s, %s", omemo_ctx.sessions_filename->str, error->message);
        g_error_free(error);
    }

    error = NULL;
    if (g_key_file_load_from_file(omemo_ctx.known_devices_keyfile, omemo_ctx.known_devices_filename->str, G_KEY_FILE_KEEP_COMMENTS, &error)) {
        _load_known_devices();
    } else if (error->code != G_FILE_ERROR_NOENT) {
        log_warning("[OMEMO] error loading known devices from: %s, %s", omemo_ctx.known_devices_filename->str, error->message);
        g_error_free(error);
    }

    omemo_devicelist_subscribe();
}

void
omemo_on_disconnect(void)
{
    if (!loaded) {
        return;
    }

    _g_hash_table_free(omemo_ctx.signed_pre_key_store);
    _g_hash_table_free(omemo_ctx.pre_key_store);
    _g_hash_table_free(omemo_ctx.device_list_handler);

    g_string_free(omemo_ctx.identity_filename, TRUE);
    g_key_file_free(omemo_ctx.identity_keyfile);
    g_string_free(omemo_ctx.trust_filename, TRUE);
    g_key_file_free(omemo_ctx.trust_keyfile);
    g_string_free(omemo_ctx.sessions_filename, TRUE);
    g_key_file_free(omemo_ctx.sessions_keyfile);
    _g_hash_table_free(omemo_ctx.session_store);
    g_string_free(omemo_ctx.known_devices_filename, TRUE);
    g_key_file_free(omemo_ctx.known_devices_keyfile);
}

void
omemo_generate_crypto_materials(ProfAccount* account)
{
    if (loaded) {
        return;
    }

    log_info("Generate long term OMEMO cryptography materials");

    /* Device ID */
    gcry_randomize(&omemo_ctx.device_id, 4, GCRY_VERY_STRONG_RANDOM);
    omemo_ctx.device_id &= 0x7fffffff;
    g_key_file_set_uint64(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_DEVICE_ID, omemo_ctx.device_id);
    log_info("[OMEMO] device id: %d", omemo_ctx.device_id);

    /* Identity key */
    signal_protocol_key_helper_generate_identity_key_pair(&omemo_ctx.identity_key_pair, omemo_ctx.signal);

    ec_public_key_serialize(&omemo_ctx.identity_key_store.public, ratchet_identity_key_pair_get_public(omemo_ctx.identity_key_pair));
    char* identity_key_public = g_base64_encode(signal_buffer_data(omemo_ctx.identity_key_store.public), signal_buffer_len(omemo_ctx.identity_key_store.public));
    g_key_file_set_string(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_IDENTITY_KEY_PUBLIC, identity_key_public);
    g_free(identity_key_public);

    ec_private_key_serialize(&omemo_ctx.identity_key_store.private, ratchet_identity_key_pair_get_private(omemo_ctx.identity_key_pair));
    char* identity_key_private = g_base64_encode(signal_buffer_data(omemo_ctx.identity_key_store.private), signal_buffer_len(omemo_ctx.identity_key_store.private));
    g_key_file_set_string(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_IDENTITY_KEY_PRIVATE, identity_key_private);
    g_free(identity_key_private);

    /* Registration ID */
    signal_protocol_key_helper_generate_registration_id(&omemo_ctx.registration_id, 0, omemo_ctx.signal);
    g_key_file_set_uint64(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_REGISTRATION_ID, omemo_ctx.registration_id);

    /* Pre keys */
    _generate_pre_keys(100);

    /* Signed pre key */
    _generate_signed_pre_key();

    omemo_identity_keyfile_save();

    loaded = TRUE;

    omemo_publish_crypto_materials();
    omemo_start_sessions();
}

void
omemo_publish_crypto_materials(void)
{
    if (loaded != TRUE) {
        cons_show("OMEMO: cannot publish crypto materials before they are generated");
        log_error("[OMEMO] cannot publish crypto materials before they are generated");
        return;
    }

    char* barejid = connection_get_barejid();

    /* Ensure we get our current device list, and it gets updated with our
     * device_id */
    g_hash_table_insert(omemo_ctx.device_list_handler, strdup(barejid), _handle_own_device_list);
    omemo_devicelist_request(barejid);

    omemo_bundle_publish(true);

    free(barejid);
}

void
omemo_start_sessions(void)
{
    GSList* contacts = roster_get_contacts(ROSTER_ORD_NAME);
    if (contacts) {
        GSList* curr;
        for (curr = contacts; curr != NULL; curr = g_slist_next(curr)) {
            PContact contact = curr->data;
            const char* jid = p_contact_barejid(contact);
            omemo_start_session(jid);
        }
        g_slist_free(contacts);
    }
}

void
omemo_start_session(const char* const barejid)
{
    if (omemo_loaded()) {
        log_info("[OMEMO] start session with %s", barejid);
        GList* device_list = g_hash_table_lookup(omemo_ctx.device_list, barejid);
        if (!device_list) {
            log_info("[OMEMO] missing device list for %s", barejid);
            omemo_devicelist_request(barejid);
            g_hash_table_insert(omemo_ctx.device_list_handler, strdup(barejid), _handle_device_list_start_session);
            return;
        }

        GList* device_id;
        for (device_id = device_list; device_id != NULL; device_id = device_id->next) {
            omemo_bundle_request(barejid, GPOINTER_TO_INT(device_id->data), omemo_start_device_session_handle_bundle, free, strdup(barejid));
        }
    }
}

void
omemo_start_muc_sessions(const char* const roomjid)
{
    GList* members = muc_members(roomjid);
    GList* iter;
    for (iter = members; iter != NULL; iter = iter->next) {
        Jid* jid = jid_create(iter->data);
        omemo_start_session(jid->barejid);
        jid_destroy(jid);
    }
    g_list_free(members);
}

gboolean
omemo_loaded(void)
{
    return loaded;
}

uint32_t
omemo_device_id(void)
{
    return omemo_ctx.device_id;
}

void
omemo_identity_key(unsigned char** output, size_t* length)
{
    signal_buffer* buffer = NULL;
    ec_public_key_serialize(&buffer, ratchet_identity_key_pair_get_public(omemo_ctx.identity_key_pair));
    *length = signal_buffer_len(buffer);
    *output = malloc(*length);
    memcpy(*output, signal_buffer_data(buffer), *length);
    signal_buffer_free(buffer);
}

void
omemo_signed_prekey(unsigned char** output, size_t* length)
{
    session_signed_pre_key* signed_pre_key;
    signal_buffer* buffer = NULL;

    if (signal_protocol_signed_pre_key_load_key(omemo_ctx.store, &signed_pre_key, omemo_ctx.signed_pre_key_id) != SG_SUCCESS) {
        *output = NULL;
        *length = 0;
        return;
    }

    ec_public_key_serialize(&buffer, ec_key_pair_get_public(session_signed_pre_key_get_key_pair(signed_pre_key)));
    SIGNAL_UNREF(signed_pre_key);
    *length = signal_buffer_len(buffer);
    *output = malloc(*length);
    memcpy(*output, signal_buffer_data(buffer), *length);
    signal_buffer_free(buffer);
}

void
omemo_signed_prekey_signature(unsigned char** output, size_t* length)
{
    session_signed_pre_key* signed_pre_key;

    if (signal_protocol_signed_pre_key_load_key(omemo_ctx.store, &signed_pre_key, omemo_ctx.signed_pre_key_id) != SG_SUCCESS) {
        *output = NULL;
        *length = 0;
        return;
    }

    *length = session_signed_pre_key_get_signature_len(signed_pre_key);
    *output = malloc(*length);
    memcpy(*output, session_signed_pre_key_get_signature(signed_pre_key), *length);
    SIGNAL_UNREF(signed_pre_key);
}

void
omemo_prekeys(GList** prekeys, GList** ids, GList** lengths)
{
    GHashTableIter iter;
    gpointer id;

    g_hash_table_iter_init(&iter, omemo_ctx.pre_key_store);
    while (g_hash_table_iter_next(&iter, &id, NULL)) {
        session_pre_key* pre_key;
        int ret;
        ret = signal_protocol_pre_key_load_key(omemo_ctx.store, &pre_key, GPOINTER_TO_INT(id));
        if (ret != SG_SUCCESS) {
            continue;
        }

        signal_buffer* public_key;
        ec_public_key_serialize(&public_key, ec_key_pair_get_public(session_pre_key_get_key_pair(pre_key)));
        SIGNAL_UNREF(pre_key);
        size_t length = signal_buffer_len(public_key);
        unsigned char* prekey_value = malloc(length);
        memcpy(prekey_value, signal_buffer_data(public_key), length);
        signal_buffer_free(public_key);

        *prekeys = g_list_append(*prekeys, prekey_value);
        *ids = g_list_append(*ids, GINT_TO_POINTER(id));
        *lengths = g_list_append(*lengths, GINT_TO_POINTER(length));
    }
}

void
omemo_set_device_list(const char* const from, GList* device_list)
{
    Jid* jid;
    if (from) {
        jid = jid_create(from);
    } else {
        jid = jid_create(connection_get_fulljid());
    }

    g_hash_table_insert(omemo_ctx.device_list, strdup(jid->barejid), device_list);

    OmemoDeviceListHandler handler = g_hash_table_lookup(omemo_ctx.device_list_handler, jid->barejid);
    if (handler) {
        gboolean keep = handler(jid->barejid, device_list);
        if (!keep) {
            g_hash_table_remove(omemo_ctx.device_list_handler, jid->barejid);
        }
    }

    // OMEMO trustmode ToFu
    if (g_strcmp0(prefs_get_string(PREF_OMEMO_TRUST_MODE), "firstusage") == 0) {
        log_info("[OMEMO] Checking firstusage state for %s", jid->barejid);
        GHashTable* trusted = g_hash_table_lookup(omemo_ctx.identity_key_store.trusted, jid->barejid);
        if (trusted) {
            if (g_hash_table_size(trusted) > 0) {
                log_info("[OMEMO] Found trusted device for %s - skip firstusage", jid->barejid);
                return;
            }
        } else {
            if (device_list) {
                cons_show("OMEMO: No trusted devices found for %s", jid->barejid);
                GList* device_id;
                for (device_id = device_list; device_id != NULL; device_id = device_id->next) {
                    GHashTable* known_identities = g_hash_table_lookup(omemo_ctx.known_devices, jid->barejid);
                    if (known_identities) {
                        GList* fp = NULL;
                        for (fp = g_hash_table_get_keys(known_identities); fp != NULL; fp = fp->next) {
                            if (device_id->data == g_hash_table_lookup(known_identities, fp->data)) {
                                cons_show("OMEMO: Adding firstusage trust for %s device %d - Fingerprint %s", jid->barejid, device_id->data, omemo_format_fingerprint(fp->data));
                                omemo_trust(jid->barejid, omemo_format_fingerprint(fp->data));
                            }
                        }
                    }
                }
            }
        }
    }
    jid_destroy(jid);
}

GKeyFile*
omemo_identity_keyfile(void)
{
    return omemo_ctx.identity_keyfile;
}

void
omemo_identity_keyfile_save(void)
{
    GError* error = NULL;

    if (!g_key_file_save_to_file(omemo_ctx.identity_keyfile, omemo_ctx.identity_filename->str, &error)) {
        log_error("[OMEMO] error saving identity to: %s, %s", omemo_ctx.identity_filename->str, error->message);
    }
}

GKeyFile*
omemo_trust_keyfile(void)
{
    return omemo_ctx.trust_keyfile;
}

void
omemo_trust_keyfile_save(void)
{
    GError* error = NULL;

    if (!g_key_file_save_to_file(omemo_ctx.trust_keyfile, omemo_ctx.trust_filename->str, &error)) {
        log_error("[OMEMO] error saving trust to: %s, %s", omemo_ctx.trust_filename->str, error->message);
    }
}

GKeyFile*
omemo_sessions_keyfile(void)
{
    return omemo_ctx.sessions_keyfile;
}

void
omemo_sessions_keyfile_save(void)
{
    GError* error = NULL;

    if (!g_key_file_save_to_file(omemo_ctx.sessions_keyfile, omemo_ctx.sessions_filename->str, &error)) {
        log_error("[OMEMO] error saving sessions to: %s, %s", omemo_ctx.sessions_filename->str, error->message);
    }
}

void
omemo_known_devices_keyfile_save(void)
{
    GError* error = NULL;

    if (!g_key_file_save_to_file(omemo_ctx.known_devices_keyfile, omemo_ctx.known_devices_filename->str, &error)) {
        log_error("[OMEMO] error saving known devices to: %s, %s", omemo_ctx.known_devices_filename->str, error->message);
    }
}

void
omemo_start_device_session(const char* const jid, uint32_t device_id,
                           GList* prekeys, uint32_t signed_prekey_id,
                           const unsigned char* const signed_prekey_raw, size_t signed_prekey_len,
                           const unsigned char* const signature, size_t signature_len,
                           const unsigned char* const identity_key_raw, size_t identity_key_len)
{
    log_info("[OMEMO] Starting device session for %s with device %d", jid, device_id);
    signal_protocol_address address = {
        .name = jid,
        .name_len = strlen(jid),
        .device_id = device_id,
    };

    ec_public_key* identity_key;
    curve_decode_point(&identity_key, identity_key_raw, identity_key_len, omemo_ctx.signal);
    _cache_device_identity(jid, device_id, identity_key);

    gboolean trusted = is_trusted_identity(&address, (uint8_t*)identity_key_raw, identity_key_len, &omemo_ctx.identity_key_store);
    log_info("[OMEMO] Trust %s (%d): %d", jid, device_id, trusted);

    if ((g_strcmp0(prefs_get_string(PREF_OMEMO_TRUST_MODE), "blind") == 0) && !trusted) {
        char* fp = _omemo_fingerprint(identity_key, TRUE);
        cons_show("Blind trust for %s device %d (%s)", jid, device_id, fp);
        omemo_trust(jid, fp);
        free(fp);
        trusted = TRUE;
    }

    if (!trusted) {
        log_info("[OMEMO] We don't trust device %d for %s\n", device_id,jid);
        goto out;
    }

    if (!contains_session(&address, omemo_ctx.session_store)) {
        log_info("[OMEMO] There is no Session for %s ( %d) ,... building session.", address.name, address.device_id );
        int res;
        session_pre_key_bundle* bundle;
        signal_protocol_address* address;

        address = malloc(sizeof(signal_protocol_address));
        address->name = strdup(jid);
        address->name_len = strlen(jid);
        address->device_id = device_id;

        session_builder* builder;
        res = session_builder_create(&builder, omemo_ctx.store, address, omemo_ctx.signal);
        if (res != 0) {
            log_error("[OMEMO] cannot create session builder for %s device %d", jid, device_id);
            goto out;
        }

        int prekey_index;
        gcry_randomize(&prekey_index, sizeof(int), GCRY_STRONG_RANDOM);
        prekey_index %= g_list_length(prekeys);
        omemo_key_t* prekey = g_list_nth_data(prekeys, prekey_index);

        ec_public_key* prekey_public;
        curve_decode_point(&prekey_public, prekey->data, prekey->length, omemo_ctx.signal);
        ec_public_key* signed_prekey;
        curve_decode_point(&signed_prekey, signed_prekey_raw, signed_prekey_len, omemo_ctx.signal);

        res = session_pre_key_bundle_create(&bundle, 0, device_id, prekey->id, prekey_public, signed_prekey_id, signed_prekey, signature, signature_len, identity_key);
        if (res != 0) {
            log_error("[OMEMO] cannot create pre key bundle for %s device %d", jid, device_id);
            goto out;
        }

        res = session_builder_process_pre_key_bundle(builder, bundle);
        if (res != 0) {
            log_error("[OMEMO] cannot process pre key bundle for %s device %d", jid, device_id);
            goto out;
        }

        log_info("[OMEMO] create session with %s device %d", jid, device_id);
    } else {
        log_info("[OMEMO] session with %s device %d exists", jid, device_id);
    }

out:
    SIGNAL_UNREF(identity_key);
}

char*
omemo_on_message_send(ProfWin* win, const char* const message, gboolean request_receipt, gboolean muc, const char* const replace_id)
{
    char* id = NULL;
    int res;
    Jid* jid = jid_create(connection_get_fulljid());
    GList* keys = NULL;

    unsigned char* key;
    unsigned char* iv;
    unsigned char* ciphertext;
    unsigned char* tag;
    unsigned char* key_tag;
    size_t ciphertext_len, tag_len;

    ciphertext_len = strlen(message);
    ciphertext = malloc(ciphertext_len);
    tag_len = AES128_GCM_TAG_LENGTH;
    tag = gcry_malloc_secure(tag_len);
    key_tag = gcry_malloc_secure(AES128_GCM_KEY_LENGTH + AES128_GCM_TAG_LENGTH);

    key = gcry_random_bytes_secure(AES128_GCM_KEY_LENGTH, GCRY_VERY_STRONG_RANDOM);
    iv = gcry_random_bytes_secure(AES128_GCM_IV_LENGTH, GCRY_VERY_STRONG_RANDOM);

    res = aes128gcm_encrypt(ciphertext, &ciphertext_len, tag, &tag_len, (const unsigned char* const)message, strlen(message), iv, key);
    if (res != 0) {
        log_error("[OMEMO][SEND] cannot encrypt message");
        goto out;
    }

    memcpy(key_tag, key, AES128_GCM_KEY_LENGTH);
    memcpy(key_tag + AES128_GCM_KEY_LENGTH, tag, AES128_GCM_TAG_LENGTH);

    // List of barejids of the recipients of this message
    GList* recipients = NULL;
    if (muc) {
        ProfMucWin* mucwin = (ProfMucWin*)win;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        GList* members = muc_members(mucwin->roomjid);
        GList* iter;
        for (iter = members; iter != NULL; iter = iter->next) {
            Jid* jid = jid_create(iter->data);
            recipients = g_list_append(recipients, strdup(jid->barejid));
            jid_destroy(jid);
        }
        g_list_free(members);
    } else {
        ProfChatWin* chatwin = (ProfChatWin*)win;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        recipients = g_list_append(recipients, strdup(chatwin->barejid));
    }

    GList* device_ids_iter;

    omemo_ctx.identity_key_store.recv = false;

    // Encrypt keys for the recipients
    GList* recipients_iter;
    for (recipients_iter = recipients; recipients_iter != NULL; recipients_iter = recipients_iter->next) {
        GList* recipient_device_id = NULL;
        recipient_device_id = g_hash_table_lookup(omemo_ctx.device_list, recipients_iter->data);
        if (!recipient_device_id) {
            log_warning("[OMEMO][SEND] cannot find device ids for %s", recipients_iter->data);
            win_println(win, THEME_ERROR, "!", "Can't find a OMEMO device id for %s.\n", recipients_iter->data);
            continue;
        }

        for (device_ids_iter = recipient_device_id; device_ids_iter != NULL; device_ids_iter = device_ids_iter->next) {

            // Don't encrypt for this device (according to
            // <https://xmpp.org/extensions/xep-0384.html#encrypt>).
            Jid* me = jid_create(connection_get_fulljid());
            if ( !g_strcmp0(me->barejid, recipients_iter->data) ) {
                jid_destroy(me);
                if (GPOINTER_TO_INT(device_ids_iter->data) == omemo_ctx.device_id) {
                    log_info("[OMEMO][SEND] Skipping %d (my device) ", GPOINTER_TO_INT(device_ids_iter->data));
                    continue;
                }
            }
            int res;
            ciphertext_message* ciphertext;
            session_cipher* cipher;
            signal_protocol_address address = {
                .name = recipients_iter->data,
                .name_len = strlen(recipients_iter->data),
                .device_id = GPOINTER_TO_INT(device_ids_iter->data)
            };

            log_info("[OMEMO][SEND] recipients with device id %d for %s", GPOINTER_TO_INT(device_ids_iter->data), recipients_iter->data);
            res = session_cipher_create(&cipher, omemo_ctx.store, &address, omemo_ctx.signal);
            if (res != SG_SUCCESS ) {
                log_error("[OMEMO][SEND] cannot create cipher for %s device id %d - code: %d", address.name, address.device_id, res);
                continue;
            }

            res = session_cipher_encrypt(cipher, key_tag, AES128_GCM_KEY_LENGTH + AES128_GCM_TAG_LENGTH, &ciphertext);
            session_cipher_free(cipher);
            if (res != SG_SUCCESS ) {
                log_error("[OMEMO][SEND] cannot encrypt key for %s device id %d - code: %d", address.name, address.device_id,res);
                continue;
            }
            signal_buffer* buffer = ciphertext_message_get_serialized(ciphertext);
            omemo_key_t* key = malloc(sizeof(omemo_key_t));
            key->length = signal_buffer_len(buffer);
            key->data = malloc(key->length);
            memcpy(key->data, signal_buffer_data(buffer), key->length);
            key->device_id = GPOINTER_TO_INT(device_ids_iter->data);
            key->prekey = ciphertext_message_get_type(ciphertext) == CIPHERTEXT_PREKEY_TYPE;
            keys = g_list_append(keys, key);
            SIGNAL_UNREF(ciphertext);
        }
    }

    g_list_free_full(recipients, free);

    // Don't send the message if no key could be encrypted.
    // (Since none of the recipients would be able to read the message.)
    if (keys == NULL) {
        win_println(win, THEME_ERROR, "!", "This message cannot be decrypted for any recipient.\n"
                "You should trust your recipients' device fingerprint(s) using \"/omemo fingerprint trust FINGERPRINT\".\n"
                "It could also be that the key bundle of the recipient(s) have not been received. "
                "In this case, you could try \"omemo end\", \"omemo start\", and send the message again.");
        goto out;
    }

    // Encrypt keys for the sender
    if (!muc) {
        GList* sender_device_id = g_hash_table_lookup(omemo_ctx.device_list, jid->barejid);
        for (device_ids_iter = sender_device_id; device_ids_iter != NULL; device_ids_iter = device_ids_iter->next) {
            if (GPOINTER_TO_INT(device_ids_iter->data) == omemo_ctx.device_id) {
                log_info("[OMEMO][SEND] Skipping %d (my device) ", GPOINTER_TO_INT(device_ids_iter->data));
                continue;
            }
            int res;
            ciphertext_message* ciphertext;
            session_cipher* cipher;
            signal_protocol_address address = {
                .name = jid->barejid,
                .name_len = strlen(jid->barejid),
                .device_id = GPOINTER_TO_INT(device_ids_iter->data)
            };
            log_info("[OMEMO][SEND] Sending to device %d for %s ", address.device_id, address.name);

            res = session_cipher_create(&cipher, omemo_ctx.store, &address, omemo_ctx.signal);
            if (res != 0) {
                log_error("[OMEMO][SEND] cannot create cipher for %s device id %d", address.name, address.device_id);
                continue;
            }

            res = session_cipher_encrypt(cipher, key_tag, AES128_GCM_KEY_LENGTH + AES128_GCM_TAG_LENGTH, &ciphertext);
            session_cipher_free(cipher);
            if (res != 0) {
                log_error("[OMEMO][SEND] cannot encrypt key for %s device id %d", address.name, address.device_id);
                continue;
            }
            signal_buffer* buffer = ciphertext_message_get_serialized(ciphertext);
            omemo_key_t* key = malloc(sizeof(omemo_key_t));
            key->length = signal_buffer_len(buffer);
            key->data = malloc(key->length);
            memcpy(key->data, signal_buffer_data(buffer), key->length);
            key->device_id = GPOINTER_TO_INT(device_ids_iter->data);
            key->prekey = ciphertext_message_get_type(ciphertext) == CIPHERTEXT_PREKEY_TYPE;
            keys = g_list_append(keys, key);
            SIGNAL_UNREF(ciphertext);
        }
    }

    // Send the message
    if (muc) {
        ProfMucWin* mucwin = (ProfMucWin*)win;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        id = message_send_chat_omemo(mucwin->roomjid, omemo_ctx.device_id, keys, iv, AES128_GCM_IV_LENGTH, ciphertext, ciphertext_len, request_receipt, TRUE, replace_id);
    } else {
        ProfChatWin* chatwin = (ProfChatWin*)win;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        id = message_send_chat_omemo(chatwin->barejid, omemo_ctx.device_id, keys, iv, AES128_GCM_IV_LENGTH, ciphertext, ciphertext_len, request_receipt, FALSE, replace_id);
    }

out:
    jid_destroy(jid);
    g_list_free_full(keys, (GDestroyNotify)omemo_key_free);
    free(ciphertext);
    gcry_free(key);
    gcry_free(iv);
    gcry_free(tag);
    gcry_free(key_tag);

    return id;
}

char*
omemo_on_message_recv(const char* const from_jid, uint32_t sid,
                      const unsigned char* const iv, size_t iv_len, GList* keys,
                      const unsigned char* const payload, size_t payload_len, gboolean muc, gboolean* trusted)
{
    unsigned char* plaintext = NULL;
    Jid* sender = NULL;
    Jid* from = jid_create(from_jid);
    if (!from) {
        log_error("[OMEMO][RECV] Invalid jid %s", from_jid);
        goto out;
    }

    int res;
    GList* key_iter;
    omemo_key_t* key = NULL;
    for (key_iter = keys; key_iter != NULL; key_iter = key_iter->next) {
        if (((omemo_key_t*)key_iter->data)->device_id == omemo_ctx.device_id) {
            key = key_iter->data;
            break;
        }
    }

    if (!key) {
        log_warning("[OMEMO][RECV] received a message with no corresponding key");
        goto out;
    }

    if (muc) {
        GList* roster = muc_roster(from->barejid);
        GList* iter;
        for (iter = roster; iter != NULL; iter = iter->next) {
            Occupant* occupant = (Occupant*)iter->data;
            if (g_strcmp0(occupant->nick, from->resourcepart) == 0) {
                sender = jid_create(occupant->jid);
                break;
            }
        }
        g_list_free(roster);
        if (!sender) {
            log_warning("[OMEMO][RECV] cannot find MUC message sender fulljid");
            goto out;
        }
    } else {
        sender = jid_create(from->barejid);
    }

    session_cipher* cipher;
    signal_buffer* plaintext_key;
    signal_protocol_address address = {
        .name = sender->barejid,
        .name_len = strlen(sender->barejid),
        .device_id = sid
    };

    res = session_cipher_create(&cipher, omemo_ctx.store, &address, omemo_ctx.signal);
    if (res != 0) {
        log_error("[OMEMO][RECV] cannot create session cipher");
        goto out;
    }

    if (key->prekey) {
        log_debug("[OMEMO][RECV] decrypting message with prekey");
        pre_key_signal_message* message;
        ec_public_key* their_identity_key;
        signal_buffer* identity_buffer = NULL;

        omemo_ctx.identity_key_store.recv = true;

        pre_key_signal_message_deserialize(&message, key->data, key->length, omemo_ctx.signal);
        their_identity_key = pre_key_signal_message_get_identity_key(message);

        res = session_cipher_decrypt_pre_key_signal_message(cipher, message, NULL, &plaintext_key);

        omemo_ctx.identity_key_store.recv = false;

        /* Perform a real check of the identity */
        ec_public_key_serialize(&identity_buffer, their_identity_key);
        *trusted = is_trusted_identity(&address, signal_buffer_data(identity_buffer),
                                       signal_buffer_len(identity_buffer), &omemo_ctx.identity_key_store);

        /* Replace used pre_key in bundle */
        uint32_t pre_key_id = pre_key_signal_message_get_pre_key_id(message);
        ec_key_pair* ec_pair;
        session_pre_key* new_pre_key;
        curve_generate_key_pair(omemo_ctx.signal, &ec_pair);
        session_pre_key_create(&new_pre_key, pre_key_id, ec_pair);
        signal_protocol_pre_key_store_key(omemo_ctx.store, new_pre_key);
        SIGNAL_UNREF(new_pre_key);
        SIGNAL_UNREF(message);
        SIGNAL_UNREF(ec_pair);
        omemo_bundle_publish(true);

        if (res == 0) {
            /* Start a new session */
            log_info("[OMEMO][RECV] Res is 0 => omemo_bundle_request");
            omemo_bundle_request(sender->barejid, sid, omemo_start_device_session_handle_bundle, free, strdup(sender->barejid));
        }
    } else {
        log_debug("[OMEMO][RECV] decrypting message with existing session");
        signal_message* message = NULL;

        res = signal_message_deserialize(&message, key->data, key->length, omemo_ctx.signal);

        if (res < 0) {
            log_error("[OMEMO][RECV] cannot deserialize message");
        } else {
            res = session_cipher_decrypt_signal_message(cipher, message, NULL, &plaintext_key);
            *trusted = true;
            SIGNAL_UNREF(message);
        }
    }

    session_cipher_free(cipher);
    if (res != 0) {
        log_error("[OMEMO][RECV] cannot decrypt message key");
        goto out;
    }

    if (signal_buffer_len(plaintext_key) != AES128_GCM_KEY_LENGTH + AES128_GCM_TAG_LENGTH) {
        log_error("[OMEMO][RECV] invalid key length");
        signal_buffer_free(plaintext_key);
        goto out;
    }

    size_t plaintext_len = payload_len;
    plaintext = malloc(plaintext_len + 1);
    res = aes128gcm_decrypt(plaintext, &plaintext_len, payload, payload_len, iv,
                            iv_len, signal_buffer_data(plaintext_key),
                            signal_buffer_data(plaintext_key) + AES128_GCM_KEY_LENGTH);
    signal_buffer_free(plaintext_key);
    if (res != 0) {
        log_error("[OMEMO][RECV] cannot decrypt message: %s", gcry_strerror(res));
        free(plaintext);
        plaintext = NULL;
        goto out;
    }

    plaintext[plaintext_len] = '\0';

out:
    jid_destroy(from);
    jid_destroy(sender);
    return (char*)plaintext;
}

char*
omemo_format_fingerprint(const char* const fingerprint)
{
    char* output = malloc(strlen(fingerprint) + strlen(fingerprint) / 8);

    int i, j;
    for (i = 0, j = 0; i < strlen(fingerprint); i++) {
        if (i > 0 && i % 8 == 0) {
            output[j++] = '-';
        }
        output[j++] = fingerprint[i];
    }

    output[j] = '\0';

    return output;
}

static char*
_omemo_unformat_fingerprint(const char* const fingerprint_formatted)
{
    /* Unformat fingerprint */
    char* fingerprint = malloc(strlen(fingerprint_formatted));
    int i;
    int j;
    for (i = 0, j = 0; fingerprint_formatted[i] != '\0'; i++) {
        if (!g_ascii_isxdigit(fingerprint_formatted[i])) {
            continue;
        }
        fingerprint[j++] = fingerprint_formatted[i];
    }

    fingerprint[j] = '\0';

    return fingerprint;
}

char*
omemo_own_fingerprint(gboolean formatted)
{
    ec_public_key* identity = ratchet_identity_key_pair_get_public(omemo_ctx.identity_key_pair);
    return _omemo_fingerprint(identity, formatted);
}

GList*
omemo_known_device_identities(const char* const jid)
{
    GHashTable* known_identities = g_hash_table_lookup(omemo_ctx.known_devices, jid);
    if (!known_identities) {
        return NULL;
    }

    return g_hash_table_get_keys(known_identities);
}

gboolean
omemo_is_trusted_identity(const char* const jid, const char* const fingerprint)
{
    GHashTable* known_identities = g_hash_table_lookup(omemo_ctx.known_devices, jid);
    if (!known_identities) {
        return FALSE;
    }

    void* device_id = g_hash_table_lookup(known_identities, fingerprint);
    if (!device_id) {
        return FALSE;
    }

    signal_protocol_address address = {
        .name = jid,
        .name_len = strlen(jid),
        .device_id = GPOINTER_TO_INT(device_id),
    };

    size_t fingerprint_len;
    unsigned char* fingerprint_raw = _omemo_fingerprint_decode(fingerprint, &fingerprint_len);
    unsigned char djb_type[] = { '\x05' };
    signal_buffer* buffer = signal_buffer_create(djb_type, 1);
    buffer = signal_buffer_append(buffer, fingerprint_raw, fingerprint_len);

    gboolean trusted = is_trusted_identity(&address, signal_buffer_data(buffer), signal_buffer_len(buffer), &omemo_ctx.identity_key_store);
    log_info("[OMEMO] Device trusted %s (%d): %d", jid, GPOINTER_TO_INT(device_id), trusted);

    free(fingerprint_raw);
    signal_buffer_free(buffer);

    return trusted;
}

static char*
_omemo_fingerprint(ec_public_key* identity, gboolean formatted)
{
    int i;
    signal_buffer* identity_public_key;

    ec_public_key_serialize(&identity_public_key, identity);
    size_t identity_public_key_len = signal_buffer_len(identity_public_key);
    unsigned char* identity_public_key_data = signal_buffer_data(identity_public_key);

    /* Skip first byte corresponding to signal DJB_TYPE */
    identity_public_key_len--;
    identity_public_key_data = &identity_public_key_data[1];

    char* fingerprint = malloc(identity_public_key_len * 2 + 1);

    for (i = 0; i < identity_public_key_len; i++) {
        fingerprint[i * 2] = (identity_public_key_data[i] & 0xf0) >> 4;
        fingerprint[i * 2] += '0';
        if (fingerprint[i * 2] > '9') {
            fingerprint[i * 2] += 0x27;
        }

        fingerprint[(i * 2) + 1] = identity_public_key_data[i] & 0x0f;
        fingerprint[(i * 2) + 1] += '0';
        if (fingerprint[(i * 2) + 1] > '9') {
            fingerprint[(i * 2) + 1] += 0x27;
        }
    }

    fingerprint[i * 2] = '\0';
    signal_buffer_free(identity_public_key);

    if (!formatted) {
        return fingerprint;
    } else {
        char* formatted_fingerprint = omemo_format_fingerprint(fingerprint);
        free(fingerprint);
        return formatted_fingerprint;
    }
}

static unsigned char*
_omemo_fingerprint_decode(const char* const fingerprint, size_t* len)
{
    unsigned char* output = malloc(strlen(fingerprint) / 2 + 1);

    int i;
    int j;
    for (i = 0, j = 0; i < strlen(fingerprint);) {
        if (!g_ascii_isxdigit(fingerprint[i])) {
            i++;
            continue;
        }

        output[j] = g_ascii_xdigit_value(fingerprint[i++]) << 4;
        output[j] |= g_ascii_xdigit_value(fingerprint[i++]);
        j++;
    }

    *len = j;

    return output;
}

void
omemo_trust(const char* const jid, const char* const fingerprint_formatted)
{
    size_t len;

    GHashTable* known_identities = g_hash_table_lookup(omemo_ctx.known_devices, jid);
    if (!known_identities) {
        log_warning("[OMEMO] cannot trust unknown device: %s", fingerprint_formatted);
        cons_show("Cannot trust unknown device: %s", fingerprint_formatted);
        return;
    }

    char* fingerprint = _omemo_unformat_fingerprint(fingerprint_formatted);

    uint32_t device_id = GPOINTER_TO_INT(g_hash_table_lookup(known_identities, fingerprint));
    free(fingerprint);

    if (!device_id) {
        log_warning("[OMEMO] cannot trust unknown device: %s", fingerprint_formatted);
        cons_show("Cannot trust unknown device: %s", fingerprint_formatted);
        return;
    }

    /* TODO should not hardcode DJB_TYPE here
     * should instead store identity key in known_identities along with
     * device_id */
    signal_protocol_address address = {
        .name = jid,
        .name_len = strlen(jid),
        .device_id = device_id,
    };

    unsigned char* fingerprint_raw = _omemo_fingerprint_decode(fingerprint_formatted, &len);
    unsigned char djb_type[] = { '\x05' };
    signal_buffer* buffer = signal_buffer_create(djb_type, 1);
    buffer = signal_buffer_append(buffer, fingerprint_raw, len);
    save_identity(&address, signal_buffer_data(buffer), signal_buffer_len(buffer), &omemo_ctx.identity_key_store);
    free(fingerprint_raw);
    signal_buffer_free(buffer);

    omemo_bundle_request(jid, device_id, omemo_start_device_session_handle_bundle, free, strdup(jid));
}

void
omemo_untrust(const char* const jid, const char* const fingerprint_formatted)
{
    size_t len;
    unsigned char* identity = _omemo_fingerprint_decode(fingerprint_formatted, &len);

    GHashTableIter iter;
    gpointer key, value;

    GHashTable* trusted = g_hash_table_lookup(omemo_ctx.identity_key_store.trusted, jid);
    if (!trusted) {
        free(identity);
        return;
    }

    g_hash_table_iter_init(&iter, trusted);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        signal_buffer* buffer = value;
        unsigned char* original = signal_buffer_data(buffer);
        /* Skip DJB_TYPE byte */
        original++;
        if ((signal_buffer_len(buffer) - 1) == len && memcmp(original, identity, len) == 0) {
            g_hash_table_remove(trusted, key);
        }
    }
    free(identity);

    char* fingerprint = _omemo_unformat_fingerprint(fingerprint_formatted);

    /* Remove existing session */
    GHashTable* known_identities = g_hash_table_lookup(omemo_ctx.known_devices, jid);
    if (!known_identities) {
        log_error("[OMEMO] cannot find known device while untrusting a fingerprint");
        goto out;
    }

    uint32_t device_id = GPOINTER_TO_INT(g_hash_table_lookup(known_identities, fingerprint));
    if (!device_id) {
        log_error("[OMEMO] cannot find device id while untrusting a fingerprint");
        goto out;
    }
    signal_protocol_address address = {
        .name = jid,
        .name_len = strlen(jid),
        .device_id = device_id
    };

    delete_session(&address, omemo_ctx.session_store);

    /* Remove from keyfile */
    char* device_id_str = g_strdup_printf("%d", device_id);
    g_key_file_remove_key(omemo_ctx.trust_keyfile, jid, device_id_str, NULL);
    g_free(device_id_str);
    omemo_trust_keyfile_save();

out:
    free(fingerprint);
}

static void
_lock(void* user_data)
{
    omemo_context* ctx = (omemo_context*)user_data;
    pthread_mutex_lock(&ctx->lock);
}

static void
_unlock(void* user_data)
{
    omemo_context* ctx = (omemo_context*)user_data;
    pthread_mutex_unlock(&ctx->lock);
}

static void
_omemo_log(int level, const char* message, size_t len, void* user_data)
{
    switch (level) {
    case SG_LOG_ERROR:
        log_error("[OMEMO][SIGNAL] %s", message);
        break;
    case SG_LOG_WARNING:
        log_warning("[OMEMO][SIGNAL] %s", message);
        break;
    case SG_LOG_NOTICE:
    case SG_LOG_INFO:
        log_info("[OMEMO][SIGNAL] %s", message);
        break;
    case SG_LOG_DEBUG:
        log_debug("[OMEMO][SIGNAL] %s", message);
        break;
    }
}

static gboolean
_handle_own_device_list(const char* const jid, GList* device_list)
{
    // We didn't find the own device id -> publish
    if (!g_list_find(device_list, GINT_TO_POINTER(omemo_ctx.device_id))) {
        log_info("[OMEMO] No device id for our device? publish device list...");
        device_list = g_list_copy(device_list);
        device_list = g_list_append(device_list, GINT_TO_POINTER(omemo_ctx.device_id));
        g_hash_table_insert(omemo_ctx.device_list, strdup(jid), device_list);
        omemo_devicelist_publish(device_list);
    }

    log_info("[OMEMO] Request OMEMO Bundles for my devices...");
    GList* device_id;
    for (device_id = device_list; device_id != NULL; device_id = device_id->next) {
        omemo_bundle_request(jid, GPOINTER_TO_INT(device_id->data), omemo_start_device_session_handle_bundle, free, strdup(jid));
    }

    return TRUE;
}

static gboolean
_handle_device_list_start_session(const char* const jid, GList* device_list)
{
    log_info("[OMEMO] Start session for %s - device_list", jid);
    omemo_start_session(jid);

    return FALSE;
}

void
omemo_key_free(omemo_key_t* key)
{
    if (key == NULL) {
        return;
    }

    free(key->data);
    free(key);
}

char*
omemo_fingerprint_autocomplete(const char* const search_str, gboolean previous, void* context)
{
    Autocomplete ac = g_hash_table_lookup(omemo_ctx.fingerprint_ac, context);
    if (ac != NULL) {
        return autocomplete_complete(ac, search_str, FALSE, previous);
    } else {
        return NULL;
    }
}

void
omemo_fingerprint_autocomplete_reset(void)
{
    gpointer value;
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, omemo_ctx.fingerprint_ac);

    while (g_hash_table_iter_next(&iter, NULL, &value)) {
        Autocomplete ac = value;
        autocomplete_reset(ac);
    }
}

gboolean
omemo_automatic_start(const char* const recipient)
{
    gboolean result = FALSE;
    char* account_name = session_get_account_name();
    ProfAccount* account = accounts_get_account(account_name);
    prof_omemopolicy_t policy;

    if (account->omemo_policy) {
        // check default account setting
        if (g_strcmp0(account->omemo_policy, "manual") == 0) {
            policy = PROF_OMEMOPOLICY_MANUAL;
        }
        if (g_strcmp0(account->omemo_policy, "opportunistic") == 0) {
            policy = PROF_OMEMOPOLICY_AUTOMATIC;
        }
        if (g_strcmp0(account->omemo_policy, "always") == 0) {
            policy = PROF_OMEMOPOLICY_ALWAYS;
        }
    } else {
        // check global setting
        char* pref_omemo_policy = prefs_get_string(PREF_OMEMO_POLICY);

        // pref defaults to manual
        policy = PROF_OMEMOPOLICY_AUTOMATIC;

        if (strcmp(pref_omemo_policy, "manual") == 0) {
            policy = PROF_OMEMOPOLICY_MANUAL;
        } else if (strcmp(pref_omemo_policy, "always") == 0) {
            policy = PROF_OMEMOPOLICY_ALWAYS;
        }

        g_free(pref_omemo_policy);
    }

    switch (policy) {
    case PROF_OMEMOPOLICY_MANUAL:
        result = FALSE;
        break;
    case PROF_OMEMOPOLICY_AUTOMATIC:
        if (g_list_find_custom(account->omemo_enabled, recipient, (GCompareFunc)g_strcmp0)) {
            result = TRUE;
        } else if (g_list_find_custom(account->omemo_disabled, recipient, (GCompareFunc)g_strcmp0)) {
            result = FALSE;
        } else {
            result = FALSE;
        }
        break;
    case PROF_OMEMOPOLICY_ALWAYS:
        if (g_list_find_custom(account->omemo_disabled, recipient, (GCompareFunc)g_strcmp0)) {
            result = FALSE;
        } else {
            result = TRUE;
        }
        break;
    }

    account_free(account);
    return result;
}

static gboolean
_load_identity(void)
{
    GError* error = NULL;
    log_info("[OMEMO] Loading OMEMO identity");

    /* Device ID */
    error = NULL;
    omemo_ctx.device_id = g_key_file_get_uint64(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_DEVICE_ID, &error);
    if (error != NULL) {
        log_error("[OMEMO] cannot load device id: %s", error->message);
        return FALSE;
    }
    log_info("[OMEMO] device id: %d", omemo_ctx.device_id);

    /* Registration ID */
    error = NULL;
    omemo_ctx.registration_id = g_key_file_get_uint64(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_REGISTRATION_ID, &error);
    if (error != NULL) {
        log_error("[OMEMO] cannot load registration id: %s", error->message);
        return FALSE;
    }

    /* Identity key */
    error = NULL;
    char* identity_key_public_b64 = g_key_file_get_string(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_IDENTITY_KEY_PUBLIC, &error);
    if (!identity_key_public_b64) {
        log_error("[OMEMO] cannot load identity public key: %s", error->message);
        return FALSE;
    }

    size_t identity_key_public_len;
    unsigned char* identity_key_public = g_base64_decode(identity_key_public_b64, &identity_key_public_len);
    g_free(identity_key_public_b64);
    omemo_ctx.identity_key_store.public = signal_buffer_create(identity_key_public, identity_key_public_len);

    error = NULL;
    char* identity_key_private_b64 = g_key_file_get_string(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_IDENTITY_KEY_PRIVATE, &error);
    if (!identity_key_private_b64) {
        log_error("[OMEMO] cannot load identity private key: %s", error->message);
        return FALSE;
    }

    size_t identity_key_private_len;
    unsigned char* identity_key_private = g_base64_decode(identity_key_private_b64, &identity_key_private_len);
    g_free(identity_key_private_b64);
    omemo_ctx.identity_key_store.private = signal_buffer_create(identity_key_private, identity_key_private_len);

    ec_public_key* public_key;
    curve_decode_point(&public_key, identity_key_public, identity_key_public_len, omemo_ctx.signal);
    ec_private_key* private_key;
    curve_decode_private_point(&private_key, identity_key_private, identity_key_private_len, omemo_ctx.signal);
    ratchet_identity_key_pair_create(&omemo_ctx.identity_key_pair, public_key, private_key);

    g_free(identity_key_public);
    g_free(identity_key_private);

    char** keys = NULL;
    int i;
    /* Pre keys */
    i = 0;
    keys = g_key_file_get_keys(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_PREKEYS, NULL, NULL);
    if (keys) {
        for (i = 0; keys[i] != NULL; i++) {
            char* pre_key_b64 = g_key_file_get_string(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_PREKEYS, keys[i], NULL);
            size_t pre_key_len;
            unsigned char* pre_key = g_base64_decode(pre_key_b64, &pre_key_len);
            g_free(pre_key_b64);
            signal_buffer* buffer = signal_buffer_create(pre_key, pre_key_len);
            g_free(pre_key);
            g_hash_table_insert(omemo_ctx.pre_key_store, GINT_TO_POINTER(strtoul(keys[i], NULL, 10)), buffer);
        }

        g_strfreev(keys);
    }

    /* Ensure we have at least 100 pre keys */
    if (i < 100) {
        _generate_pre_keys(100 - i);
    }

    /* Signed pre keys */
    i = 0;
    keys = g_key_file_get_keys(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_SIGNED_PREKEYS, NULL, NULL);
    if (keys) {
        for (i = 0; keys[i] != NULL; i++) {
            char* signed_pre_key_b64 = g_key_file_get_string(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_SIGNED_PREKEYS, keys[i], NULL);
            size_t signed_pre_key_len;
            unsigned char* signed_pre_key = g_base64_decode(signed_pre_key_b64, &signed_pre_key_len);
            g_free(signed_pre_key_b64);
            signal_buffer* buffer = signal_buffer_create(signed_pre_key, signed_pre_key_len);
            g_free(signed_pre_key);
            g_hash_table_insert(omemo_ctx.signed_pre_key_store, GINT_TO_POINTER(strtoul(keys[i], NULL, 10)), buffer);
            omemo_ctx.signed_pre_key_id = strtoul(keys[i], NULL, 10);
        }
        g_strfreev(keys);
    }

    if (i == 0) {
        _generate_signed_pre_key();
    }

    loaded = TRUE;

    omemo_identity_keyfile_save();
    omemo_start_sessions();

    return TRUE;
}

static void
_load_trust(void)
{
    char** keys = NULL;
    char** groups = g_key_file_get_groups(omemo_ctx.trust_keyfile, NULL);
    if (groups) {
        int i;
        for (i = 0; groups[i] != NULL; i++) {
            GHashTable* trusted;

            trusted = g_hash_table_lookup(omemo_ctx.identity_key_store.trusted, groups[i]);
            if (!trusted) {
                trusted = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
                g_hash_table_insert(omemo_ctx.identity_key_store.trusted, strdup(groups[i]), trusted);
            }

            keys = g_key_file_get_keys(omemo_ctx.trust_keyfile, groups[i], NULL, NULL);
            int j;
            for (j = 0; keys[j] != NULL; j++) {
                char* key_b64 = g_key_file_get_string(omemo_ctx.trust_keyfile, groups[i], keys[j], NULL);
                size_t key_len;
                unsigned char* key = g_base64_decode(key_b64, &key_len);
                g_free(key_b64);
                signal_buffer* buffer = signal_buffer_create(key, key_len);
                g_free(key);
                uint32_t device_id = strtoul(keys[j], NULL, 10);
                g_hash_table_insert(trusted, GINT_TO_POINTER(device_id), buffer);
            }
            g_strfreev(keys);
        }
        g_strfreev(groups);
    }
}

static void
_load_sessions(void)
{
    int i;
    char** groups = g_key_file_get_groups(omemo_ctx.sessions_keyfile, NULL);
    if (groups) {
        for (i = 0; groups[i] != NULL; i++) {
            int j;
            GHashTable* device_store = NULL;

            device_store = g_hash_table_lookup(omemo_ctx.session_store, groups[i]);
            if (!device_store) {
                device_store = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
                g_hash_table_insert(omemo_ctx.session_store, strdup(groups[i]), device_store);
            }

            char** keys = g_key_file_get_keys(omemo_ctx.sessions_keyfile, groups[i], NULL, NULL);
            for (j = 0; keys[j] != NULL; j++) {
                uint32_t id = strtoul(keys[j], NULL, 10);
                char* record_b64 = g_key_file_get_string(omemo_ctx.sessions_keyfile, groups[i], keys[j], NULL);
                size_t record_len;
                unsigned char* record = g_base64_decode(record_b64, &record_len);
                g_free(record_b64);
                signal_buffer* buffer = signal_buffer_create(record, record_len);
                g_free(record);
                g_hash_table_insert(device_store, GINT_TO_POINTER(id), buffer);
            }
            g_strfreev(keys);
        }
        g_strfreev(groups);
    }
}

static void
_load_known_devices(void)
{
    int i;
    char** groups = g_key_file_get_groups(omemo_ctx.known_devices_keyfile, NULL);
    if (groups) {
        for (i = 0; groups[i] != NULL; i++) {
            int j;
            GHashTable* known_identities = NULL;

            known_identities = g_hash_table_lookup(omemo_ctx.known_devices, groups[i]);
            if (!known_identities) {
                known_identities = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
                g_hash_table_insert(omemo_ctx.known_devices, strdup(groups[i]), known_identities);
            }

            char** keys = g_key_file_get_keys(omemo_ctx.known_devices_keyfile, groups[i], NULL, NULL);
            for (j = 0; keys[j] != NULL; j++) {
                uint32_t device_id = strtoul(keys[j], NULL, 10);
                char* fingerprint = g_key_file_get_string(omemo_ctx.known_devices_keyfile, groups[i], keys[j], NULL);
                g_hash_table_insert(known_identities, strdup(fingerprint), GINT_TO_POINTER(device_id));
                g_free(fingerprint);
            }
            g_strfreev(keys);
        }
        g_strfreev(groups);
    }
}

static void
_cache_device_identity(const char* const jid, uint32_t device_id, ec_public_key* identity)
{
    GHashTable* known_identities = g_hash_table_lookup(omemo_ctx.known_devices, jid);
    if (!known_identities) {
        known_identities = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
        g_hash_table_insert(omemo_ctx.known_devices, strdup(jid), known_identities);
    }

    char* fingerprint = _omemo_fingerprint(identity, FALSE);
    log_info("[OMEMO] cache identity for %s:%d: %s", jid, device_id, fingerprint);
    g_hash_table_insert(known_identities, strdup(fingerprint), GINT_TO_POINTER(device_id));

    char* device_id_str = g_strdup_printf("%d", device_id);
    g_key_file_set_string(omemo_ctx.known_devices_keyfile, jid, device_id_str, fingerprint);
    g_free(device_id_str);
    omemo_known_devices_keyfile_save();

    Autocomplete ac = g_hash_table_lookup(omemo_ctx.fingerprint_ac, jid);
    if (ac == NULL) {
        ac = autocomplete_new();
        g_hash_table_insert(omemo_ctx.fingerprint_ac, strdup(jid), ac);
    }

    char* formatted_fingerprint = omemo_format_fingerprint(fingerprint);
    autocomplete_add(ac, formatted_fingerprint);
    free(formatted_fingerprint);
    free(fingerprint);
}

static void
_g_hash_table_free(GHashTable* hash_table)
{
    g_hash_table_remove_all(hash_table);
    g_hash_table_unref(hash_table);
}

static void
_generate_pre_keys(int count)
{
    unsigned int start;
    gcry_randomize(&start, sizeof(unsigned int), GCRY_VERY_STRONG_RANDOM);
    signal_protocol_key_helper_pre_key_list_node* pre_keys_head;
    signal_protocol_key_helper_generate_pre_keys(&pre_keys_head, start, count, omemo_ctx.signal);

    signal_protocol_key_helper_pre_key_list_node* p;
    for (p = pre_keys_head; p != NULL; p = signal_protocol_key_helper_key_list_next(p)) {
        session_pre_key* prekey = signal_protocol_key_helper_key_list_element(p);
        signal_protocol_pre_key_store_key(omemo_ctx.store, prekey);
    }
    signal_protocol_key_helper_key_list_free(pre_keys_head);
}

static void
_generate_signed_pre_key(void)
{
    session_signed_pre_key* signed_pre_key;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long timestamp = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;

    omemo_ctx.signed_pre_key_id = 1;
    signal_protocol_key_helper_generate_signed_pre_key(&signed_pre_key, omemo_ctx.identity_key_pair, omemo_ctx.signed_pre_key_id, timestamp, omemo_ctx.signal);
    signal_protocol_signed_pre_key_store_key(omemo_ctx.store, signed_pre_key);
    SIGNAL_UNREF(signed_pre_key);
}

void
omemo_free(void* a)
{
    gcry_free(a);
}

char*
omemo_encrypt_file(FILE* in, FILE* out, off_t file_size, int* gcry_res)
{
    unsigned char* key = gcry_random_bytes_secure(
        OMEMO_AESGCM_KEY_LENGTH,
        GCRY_VERY_STRONG_RANDOM);

    // Create nonce/IV with random bytes.
    unsigned char nonce[OMEMO_AESGCM_NONCE_LENGTH];
    gcry_create_nonce(nonce, OMEMO_AESGCM_NONCE_LENGTH);

    char* fragment = aes256gcm_create_secure_fragment(key, nonce);
    *gcry_res = aes256gcm_crypt_file(in, out, file_size, key, nonce, true);

    if (*gcry_res != GPG_ERR_NO_ERROR) {
        gcry_free(fragment);
        fragment = NULL;
    }

    gcry_free(key);

    return fragment;
}

void
_bytes_from_hex(const char* hex, size_t hex_size,
                unsigned char* bytes, size_t bytes_size)
{
    const unsigned char ht[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // 01234567
        0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 89:;<=>?
        0x00, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x00, // @ABCDEFG
    };
    const size_t ht_size = sizeof(ht);

    unsigned char b0;
    unsigned char b1;

    memset(bytes, 0, bytes_size);

    for (int i = 0; (i < hex_size) && (i / 2 < bytes_size); i += 2) {
        b0 = ((unsigned char)hex[i + 0] & 0x1f) ^ 0x10;
        b1 = ((unsigned char)hex[i + 1] & 0x1f) ^ 0x10;

        if (b0 <= ht_size && b1 <= ht_size) {
            bytes[i / 2] = (unsigned char)(ht[b0] << 4) | ht[b1];
        }
    }
}

gcry_error_t
omemo_decrypt_file(FILE* in, FILE* out, off_t file_size, const char* fragment)
{
    char nonce_hex[AESGCM_URL_NONCE_LEN];
    char key_hex[AESGCM_URL_KEY_LEN];

    const int nonce_pos = 0;
    const int key_pos = AESGCM_URL_NONCE_LEN;

    memcpy(nonce_hex, &(fragment[nonce_pos]), AESGCM_URL_NONCE_LEN);
    memcpy(key_hex, &(fragment[key_pos]), AESGCM_URL_KEY_LEN);

    unsigned char nonce[OMEMO_AESGCM_NONCE_LENGTH];
    unsigned char* key = gcry_malloc_secure(OMEMO_AESGCM_KEY_LENGTH);

    _bytes_from_hex(nonce_hex, AESGCM_URL_NONCE_LEN,
                    nonce, OMEMO_AESGCM_NONCE_LENGTH);
    _bytes_from_hex(key_hex, AESGCM_URL_KEY_LEN,
                    key, OMEMO_AESGCM_KEY_LENGTH);

    gcry_error_t crypt_res;
    crypt_res = aes256gcm_crypt_file(in, out, file_size, key, nonce, false);

    gcry_free(key);

    return crypt_res;
}

int
omemo_parse_aesgcm_url(const char* aesgcm_url,
                       char** https_url,
                       char** fragment)
{
    CURLUcode ret;
    CURLU* url = curl_url();

    // Required to allow for the "aesgcm://" scheme that OMEMO Media Sharing
    // uses.
    unsigned int curl_flags = CURLU_NON_SUPPORT_SCHEME;

    ret = curl_url_set(url, CURLUPART_URL, aesgcm_url, curl_flags);
    if (ret) {
        goto out;
    }

    ret = curl_url_get(url, CURLUPART_FRAGMENT, fragment, curl_flags);
    if (ret) {
        goto out;
    }

    if (strlen(*fragment) != AESGCM_URL_NONCE_LEN + AESGCM_URL_KEY_LEN) {
        goto out;
    }

    // Clear fragment from HTTPS URL as it's not required for download.
    ret = curl_url_set(url, CURLUPART_FRAGMENT, NULL, curl_flags);
    if (ret) {
        goto out;
    }

    ret = curl_url_set(url, CURLUPART_SCHEME, "https", curl_flags);
    if (ret) {
        goto out;
    }

    ret = curl_url_get(url, CURLUPART_URL, https_url, curl_flags);
    if (ret) {
        goto out;
    }

out:
    curl_url_cleanup(url);
    return ret;
}
