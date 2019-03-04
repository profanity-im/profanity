#include <sys/time.h>
#include <sys/stat.h>

#include <errno.h>
#include <glib.h>
#include <pthread.h>
#include <signal/key_helper.h>
#include <signal/protocol.h>
#include <signal/signal_protocol.h>
#include <signal/session_builder.h>
#include <signal/session_cipher.h>
#include <sodium.h>

#include "config/account.h"
#include "log.h"
#include "omemo/store.h"
#include "omemo/crypto.h"
#include "omemo/omemo.h"
#include "ui/ui.h"
#include "xmpp/xmpp.h"
#include "xmpp/connection.h"
#include "xmpp/omemo.h"
#include "config/files.h"

static gboolean loaded;

static void omemo_generate_short_term_crypto_materials(ProfAccount *account);
static void lock(void *user_data);
static void unlock(void *user_data);
static void omemo_log(int level, const char *message, size_t len, void *user_data);
static gboolean handle_own_device_list(const char *const jid, GList *device_list);
static gboolean handle_device_list_start_session(const char *const jid, GList *device_list);

typedef gboolean (*OmemoDeviceListHandler)(const char *const jid, GList *device_list);

struct omemo_context_t {
    pthread_mutexattr_t attr;
    pthread_mutex_t lock;
    signal_context *signal;
    uint32_t device_id;
    GHashTable *device_list;
    GHashTable *device_list_handler;
    ratchet_identity_key_pair *identity_key_pair;
    uint32_t registration_id;
    signal_protocol_key_helper_pre_key_list_node *pre_keys_head;
    session_signed_pre_key *signed_pre_key;
    signal_protocol_store_context *store;
    GHashTable *session_store;
    GHashTable *pre_key_store;
    GHashTable *signed_pre_key_store;
    identity_key_store_t identity_key_store;
    GHashTable *device_ids;
    GString *identity_filename;
    GKeyFile *identity_keyfile;
};

static omemo_context omemo_ctx;

void
omemo_init(void)
{
    log_info("Initialising OMEMO");
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

    if (omemo_crypto_init() != 0) {
        cons_show("Error initializing OMEMO crypto");
    }

    pthread_mutexattr_init(&omemo_ctx.attr);
    pthread_mutexattr_settype(&omemo_ctx.attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&omemo_ctx.lock, &omemo_ctx.attr);

    if (signal_context_create(&omemo_ctx.signal, &omemo_ctx) != 0) {
        cons_show("Error initializing OMEMO context");
        return;
    }

    if (signal_context_set_log_function(omemo_ctx.signal, omemo_log) != 0) {
        cons_show("Error initializing OMEMO log");
    }

    if (signal_context_set_crypto_provider(omemo_ctx.signal, &crypto_provider) != 0) {
        cons_show("Error initializing OMEMO crypto");
        return;
    }

    signal_context_set_locking_functions(omemo_ctx.signal, lock, unlock);

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
        .user_data = omemo_ctx.pre_key_store
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


    loaded = FALSE;
    omemo_ctx.device_list = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)g_list_free);
    omemo_ctx.device_list_handler = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
}

