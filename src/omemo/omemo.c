#include <sys/time.h>

#include <glib.h>
#include <pthread.h>
#include <signal/key_helper.h>
#include <signal/signal_protocol.h>
#include <sodium.h>

#include "config/account.h"
#include "log.h"
#include "omemo/crypto.h"
#include "omemo/omemo.h"
#include "ui/ui.h"
#include "xmpp/omemo.h"

static gboolean loaded;

static void lock(void *user_data);
static void unlock(void *user_data);

struct omemo_context_t {
    pthread_mutexattr_t attr;
    pthread_mutex_t lock;
    signal_context *signal;
    uint32_t device_id;
    GList *device_list;
    ratchet_identity_key_pair *identity_key_pair;
    uint32_t registration_id;
    signal_protocol_key_helper_pre_key_list_node *pre_keys_head;
    session_signed_pre_key *signed_pre_key;
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

    loaded = FALSE;
    omemo_ctx.device_list = NULL;
}

void
omemo_generate_crypto_materials(ProfAccount *account)
{
    omemo_ctx.device_id = randombytes_uniform(0x80000000);
    omemo_ctx.device_list = g_list_append(omemo_ctx.device_list, GINT_TO_POINTER(omemo_ctx.device_id));
    signal_protocol_key_helper_generate_identity_key_pair(&omemo_ctx.identity_key_pair, omemo_ctx.signal);
    signal_protocol_key_helper_generate_registration_id(&omemo_ctx.registration_id, 0, omemo_ctx.signal);
    signal_protocol_key_helper_generate_pre_keys(&omemo_ctx.pre_keys_head, randombytes_random(), 100, omemo_ctx.signal);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long timestamp = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;
    signal_protocol_key_helper_generate_signed_pre_key(&omemo_ctx.signed_pre_key, omemo_ctx.identity_key_pair, 5, timestamp, omemo_ctx.signal);

    loaded = TRUE;

    omemo_devicelist_publish();
    omemo_bundle_publish();
}

void
omemo_start_session(ProfAccount *account, char *barejid)
{

}

gboolean
omemo_loaded(void)
{
    return loaded;
}

GList * const
omemo_device_list(void)
{
    return omemo_ctx.device_list;
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
omemo_prekeys(GList ** const prekeys, GList ** const ids, GList ** const lengths)
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
