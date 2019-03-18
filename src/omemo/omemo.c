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
#include <gcrypt.h>

#include "config/account.h"
#include "config/files.h"
#include "log.h"
#include "omemo/crypto.h"
#include "omemo/omemo.h"
#include "omemo/store.h"
#include "ui/ui.h"
#include "ui/window_list.h"
#include "xmpp/connection.h"
#include "xmpp/muc.h"
#include "xmpp/omemo.h"
#include "xmpp/xmpp.h"

static gboolean loaded;

static void omemo_generate_short_term_crypto_materials(ProfAccount *account);
static void load_identity(void);
static void load_sessions(void);
static void lock(void *user_data);
static void unlock(void *user_data);
static void omemo_log(int level, const char *message, size_t len, void *user_data);
static gboolean handle_own_device_list(const char *const jid, GList *device_list);
static gboolean handle_device_list_start_session(const char *const jid, GList *device_list);
static void free_omemo_key(omemo_key_t *key);
static char * omemo_fingerprint(ec_public_key *identity, gboolean formatted);
static unsigned char *omemo_fingerprint_decode(const char *const fingerprint, size_t *len);
static void cache_device_identity(const char *const jid, uint32_t device_id, ec_public_key *identity);
static void g_hash_table_free(GHashTable *hash_table);

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
    uint32_t signed_pre_key_id;
    signal_protocol_store_context *store;
    GHashTable *session_store;
    GHashTable *pre_key_store;
    GHashTable *signed_pre_key_store;
    identity_key_store_t identity_key_store;
    GHashTable *device_ids;
    GString *identity_filename;
    GKeyFile *identity_keyfile;
    GString *sessions_filename;
    GKeyFile *sessions_keyfile;
    GHashTable *known_devices;
};

static omemo_context omemo_ctx;

void
omemo_init(void)
{
    log_info("OMEMO: initialising");
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


    loaded = FALSE;
    omemo_ctx.device_list = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)g_list_free);
    omemo_ctx.device_list_handler = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
    omemo_ctx.known_devices = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)g_hash_table_free);
}

void
omemo_on_connect(ProfAccount *account)
{
    GError *error = NULL;
    char *omemodir = files_get_data_path(DIR_OMEMO);
    GString *basedir = g_string_new(omemodir);
    free(omemodir);
    gchar *account_dir = str_replace(account->jid, "@", "_at_");
    g_string_append(basedir, "/");
    g_string_append(basedir, account_dir);
    g_string_append(basedir, "/");
    free(account_dir);

    omemo_ctx.identity_filename = g_string_new(basedir->str);
    g_string_append(omemo_ctx.identity_filename, "identity.txt");
    omemo_ctx.sessions_filename = g_string_new(basedir->str);
    g_string_append(omemo_ctx.sessions_filename, "sessions.txt");


    errno = 0;
    int res = g_mkdir_with_parents(basedir->str, S_IRWXU);
    if (res == -1) {
        char *errmsg = strerror(errno);
        if (errmsg) {
            log_error("OMEMO: error creating directory: %s, %s", basedir->str, errmsg);
        } else {
            log_error("OMEMO: creating directory: %s", basedir->str);
        }
    }

    omemo_devicelist_subscribe();

    omemo_ctx.identity_keyfile = g_key_file_new();
    omemo_ctx.sessions_keyfile = g_key_file_new();

    if (g_key_file_load_from_file(omemo_ctx.identity_keyfile, omemo_ctx.identity_filename->str, G_KEY_FILE_KEEP_COMMENTS, &error)) {
        load_identity();
        omemo_generate_short_term_crypto_materials(account);
    } else if (error->code != G_FILE_ERROR_NOENT) {
        log_warning("OMEMO: error loading identity from: %s, %s", omemo_ctx.identity_filename->str, error->message);
        return;
    }

    error = NULL;
    if (g_key_file_load_from_file(omemo_ctx.sessions_keyfile, omemo_ctx.sessions_filename->str, G_KEY_FILE_KEEP_COMMENTS, &error)) {
        load_sessions();
    } else if (error->code != G_FILE_ERROR_NOENT) {
        log_warning("OMEMO: error loading sessions from: %s, %s", omemo_ctx.sessions_filename->str, error->message);
    }

}

