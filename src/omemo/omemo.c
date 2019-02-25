#include <sys/time.h>

#include <glib.h>
#include <pthread.h>
#include <signal/key_helper.h>
#include <signal/signal_protocol.h>
#include <signal/session_builder.h>
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

static gboolean loaded;

static void lock(void *user_data);
static void unlock(void *user_data);

struct omemo_context_t {
    pthread_mutexattr_t attr;
    pthread_mutex_t lock;
    signal_context *signal;
    uint32_t device_id;
    GHashTable *device_list;
    ratchet_identity_key_pair *identity_key_pair;
    uint32_t registration_id;
    signal_protocol_key_helper_pre_key_list_node *pre_keys_head;
    session_signed_pre_key *signed_pre_key;
    signal_protocol_store_context *store;
    GHashTable *session_store;
    GHashTable *pre_key_store;
    GHashTable *signed_pre_key_store;
    identity_key_store_t identity_key_store;
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
    };
    signal_protocol_store_context_set_identity_key_store(omemo_ctx.store, &identity_key_store);


    loaded = FALSE;
    omemo_ctx.device_list = g_hash_table_new_full(g_str_hash, g_str_equal, free, (GDestroyNotify)g_list_free);
}

void
omemo_generate_crypto_materials(ProfAccount *account)
{
    xmpp_ctx_t * const ctx = connection_get_ctx();
    char *barejid = xmpp_jid_bare(ctx, session_get_account_name());

    omemo_ctx.device_id = randombytes_uniform(0x80000000);

    signal_protocol_key_helper_generate_identity_key_pair(&omemo_ctx.identity_key_pair, omemo_ctx.signal);
    signal_protocol_key_helper_generate_registration_id(&omemo_ctx.registration_id, 0, omemo_ctx.signal);
    signal_protocol_key_helper_generate_pre_keys(&omemo_ctx.pre_keys_head, randombytes_random(), 100, omemo_ctx.signal);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long timestamp = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
    signal_protocol_key_helper_generate_signed_pre_key(&omemo_ctx.signed_pre_key, omemo_ctx.identity_key_pair, 5, timestamp, omemo_ctx.signal);

    loaded = TRUE;

    /* Ensure we get our current device list, and it gets updated with our
     * device_id */
    omemo_devicelist_request(barejid);
    omemo_bundle_publish();
}

void
omemo_start_session(const char *const barejid)
{
    GList *device_list = g_hash_table_lookup(omemo_ctx.device_list, barejid);
    if (!device_list) {
        omemo_devicelist_request(barejid);
        /* TODO handle response */
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
    memcpy(*output, signal_buffer_const_data(buffer), *length);
    signal_buffer_free(buffer);
}

void
omemo_signed_prekey(unsigned char **output, size_t *length)
{
    signal_buffer *buffer = NULL;
    ec_public_key_serialize(&buffer, ec_key_pair_get_public(session_signed_pre_key_get_key_pair(omemo_ctx.signed_pre_key)));
    *length = signal_buffer_len(buffer);
    *output = malloc(*length);
    memcpy(*output, signal_buffer_const_data(buffer), *length);
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
        memcpy(prekey_value, signal_buffer_const_data(buffer), length);
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
    char *barejid = xmpp_jid_bare(ctx, session_get_account_name());

    if (g_strcmp0(jid, barejid) == 0) {
        if (!g_list_find(device_list, GINT_TO_POINTER(omemo_ctx.device_id))) {
            device_list = g_list_append(device_list, GINT_TO_POINTER(omemo_ctx.device_id));
            omemo_devicelist_publish(device_list);
        }
    }

    g_hash_table_insert(omemo_ctx.device_list, strdup(jid), device_list);
}

void
omemo_start_device_session(const char *const jid, uint32_t device_id, const unsigned char *const prekey, size_t prekey_len)
{
    signal_protocol_address address = {
        jid, strlen(jid), device_id
    };

    session_builder *builder;
    session_builder_create(&builder, omemo_ctx.store, &address, omemo_ctx.signal);
    //session_builder_process_pre_key_bundle(builder, prekey);
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