void
omemo_on_connect(ProfAccount *account)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    char *barejid = xmpp_jid_bare(ctx, session_get_account_name());
    char *omemodir = files_get_data_path(DIR_OMEMO);
    GString *basedir = g_string_new(omemodir);
    free(omemodir);
    gchar *account_dir = str_replace(barejid, "@", "_at_");
    g_string_append(basedir, "/");
    g_string_append(basedir, account_dir);
    g_string_append(basedir, "/");
    free(account_dir);

    omemo_ctx.identity_filename = g_string_new(basedir->str);
    g_string_append(omemo_ctx.identity_filename, "identity.txt");

    errno = 0;
    int res = g_mkdir_with_parents(basedir->str, S_IRWXU);
    if (res == -1) {
        char *errmsg = strerror(errno);
        if (errmsg) {
            log_error("Error creating directory: %s, %s", omemo_ctx.identity_filename->str, errmsg);
        } else {
            log_error("Error creating directory: %s", omemo_ctx.identity_filename->str);
        }
    }

    omemo_ctx.identity_keyfile = g_key_file_new();
    if (g_key_file_load_from_file(omemo_ctx.identity_keyfile, omemo_ctx.identity_filename->str, G_KEY_FILE_KEEP_COMMENTS, NULL)) {
        omemo_ctx.device_id = g_key_file_get_uint64(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_DEVICE_ID, NULL);
        omemo_ctx.registration_id = g_key_file_get_uint64(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_REGISTRATION_ID, NULL);

        char *identity_key_public_b64 = g_key_file_get_string(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_IDENTITY_KEY_PUBLIC, NULL);
        size_t identity_key_public_len;
        unsigned char *identity_key_public = g_base64_decode(identity_key_public_b64, &identity_key_public_len);
        omemo_ctx.identity_key_store.public = signal_buffer_create(identity_key_public, identity_key_public_len);

        char *identity_key_private_b64 = g_key_file_get_string(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_IDENTITY_KEY_PRIVATE, NULL);
        size_t identity_key_private_len;
        unsigned char *identity_key_private = g_base64_decode(identity_key_private_b64, &identity_key_private_len);
        omemo_ctx.identity_key_store.private = signal_buffer_create(identity_key_private, identity_key_private_len);
        signal_buffer_create(identity_key_private, identity_key_private_len);

        ec_public_key *public_key;
        curve_decode_point(&public_key, identity_key_public, identity_key_public_len, omemo_ctx.signal);
        ec_private_key *private_key;
        curve_decode_private_point(&private_key, identity_key_private, identity_key_private_len, omemo_ctx.signal);
        ratchet_identity_key_pair_create(&omemo_ctx.identity_key_pair, public_key, private_key);

        g_free(identity_key_public);
        g_free(identity_key_private);

        omemo_generate_short_term_crypto_materials(account);
    }
}

void
omemo_generate_crypto_materials(ProfAccount *account)
{
    GError *error = NULL;

    if (loaded) {
        return;
    }

    omemo_ctx.device_id = randombytes_uniform(0x80000000);

    signal_protocol_key_helper_generate_identity_key_pair(&omemo_ctx.identity_key_pair, omemo_ctx.signal);
    signal_protocol_key_helper_generate_registration_id(&omemo_ctx.registration_id, 0, omemo_ctx.signal);

    ec_public_key_serialize(&omemo_ctx.identity_key_store.public, ratchet_identity_key_pair_get_public(omemo_ctx.identity_key_pair));
    ec_private_key_serialize(&omemo_ctx.identity_key_store.private, ratchet_identity_key_pair_get_private(omemo_ctx.identity_key_pair));

    g_key_file_set_uint64(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_DEVICE_ID, omemo_ctx.device_id);
    g_key_file_set_uint64(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_REGISTRATION_ID, omemo_ctx.registration_id);
    char *identity_key_public = g_base64_encode(signal_buffer_data(omemo_ctx.identity_key_store.public), signal_buffer_len(omemo_ctx.identity_key_store.public));
    g_key_file_set_string(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_IDENTITY_KEY_PUBLIC, identity_key_public);
    g_free(identity_key_public);
    char *identity_key_private = g_base64_encode(signal_buffer_data(omemo_ctx.identity_key_store.private), signal_buffer_len(omemo_ctx.identity_key_store.private));
    g_key_file_set_string(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_IDENTITY_KEY_PRIVATE, identity_key_private);
    g_free(identity_key_private);

    if (!g_key_file_save_to_file(omemo_ctx.identity_keyfile, omemo_ctx.identity_filename->str, &error)) {
        log_error("Error saving OMEMO identity to: %s, %s", omemo_ctx.identity_filename->str, error->message);
    }

    omemo_generate_short_term_crypto_materials(account);
}