void
omemo_generate_crypto_materials(ProfAccount *account)
{
    if (loaded) {
        return;
    }

    log_info("Generate long term OMEMO cryptography metarials");
    gcry_randomize(&omemo_ctx.device_id, 4, GCRY_VERY_STRONG_RANDOM);
    omemo_ctx.device_id &= 0x7fffffff;
    log_info("OMEMO: device id: %d", omemo_ctx.device_id);

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

    omemo_identity_keyfile_save();

    omemo_generate_short_term_crypto_materials(account);
}

static void
omemo_generate_short_term_crypto_materials(ProfAccount *account)
{
    unsigned int start;

    log_info("Generate short term OMEMO cryptography metarials");

    gcry_randomize(&start, sizeof(unsigned int), GCRY_VERY_STRONG_RANDOM);
    signal_protocol_key_helper_pre_key_list_node *pre_keys_head;
    signal_protocol_key_helper_generate_pre_keys(&pre_keys_head, start, 100, omemo_ctx.signal);

    session_signed_pre_key *signed_pre_key;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long timestamp = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;

    omemo_ctx.signed_pre_key_id = 1;
    signal_protocol_key_helper_generate_signed_pre_key(&signed_pre_key, omemo_ctx.identity_key_pair, omemo_ctx.signed_pre_key_id, timestamp, omemo_ctx.signal);

    signal_protocol_key_helper_pre_key_list_node *p;
    for (p = pre_keys_head; p != NULL; p = signal_protocol_key_helper_key_list_next(p)) {
        session_pre_key *prekey = signal_protocol_key_helper_key_list_element(p);
        signal_protocol_pre_key_store_key(omemo_ctx.store, prekey);
    }
    signal_protocol_signed_pre_key_store_key(omemo_ctx.store, signed_pre_key);

    loaded = TRUE;

    /* Ensure we get our current device list, and it gets updated with our
     * device_id */
    g_hash_table_insert(omemo_ctx.device_list_handler, strdup(account->jid), handle_own_device_list);
    omemo_devicelist_request(account->jid);

    omemo_bundle_publish();
}

void
omemo_start_session(const char *const barejid)
{
    log_info("OMEMO: start session with %s", barejid);
    GList *device_list = g_hash_table_lookup(omemo_ctx.device_list, barejid);
    if (!device_list) {
        log_info("OMEMO: missing device list for %s", barejid);
        omemo_devicelist_request(barejid);
        g_hash_table_insert(omemo_ctx.device_list_handler, strdup(barejid), handle_device_list_start_session);
        return;
    }

    GList *device_id;
    for (device_id = device_list; device_id != NULL; device_id = device_id->next) {
        omemo_bundle_request(barejid, GPOINTER_TO_INT(device_id->data), omemo_start_device_session_handle_bundle, free, strdup(barejid));
    }
}

