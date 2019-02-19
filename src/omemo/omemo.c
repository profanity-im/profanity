#include <pthread.h>
#include <signal/signal_protocol.h>

#include "config/account.h"
#include "ui/ui.h"
#include "omemo/omemo.h"
#include "omemo/crypto.h"

static void lock(void *user_data);
static void unlock(void *user_data);

struct omemo_context_t {
    pthread_mutexattr_t attr;
    pthread_mutex_t lock;
};

void
omemo_init(ProfAccount *account)
{
    signal_context *signal_ctx;
    omemo_context *ctx = malloc(sizeof(omemo_context));
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
        cons_show("Error initializing Omemo crypto");
    }

    pthread_mutexattr_init(&ctx->attr);
    pthread_mutexattr_settype(&ctx->attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&ctx->lock, &ctx->attr);

    if (signal_context_create(&signal_ctx, ctx) != 0) {
        cons_show("Error initializing Omemo context");
        return;
    }

    if (signal_context_set_crypto_provider(signal_ctx, &crypto_provider) != 0) {
        cons_show("Error initializing Omemo crypto");
        return;
    }

    signal_context_set_locking_functions(signal_ctx, lock, unlock);
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