static void
omemo_generate_short_term_crypto_materials(ProfAccount *account)
{
    signal_protocol_key_helper_generate_pre_keys(&omemo_ctx.pre_keys_head, randombytes_random(), 100, omemo_ctx.signal);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long timestamp = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
    signal_protocol_key_helper_generate_signed_pre_key(&omemo_ctx.signed_pre_key, omemo_ctx.identity_key_pair, 5, timestamp, omemo_ctx.signal);

    loaded = TRUE;

    /* Ensure we get our current device list, and it gets updated with our
     * device_id */
    xmpp_ctx_t * const ctx = connection_get_ctx();
    char *barejid = xmpp_jid_bare(ctx, session_get_account_name());
    g_hash_table_insert(omemo_ctx.device_list_handler, strdup(barejid), handle_own_device_list);
    omemo_devicelist_request(barejid);
    free(barejid);

    omemo_bundle_publish();
}

void
omemo_start_session(const char *const barejid)
{
    GList *device_list = g_hash_table_lookup(omemo_ctx.device_list, barejid);
    if (!device_list) {
        omemo_devicelist_request(barejid);
        g_hash_table_insert(omemo_ctx.device_list_handler, strdup(barejid), handle_device_list_start_session);
        return;
    }

    GList *device_id;
    for (device_id = device_list; device_id != NULL; device_id = device_id->next) {
        omemo_bundle_request(barejid, GPOINTER_TO_INT(device_id->data), omemo_start_device_session_handle_bundle, free, strdup(barejid));
    }
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
omemo_identity_key(unsigned char **output, size_t *length)
{
    signal_buffer *buffer = NULL;
    ec_public_key_serialize(&buffer, ratchet_identity_key_pair_get_public(omemo_ctx.identity_key_pair));
    *length = signal_buffer_len(buffer);
    *output = malloc(*length);
    memcpy(*output, signal_buffer_data(buffer), *length);
    signal_buffer_free(buffer);
}

void
omemo_signed_prekey(unsigned char **output, size_t *length)
{
    signal_buffer *buffer = NULL;
    ec_public_key_serialize(&buffer, ec_key_pair_get_public(session_signed_pre_key_get_key_pair(omemo_ctx.signed_pre_key)));
    *length = signal_buffer_len(buffer);
    *output = malloc(*length);
    memcpy(*output, signal_buffer_data(buffer), *length);
    signal_buffer_free(buffer);
}

void
omemo_signed_prekey_signature(unsigned char **output, size_t *length)
{
    *length = session_signed_pre_key_get_signature_len(omemo_ctx.signed_pre_key);
    *output = malloc(*length);
    memcpy(*output, session_signed_pre_key_get_signature(omemo_ctx.signed_pre_key), *length);
}

void
omemo_prekeys(GList **prekeys, GList **ids, GList **lengths)
{
    signal_protocol_key_helper_pre_key_list_node *p;
    for (p = omemo_ctx.pre_keys_head; p != NULL; p = signal_protocol_key_helper_key_list_next(p)) {
        session_pre_key *prekey = signal_protocol_key_helper_key_list_element(p);
        signal_buffer *buffer = NULL;
        ec_public_key_serialize(&buffer, ec_key_pair_get_public(session_pre_key_get_key_pair(prekey)));
        size_t length = signal_buffer_len(buffer);
        unsigned char *prekey_value = malloc(length);
        memcpy(prekey_value, signal_buffer_data(buffer), length);
        signal_buffer_free(buffer);
        *prekeys = g_list_append(*prekeys, prekey_value);
        *ids = g_list_append(*ids, GINT_TO_POINTER(session_pre_key_get_id(prekey)));
        *lengths = g_list_append(*lengths, GINT_TO_POINTER(length));
    }
}

void
omemo_set_device_list(const char *const jid, GList * device_list)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    char *barejid = xmpp_jid_bare(ctx, jid);

    g_hash_table_insert(omemo_ctx.device_list, strdup(barejid), device_list);

    OmemoDeviceListHandler handler = g_hash_table_lookup(omemo_ctx.device_list_handler, barejid);
    if (handler) {
        gboolean keep = handler(barejid, device_list);
        if (!keep) {
            g_hash_table_remove(omemo_ctx.device_list_handler, barejid);
        }
    }

    free(barejid);
}