void
omemo_start_muc_sessions(const char *const roomjid)
{
    GList *roster = muc_roster(roomjid);
    GList *iter;
    for (iter = roster; iter != NULL; iter = iter->next) {
        Occupant *occupant = (Occupant *)iter->data;
        Jid *jid = jid_create(occupant->jid);
        omemo_start_session(jid->barejid);
        jid_destroy(jid);
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
    session_signed_pre_key *signed_pre_key;
    signal_buffer *buffer = NULL;

    signal_protocol_signed_pre_key_load_key(omemo_ctx.store, &signed_pre_key, omemo_ctx.signed_pre_key_id);
    ec_public_key_serialize(&buffer, ec_key_pair_get_public(session_signed_pre_key_get_key_pair(signed_pre_key)));
    *length = signal_buffer_len(buffer);
    *output = malloc(*length);
    memcpy(*output, signal_buffer_data(buffer), *length);
    signal_buffer_free(buffer);
}

void
omemo_signed_prekey_signature(unsigned char **output, size_t *length)
{
    session_signed_pre_key *signed_pre_key;

    signal_protocol_signed_pre_key_load_key(omemo_ctx.store, &signed_pre_key, omemo_ctx.signed_pre_key_id);
    *length = session_signed_pre_key_get_signature_len(signed_pre_key);
    *output = malloc(*length);
    memcpy(*output, session_signed_pre_key_get_signature(signed_pre_key), *length);
}

void
omemo_prekeys(GList **prekeys, GList **ids, GList **lengths)
{
    GHashTableIter iter;
    gpointer id;

    g_hash_table_iter_init(&iter, omemo_ctx.pre_key_store);
    while (g_hash_table_iter_next(&iter, &id, NULL)) {
        session_pre_key *pre_key;
        int ret;
        ret = signal_protocol_pre_key_load_key(omemo_ctx.store, &pre_key, GPOINTER_TO_INT(id));
        if (ret != SG_SUCCESS) {
            continue;
        }

        signal_buffer *public_key;
        ec_public_key_serialize(&public_key, ec_key_pair_get_public(session_pre_key_get_key_pair(pre_key)));
        size_t length = signal_buffer_len(public_key);
        unsigned char *prekey_value = malloc(length);
        memcpy(prekey_value, signal_buffer_data(public_key), length);
        *prekeys = g_list_append(*prekeys, prekey_value);
        *ids = g_list_append(*ids, GINT_TO_POINTER(id));
        *lengths = g_list_append(*lengths, GINT_TO_POINTER(length));
    }
}

void
omemo_set_device_list(const char *const from, GList * device_list)
{
    Jid *jid;
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

    jid_destroy(jid);
}

GKeyFile *
omemo_identity_keyfile(void)
{
    return omemo_ctx.identity_keyfile;
}

void
omemo_identity_keyfile_save(void)
{
    GError *error = NULL;

    if (!g_key_file_save_to_file(omemo_ctx.identity_keyfile, omemo_ctx.identity_filename->str, &error)) {
        log_error("OMEMO: error saving identity to: %s, %s", omemo_ctx.identity_filename->str, error->message);
    }
}

GKeyFile *
omemo_sessions_keyfile(void)
{
    return omemo_ctx.sessions_keyfile;
}

void
omemo_sessions_keyfile_save(void)
{
    GError *error = NULL;

    if (!g_key_file_save_to_file(omemo_ctx.sessions_keyfile, omemo_ctx.sessions_filename->str, &error)) {
        log_error("OMEMO: error saving sessions to: %s, %s", omemo_ctx.sessions_filename->str, error->message);
    }
}

void
omemo_start_device_session(const char *const jid, uint32_t device_id,
    GList *prekeys, uint32_t signed_prekey_id,
    const unsigned char *const signed_prekey_raw, size_t signed_prekey_len,
    const unsigned char *const signature, size_t signature_len,
    const unsigned char *const identity_key_raw, size_t identity_key_len)
{
    signal_protocol_address address = {
        .name = strdup(jid),
        .name_len = strlen(jid),
        .device_id = device_id,
    };

    ec_public_key *identity_key;
    curve_decode_point(&identity_key, identity_key_raw, identity_key_len, omemo_ctx.signal);
    cache_device_identity(jid, device_id, identity_key);

    gboolean trusted = is_trusted_identity(&address, (uint8_t *)identity_key_raw, identity_key_len, &omemo_ctx.identity_key_store);

    Jid *ownjid = jid_create(connection_get_fulljid());
    if (g_strcmp0(jid, ownjid->barejid) == 0) {
        char *fingerprint = omemo_fingerprint(identity_key, TRUE);

        cons_show("Available device identity for %s: %s%s", ownjid->barejid, fingerprint, trusted ? " (trusted)" : "");
        if (trusted) {
            cons_show("You can untrust it with '/omemo untrust %s <fingerprint>'", ownjid->barejid);
        } else {
            cons_show("You can trust it with '/omemo trust %s <fingerprint>'", ownjid->barejid);
        }
        free(fingerprint);
    }

    ProfChatWin *chatwin = wins_get_chat(jid);
    if (chatwin) {
        char *fingerprint = omemo_fingerprint(identity_key, TRUE);

        win_println((ProfWin *)chatwin, THEME_DEFAULT, '-', "Available device identity: %s%s", fingerprint, trusted ? " (trusted)" : "");
        if (trusted) {
            win_println((ProfWin *)chatwin, THEME_DEFAULT, '-', "You can untrust it with '/omemo untrust <fingerprint>'");
        } else {
            win_println((ProfWin *)chatwin, THEME_DEFAULT, '-', "You can trust it with '/omemo trust <fingerprint>'");
        }
        free(fingerprint);
    }

    if (!trusted) {
        goto out;
    }

    if (!contains_session(&address, omemo_ctx.session_store)) {
        int res;
        session_pre_key_bundle *bundle;
        signal_protocol_address *address;

        address = malloc(sizeof(signal_protocol_address));
        address->name = strdup(jid);
        address->name_len = strlen(jid);
        address->device_id = device_id;

        session_builder *builder;
        res = session_builder_create(&builder, omemo_ctx.store, address, omemo_ctx.signal);
        if (res != 0) {
            log_error("OMEMO: cannot create session builder for %s device %d", jid, device_id);
            goto out;
        }

        int prekey_index;
        gcry_randomize(&prekey_index, sizeof(int), GCRY_STRONG_RANDOM);
        prekey_index %= g_list_length(prekeys);
        omemo_key_t *prekey = g_list_nth_data(prekeys, prekey_index);

        ec_public_key *prekey_public;
        curve_decode_point(&prekey_public, prekey->data, prekey->length, omemo_ctx.signal);
        ec_public_key *signed_prekey;
        curve_decode_point(&signed_prekey, signed_prekey_raw, signed_prekey_len, omemo_ctx.signal);

        res = session_pre_key_bundle_create(&bundle, 0, device_id, prekey->id, prekey_public, signed_prekey_id, signed_prekey, signature, signature_len, identity_key);
        if (res != 0) {
            log_error("OMEMO: cannot create pre key bundle for %s device %d", jid, device_id);
            goto out;
        }

        res = session_builder_process_pre_key_bundle(builder, bundle);
        if (res != 0) {
            log_error("OMEMO: cannot process pre key bundle for %s device %d", jid, device_id);
            goto out;
        }

        log_info("OMEMO: create session with %s device %d", jid, device_id);
    }

out:
    g_list_free_full(prekeys, (GDestroyNotify)free_omemo_key);
    jid_destroy(ownjid);
}

char *
omemo_on_message_send(ProfWin *win, const char *const message, gboolean request_receipt, gboolean muc)
{
    char *id = NULL;
    int res;
    Jid *jid = jid_create(connection_get_fulljid());
    GList *keys = NULL;

    unsigned char *key;
    unsigned char *iv;
    unsigned char *ciphertext;
    unsigned char *tag;
    unsigned char *key_tag;
    size_t ciphertext_len, tag_len;

    ciphertext_len = strlen(message);
    ciphertext = malloc(ciphertext_len);
    tag_len = AES128_GCM_TAG_LENGTH;
    tag = gcry_malloc_secure(tag_len);
    key_tag = gcry_malloc_secure(AES128_GCM_KEY_LENGTH + AES128_GCM_TAG_LENGTH);

    key = gcry_random_bytes_secure(AES128_GCM_KEY_LENGTH, GCRY_VERY_STRONG_RANDOM);
    iv = gcry_random_bytes_secure(AES128_GCM_IV_LENGTH, GCRY_VERY_STRONG_RANDOM);

    res = aes128gcm_encrypt(ciphertext, &ciphertext_len, tag, &tag_len, (const unsigned char * const)message, strlen(message), iv, key);
    if (res != 0) {
        log_error("OMEMO: cannot encrypt message");
        goto out;
    }

    memcpy(key_tag, key, AES128_GCM_KEY_LENGTH);
    memcpy(key_tag + AES128_GCM_KEY_LENGTH, tag, AES128_GCM_TAG_LENGTH);

    GList *recipients = NULL;
    if (muc) {
        ProfMucWin *mucwin = (ProfMucWin *)win;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        GList *roster = muc_roster(mucwin->roomjid);
        GList *iter;
        for (iter = roster; iter != NULL; iter = iter->next) {
            Occupant *occupant = (Occupant *)iter->data;
            Jid *jid = jid_create(occupant->jid);
            if (!jid->barejid) {
                log_warning("OMEMO: missing barejid for MUC %s occupant %s", mucwin->roomjid, occupant->nick);
            } else {
                recipients = g_list_append(recipients, strdup(jid->barejid));
            }
            jid_destroy(jid);
        }
    } else {
        ProfChatWin *chatwin = (ProfChatWin *)win;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        recipients = g_list_append(recipients, strdup(chatwin->barejid));
    }

    GList *device_ids_iter;

    GList *recipients_iter;
    for (recipients_iter = recipients; recipients_iter != NULL; recipients_iter = recipients_iter->next) {
        GList *recipient_device_id = NULL;
        recipient_device_id = g_hash_table_lookup(omemo_ctx.device_list, recipients_iter->data);
        if (!recipient_device_id) {
            log_warning("OMEMO: cannot find device ids for %s", recipients_iter->data);
            continue;
        }

        for (device_ids_iter = recipient_device_id; device_ids_iter != NULL; device_ids_iter = device_ids_iter->next) {
            int res;
            ciphertext_message *ciphertext;
            session_cipher *cipher;
            signal_protocol_address address = {
                .name = recipients_iter->data,
                .name_len = strlen(recipients_iter->data),
                .device_id = GPOINTER_TO_INT(device_ids_iter->data)
            };

            res = session_cipher_create(&cipher, omemo_ctx.store, &address, omemo_ctx.signal);
            if (res != 0) {
                log_error("OMEMO: cannot create cipher for %s device id %d", address.name, address.device_id);
                continue;
            }

            res = session_cipher_encrypt(cipher, key_tag, AES128_GCM_KEY_LENGTH + AES128_GCM_TAG_LENGTH, &ciphertext);
            if (res != 0) {
                log_error("OMEMO: cannot encrypt key for %s device id %d", address.name, address.device_id);
                continue;
            }
            signal_buffer *buffer = ciphertext_message_get_serialized(ciphertext);
            omemo_key_t *key = malloc(sizeof(omemo_key_t));
            key->data = signal_buffer_data(buffer);
            key->length = signal_buffer_len(buffer);
            key->device_id = GPOINTER_TO_INT(device_ids_iter->data);
            key->prekey = ciphertext_message_get_type(ciphertext) == CIPHERTEXT_PREKEY_TYPE;
            keys = g_list_append(keys, key);
        }
    }

    g_list_free_full(recipients, free);

    if (!muc) {
        GList *sender_device_id = g_hash_table_lookup(omemo_ctx.device_list, jid->barejid);
        for (device_ids_iter = sender_device_id; device_ids_iter != NULL; device_ids_iter = device_ids_iter->next) {
            int res;
            ciphertext_message *ciphertext;
            session_cipher *cipher;
            signal_protocol_address address = {
                .name = jid->barejid,
                .name_len = strlen(jid->barejid),
                .device_id = GPOINTER_TO_INT(device_ids_iter->data)
            };

            res = session_cipher_create(&cipher, omemo_ctx.store, &address, omemo_ctx.signal);
            if (res != 0) {
                log_error("OMEMO: cannot create cipher for %s device id %d", address.name, address.device_id);
                continue;
            }

            res = session_cipher_encrypt(cipher, key_tag, AES128_GCM_KEY_LENGTH + AES128_GCM_TAG_LENGTH, &ciphertext);
            if (res != 0) {
                log_error("OMEMO: cannot encrypt key for %s device id %d", address.name, address.device_id);
                continue;
            }
            signal_buffer *buffer = ciphertext_message_get_serialized(ciphertext);
            omemo_key_t *key = malloc(sizeof(omemo_key_t));
            key->data = signal_buffer_data(buffer);
            key->length = signal_buffer_len(buffer);
            key->device_id = GPOINTER_TO_INT(device_ids_iter->data);
            key->prekey = ciphertext_message_get_type(ciphertext) == CIPHERTEXT_PREKEY_TYPE;
            keys = g_list_append(keys, key);
        }
    }

    if (muc) {
        ProfMucWin *mucwin = (ProfMucWin *)win;
        assert(mucwin->memcheck == PROFMUCWIN_MEMCHECK);
        id = message_send_chat_omemo(mucwin->roomjid, omemo_ctx.device_id, keys, iv, AES128_GCM_IV_LENGTH, ciphertext, ciphertext_len, request_receipt, TRUE);
    } else {
        ProfChatWin *chatwin = (ProfChatWin *)win;
        assert(chatwin->memcheck == PROFCHATWIN_MEMCHECK);
        id = message_send_chat_omemo(chatwin->barejid, omemo_ctx.device_id, keys, iv, AES128_GCM_IV_LENGTH, ciphertext, ciphertext_len, request_receipt, FALSE);
    }

out:
    jid_destroy(jid);
    g_list_free_full(keys, free);
    free(ciphertext);
    gcry_free(key);
    gcry_free(iv);
    gcry_free(tag);
    gcry_free(key_tag);

    return id;
}

char *
omemo_on_message_recv(const char *const from_jid, uint32_t sid,
    const unsigned char *const iv, size_t iv_len, GList *keys,
    const unsigned char *const payload, size_t payload_len, gboolean muc)
{
    unsigned char *plaintext = NULL;
    Jid *sender = NULL;
    Jid *from = jid_create(from_jid);
    if (!from) {
        log_error("Invalid jid %s", from_jid);
        goto out;
    }

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
        log_warning("OMEMO: Received a message with no corresponding key");
        goto out;
    }

    if (muc) {
        GList *roster = muc_roster(from->barejid);
        GList *iter;
        for (iter = roster; iter != NULL; iter = iter->next) {
            Occupant *occupant = (Occupant *)iter->data;
            if (g_strcmp0(occupant->nick, from->resourcepart) == 0) {
                sender = jid_create(occupant->jid);
                break;
            }
        }
    } else {
        sender = jid_create(from->barejid);
    }

    session_cipher *cipher;
    signal_buffer *plaintext_key;
    signal_protocol_address address = {
        .name = sender->barejid,
        .name_len = strlen(sender->barejid),
        .device_id = sid
    };

    res = session_cipher_create(&cipher, omemo_ctx.store, &address, omemo_ctx.signal);
    if (res != 0) {
        log_error("OMEMO: cannot create session cipher");
        goto out;
    }

    if (key->prekey) {
        log_debug("OMEMO: decrypting message with prekey");
        pre_key_signal_message *message;

        pre_key_signal_message_deserialize(&message, key->data, key->length, omemo_ctx.signal);

        res = session_cipher_decrypt_pre_key_signal_message(cipher, message, NULL, &plaintext_key);
        /* Replace used pre_key in bundle */
        uint32_t pre_key_id = pre_key_signal_message_get_pre_key_id(message);
        ec_key_pair *ec_pair;
        session_pre_key *new_pre_key;
        curve_generate_key_pair(omemo_ctx.signal, &ec_pair);
        session_pre_key_create(&new_pre_key, pre_key_id, ec_pair);
        signal_protocol_pre_key_store_key(omemo_ctx.store, new_pre_key);
        omemo_bundle_publish();

        if (res == 0) {
            /* Start a new session */
            omemo_bundle_request(sender->barejid, sid, omemo_start_device_session_handle_bundle, free, strdup(sender->barejid));
        }
    } else {
        log_debug("OMEMO: decrypting message with existing session");
        signal_message *message;
        signal_message_deserialize(&message, key->data, key->length, omemo_ctx.signal);
        res = session_cipher_decrypt_signal_message(cipher, message, NULL, &plaintext_key);
    }
    if (res != 0) {
        log_error("OMEMO: cannot decrypt message key");
        return NULL;
    }

    if (signal_buffer_len(plaintext_key) != AES128_GCM_KEY_LENGTH + AES128_GCM_TAG_LENGTH) {
        log_error("OMEMO: invalid key length");
        signal_buffer_free(plaintext_key);
        return NULL;
    }

    size_t plaintext_len = payload_len;
    plaintext = malloc(plaintext_len + 1);
    res = aes128gcm_decrypt(plaintext, &plaintext_len, payload, payload_len, iv,
        signal_buffer_data(plaintext_key),
        signal_buffer_data(plaintext_key) + AES128_GCM_KEY_LENGTH);
    if (res != 0) {
        log_error("OMEMO: cannot decrypt message: %s", gcry_strerror(res));
        signal_buffer_free(plaintext_key);
        free(plaintext);
        return NULL;
    }

    plaintext[plaintext_len] = '\0';

out:
    jid_destroy(from);
    jid_destroy(sender);
    return (char *)plaintext;
}

