#include <signal/signal_protocol.h>

#include "config/account.h"
#include "ui/ui.h"
#include "omemo/crypto.h"

void
omemo_init(ProfAccount *account)
{
    signal_context *global_context;
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

    if (signal_context_create(&global_context, NULL) != 0) {
        cons_show("Error initializing Omemo context");
        return;
    }

    if (signal_context_set_crypto_provider(global_context, &crypto_provider) != 0) {
        cons_show("Error initializing Omemo crypto");
        return;
    }
    //signal_context_set_locking_functions(global_context, lock_function, unlock_function);
}