void
omemo_start_device_session(const char *const jid, uint32_t device_id,
    uint32_t prekey_id, const unsigned char *const prekey_raw, size_t prekey_len,
    uint32_t signed_prekey_id, const unsigned char *const signed_prekey_raw,
    size_t signed_prekey_len, const unsigned char *const signature,
    size_t signature_len, const unsigned char *const identity_key_raw,
    size_t identity_key_len)
{
    session_pre_key_bundle *bundle;
    signal_protocol_address *address;

    address = malloc(sizeof(signal_protocol_address));
    address->name = strdup(jid);
    address->name_len = strlen(jid);
    address->device_id = device_id;

    session_builder *builder;
    session_builder_create(&builder, omemo_ctx.store, address, omemo_ctx.signal);

    ec_public_key *prekey;
    curve_decode_point(&prekey, prekey_raw, prekey_len, omemo_ctx.signal);
    ec_public_key *signed_prekey;
    curve_decode_point(&signed_prekey, signed_prekey_raw, signed_prekey_len, omemo_ctx.signal);
    ec_public_key *identity_key;
    curve_decode_point(&identity_key, identity_key_raw, identity_key_len, omemo_ctx.signal);

    session_pre_key_bundle_create(&bundle, 0, device_id, prekey_id, prekey, signed_prekey_id, signed_prekey, signature, signature_len, identity_key);
    session_builder_process_pre_key_bundle(builder, bundle);
}

gboolean
omemo_on_message_send(ProfChatWin *chatwin, const char *const message, gboolean request_receipt)
{
    int res;
    xmpp_ctx_t * const ctx = connection_get_ctx();
    char *barejid = xmpp_jid_bare(ctx, session_get_account_name());
    GList *device_ids = NULL;

    GList *recipient_device_id = g_hash_table_lookup(omemo_ctx.device_list, chatwin->barejid);
    if (!recipient_device_id) {
        return FALSE;
    }
    device_ids = g_list_copy(recipient_device_id);

    GList *sender_device_id = g_hash_table_lookup(omemo_ctx.device_list, barejid);
    device_ids = g_list_concat(device_ids, g_list_copy(sender_device_id));

    /* TODO generate fresh AES-GCM materials */
    /* TODO encrypt message */
    unsigned char *key;
    unsigned char *iv;
    unsigned char *ciphertext;
    size_t ciphertext_len;

    key = sodium_malloc(AES128_GCM_KEY_LENGTH);
    iv = sodium_malloc(AES128_GCM_IV_LENGTH);
    ciphertext_len = strlen(message) + AES128_GCM_TAG_LENGTH;
    ciphertext = malloc(ciphertext_len);

    randombytes_buf(key, 16);
    randombytes_buf(iv, 16);

    res = aes128gcm_encrypt(ciphertext, &ciphertext_len, (const unsigned char * const)message, strlen(message), iv, key);
    if (res != 0) {
        return FALSE;
    }

    GList *keys = NULL;
    GList *device_ids_iter;
    for (device_ids_iter = device_ids; device_ids_iter != NULL; device_ids_iter = device_ids_iter->next) {
        int res;
        ciphertext_message *ciphertext;
        session_cipher *cipher;
        signal_protocol_address address = {
            chatwin->barejid, strlen(chatwin->barejid), GPOINTER_TO_INT(device_ids_iter->data)
        };

        res = session_cipher_create(&cipher, omemo_ctx.store, &address, omemo_ctx.signal);
        if (res != 0) {
            continue;
        }

        res = session_cipher_encrypt(cipher, key, AES128_GCM_KEY_LENGTH, &ciphertext);
        if (res != 0) {
            continue;
        }
        signal_buffer *buffer = ciphertext_message_get_serialized(ciphertext);
        omemo_key_t *key = malloc(sizeof(omemo_key_t));
        key->data = signal_buffer_data(buffer);
        key->length = signal_buffer_len(buffer);
        key->device_id = GPOINTER_TO_INT(device_ids_iter->data);
        key->prekey = TRUE;
        keys = g_list_append(keys, key);
    }

    char *id = message_send_chat_omemo(chatwin->barejid, omemo_ctx.device_id, keys, iv, AES128_GCM_IV_LENGTH, ciphertext, ciphertext_len, request_receipt);
    chat_log_omemo_msg_out(chatwin->barejid, message);
    chatwin_outgoing_msg(chatwin, message, id, PROF_MSG_OMEMO, request_receipt);

    free(id);
    g_list_free_full(keys, free);
    free(ciphertext);
    sodium_free(key);
    sodium_free(iv);
    g_list_free(device_ids);

    return TRUE;
}