char *
omemo_format_fingerprint(const char *const fingerprint)
{
    char *output = malloc(strlen(fingerprint) + strlen(fingerprint) / 8);

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

char *
omemo_own_fingerprint(gboolean formatted)
{
    ec_public_key *identity = ratchet_identity_key_pair_get_public(omemo_ctx.identity_key_pair);
    return omemo_fingerprint(identity, formatted);
}

static char *
omemo_fingerprint(ec_public_key *identity, gboolean formatted)
{
    int i;
    signal_buffer *identity_public_key;

    ec_public_key_serialize(&identity_public_key, identity);
    size_t identity_public_key_len = signal_buffer_len(identity_public_key);
    unsigned char *identity_public_key_data = signal_buffer_data(identity_public_key);

    /* Skip first byte corresponding to signal DJB_TYPE */
    identity_public_key_len--;
    identity_public_key_data = &identity_public_key_data[1];

    char *fingerprint = malloc(identity_public_key_len * 2 + 1);

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
        char *formatted_fingerprint = omemo_format_fingerprint(fingerprint);
        free(fingerprint);
        return formatted_fingerprint;
    }
}

static unsigned char *
omemo_fingerprint_decode(const char *const fingerprint, size_t *len)
{
    unsigned char *output = malloc(strlen(fingerprint) / 2 + 1);

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
omemo_trust(const char *const jid, const char *const fingerprint_formatted)
{
    size_t len;

    GHashTable *known_identities = g_hash_table_lookup(omemo_ctx.known_devices, jid);
    if (!known_identities) {
        log_warning("OMEMO: cannot trust unknown device: %s", fingerprint_formatted);
        cons_show("Cannot trust unknown device: %s", fingerprint_formatted);
        return;
    }

    /* Unformat fingerprint */
    char *fingerprint = malloc(strlen(fingerprint_formatted));
    int i;
    int j;
    for (i = 0, j = 0; fingerprint_formatted[i] != '\0'; i++) {
        if (!g_ascii_isxdigit(fingerprint_formatted[i])) {
            continue;
        }
        fingerprint[j++] = fingerprint_formatted[i];
    }

    fingerprint[j] = '\0';

    uint32_t device_id = GPOINTER_TO_INT(g_hash_table_lookup(known_identities, fingerprint));
    free(fingerprint);

    if (!device_id) {
        log_warning("OMEMO: cannot trust unknown device: %s", fingerprint_formatted);
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
    unsigned char *fingerprint_raw = omemo_fingerprint_decode(fingerprint_formatted, &len);
    unsigned char djb_type[] = {'\x05'};
    signal_buffer *buffer = signal_buffer_create(djb_type, 1);
    buffer = signal_buffer_append(buffer, fingerprint_raw, len);
    save_identity(&address, signal_buffer_data(buffer), signal_buffer_len(buffer), &omemo_ctx.identity_key_store);
    free(fingerprint_raw);
    signal_buffer_free(buffer);

    omemo_bundle_request(jid, device_id, omemo_start_device_session_handle_bundle, free, strdup(jid));
}

void
omemo_untrust(const char *const jid, const char *const fingerprint_formatted)
{
    size_t len;
    unsigned char *fingerprint = omemo_fingerprint_decode(fingerprint_formatted, &len);

    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, omemo_ctx.identity_key_store.trusted);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        signal_buffer *buffer = value;
        unsigned char *original = signal_buffer_data(buffer);
        /* Skip DJB_TYPE byte */
        original++;
        if ((signal_buffer_len(buffer) - 1) == len && memcmp(original, fingerprint, len) == 0) {
            g_hash_table_remove(omemo_ctx.identity_key_store.trusted, key);
        }
    }
    free(fingerprint);
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
    switch (level) {
        case SG_LOG_ERROR:
            log_error("OMEMO: %s", message);
            break;
        case SG_LOG_WARNING:
            log_warning("OMEMO: %s", message);
            break;
        case SG_LOG_NOTICE:
        case SG_LOG_INFO:
            log_info("OMEMO: %s", message);
            break;
        case SG_LOG_DEBUG:
            log_debug("OMEMO: %s", message);
            break;
    }
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

    GList *device_id;
    for (device_id = device_list; device_id != NULL; device_id = device_id->next) {
        omemo_bundle_request(jid, GPOINTER_TO_INT(device_id->data), omemo_start_device_session_handle_bundle, free, strdup(jid));
    }

    return TRUE;
}