char *
omemo_on_message_recv(const char *const from, uint32_t sid,
    const unsigned char *const iv, size_t iv_len, GList *keys,
    const unsigned char *const payload, size_t payload_len)
{
    int res;
    GList *key_iter;
    omemo_key_t *key = NULL;
    for (key_iter = keys; key_iter != NULL; key_iter = key_iter->next) {
        if (((omemo_key_t *)key_iter->data)->device_id == omemo_ctx.device_id) {
            key = key_iter->data;
            break;
        }
    }

    if (!key) {
        return NULL;
    }

    session_cipher *cipher;
    signal_buffer *plaintext_key;
    signal_protocol_address address = {
        from, strlen(from), sid
    };

    res = session_cipher_create(&cipher, omemo_ctx.store, &address, omemo_ctx.signal);
    if (res != 0) {
        return NULL;
    }

    if (key->prekey) {
        pre_key_signal_message *message;
        pre_key_signal_message_deserialize(&message, key->data, key->length, omemo_ctx.signal);
        res = session_cipher_decrypt_pre_key_signal_message(cipher, message, NULL, &plaintext_key);
    } else {
        signal_message *message;
        signal_message_deserialize(&message, key->data, key->length, omemo_ctx.signal);
        res = session_cipher_decrypt_signal_message(cipher, message, NULL, &plaintext_key);
    }
    if (res != 0) {
        return NULL;
    }

    size_t plaintext_len = payload_len;
    unsigned char *plaintext = malloc(plaintext_len + 1);
    res = aes128gcm_decrypt(plaintext, &plaintext_len, payload, payload_len, iv, signal_buffer_data(plaintext_key));
    if (res != 0) {
        free(plaintext);
        return NULL;
    }

    plaintext[plaintext_len] = '\0';

    return (char *)plaintext;

}

static void
lock(void *user_data)
{
    omemo_context *ctx = (omemo_context *)user_data;
    pthread_mutex_lock(&ctx->lock);
}

static void
unlock(void *user_data)
{
    omemo_context *ctx = (omemo_context *)user_data;
    pthread_mutex_unlock(&ctx->lock);
}

static void
omemo_log(int level, const char *message, size_t len, void *user_data)
{
        cons_show(message);
}

static gboolean
handle_own_device_list(const char *const jid, GList *device_list)
{
    if (!g_list_find(device_list, GINT_TO_POINTER(omemo_ctx.device_id))) {
        gpointer original_jid;
        g_hash_table_steal_extended(omemo_ctx.device_list, jid, &original_jid, NULL);
        free(original_jid);
        device_list = g_list_append(device_list, GINT_TO_POINTER(omemo_ctx.device_id));
        g_hash_table_insert(omemo_ctx.device_list, strdup(jid), device_list);
        omemo_devicelist_publish(device_list);
    }

    return TRUE;
}

static gboolean
handle_device_list_start_session(const char *const jid, GList *device_list)
{
    omemo_start_session(jid);

    return FALSE;
}