static gboolean
handle_device_list_start_session(const char *const jid, GList *device_list)
{
    omemo_start_session(jid);

    return FALSE;
}

static void
free_omemo_key(omemo_key_t *key)
{
    if (key == NULL) {
        return;
    }

    free((void *)key->data);
    free(key);
}

static void
load_identity(void)
{
    log_info("Loading OMEMO identity");
    omemo_ctx.device_id = g_key_file_get_uint64(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_IDENTITY, OMEMO_STORE_KEY_DEVICE_ID, NULL);
    log_info("OMEMO: device id: %d", omemo_ctx.device_id);
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

    char **keys = g_key_file_get_keys(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_TRUST, NULL, NULL);
    if (keys) {
        int i;
        for (i = 0; keys[i] != NULL; i++) {
            char *key_b64 = g_key_file_get_string(omemo_ctx.identity_keyfile, OMEMO_STORE_GROUP_TRUST, keys[i], NULL);
            size_t key_len;
            unsigned char *key = g_base64_decode(key_b64, &key_len);
            signal_buffer *buffer = signal_buffer_create(key, key_len);
            g_hash_table_insert(omemo_ctx.identity_key_store.trusted, keys[i], buffer);
            free(key_b64);
        }
    }
}

static void
load_sessions(void)
{
    int i;
    char **groups = g_key_file_get_groups(omemo_ctx.sessions_keyfile, NULL);
    if (groups) {
        for (i = 0; groups[i] != NULL; i++) {
            int j;
            GHashTable *device_store = NULL;

            device_store = g_hash_table_lookup(omemo_ctx.session_store, groups[i]);
            if (!device_store) {
                device_store = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)signal_buffer_free);
                g_hash_table_insert(omemo_ctx.session_store, groups[i], device_store);
            }

            char **keys = g_key_file_get_keys(omemo_ctx.sessions_keyfile, groups[i], NULL, NULL);
            for (j = 0; keys[j] != NULL; j++) {
                uint32_t id = strtoul(keys[j], NULL, 10);
                char *record_b64 = g_key_file_get_string(omemo_ctx.sessions_keyfile, groups[i], keys[j], NULL);
                size_t record_len;
                unsigned char *record = g_base64_decode(record_b64, &record_len);
                signal_buffer *buffer = signal_buffer_create(record, record_len);
                g_hash_table_insert(device_store, GINT_TO_POINTER(id), buffer);
                free(record_b64);
            }
        }
    }
}

static void
cache_device_identity(const char *const jid, uint32_t device_id, ec_public_key *identity)
{
    GHashTable *known_identities = g_hash_table_lookup(omemo_ctx.known_devices, jid);
    if (!known_identities) {
        known_identities = g_hash_table_new_full(g_str_hash, g_str_equal, free, NULL);
        g_hash_table_insert(omemo_ctx.known_devices, strdup(jid), known_identities);
    }

    char *fingerprint = omemo_fingerprint(identity, FALSE);
    log_info("OMEMO: cache identity for %s:%d: %s", jid, device_id, fingerprint);
    g_hash_table_insert(known_identities, fingerprint, GINT_TO_POINTER(device_id));
}

static void
g_hash_table_free(GHashTable *hash_table)
{
    g_hash_table_remove_all(hash_table);
    g_hash_table_unref(hash_table);
}
